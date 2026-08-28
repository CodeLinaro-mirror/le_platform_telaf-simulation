// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// L2tpManager.hpp - SimulaL2tpManager, 2-state readiness shell (same shell as
// SimulaVlanManager / SimulaDataProfileManager: every L2TP operation is a pure
// request/response round-trip, so no sub-AO is needed).
//
// Tunnel and session tables live MPSS-side (World State invariant (d)):
// requestConfig is answered straight from an RPC, never from a local cache, so
// the PA and MPSS cannot disagree about which tunnels exist.

#ifndef TELUX_DATA_NET_SIMULA_L2TP_MANAGER_HPP
#define TELUX_DATA_NET_SIMULA_L2TP_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "../common/InitCallbackGate.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <telux/data/net/L2tpManager.hpp>
#include <vector>

namespace telux::data::net::simula {

class SimulaL2tpManager final
    : public telux::data::net::IL2tpManager
    , private chart::ActiveObject
{
public:
    explicit SimulaL2tpManager(
      common::simula::IModemBridge& bridge, telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaL2tpManager() override;

    SimulaL2tpManager(const SimulaL2tpManager&) = delete;
    SimulaL2tpManager& operator=(const SimulaL2tpManager&) = delete;

    void start();

    // Registers an additional InitResponseCb after construction. The target PA
    // calls the factory getter twice -- second call carries the callback that
    // unblocks its 30s promise wait -- so this must be honoured, not dropped.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::data::net::IL2tpManager
    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::Status setConfig(
      bool enable, bool enableMss, bool enableMtu,
      telux::common::ResponseCallback callback = nullptr, uint32_t mtuSize = 0
    ) override;
    telux::common::Status addTunnel(
      const L2tpTunnelConfig& l2tpTunnelConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestConfig(L2tpConfigCb l2tpConfigCb) override;
    telux::common::Status removeTunnel(
      uint32_t tunnelId, telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status addSession(
      uint32_t tunnelId, L2tpSessionConfig sessionConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status removeSession(
      uint32_t tunnelId, uint32_t sessionId,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status bindSessionToBackhaul(
      L2tpSessionBindConfig sessionBindConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status unbindSessionFromBackhaul(
      L2tpSessionBindConfig sessionBindConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status querySessionToBackhaulBindings(
      BackhaulType backhaul, L2tpSessionBindingsResponseCb callback
    ) override;
    telux::common::Status registerListener(std::weak_ptr<IL2tpListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IL2tpListener> listener) override;

private:
    friend chart::Status L2tpNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status L2tpReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status L2tpOperating_St(chart::Hsm*, chart::Event const*);

    void handleInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<IL2tpListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    common::simula::InitCallbackGate init_gate_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<IL2tpListener>> listeners_;
};

}  // namespace telux::data::net::simula

#endif  // TELUX_DATA_NET_SIMULA_L2TP_MANAGER_HPP
