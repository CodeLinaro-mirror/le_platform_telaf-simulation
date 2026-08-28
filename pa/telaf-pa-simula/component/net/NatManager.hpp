// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// NatManager.hpp - SimulaNatManager, 2-state readiness shell.
//
// INatManager exposes each operation twice: a current BackhaulInfo overload and
// a deprecated profileId one. Both are implemented, and both funnel into the
// same RPC -- the deprecated form is BackhaulInfo with backhaul pinned to WWAN
// -- so the two can never drift apart in behaviour.

#ifndef TELUX_DATA_NET_SIMULA_NAT_MANAGER_HPP
#define TELUX_DATA_NET_SIMULA_NAT_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "../common/InitCallbackGate.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <telux/data/net/NatManager.hpp>
#include <vector>

namespace telux::data::net::simula {

class SimulaNatManager final
    : public telux::data::net::INatManager
    , private chart::ActiveObject
{
public:
    SimulaNatManager(
      telux::data::OperationType opType, common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaNatManager() override;

    SimulaNatManager(const SimulaNatManager&) = delete;
    SimulaNatManager& operator=(const SimulaNatManager&) = delete;

    void start();

    // Registers an additional InitResponseCb after construction. The target PA
    // calls the factory getter twice -- second call carries the callback that
    // unblocks its 30s promise wait -- so this must be honoured, not dropped.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::data::net::INatManager
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status addStaticNatEntry(
      const BackhaulInfo& bhInfo, const NatConfig& snatConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status removeStaticNatEntry(
      const BackhaulInfo& bhInfo, const NatConfig& snatConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestStaticNatEntries(
      const BackhaulInfo& bhInfo, StaticNatEntriesCb snatEntriesCb
    ) override;
    telux::common::Status registerListener(std::weak_ptr<INatListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<INatListener> listener) override;
    telux::data::OperationType getOperationType() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    // Deprecated profileId overloads. The trailing SlotId parameter is part of
    // the INatManager signature -- omitting it makes these plain overloads that
    // hide, rather than override, the pure virtuals (leaving the class abstract).
    telux::common::Status addStaticNatEntry(
      int profileId, const NatConfig& snatConfig,
      telux::common::ResponseCallback callback = nullptr,
      SlotId slotId = DEFAULT_SLOT_ID
    ) override;
    telux::common::Status removeStaticNatEntry(
      int profileId, const NatConfig& snatConfig,
      telux::common::ResponseCallback callback = nullptr,
      SlotId slotId = DEFAULT_SLOT_ID
    ) override;
    telux::common::Status requestStaticNatEntries(
      int profileId, StaticNatEntriesCb snatEntriesCb,
      SlotId slotId = DEFAULT_SLOT_ID
    ) override;

private:
    friend chart::Status NatNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status NatReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status NatOperating_St(chart::Hsm*, chart::Event const*);

    void handleInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<INatListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    void unsubscribeFromBridge_();
    // Shared by the BackhaulInfo and deprecated profileId overloads.
    telux::common::Status postEntryOp_(
      bool add, const BackhaulInfo& bh, const NatConfig& cfg,
      telux::common::ResponseCallback cb
    );

    telux::data::OperationType op_type_;
    common::simula::IModemBridge& bridge_;
    common::simula::InitCallbackGate init_gate_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<INatListener>> listeners_;
};

}  // namespace telux::data::net::simula

#endif  // TELUX_DATA_NET_SIMULA_NAT_MANAGER_HPP
