// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// WakeupManager.hpp - SimulaWakeupManager, a thin subscribe-and-relay
// business AO for wakeup indications.
//
// Simpler than SimulaTcuActivityManager: no master/slave RPCs, just one
// indication topic to demux and one readiness topic to gate it. Still uses a
// chart:: HSM readiness shell (invariant h) even though it carries no other
// business logic, so readiness is never a bare bool.

#ifndef TELUX_POWER_SIMULA_WAKEUP_MANAGER_HPP
#define TELUX_POWER_SIMULA_WAKEUP_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/power/WakeupManager.hpp>
#include <vector>

namespace telux::power::simula {

class SimulaWakeupManager final
    : public telux::power::IWakeupManager
    , private chart::ActiveObject
{
public:
    explicit SimulaWakeupManager(
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaWakeupManager() override;

    SimulaWakeupManager(const SimulaWakeupManager&) = delete;
    SimulaWakeupManager& operator=(const SimulaWakeupManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // PowerFactory after construction.
    void start();

    // telux::power::IWakeupManager
    telux::common::ErrorCode registerListener(std::weak_ptr<telux::power::IWakeupListener> listener) override;
    telux::common::ErrorCode deRegisterListener(std::weak_ptr<telux::power::IWakeupListener> listener) override;
    telux::common::ServiceStatus getServiceStatus() override;

private:
    friend chart::Status WakeupNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status WakeupReady_St(chart::Hsm*, chart::Event const*);

    void handleWakeupInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(std::function<void(const std::shared_ptr<telux::power::IWakeupListener>&)> invoke);
    void unsubscribeFromBridge_();
    bool isReadyDerived_() const;

    common::simula::IModemBridge& bridge_;

    telux::common::InitResponseCb init_cb_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::power::IWakeupListener>> listeners_;
};

}  // namespace telux::power::simula

#endif  // TELUX_POWER_SIMULA_WAKEUP_MANAGER_HPP
