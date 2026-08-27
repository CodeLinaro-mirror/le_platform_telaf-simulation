// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "WakeupManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <algorithm>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>

namespace telux::power::simula {

using common::simula::Envelope;
using common::simula::event_cast;

namespace {

struct EnvPld
{
    Envelope env;
};

telux::power::QmiWakeupInfo
decodeQmiWakeupInfo(const nlohmann::json& data)
{
    telux::power::QmiWakeupInfo info{};
    info.serviceId = data.value("serviceId", 0u);
    info.sourceNodeId = data.value("sourceNodeId", 0u);
    info.destinationNodeId = data.value("destinationNodeId", 0u);
    info.isMsgIdValid = data.value("isMsgIdValid", false);
    info.msgId = data.value("msgId", 0u);
    info.isPIDValid = data.value("isPIDValid", false);
    info.pid = data.value("pid", 0);
    info.isProcessNameValid = data.value("isProcessNameValid", false);
    info.processName = data.value("processName", std::string());
    return info;
}

}  // namespace

chart::Status
WakeupNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
WakeupReady_St(chart::Hsm*, chart::Event const*);

SimulaWakeupManager::SimulaWakeupManager(
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("WakeupManager")
    , bridge_(bridge)
    , init_cb_(std::move(initCb))
{}

SimulaWakeupManager::~SimulaWakeupManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaWakeupManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::power::subsys_ready_wakeup::ind);
    bridge_.unsubscribe_event(topics::power::wakeup::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaWakeupManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(WakeupNotReady_St);

    bridge_.subscribe_event(
      topics::power::subsys_ready_wakeup::ind,
      "power.subsys_ready_wakeup.ind",
      [this](std::string_view topic, const Envelope& env) { handleWakeupInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::power::wakeup::ind,
      "power.wakeup.ind",
      [this](std::string_view topic, const Envelope& env) { handleWakeupInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ PowerSignals::BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaWakeupManager::handleWakeupInd_(std::string_view topic, const Envelope& env)
{
    auto pld = std::make_shared<EnvPld>();
    pld->env = env;
    if (topic == topics::power::subsys_ready_wakeup::ind)
        post_fifo({ PowerSignals::ReadinessEvt_Signal, pld });
    else
        post_fifo({ PowerSignals::WakeupInd_Signal, pld });
}

void
SimulaWakeupManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::power::IWakeupListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "WakeupManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::power::IWakeupListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaWakeupManager::isReadyDerived_() const
{
    return const_cast<SimulaWakeupManager*>(this)->current_state() == WakeupReady_St;
}

// ---------------------------------------------------------------------------
// telux::power::IWakeupManager

telux::common::ErrorCode
SimulaWakeupManager::registerListener(std::weak_ptr<telux::power::IWakeupListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode
SimulaWakeupManager::deRegisterListener(std::weak_ptr<telux::power::IWakeupListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(), listeners_.end(),
        [&target](const std::weak_ptr<telux::power::IWakeupListener>& w) {
            auto sp = w.lock();
            return !sp || sp == target;
        }
      ),
      listeners_.end()
    );
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ServiceStatus
SimulaWakeupManager::getServiceStatus()
{
    return isReadyDerived_() ? telux::common::ServiceStatus::SERVICE_AVAILABLE
                              : telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
WakeupNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaWakeupManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case PowerSignals::ReadinessEvt_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(WakeupReady_St);
            return chart::Status::HANDLED;
        }
        case PowerSignals::BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
WakeupReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaWakeupManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            if (self->init_cb_)
            {
                self->init_cb_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                self->init_cb_ = nullptr;
            }
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;

        case PowerSignals::ReadinessEvt_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "UNAVAILABLE")
                return self->to(WakeupNotReady_St);
            return chart::Status::HANDLED;
        }

        case PowerSignals::BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
                return self->to(WakeupNotReady_St);
            return chart::Status::HANDLED;
        }

        case PowerSignals::WakeupInd_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto& data = *pld->env.data;
            telux::power::WakeupInfo info{};
            info.wakeupType = data.value("wakeupType", std::string()) == "QMI"
                                 ? telux::power::WakeupType::QMI
                                 : telux::power::WakeupType::UNKNOWN;
            if (data.contains("qmiWakeupInfo"))
                info.qmiWakeupInfo = decodeQmiWakeupInfo(data["qmiWakeupInfo"]);
            self->broadcastToListeners_(
              [info](const std::shared_ptr<telux::power::IWakeupListener>& l) {
                  l->onWakeup(info);
              }
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

}  // namespace telux::power::simula
