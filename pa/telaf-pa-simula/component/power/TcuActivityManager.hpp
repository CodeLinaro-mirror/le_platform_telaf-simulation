// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// TcuActivityManager.hpp - SimulaTcuActivityManager, the master/slave TCU
// activity-state business AO.
//
// 2-state readiness shell (NotReady_St/Ready_St) mirroring SimulaWakeupManager
// (invariant h). Master role issues setActivityState/getAllMachineNames RPCs;
// slave role publishes sendActivityStateAck as fire-and-forget. Both roles
// subscribe to state_update/machine_update/slave_ack_status indications and
// demux them to registered ITcuActivityListeners via ListenerDispatchAO.

#ifndef TELUX_POWER_SIMULA_TCU_ACTIVITY_MANAGER_HPP
#define TELUX_POWER_SIMULA_TCU_ACTIVITY_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <chart/defer.hpp>
#include <memory>
#include <mutex>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <vector>

namespace telux::power::simula {

class SimulaTcuActivityManager final
    : public telux::power::ITcuActivityManager
    , private chart::ActiveObject
{
public:
    SimulaTcuActivityManager(
      common::simula::IModemBridge& bridge,
      telux::power::ClientInstanceConfig config,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaTcuActivityManager() override;

    SimulaTcuActivityManager(const SimulaTcuActivityManager&) = delete;
    SimulaTcuActivityManager& operator=(const SimulaTcuActivityManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // PowerFactory after construction.
    void start();

    // telux::power::ITcuActivityManager
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(
      std::weak_ptr<telux::power::ITcuActivityListener> listener
    ) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<telux::power::ITcuActivityListener> listener
    ) override;
    telux::common::Status registerServiceStateListener(
      std::weak_ptr<telux::common::IServiceStatusListener> listener
    ) override;
    telux::common::Status deregisterServiceStateListener(
      std::weak_ptr<telux::common::IServiceStatusListener> listener
    ) override;
    telux::common::Status getMachineName(std::string& machineName) override;
    telux::common::Status getAllMachineNames(std::vector<std::string>& machineNames) override;
    telux::common::Status setActivityState(
      telux::power::TcuActivityState state,
      std::string machineName,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::ErrorCode getActivityState(
      std::string machineName,
      telux::power::TcuActivityState& state
    ) override;
    telux::common::Status sendActivityStateAck(
      telux::power::StateChangeResponse ack,
      telux::power::TcuActivityState state
    ) override;
    telux::common::Status setModemActivityState(telux::power::TcuActivityState state) override;
    bool isReady() override;
    std::future<bool> onReady() override;
    telux::common::Status setActivityState(
      telux::power::TcuActivityState state,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status sendActivityStateAck(telux::power::TcuActivityStateAck ack) override;
    telux::power::TcuActivityState getActivityState() override;

private:
    friend chart::Status NotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Ready_St(chart::Hsm*, chart::Event const*);

    // Wired to IModemBridge::subscribe_event for each of the three ind
    // topics. Fires on the bridge's own worker thread -- just forwards via
    // post_fifo into this Manager's own queue; the actual listener fan-out
    // happens on the AO thread inside Ready_St.
    void handleStateUpdateInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleMachineUpdateInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleSlaveAckStatusInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(std::function<void(const std::shared_ptr<telux::power::ITcuActivityListener>&)> invoke);
    // Fires a client ResponseCallback off the AO worker thread (via
    // ListenerDispatchAO), so a caller that blocks inside its callback cannot
    // stall this AO's own indication dispatch.
    void dispatchResponse_(telux::common::ResponseCallback cb, telux::common::ErrorCode error);
    void broadcastServiceState_(telux::common::ServiceStatus status);
    // Mirror of start()'s bridge registrations, ending in a drain() fence.
    // Called from the dtor before any member teardown -- see the dtor.
    void unsubscribeFromBridge_();
    // Single owner for readiness truth: derives Ready/NotReady from the
    // chart's current state pointer, never a shadow bool/atom (invariant h).
    bool isReadyDerived_() const;

    common::simula::IModemBridge& bridge_;
    telux::power::ClientInstanceConfig config_;
    telux::common::InitResponseCb init_cb_;
    bool init_cb_fired_{ false };
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };

    // AO-thread-only: touched exclusively from state handlers.
    telux::power::TcuActivityState local_state_{ telux::power::TcuActivityState::UNKNOWN };

    // Cross-thread: register/deregisterListener (client thread) vs
    // broadcastToListeners_ (bridge worker thread, via post_fifo -> AO
    // thread dispatch, but the listener vector itself is also read from
    // registerServiceStateListener on arbitrary client threads).
    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::power::ITcuActivityListener>> listeners_;
    std::vector<std::weak_ptr<telux::common::IServiceStatusListener>> service_listeners_;
};

}  // namespace telux::power::simula

#endif  // TELUX_POWER_SIMULA_TCU_ACTIVITY_MANAGER_HPP
