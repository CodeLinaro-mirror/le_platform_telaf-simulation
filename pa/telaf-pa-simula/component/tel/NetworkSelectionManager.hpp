// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//


#ifndef TELUX_TEL_SIMULA_NETWORK_SELECTION_MANAGER_HPP
#define TELUX_TEL_SIMULA_NETWORK_SELECTION_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <telux/tel/NetworkSelectionManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaNetworkSelectionManager final
    : public telux::tel::INetworkSelectionManager
    , private chart::ActiveObject
{
public:
    SimulaNetworkSelectionManager(
      int slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaNetworkSelectionManager() override;

    SimulaNetworkSelectionManager(const SimulaNetworkSelectionManager&) = delete;
    SimulaNetworkSelectionManager& operator=(const SimulaNetworkSelectionManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // PhoneFactoryStub after construction.
    void start();

    // Re-arms the init callback on an already-constructed manager, so the
    // factory's cache-hit path never drops a caller's InitResponseCb.
    void setInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::INetworkSelectionManager
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status
    requestNetworkSelectionMode(telux::tel::SelectionModeInfoCb callback) override;
    telux::common::Status requestNetworkSelectionMode(
      telux::tel::SelectionModeResponseCallback callback
    ) override;
    telux::common::Status setNetworkSelectionMode(
      telux::tel::NetworkSelectionMode selectMode,
      std::string mcc,
      std::string mnc,
      telux::common::ResponseCallback callback = nullptr
    ) override;

    telux::common::Status
    requestPreferredNetworks(telux::tel::PreferredNetworksCallback callback) override;
    telux::common::Status setPreferredNetworks(
      std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo,
      bool clearPrevious,
      telux::common::ResponseCallback callback = nullptr
    ) override;

    telux::common::Status performNetworkScan(telux::tel::NetworkScanCallback callback) override;
    telux::common::Status performNetworkScan(
      telux::tel::NetworkScanInfo info,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::ErrorCode abortNetworkScan() override;

    telux::common::ErrorCode
    setLteDubiousCell(const std::vector<telux::tel::LteDubiousCell>& lteDbCellList) override;
    telux::common::ErrorCode
    setNrDubiousCell(const std::vector<telux::tel::NrDubiousCell>& nrDbCellList) override;

    telux::common::Status
    registerListener(std::weak_ptr<telux::tel::INetworkSelectionListener> listener) override;
    telux::common::Status
    deregisterListener(std::weak_ptr<telux::tel::INetworkSelectionListener> listener) override;

private:
    friend chart::Status NetSelNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status NetSelReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::INetworkSelectionListener>&)> invoke
    );
    bool isReadyDerived_() const;
    void publishStatus_(telux::common::ServiceStatus s);
    // Mirror of start()'s bridge registrations, ending in a drain() fence.
    // Called from the dtor before any member teardown -- see the dtor.
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    // Handle for the connectivity observer registered in start(), withdrawn
    // in the dtor. 0 until start() runs, which unsubscribe_connectivity
    // treats as a no-op.
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    int slotId_;
    // Every not-yet-notified init callback. A NotReady-period caller appends
    // here (SetInitCb_Signal, tel/NetworkSelectionManager.cpp); NetSelReady_St's
    // Entry drains and fires all of them. AO-thread-only after construction,
    // so no separate mutex is needed (see setInitCallback()'s comment).
    std::vector<telux::common::InitResponseCb> init_cbs_;
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::INetworkSelectionListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_NETWORK_SELECTION_MANAGER_HPP
