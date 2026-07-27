// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "MultiSimManager.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <algorithm>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using namespace SimSignals;

namespace {

using common::simula::Envelope;

telux::tel::CardState
wireToCardState(const std::string& s)
{
    if (s == "ABSENT")
        return telux::tel::CardState::CARDSTATE_ABSENT;
    if (s == "PRESENT")
        return telux::tel::CardState::CARDSTATE_PRESENT;
    if (s == "ERROR")
        return telux::tel::CardState::CARDSTATE_ERROR;
    if (s == "RESTRICTED")
        return telux::tel::CardState::CARDSTATE_RESTRICTED;
    return telux::tel::CardState::CARDSTATE_UNKNOWN;
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct SlotStatusReqPld
{
    SlotStatusCallback cb;
};

struct HighCapabilityReqPld
{
    HighCapabilityCallback cb;
};

struct SwitchSlotPld
{
    int slotId;
    telux::common::ResponseCallback cb;
};

template<typename T>
std::shared_ptr<T>
as(const chart::Event& e)
{
    return std::static_pointer_cast<T>(e.payload);
}

}  // namespace

chart::Status
MultiSimNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
MultiSimReady_St(chart::Hsm*, chart::Event const*);

SimulaMultiSimManager::SimulaMultiSimManager(
  int slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("MultiSimManager")
    , bridge_(bridge)
    , slotId_(slotId)
    , active_slot_(slotId)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaMultiSimManager::~SimulaMultiSimManager()
{
    // Withdraw from the bridge before anything else: the connectivity
    // callback registered in start() captures raw `this` and the bridge
    // holds its own copy. unsubscribe_connectivity is only a queued
    // removal, so the drain() fence inside is what actually makes "no
    // callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaMultiSimManager::unsubscribeFromBridge_()
{
    // No event topics: this Manager's readiness follows bridge connectivity
    // alone (see start()).
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaMultiSimManager::addInitCallback(telux::common::InitResponseCb cb)
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
SimulaMultiSimManager::fireInitCallbacks_(telux::common::ServiceStatus status)
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
            LOG_WARN("[MultiSimManager] init callback fired after PA timeout (%s); dropping",
                     e.what());
        }
        catch (...)
        {
            LOG_WARN("[MultiSimManager] init callback threw unexpectedly; dropping");
        }
    }
}

void
SimulaMultiSimManager::start()
{
    if (running())
        return;
    // Instrumentation must be attached before start_at().
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(MultiSimNotReady_St);

    // Multi-SIM readiness follows bridge connectivity.
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaMultiSimManager::notifySlotStatusChanged(bool cardPresent)
{
    // Notify listeners so the PA refreshes its cached card pointer.
    const int slot = active_slot_.load();
    telux::tel::SlotStatus st{};
    st.slotState  = telux::tel::SlotState::ACTIVE;
    st.cardState  = cardPresent
                    ? telux::tel::CardState::CARDSTATE_PRESENT
                    : telux::tel::CardState::CARDSTATE_ABSENT;
    st.cardError  = telux::tel::CardError::UNKNOWN;

    std::map<SlotId, telux::tel::SlotStatus> statusMap;
    statusMap[SlotId(slot)] = st;

    broadcastToListeners_(
      [statusMap](const std::shared_ptr<telux::tel::IMultiSimListener>& l) {
          l->onSlotStatusChanged(statusMap);
      }
    );
}

void
SimulaMultiSimManager::fetchSlotStatus_(SlotStatusCallback callback)
{
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = slotId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));

