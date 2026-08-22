// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// ServingSystemManager.hpp - SimulaServingSystemManager, per-slot AO,
// 2-state readiness shell (same shell as DataConnectionManager/
// DataProfileManager; Ready state forwards service/roaming/nrIconType/
// dormant RPCs -- pure request/response, no multi-step lifecycle, so no
// sub-AO needed).

#ifndef TELUX_DATA_SIMULA_SERVING_SYSTEM_MANAGER_HPP
#define TELUX_DATA_SIMULA_SERVING_SYSTEM_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <telux/data/ServingSystemManager.hpp>
#include <vector>

namespace telux::data::simula {

class SimulaServingSystemManager final
    : public telux::data::IServingSystemManager
    , private chart::ActiveObject
{
public:
    SimulaServingSystemManager(
      SlotId slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaServingSystemManager() override;

    SimulaServingSystemManager(const SimulaServingSystemManager&) = delete;
    SimulaServingSystemManager& operator=(const SimulaServingSystemManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // DataFactory after construction.
    void start();

    // telux::data::IServingSystemManager
    telux::common::ServiceStatus getServiceStatus() override;
    telux::data::DrbStatus getDrbStatus() override;
    telux::common::Status requestServiceStatus(
      telux::data::RequestServiceStatusResponseCb callback
    ) override;
    telux::common::Status requestRoamingStatus(
      telux::data::RequestRoamingStatusResponseCb callback
    ) override;
    telux::common::Status requestNrIconType(
      telux::data::RequestNrIconTypeResponseCb callback
    ) override;
    telux::common::Status makeDormant(
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status registerListener(
      std::weak_ptr<telux::data::IServingSystemListener> listener
    ) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<telux::data::IServingSystemListener> listener
    ) override;
    SlotId getSlotId() override;

private:
    friend chart::Status ServNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status ServReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleServStateInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleServRoamingInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::data::IServingSystemListener>&)> invoke
    );
    // Single owner for readiness truth: derives Ready/NotReady from the
    // chart's current state pointer (see DataProfileManager for rationale).
    bool isReadyDerived_() const;
    // Single-owner mutator for last_status_ (orthogonal FAILED-vs-UNAVAILABLE
    // reason bit not modeled by the 2-state chart itself).
    void publishStatus_(telux::common::ServiceStatus s);
    // Single-owner mutator for drb_status_ (data-radio-bearer flag,
    // orthogonal to the chart's readiness state — driven off the wire's
    // serv_state indication, not off state transitions).
    void publishDrb_(telux::data::DrbStatus d);
    // Mirror of start()'s bridge registrations, ending in a drain() fence.
    // Called from the dtor before any member teardown -- see the dtor.
    void unsubscribeFromBridge_();

    common::simula::IModemBridge& bridge_;
    SlotId slotId_;
    telux::common::InitResponseCb init_cb_;
    bool init_cb_fired_{ false };
    // Handle for the connectivity observer registered in start(), withdrawn
    // in the dtor. 0 until start() runs, which unsubscribe_connectivity
    // treats as a no-op.
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };
    // Last-observed DRB status, updated only from ServStateEvt_Signal --
    // no dedicated drb_status wire event exists, so this simulator tracks it
    // purely off whatever the serv_state indication's payload carries
    // (defaults to UNKNOWN until the first indication).
    std::atomic<telux::data::DrbStatus> drb_status_{ telux::data::DrbStatus::UNKNOWN };

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::data::IServingSystemListener>> listeners_;
};

}  // namespace telux::data::simula

#endif  // TELUX_DATA_SIMULA_SERVING_SYSTEM_MANAGER_HPP
