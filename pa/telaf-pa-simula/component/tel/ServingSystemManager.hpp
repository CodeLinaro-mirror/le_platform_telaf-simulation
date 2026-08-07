// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// ServingSystemManager.hpp - SimulaTelServingSystemManager, the MQTT-driven
// telux::tel::IServingSystemManager implementation.


#ifndef TELUX_TEL_SIMULA_SERVING_SYSTEM_MANAGER_HPP
#define TELUX_TEL_SIMULA_SERVING_SYSTEM_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <memory>
#include <mutex>
#include <telux/tel/ServingSystemManager.hpp>
#include <vector>

namespace telux::tel::simula {

class SimulaTelServingSystemManager final
    : public telux::tel::IServingSystemManager
    , private chart::ActiveObject
{
public:
    SimulaTelServingSystemManager(
      int slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaTelServingSystemManager() override;

    SimulaTelServingSystemManager(const SimulaTelServingSystemManager&) = delete;
    SimulaTelServingSystemManager& operator=(const SimulaTelServingSystemManager&) = delete;

 
    void start();

    void setInitCallback(telux::common::InitResponseCb cb);

    // telux::tel::IServingSystemManager
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status setRatPreference(
      telux::tel::RatPreference ratPref,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status
    requestRatPreference(telux::tel::RatPreferenceCallback callback) override;

    telux::common::Status setServiceDomainPreference(
      telux::tel::ServiceDomainPreference serviceDomain,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestServiceDomainPreference(
      telux::tel::ServiceDomainPreferenceCallback callback
    ) override;

    telux::common::Status getSystemInfo(telux::tel::ServingSystemInfo& sysInfo) override;
    telux::tel::DcStatus getDcStatus() override;

    telux::common::Status
    requestNetworkTime(telux::tel::NetworkTimeResponseCallback callback) override;
    telux::common::Status
    requestLteSib16NetworkTime(telux::tel::NetworkTimeResponseCallback callback) override;
    telux::common::Status
    requestNr5gRrcUtcTime(telux::tel::NetworkTimeResponseCallback callback) override;

    telux::common::Status requestRFBandInfo(telux::tel::RFBandInfoCallback callback) override;
    telux::common::Status
    getNetworkRejectInfo(telux::tel::NetworkRejectInfo& rejectInfo) override;
    telux::common::Status
    getCallBarringInfo(std::vector<telux::tel::CallBarringInfo>& barringInfo) override;
    telux::common::Status
    getSmsCapabilityOverNetwork(telux::tel::SmsCapability& smsCapability) override;
    telux::common::Status getLteCsCapability(telux::tel::LteCsCapability& lteCapability) override;

    telux::common::Status
    requestRFBandPreferences(telux::tel::RFBandPrefCallback callback) override;
    telux::common::Status setRFBandPreferences(
      std::shared_ptr<telux::tel::IRFBandList> prefList,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status
    requestRFBandCapability(telux::tel::RFBandCapabilityCallback callback) override;

    telux::common::ErrorCode setHplmnSearchTime(uint32_t time) override;
    telux::common::ErrorCode getHplmnSearchTime(uint32_t& time) override;

    telux::common::Status registerListener(
      std::weak_ptr<telux::tel::IServingSystemListener> listener,
      telux::tel::ServingSystemNotificationMask mask = ALL_NOTIFICATIONS
    ) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<telux::tel::IServingSystemListener> listener,
      telux::tel::ServingSystemNotificationMask mask = ALL_NOTIFICATIONS
    ) override;

private:
    friend chart::Status TelServNotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status TelServReady_St(chart::Hsm*, chart::Event const*);

    void handleReadinessInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleSysInfoInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleDcStatusInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleRatPrefInd_(std::string_view topic, const common::simula::Envelope& env);
    void handleLteCsCapabilityInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      std::function<void(const std::shared_ptr<telux::tel::IServingSystemListener>&)> invoke
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
    // here (SetInitCb_Signal, tel/ServingSystemManager.cpp); TelServReady_St's
    // Entry drains and fires all of them. AO-thread-only after construction,
    // so no separate mutex is needed (see setInitCallback()'s comment).
    std::vector<telux::common::InitResponseCb> init_cbs_;
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };
    
    std::mutex sys_info_mutex_;
    telux::tel::ServingSystemInfo last_sys_info_{};
    telux::tel::DcStatus last_dc_status_{
        telux::tel::EndcAvailability::UNKNOWN, telux::tel::DcnrRestriction::UNKNOWN
    };
    
    telux::tel::LteCsCapability last_lte_cs_capability_{telux::tel::LteCsCapability::UNKNOWN};

    std::mutex listeners_mutex_;
    std::vector<std::weak_ptr<telux::tel::IServingSystemListener>> listeners_;
};

}  // namespace telux::tel::simula

#endif  // TELUX_TEL_SIMULA_SERVING_SYSTEM_MANAGER_HPP
