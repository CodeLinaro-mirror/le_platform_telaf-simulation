// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "SocksManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <optional>

namespace telux::data::net::simula {

using namespace NetSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct IndPld
{
    Envelope env;
};

struct EnablePld
{
    bool enable;
    telux::common::ResponseCallback cb;
};

void
completeResponseCb(const telux::common::ResponseCallback& cb, std::optional<Envelope> rsp)
{
    if (!cb)
        return;
    if (!rsp)
    {
        cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
        return;
    }
    if (rsp->error)
    {
        cb(common::simula::parseErrorCode(rsp->error->value("code", std::string())));
        return;
    }
    cb(telux::common::ErrorCode::SUCCESS);
}

}  // namespace

chart::Status SocksNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status SocksReady_St(chart::Hsm*, chart::Event const*);
chart::Status SocksOperating_St(chart::Hsm*, chart::Event const*);

// ============================================================================

SimulaSocksManager::SimulaSocksManager(
  telux::data::OperationType opType, common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("SocksManager")
    , op_type_(opType)
    , bridge_(bridge)
    , init_gate_(std::move(initCb))
{}

SimulaSocksManager::~SimulaSocksManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaSocksManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::net_socks::subsys_ready_net_socks::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaSocksManager::addInitCallback(telux::common::InitResponseCb cb)
{
    init_gate_.add(std::move(cb));
}

void
SimulaSocksManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(SocksNotReady_St);

    bridge_.subscribe_event(
      topics::net_socks::subsys_ready_net_socks::ind, "net_socks.subsys_ready_net_socks.ind",
      [this](std::string_view topic, const Envelope& env) { handleInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaSocksManager::handleInd_(std::string_view, const Envelope& env)
{
    auto pld = std::make_shared<IndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaSocksManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<ISocksListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "SocksManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<ISocksListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaSocksManager::isReadyDerived_() const
{
    return const_cast<SimulaSocksManager*>(this)->current_state() == SocksReady_St;
}

void
SimulaSocksManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::ServiceStatus
SimulaSocksManager::getServiceStatus()
{
    return last_status_.load();
}

telux::data::OperationType
SimulaSocksManager::getOperationType()
{
    return op_type_;
}

bool
SimulaSocksManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaSocksManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::Status
SimulaSocksManager::enableSocks(bool enable, telux::common::ResponseCallback callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<EnablePld>();
    pld->enable = enable;
    pld->cb = std::move(callback);
    post_fifo({ EnableSocks_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaSocksManager::registerListener(std::weak_ptr<ISocksListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaSocksManager::deregisterListener(std::weak_ptr<ISocksListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    for (auto it = listeners_.begin(); it != listeners_.end();)
    {
        auto sp = it->lock();
        if (!sp || (target && sp == target))
            it = listeners_.erase(it);
        else
            ++it;
    }
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
SocksOperating_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaSocksManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
SocksNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaSocksManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(SocksReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        case EnableSocks_Signal:
        {
            auto pld = event_cast<EnablePld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(SocksOperating_St);
    }
}

chart::Status
SocksReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaSocksManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_gate_.markReadyAndFire(
                  telux::common::ServiceStatus::SERVICE_AVAILABLE))
            {
                self->broadcastToListeners_([](const std::shared_ptr<ISocksListener>& l) {
                    l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                });
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_([](const std::shared_ptr<ISocksListener>& l) {
                l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
            });
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(SocksNotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(SocksNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(SocksNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case EnableSocks_Signal:
        {
            auto pld = event_cast<EnablePld>(*e);
            nlohmann::json data = { { "enable", pld->enable } };
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_socks::enable_socks::req, "net_socks.enable_socks.rsp", req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(SocksOperating_St);
    }
}

CHART_NAMED_STATE(SocksNotReady_St,  "SocksManager::NotReady");
CHART_NAMED_STATE(SocksReady_St,     "SocksManager::Ready");
CHART_NAMED_STATE(SocksOperating_St, "SocksManager::Operating");

}  // namespace telux::data::net::simula
