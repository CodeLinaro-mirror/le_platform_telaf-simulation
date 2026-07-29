// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// DataConnectionManager.hpp - SimulaDataCall + SimulaDataCallSession +
// SimulaDataConnectionManager bundled (one file pair per closely-related
// object group, region-banner comments substitute for file-per-class).
//
// SimulaDataCall        - passive value object, mutated only by its owning
//                         Session (mutex-guarded since IDataCall getters
//                         are called synchronously from arbitrary client
//                         threads).
// SimulaDataCallSession - per-call AO, full lifecycle state machine.
// SimulaDataConnectionManager - per-slot AO, 2-state readiness shell,
//                         owns active_sessions_ and
//                         demuxes the shared call_state indication topic
//                         to the right Session by profileId (see note in
//                         the .cpp: IModemBridge::subscribe_event holds one
//                         callback per topic, so per-Session subscription
//                         would silently steal indications across
//                         concurrent calls -- centralizing the
//                         subscription here avoids that).
//
//                         IModemBridge::EventCallback fires on the
//                         bridge's own worker thread, never the Manager's
//                         AO thread -- so the callback wired via
//                         subscribe_event must not touch active_sessions_
//                         directly. It only post_fifo()s a StateInd_Signal
//                         into the Manager's own queue; the actual
//                         profileId->Session lookup and forward happens
//                         inside Ready_St, on the Manager's own AO thread,
//                         same as every other active_sessions_ access.

#ifndef TELUX_DATA_SIMULA_DATA_CONNECTION_MANAGER_HPP
#define TELUX_DATA_SIMULA_DATA_CONNECTION_MANAGER_HPP

#include "../common/IModemBridge.hpp"

#include <chart/active_object.hpp>
#include <chart/defer.hpp>
#include <chart/time_event.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataDefines.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

namespace telux::data::simula {

// ============================================================================
// SimulaDataCall
// ============================================================================
//
// Passive: no thread, no state machine. Every field is guarded by a mutex
// because getters are called synchronously from arbitrary client threads
// while the owning Session mutates fields from its own worker thread.
class SimulaDataCall final : public telux::data::IDataCall
{
public:
    SimulaDataCall(
      common::simula::IModemBridge& bridge,
      int profileId,
      SlotId slotId,
      telux::data::IpFamilyType ipFamilyType,
      telux::data::OperationType operationType,
      std::string interfaceName
    );

