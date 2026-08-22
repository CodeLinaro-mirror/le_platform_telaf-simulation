// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// DataProfileManager.hpp - SimulaDataProfileManager, per-slot AO, 2-state
// readiness shell (same shell as SimulaDataConnectionManager; Ready state
// forwards profile RPCs -- pure request/response, no multi-step lifecycle,
// so no sub-AO is needed here).

#ifndef TELUX_DATA_SIMULA_DATA_PROFILE_MANAGER_HPP
#define TELUX_DATA_SIMULA_DATA_PROFILE_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <telux/data/DataProfileManager.hpp>
#include <vector>

namespace telux::data::simula {

class SimulaDataProfileManager final
    : public telux::data::IDataProfileManager
    , private chart::ActiveObject
{
public:
    SimulaDataProfileManager(
      SlotId slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaDataProfileManager() override;

    SimulaDataProfileManager(const SimulaDataProfileManager&) = delete;
    SimulaDataProfileManager& operator=(const SimulaDataProfileManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // DataFactory after construction.
    void start();

    // telux::data::IDataProfileManager
    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::Status requestProfileList(
      std::shared_ptr<telux::data::IDataProfileListCallback> callback = nullptr
    ) override;
    telux::common::Status createProfile(
      const telux::data::ProfileParams& profileParams,
      std::shared_ptr<telux::data::IDataCreateProfileCallback> callback = nullptr
    ) override;
    telux::common::Status deleteProfile(
      uint8_t profileId,
      telux::data::TechPreference techPreference,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status modifyProfile(
      uint8_t profileId,
      const telux::data::ProfileParams& profileParams,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr
    ) override;
    telux::common::Status queryProfile(
      const telux::data::ProfileParams& profileParams,
      std::shared_ptr<telux::data::IDataProfileListCallback> callback = nullptr
    ) override;
    telux::common::Status requestProfile(
      uint8_t profileId,
      telux::data::TechPreference techPreference,
      std::shared_ptr<telux::data::IDataProfileCallback> callback = nullptr
    ) override;
    int getSlotId() override;
    telux::common::Status registerListener(
      std::weak_ptr<telux::data::IDataProfileListener> listener
    ) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<telux::data::IDataProfileListener> listener
    ) override;

private:
    friend chart::Status ProfileNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status ProfileReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleProfileChangedInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::data::IDataProfileListener>&)> invoke
    );
    // Single owner for readiness truth: derives Ready/NotReady from the
    // chart's current state pointer. Safe cross-thread on x86-64 (aligned
    // pointer read); worst case a reader sees pre- or post-transition,
    // which is exactly what an atomic bool would have offered.
    bool isReadyDerived_() const;
    // Single-owner mutator for last_status_ (orthogonal to the 2-state
    // chart: encodes the FAILED-vs-UNAVAILABLE reason distinction that the
    // chart itself does not model). Called from state handlers only.
    void publishStatus_(telux::common::ServiceStatus s);
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

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::data::IDataProfileListener>> listeners_;
};

}  // namespace telux::data::simula

#endif  // TELUX_DATA_SIMULA_DATA_PROFILE_MANAGER_HPP