    const int slot = active_slot_.load();
    bridge_.send_request(
      topics::sim::get_state::req,
      "sim.get_state.rsp",
      req,
      [callback, slot](std::optional<Envelope> rsp) {
          if (!callback)
              return;

          std::map<SlotId, telux::tel::SlotStatus> slotStatus;
          telux::tel::SlotStatus st{};
          // The simulated physical slot remains active even without a card response.
          st.slotState = telux::tel::SlotState::ACTIVE;
          st.cardError = telux::tel::CardError::UNKNOWN;

          if (!rsp || rsp->error || !rsp->data)
          {
              st.cardState = telux::tel::CardState::CARDSTATE_UNKNOWN;
          }
          else
          {
              st.cardState = wireToCardState(rsp->data->value("cardState", std::string()));
          }

          slotStatus[SlotId(slot)] = st;
          callback(slotStatus, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
}

// ---------------------------------------------------------------------------
// telux::tel::IMultiSimManager

bool
SimulaMultiSimManager::isSubsystemReady()
{
    return ready_flag_.load();
}

std::future<bool>
SimulaMultiSimManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(ready_flag_.load());
    return p.get_future();
}

telux::common::ServiceStatus
SimulaMultiSimManager::getServiceStatus()
{
    return last_status_.load();
}

telux::common::Status
SimulaMultiSimManager::getSlotCount(int& count)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    // The simulation models one slot.
    count = 1;
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaMultiSimManager::requestHighCapability(HighCapabilityCallback callback)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<HighCapabilityReqPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestHighCapability_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaMultiSimManager::setHighCapability(int slotId, common::ResponseCallback callback)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    if (slotId != slotId_ && slotId != DEFAULT_SLOT_ID)
        return telux::common::Status::NOSUCH;
    auto pld = std::make_shared<SwitchSlotPld>();
    pld->slotId = slotId;
    pld->cb = std::move(callback);
    post_fifo({ SetHighCapability_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaMultiSimManager::switchActiveSlot(SlotId slotId, common::ResponseCallback callback)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SwitchSlotPld>();
    pld->slotId = static_cast<int>(slotId);
    pld->cb = std::move(callback);
    post_fifo({ SwitchActiveSlot_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaMultiSimManager::requestSlotStatus(SlotStatusCallback callback)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SlotStatusReqPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestSlotStatus_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaMultiSimManager::registerListener(std::weak_ptr<telux::tel::IMultiSimListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaMultiSimManager::deregisterListener(std::weak_ptr<telux::tel::IMultiSimListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::IMultiSimListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

void
SimulaMultiSimManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::IMultiSimListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "MultiSimManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::tel::IMultiSimListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
MultiSimNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaMultiSimManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case BridgeConnectivityChanged_Signal:
        {
            auto pld = as<bool>(*e);
            if (pld && *pld)
                return self->to(MultiSimReady_St);
            return chart::Status::HANDLED;
        }
        case RequestSlotStatus_Signal:
        case RequestHighCapability_Signal:
        case SetHighCapability_Signal:
        case SwitchActiveSlot_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
MultiSimReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaMultiSimManager*>(h);
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
                  [](const std::shared_ptr<telux::tel::IMultiSimListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->ready_flag_.store(false);
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::IMultiSimListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = as<bool>(*e);
            if (pld && !*pld)
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(MultiSimNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case RequestSlotStatus_Signal:
        {
            auto pld = as<SlotStatusReqPld>(*e);
            self->fetchSlotStatus_(pld->cb);
            return chart::Status::HANDLED;
        }

        case RequestHighCapability_Signal:
        {
            auto pld = as<HighCapabilityReqPld>(*e);
            if (pld->cb)
                pld->cb(self->active_slot_.load(), telux::common::ErrorCode::SUCCESS);
            return chart::Status::HANDLED;
        }

        case SetHighCapability_Signal:
        {
            // The single simulated slot already has highest capability.
            auto pld = as<SwitchSlotPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::SUCCESS);
            return chart::Status::HANDLED;
        }

        case SwitchActiveSlot_Signal:
        {
            auto pld = as<SwitchSlotPld>(*e);
            const int cur = self->active_slot_.load();
            telux::common::ErrorCode code;
            if (pld->slotId == cur)
            {
                code = telux::common::ErrorCode::NO_EFFECT;
            }
            else
            {
                code = telux::common::ErrorCode::SIM_ERR;
            }
            if (pld->cb)
                pld->cb(code);
            if (code == telux::common::ErrorCode::NO_EFFECT)
            {
                self->broadcastToListeners_(
                  [cur](const std::shared_ptr<telux::tel::IMultiSimListener>& l) {
                      l->onHighCapabilityChanged(cur);
                  }
                );
            }
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

// State names for chart::Spy output.
CHART_NAMED_STATE(MultiSimNotReady_St, "MultiSimManager::NotReady");
CHART_NAMED_STATE(MultiSimReady_St,    "MultiSimManager::Ready");

}  // namespace telux::tel::simula