    // telux::data::IDataCall
    const std::string& getInterfaceName() override;
    telux::data::DataCallEndReason getDataCallEndReason() override;
    telux::data::DataCallStatus getDataCallStatus() override;
    telux::data::IpFamilyInfo getIpv4Info() override;
    telux::data::IpFamilyInfo getIpv6Info() override;
    telux::data::TechPreference getTechPreference() override;
    std::list<telux::data::IpAddrInfo> getIpAddressInfo() override;
    telux::data::IpFamilyType getIpFamilyType() override;
    int getProfileId() override;
    SlotId getSlotId() override;
    telux::data::OperationType getOperationType() override;
    telux::common::Status requestTrafficFlowTemplate(
      telux::data::IpFamilyType ipFamilyType,
      telux::data::TrafficFlowTemplateCb callback
    ) override;
    telux::common::Status requestDataCallStatistics(
      telux::data::StatisticsResponseCb callback = nullptr
    ) override;
    telux::common::Status resetDataCallStatistics(
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestDataCallBitRate(
      telux::data::requestDataCallBitRateResponseCb callback
    ) override;
    telux::data::DataBearerTechnology getCurrentBearerTech() override;

    // Mutators. Called only from the owning Session's worker thread.
    void setStatus(telux::data::DataCallStatus status);
    void setEndReason(telux::common::DataCallEndReason reason);
    void setIpv4(telux::data::IpFamilyInfo info);
    void setIpv6(telux::data::IpFamilyInfo info);
    void setInterfaceName(std::string name);
    void setTechPreference(telux::data::TechPreference tech);
    void setBearerTech(telux::data::DataBearerTechnology bearer);

private:
    // Only used by requestDataCallBitRate to dispatch its RPC; every other
    // member here is the passive data this call carries, mutated only by
    // the owning Session.
    common::simula::IModemBridge& bridge_;
    mutable std::mutex m_;
    int profileId_;
    SlotId slotId_;
    telux::data::IpFamilyType ipFamilyType_;
    telux::data::OperationType operationType_;
    std::string interfaceName_;
    telux::data::TechPreference techPreference_{ telux::data::TechPreference::UNKNOWN };
    telux::data::DataBearerTechnology bearerTech_{ telux::data::DataBearerTechnology::UNKNOWN };
    telux::data::DataCallStatus status_{ telux::data::DataCallStatus::NET_IDLE };
    telux::common::DataCallEndReason endReason_{};
    telux::data::IpFamilyInfo ipv4_{};
    telux::data::IpFamilyInfo ipv6_{};
};

// ============================================================================
// SimulaDataCallSession
// ============================================================================
//
// Per-call AO (own thread per in-flight call, no Manager-owned in-flight
// table). Owns the full startDataCall/stopDataCall lifecycle including the
// Reconciling verify-then-land step on stop failure/timeout.
class SimulaDataCallSession final
    : private chart::ActiveObject
    , public std::enable_shared_from_this<SimulaDataCallSession>
{
public:
    SimulaDataCallSession(
      common::simula::IModemBridge& bridge,
      SlotId slotId,
      int profileId,
      telux::data::IpFamilyType ipFamilyType,
      telux::data::OperationType operationType,
      std::string interfaceName,
      std::function<void(std::shared_ptr<telux::data::IDataCall>)> notifyListeners,
      std::function<void(int profileId)> onFinished
    );
    ~SimulaDataCallSession();

    SimulaDataCallSession(const SimulaDataCallSession&) = delete;
    SimulaDataCallSession& operator=(const SimulaDataCallSession&) = delete;

    // Kicks off Idle -> SendingStartReq. Called exactly once, immediately
    // after construction, by the owning Manager.
    void start(telux::data::DataCallResponseCb startCb);

    // Forwarded from Manager's StopDataCall_Signal handler.
    void requestStop(telux::data::DataCallResponseCb stopCb);

    // Forwarded from Manager's call_state indication demux (see the note
    // on IModemBridge::subscribe_event at the top of this file).
    void deliverStateInd(const common::simula::Envelope& env);

    std::shared_ptr<telux::data::IDataCall> dataCall() const { return call_; }

private:
    // State handlers (chart-style free functions). Friends so they can
    // reach private members directly.
    friend chart::Status Idle_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Active_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SendingStartReq_St(chart::Hsm*, chart::Event const*);
    friend chart::Status WaitingForUp_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Connected_St(chart::Hsm*, chart::Event const*);
    friend chart::Status SendingStopReq_St(chart::Hsm*, chart::Event const*);
    friend chart::Status WaitingForDown_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Reconciling_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Terminal_St(chart::Hsm*, chart::Event const*);

    common::simula::IModemBridge& bridge_;
    SlotId slotId_;
    int profileId_;
    telux::data::OperationType operationType_;
    std::shared_ptr<SimulaDataCall> call_;

    telux::data::DataCallResponseCb startCb_;
    telux::data::DataCallResponseCb stopCb_;
    std::function<void(std::shared_ptr<telux::data::IDataCall>)> notifyListeners_;
    std::function<void(int)> onFinished_;

    chart::TimeEvent bringup_timeout_;
    chart::TimeEvent teardown_timeout_;
    chart::DeferQueue deferred_;
};

// ============================================================================
// SimulaDataConnectionManager
// ============================================================================
//
// Per-slot AO, 2-state readiness shell. Owns
// active_sessions_ (AO-thread-only, no lock needed) and listeners_
// (mutex-guarded: registerListener/deregisterListener run on arbitrary
// client threads, broadcastDataCallInfoChanged runs on a Session's worker
// thread -- neither is the Manager's own AO thread).
class SimulaDataConnectionManager final
    : public telux::data::IDataConnectionManager
    , private chart::ActiveObject
{
public:
    SimulaDataConnectionManager(
      SlotId slotId,
      common::simula::IModemBridge& bridge,
      telux::common::InitResponseCb initCb = nullptr
    );
    ~SimulaDataConnectionManager() override;

    SimulaDataConnectionManager(const SimulaDataConnectionManager&) = delete;
    SimulaDataConnectionManager& operator=(const SimulaDataConnectionManager&) = delete;

    // Boots the AO and wires up bridge subscriptions. Called once by
    // DataFactory after construction.
    void start();

    // telux::data::IDataConnectionManager
    telux::common::ServiceStatus getServiceStatus() override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::Status setDefaultProfile(
      telux::data::OperationType operationType,
      uint8_t profileId,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status getDefaultProfile(
      telux::data::OperationType operationType,
      telux::data::DefaultProfileIdResponseCb callback
    ) override;
    telux::common::Status setRoamingMode(
      bool enable,
      uint8_t profileId,
      telux::data::OperationType operationType,
      telux::common::ResponseCallback callback = nullptr
    ) override;
    telux::common::Status requestRoamingMode(
      uint8_t profileId,
      telux::data::OperationType operationType,
      telux::data::requestRoamingModeResponseCb callback
    ) override;
    telux::common::Status startDataCall(
      const telux::data::DataCallParams& dataCallParams,
      telux::data::DataCallResponseCb callback = nullptr
    ) override;
    telux::common::Status stopDataCall(
      const telux::data::DataCallParams& dataCallParams,
      telux::data::DataCallResponseCb callback = nullptr
    ) override;
    telux::common::Status registerListener(
      std::weak_ptr<telux::data::IDataConnectionListener> listener,
      telux::data::DataConnectionIndications indicationList = telux::data::DEFAULT_INDICATIONS
    ) override;
    telux::common::Status deregisterListener(
      std::weak_ptr<telux::data::IDataConnectionListener> listener,
      telux::data::DataConnectionIndications indicationList = telux::data::DEFAULT_INDICATIONS
    ) override;
    int getSlotId() override;
    telux::common::Status requestDataCallList(
      telux::data::OperationType operationType,
      telux::data::DataCallListResponseCb callback
    ) override;
    telux::common::Status startDataCall(
      int profileId,
      telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4V6,
      telux::data::DataCallResponseCb callback = nullptr,
      telux::data::OperationType operationType = telux::data::OperationType::DATA_LOCAL,
      std::string apn = ""
    ) override;
    telux::common::Status stopDataCall(
      int profileId,
      telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4V6,
      telux::data::DataCallResponseCb callback = nullptr,
      telux::data::OperationType operationType = telux::data::OperationType::DATA_LOCAL,
      std::string apn = ""
    ) override;
    telux::common::Status requestThrottledApnInfo(telux::data::ThrottleInfoCb callback) override;
    telux::common::ErrorCode setThroughputInterval(uint32_t reportInterval) override;
    telux::common::ErrorCode getLastThroughputInfo(
      std::vector<telux::data::ThroughputInfo>& info
    ) override;

    // Called by SimulaDataCallSession (via broadcastDataCallInfoChanged) to
    // fan out onDataCallInfoChanged. Thread-safe; may be called from any
    // Session's worker thread.
    void broadcastDataCallInfoChanged(std::shared_ptr<telux::data::IDataCall> call);

private:
    friend chart::Status NotReady_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Ready_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Operating_St(chart::Hsm*, chart::Event const*);

    // Wired to IModemBridge::subscribe_event for the call_state indication
    // topic. Fires on the bridge's own worker thread -- must not touch
    // active_sessions_ here. Just forwards the envelope via post_fifo into
    // this Manager's own queue as StateInd_Signal; Ready_St does the
    // profileId->Session lookup on the AO thread.
    void handleCallStateInd_(std::string_view topic, const common::simula::Envelope& env);
    void broadcastToListeners_(
      telux::data::DataConnectionIndicationsType bit,
      std::function<void(const std::shared_ptr<telux::data::IDataConnectionListener>&)> invoke
    );
    // Mirror of start()'s bridge registrations, ending in a drain() fence.
    // Called from the dtor before any member teardown -- see the dtor.
    void unsubscribeFromBridge_();
    // Single owner for readiness truth: derives Ready/NotReady from the
    // chart's current state pointer (T-13: shadow atomic collapsed).
    bool isReadyDerived_() const;
    // Single-owner mutator for last_status_. Kept as an atomic because it
    // encodes the FAILED-vs-UNAVAILABLE reason distinction that the 2-state
    // chart itself does not model (see field comment below).
    void publishStatus_(telux::common::ServiceStatus s);

    common::simula::IModemBridge& bridge_;
    SlotId slotId_;
    telux::common::InitResponseCb init_cb_;
    bool init_cb_fired_{ false };
    // Handle for the connectivity observer registered in start(), withdrawn
    // in the dtor. 0 until start() runs, which unsubscribe_connectivity
    // treats as a no-op.
    common::simula::IModemBridge::ConnectivityToken conn_token_{ 0 };
    // Last readiness reason distinct from the 2-chart-state simplification,
    // so getServiceStatus() can still report SERVICE_FAILED distinctly from
    // SERVICE_UNAVAILABLE without adding a third chart state.
    std::atomic<telux::common::ServiceStatus> last_status_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE
    };

    // AO-thread-only: touched exclusively from state handlers.
    std::unordered_map<int, std::shared_ptr<SimulaDataCallSession>> active_sessions_;

    // Cross-thread: registerListener/deregisterListener (client thread) vs
    // broadcastDataCallInfoChanged (a Session's worker thread).
    std::mutex listeners_mutex_;
    std::vector<std::pair<std::weak_ptr<telux::data::IDataConnectionListener>,
                          telux::data::DataConnectionIndications>>
      listeners_;
};

}  // namespace telux::data::simula

#endif  // TELUX_DATA_SIMULA_DATA_CONNECTION_MANAGER_HPP
