// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// VlanManager.hpp - SimulaVlanManager, 2-state readiness shell (the same
// shell SimulaDataProfileManager uses: every VLAN operation is a pure
// request/response round-trip with no multi-step lifecycle, so no sub-AO is
// needed here -- contrast DataConnectionManager, which owns per-call
// Sessions).
//
// The VLAN table itself lives on the MPSS side (World State invariant (d)):
// this manager holds no VLAN list of its own and answers queryVlanInfo /
// queryVlanToBackhaulBindings straight from an RPC. That is deliberate --
// caching here would let the PA and MPSS disagree about which VLANs exist.

#ifndef TELUX_DATA_NET_SIMULA_VLAN_MANAGER_HPP
#define TELUX_DATA_NET_SIMULA_VLAN_MANAGER_HPP

#include "../common/IModemBridge.hpp"
#include "../common/InitCallbackGate.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <telux/data/net/VlanManager.hpp>
#include <vector>

namespace telux::data::net::simula {

class SimulaVlanManager final
    : public telux::data::net::IVlanManager
    , private chart::ActiveObject
{
public:
    SimulaVlanManager(
      telux::data::OperationType opType,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaVlanManager() override;

    SimulaVlanManager(const SimulaVlanManager&) = delete;
    SimulaVlanManager& operator=(const SimulaVlanManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // DataFactory after construction.
    void start();

    // Registers an additional InitResponseCb after construction. The target PA
    // calls the factory getter twice -- second call carries the callback that
    // unblocks its 30s promise wait -- so this must be honoured, not dropped.
    void addInitCallback(telux::common::InitResponseCb cb);

    // telux::data::net::IVlanManager
    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::Status createVlan(
      const VlanConfig& vlanConfig, CreateVlanCb callback = nullptr
    ) override;
    telux::common::Status removeVlan(
      int16_t vlanId, InterfaceType ifaceType,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status queryVlanInfo(QueryVlanResponseCb callback) override;
    telux::common::Status bindToBackhaul(
      VlanBindConfig vlanBindConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status unbindFromBackhaul(
      VlanBindConfig vlanBindConfig,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status queryVlanToBackhaulBindings(
      BackhaulType backhaulType, VlanBindingsResponseCb callback,
      SlotId slotId = DEFAULT_SLOT_ID
    ) override;
    telux::common::Status registerListener(
      std::weak_ptr<IVlanListener> listener
    ) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<IVlanListener> listener
    ) override;
    telux::data::OperationType getOperationType() override;
    // Deprecated in the real SDK in favour of bindToBackhaul /
    // unbindFromBackhaul / queryVlanToBackhaulBindings. Implemented rather
    // than stubbed because taf_net's BindVlanWithProfile /
    // GetVlanBoundProfileId still call them: each forwards to its
    // backhaul-based equivalent with backhaul pinned to WWAN.
    telux::common::Status bindWithProfile(
      int profileId, int vlanId,
      telux::common::ResponseCallback callback = nullptr,
      SlotId slotId = DEFAULT_SLOT_ID
    ) override;
    telux::common::Status unbindFromProfile(
      int profileId, int vlanId,
      telux::common::ResponseCallback callback = nullptr,
      SlotId slotId = DEFAULT_SLOT_ID
    ) override;
    telux::common::Status queryVlanMappingList(
      VlanMappingResponseCb callback, SlotId slotId = DEFAULT_SLOT_ID
    ) override;

private:
    friend chart::Status VlanNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status VlanReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status VlanOperating_St(chart::Hsm*, chart::Event const*);

    void handleInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<IVlanListener>&)> invoke
    );
    // Sole authority for readiness: derived from the chart's current-state
    // pointer, exactly as the data-domain managers do it.
    bool isReadyDerived_() const;
    // Single-owner mutator for last_status_, which encodes the
    // FAILED-vs-UNAVAILABLE distinction the 2-state chart doesn't model.
    void publishStatus_(telux::common::ServiceStatus s);
    // Mirror of start()'s registrations, ending in a drain() fence. Called
    // from the dtor before any member teardown.
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    telux::data::OperationType opType_;
    common::simula::InitCallbackGate init_gate_;
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<IVlanListener>> listeners_;
};

}  // namespace telux::data::net::simula

#endif  // TELUX_DATA_NET_SIMULA_VLAN_MANAGER_HPP
