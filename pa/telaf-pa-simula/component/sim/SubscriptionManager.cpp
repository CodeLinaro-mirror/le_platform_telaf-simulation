// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SubscriptionManager.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <algorithm>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>

namespace telux::tel::simula {

using namespace SimSignals;

namespace {

using common::simula::Envelope;

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct StateIndPld
{
    Envelope env;
};

template<typename T>
std::shared_ptr<T>
as(const chart::Event& e)
{
    return std::static_pointer_cast<T>(e.payload);
}

}  // namespace

chart::Status
SubNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
SubReady_St(chart::Hsm*, chart::Event const*);

SimulaSubscriptionManager::SimulaSubscriptionManager(
  int slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("SubscriptionManager")
    , bridge_(bridge)
    , slotId_(slotId)
    , subscription_(std::make_shared<SimulaSubscription>(slotId, "", ""))
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaSubscriptionManager::~SimulaSubscriptionManager()
{
    // Withdraw from the bridge before anything else: the callbacks below
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaSubscriptionManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::sim::subsys_ready_sub::ind);
    bridge_.unsubscribe_event(topics::sim::sub_info_changed::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaSubscriptionManager::addInitCallback(telux::common::InitResponseCb cb)
{
    if (!cb)
        return;
    {
        std::lock_guard<std::mutex> lk(init_cbs_mutex_);
        if (!init_reported_)
        {
            init_cbs_.push_back(std::move(cb));
            return;
        }
    }
    cb(last_status_.load());
}

void
SimulaSubscriptionManager::fireInitCallbacks_(telux::common::ServiceStatus status)
{
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::lock_guard<std::mutex> lk(init_cbs_mutex_);
        init_reported_ = true;
        cbs.swap(init_cbs_);
    }
    for (auto& cb : cbs)
    {
        if (!cb)
            continue;
        try
        {
            cb(status);
        }
        catch (const std::future_error& e)
        {
            // A late callback may target a promise destroyed after the PA timeout.
            LOG_WARN("[SubscriptionManager] init callback fired after PA timeout (%s); dropping",
                     e.what());
        }
        catch (...)
        {
            LOG_WARN("[SubscriptionManager] init callback threw unexpectedly; dropping");
        }
    }
}

void
SimulaSubscriptionManager::start()
{
    if (running())
        return;
    // Instrumentation must be attached before start_at().
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(SubNotReady_St);

    bridge_.subscribe_event(
      topics::sim::subsys_ready_sub::ind,
      "sim.subsys_ready_sub.ind",
      [this](std::string_view topic, const Envelope& env) { handleSubInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::sim::sub_info_changed::ind,
      "sim.sub_info_changed.ind",
      [this](std::string_view topic, const Envelope& env) { handleSubInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaSubscriptionManager::handleSubInd_(std::string_view topic, const Envelope& env)
{
    // Bridge callbacks run off the AO thread; state changes are posted to it.
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    if (topic == topics::sim::subsys_ready_sub::ind)
        post_fifo({ ReadinessEvt_Signal, pld });
    else
        post_fifo({ SubInfoChangedEvt_Signal, pld });
}

void
SimulaSubscriptionManager::resyncSubInfo_()
{
    // sub_info_changed is non-retained; join both RPC results before the normal update path.
    struct Accum
    {
        std::mutex m;
        std::optional<std::string> iccid;
        std::optional<std::string> imsi;
    };
    auto acc = std::make_shared<Accum>();

    auto complete = [this, acc]() {
        std::string iccid, imsi;
        {
            std::lock_guard<std::mutex> lk(acc->m);
            if (!acc->iccid || !acc->imsi)
                return;  // still waiting on the other RPC
            iccid = *acc->iccid;
            imsi = *acc->imsi;
        }
        nlohmann::json data = nlohmann::json::object();
        data["iccid"] = iccid;
        data["imsi"] = imsi;
        auto pld = std::make_shared<StateIndPld>();
        pld->env.data = std::move(data);
        post_fifo({ SubInfoChangedEvt_Signal, pld });
    };

    const auto paId = bridge_.currentPaId();

    nlohmann::json iccidReq = nlohmann::json::object();
    iccidReq["slot"] = slotId_;
    bridge_.send_request(
      topics::sim::get_iccid::req,
      "sim.get_iccid.rsp",
      common::simula::makeRequestEnvelope(paId, std::move(iccidReq)),
      [acc, complete](std::optional<Envelope> rsp) {
          {
              std::lock_guard<std::mutex> lk(acc->m);
              acc->iccid = (rsp && !rsp->error && rsp->data)
                             ? rsp->data->value("iccid", std::string())
                             : std::string();
          }
          complete();
      },
      kRpcTimeout
    );

    nlohmann::json imsiReq = nlohmann::json::object();
    imsiReq["slot"] = slotId_;
    bridge_.send_request(
      topics::sim::get_imsi::req,
      "sim.get_imsi.rsp",
      common::simula::makeRequestEnvelope(paId, std::move(imsiReq)),
      [acc, complete](std::optional<Envelope> rsp) {
          {
              std::lock_guard<std::mutex> lk(acc->m);
              acc->imsi = (rsp && !rsp->error && rsp->data)
                            ? rsp->data->value("imsi", std::string())
                            : std::string();
          }
          complete();
      },
      kRpcTimeout
    );
}

void
SimulaSubscriptionManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::ISubscriptionListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "SubscriptionManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& weak : listeners_)
        {
            if (auto sp = weak.lock())
                task->listeners.push_back(sp);
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<telux::tel::ISubscriptionListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// telux::tel::ISubscriptionManager

bool
SimulaSubscriptionManager::isSubsystemReady()
{
    return ready_flag_.load();
}

std::future<bool>
SimulaSubscriptionManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(ready_flag_.load());
    return p.get_future();
}

telux::common::ServiceStatus
SimulaSubscriptionManager::getServiceStatus()
{
    return last_status_.load();
}

std::shared_ptr<telux::tel::ISubscription>
SimulaSubscriptionManager::getSubscription(int slotId, telux::common::Status* status)
{
    if (!ready_flag_.load())
    {
        if (status)
            *status = telux::common::Status::NOTREADY;
        return nullptr;
    }
    if (slotId != slotId_ && slotId != DEFAULT_SLOT_ID)
    {
        if (status)
            *status = telux::common::Status::NOSUCH;
        return nullptr;
    }
    if (status)
        *status = telux::common::Status::SUCCESS;
    return subscription_;
}

std::vector<std::shared_ptr<telux::tel::ISubscription>>
SimulaSubscriptionManager::getAllSubscriptions(telux::common::Status* status)
{
    if (!ready_flag_.load())
    {
        if (status)
            *status = telux::common::Status::NOTREADY;
        return {};
    }
    if (status)
        *status = telux::common::Status::SUCCESS;
    return { subscription_ };
}

telux::common::Status
SimulaSubscriptionManager::registerListener(std::weak_ptr<telux::tel::ISubscriptionListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaSubscriptionManager::removeListener(std::weak_ptr<telux::tel::ISubscriptionListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::ISubscriptionListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
SubNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaSubscriptionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = as<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(SubReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
SubReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaSubscriptionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->ready_flag_.store(true);
            self->last_status_.store(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            bool first_report;
            {
                std::lock_guard<std::mutex> lk(self->init_cbs_mutex_);
                first_report = !self->init_reported_;
            }
            self->fireInitCallbacks_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!first_report)
            {
                self->broadcastToListeners_(
                  [](const std::shared_ptr<telux::tel::ISubscriptionListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            self->resyncSubInfo_();
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->ready_flag_.store(false);
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::ISubscriptionListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = as<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            if (state == "UNAVAILABLE")
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(SubNotReady_St);
            }
            if (state == "FAILED")
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(SubNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = as<bool>(*e);
            if (pld && !*pld)
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(SubNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case SubInfoChangedEvt_Signal:
        {
            auto pld = as<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            self->subscription_->setIds(
              pld->env.data->value("iccid", std::string()),
              pld->env.data->value("imsi", std::string())
            );
            auto sub = self->subscription_;
            self->broadcastToListeners_(
              [sub](const std::shared_ptr<telux::tel::ISubscriptionListener>& l) {
                  l->onSubscriptionInfoChanged(sub);
              }
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

// State names for chart::Spy output.
CHART_NAMED_STATE(SubNotReady_St, "SubscriptionManager::NotReady");
CHART_NAMED_STATE(SubReady_St,    "SubscriptionManager::Ready");

}  // namespace telux::tel::simula
