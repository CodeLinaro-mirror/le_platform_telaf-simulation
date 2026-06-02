/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataConnectionManager.hpp
 *
 * @brief      DataConnectionManager is a primary interface for cellular connectivity. This
 *             interface provides APIs for start and stop data call connections, get data
 *             call information and add or remove listeners for monitoring data calls
 *             status.
 *
 */

#ifndef TELUX_DATA_DATACONNECTIONMANAGER_HPP
#define TELUX_DATA_DATACONNECTIONMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/IpFilter.hpp>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataConnectionListener;
class IDataCall;

/**
 * IP Family related Info
 */
struct IpFamilyInfo {
    DataCallStatus status;  /**< Data call status */
    IpAddrInfo addr;       /**< IP address info */
};

/**
 * Encapsulate the Qos Filter rule
 */
struct QosFilterRule{
    std::vector<std::shared_ptr<IIpFilter>> filter; /**< @ref IIpFilter */
    uint16_t filterId;                              /**< Unique identifier for each filter. */
    uint16_t filterPrecedence;                      /**< Specifies the order in which filters are applied. A lower numerical value has a higher precedence. */
};

/**
 * QOS TFT Flow info
 */
struct TrafficFlowTemplate {
    /** Mandatory */
    QosFlowId qosId;                            /**< defines current flow id */
    QosFlowStateChangeEvent stateChange;        /**< Flow state change event */

    QosFlowMask mask;                           /**< bitmask to denote which of the optional fields in TrafficFlowTemplate are valid */
    /** Optional */
    QosIPFlowInfo txGrantedFlow;                /**< Tx Granted Flow IP info */
    QosIPFlowInfo rxGrantedFlow;                /**< Rx Granted Flow IP info */

    uint32_t txFiltersLength;                   /**< Tx Filters length */
    QosFilterRule txFilters[MAX_QOS_FILTERS];   /**< Tx QoS Filters that apply to a
                                                   granted Tx QoS flow. */

    uint32_t rxFiltersLength;                   /**< Rx Filters length*/
    QosFilterRule rxFilters[MAX_QOS_FILTERS];   /**< Rx QoS Filters that apply to a
                                                   granted Rx QoS flow. */
};

/**
 * QOS TFT flow change info
 */
struct TftChangeInfo {
    std::shared_ptr<TrafficFlowTemplate> tft;   /**< TFT flow info @ref TrafficFlowTemplate */
    QosFlowStateChangeEvent stateChange;        /**< Flow state change event */
};

/**
 * Data call bit rate info
 */
struct BitRateInfo {
    uint64_t txRate;      /**< @deprecated Unused */
    uint64_t rxRate;      /**< @deprecated Unused */
    uint64_t maxTxRate;   /**< Maximum transmit rate that can be assigned to device in bits/sec */
    uint64_t maxRxRate;   /**< Maximum receive rate that can be assigned to device in bits/sec  */
};

/**
 * Throttle information for the corresponding APN
 */
struct APNThrottleInfo {
    std::string apn;                            /**< APN name */
    std::vector<int> profileIds;                /**< Profile IDs with the same APN */
    uint32_t ipv4Time;                          /**< Remaining IPv4 throttled time in milliseconds*/
    uint32_t ipv6Time;                          /**< Remaining IPv6 throttled time in milliseconds*/
    bool isBlocked;                             /**< Is APN blocked on all plmns */
    std::string mcc;                            /**< Mobile Country Code */
    std::string mnc;                            /**< Mobile Network Code */
};

/**
 * Data call related parameters
 */
struct DataCallParams {
    int profileId;                      /** Profile identifier corresponding to which data call
                                            bring up will be done. Use
                                            @ref IDataProfileManager::requestProfileList to get
                                            the list of available profiles. */
    std::string interfaceName = "";     /** Optional name to be assigned to the network interface
                                            representing this data call. This option is not
                                            supported on all platforms. See
                                            @ref  telux::data::DataCallResponseCb for the error
                                            returned when not supported. It is intended to be
                                            used with @ref telux::data::startDataCall
                                            only and is ignored if used with
                                            @ref telux::data::stopDataCall*/
    IpFamilyType ipFamilyType = IpFamilyType::IPV4V6;        /** Identifies IP family type */
    OperationType operationType = OperationType::DATA_LOCAL; /** Optional
                                                                 @ref telux::data::OperationType*/
};

/**
 * Represents uplink throughput information for a profile.
 */
struct UplinkThroughputInfo {
    uint32_t throughput;                       /**< Current uplink throughput in kbps. */
    uint32_t maxThroughput;                    /**< Maximum allowed uplink throughput in kbps. */
    uint32_t queueSize;                        /**< Number of bytes pending in the uplink queue. */
};

/**
 * Represents downlink throughput information for a profile.
 */
struct DownlinkThroughputInfo {
    uint32_t throughput;                       /**< Current downlink throughput in kbps. */
};

/**
 * Represents combined uplink and downlink throughput information for a profile.
 */
struct ThroughputInfo {
    SlotId slot;                   /**< Slot on which throughput info is received. */
    int profileId;                 /**< Profile identifier for which throughput info is received. */
    UplinkThroughputInfo ulThroughput;         /**< Uplink throughput details. */
    DownlinkThroughputInfo dlThroughput;       /**< Downlink throughput details. */
};

/** Enum of all the possible indications invoked by a data connection listener.  */
enum DataConnectionIndicationsType {
    /** Register to receive below updates -
     * DataCallInfoChanged -
     * @ref telux::data::IDataConnectionListener::onDataCallInfoChanged
     * HwAccelerationChanged -
     * @ref telux::data::IDataConnectionListener::onHwAccelerationChanged
     * TrafficFlowTemplateChange -
     * @ref telux::data::IDataConnectionListener::onTrafficFlowTemplateChange
     * WwanConnectivityConfigChange -
     * @ref telux::data::IDataConnectionListener:: onWwanConnectivityConfigChange
     * ThrottledApnInfoChanged -
     * @ref telux::data::IDataConnectionListener::onThrottledApnInfoChanged
     * SSR indication -
     * @ref telux::data::IDataConnectionListener::onServiceStatusChange
    */
    DEFAULT = 0,
    /** Register to receive throughput updates -
     * @ref telux::data::IDataConnectionListener::onThroughputInfoAvailable
     */
    THROUGHPUT
};

/** This bitset represents the list of the data connection indications selected by the client. */
using DataConnectionIndications = std::bitset<32>;

/**
 * Default indication list subscribes to DEFAULT indications only (excludes THROUGHPUT).
 */
const DataConnectionIndications DEFAULT_INDICATIONS =
    DataConnectionIndications().set(DataConnectionIndicationsType::DEFAULT);

/**
 * This function is called with the response to startDataCall / stopDataCall API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * When callback is used with startDataCall, expected behavior is as following:
 *  - If this is first client to start datacall in the system and no error is detected,
 *    getDataCallStatus() of @ref telux::data::IDataCall object will return NET_CONNECTING and
 *    onDataCallInfoChanged will be called once data call is brought up successfully or failed.
 *  - If client tries to start data call that is already up and no error is detected,
 *    getDataCallStatus() of @ref telux::data::IDataCall object will return NET_CONNECTED and
 *    onDataCallInfoChanged will not get called.
 *  - If any client start data call and error is detected, error argument will contain error
 *    code and onDataCallInfoChanged will not get called.

 * When callback is used with stopDataCall, expected behavior is as following:
 *  - First/Last client that attempts to stop data call and no error is detected,
 *    getDataCallStatus() of @ref telux::data::IDataCall object will return NET_DISCONNECTING and
 *    onDataCallInfoChanged will be called once data call is down.
 *  - If a client attempt to stop data call it started earlier while same data call is started by
 *    other clients in the system, and no error is detected, getDataCallStatus() of
 *    @ref telux::data::IDataCall object will return NET_CONNECTED and onDataCallInfoChanged will
 *    not get called.
 *  - If any client attemp to stop data call and error detected, error argument will contain error
 *    code and onDataCallInfoChanged will not get called.
 *  - If the client tries to start a data call on specific profile id and network interface name and:
*       - If data call already exists with the same profile ID but a different network interface
 *        then error will be set to telux::ErrorCode::DEVICE_IN_USE and onDataCallInfoChanged
 *        will not get called.
 *      - If data call already exists with the same network interface but different profile ID then
 *        error will be set to telux::ErrorCode::INVALID_OPERATION and onDataCallInfoChanged will
 *        not get called.
 *      - Another client has already requested to tear down an existing data call with
 *        the same profile ID and network interface name and this operation is in progress, an
 *        telux::ErrorCode::OP_IN_PROGRESS error occurs and onDataCallInfoChanged will not be
 *        called.
 *  - On platforms where a client is not allowed to specify the name of the network interface
 *    name when starting a data call, the callback will be invoked with error
 *    telux::ErrorCode::REQUEST_NOT_SUPPORTED and onDataCallInfoChanged will not be called.
 *
 *  Note: Telsdk broadcasts changes in any data call status to all available clients through the
 *        listener interface telux::data::IDataConnectionListener::onDataCallInfoChanged.
 *        Therefore, clients could get notification about data calls and their status changes that
 *        might not be relevent to the client. Client could also get successful connection
 *        notifications of data call client previously failed to connect.
 *        The notification could be a consequence of another client activity or system is set to
 *        automatically reconnect failed data calls.
 *
 * @param [in] dataCall        Pointer to IDataCall
 * @param [in] error           Return code for whether the operation succeeded or failed
 *
 */
using DataCallResponseCb = std::function<void(
    const std::shared_ptr<IDataCall> &dataCall, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestDataCallStatistics API.
 * Returned dataStats will contain data call statistics @ref telux::data::DataCallStats if reported
 * error is telux::common::ErrorCode::SUCCESS. If error is reported, content of dataStats is
 * don't care
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] dataStats       Data Call statistics
 * @param [in] error           Return code for whether the operation succeeded or failed
 */
using StatisticsResponseCb
    = std::function<void(const DataCallStats dataStats, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestDataCallList API.
 * Returned dataCallList will contain current active and connected DataCall objects if reported
 * error is telux::common::ErrorCode::SUCCESS. If error detected, contents of dataCallList is
 * don't care
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] dataCall        vector of of IDataCall list
 * @param [in] error           Return code for whether the operation succeeded or failed
 *
 */
using DataCallListResponseCb = std::function<void(
    const std::vector<std::shared_ptr<IDataCall>> &dataCallList, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to getDefaultProfile API.
 * Returned profileId is current default profile contained in slotId if reported error is
 * telux::common::ErrorCode::SUCCESS. If error detected, content of profileId and slotId is
 * don't care.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] profileId       Current default profile id
 * @param [in] slotId          Slot id that contains current default profile id
 * @param [in] error           Return code for whether the operation
 *                             succeeded or failed
 */
using DefaultProfileIdResponseCb
    = std::function<void(int profileId, SlotId slotId, telux::common::ErrorCode error)>;

/**
 * This function is called in the response to requestTrafficFlowTemplate().
 *
 * @param [in] tft        Vector of TFT flow info. @ref TrafficFlowTemplate
 * @param [in] error      Code which indicates whether the operation succeeded or not.
 *                        @ref ErrorCode.
 */
using TrafficFlowTemplateCb =
    std::function<void(const std::vector<std::shared_ptr<TrafficFlowTemplate>> &tft,
        telux::common::ErrorCode error)>;

/**
 * This function is called in response to requestDataCallBitRate.
 * Returned bitRate will contain data call bit rates @ref telux::data::BitRateInfo if reported
 * error is telux::common::ErrorCode::SUCCESS. If error is reported, content of bitRate is
 * don't care
*
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] bitRate         Bit Rate Info for requested data call
 * @param [in] error           Return code for whether the operation succeeded or failed
 *
 */
using requestDataCallBitRateResponseCb
    = std::function<void(BitRateInfo& bitRate, telux::common::ErrorCode error)>;

/**
 * This function is called in response to requestRoamingMode.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] isRoamingEnabled    True - roaming mode enabled, False - roaming mode disabled.
 * @param [in] profileId           Profile Id on which roaming mode is enabled.
 * @param [in] error               Return code for whether the operation succeeded or failed.
 *
 */
using requestRoamingModeResponseCb =
    std::function<void(bool isRoamingEnabled, int profileId, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestThrottledApnInfo API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] throttleInfoList         List of APNThrottleInfo
 * @param [in] error                    Return code for whether the operation
 *                                      succeeded or failed
 */
using ThrottleInfoCb = std::function<void(
    const std::vector<APNThrottleInfo> &throttleInfoList,
    telux::common::ErrorCode error)>;

/**
 *@brief IDataConnectionManager is a primary interface for cellular connectivity
 *       This interface provides APIs for start and stop data call connections,
 *       get data call information and listener for monitoring data calls.
 *       It also provides interface to Subsystem Restart events by registering as listener.
 *       Notifications will be received when modem is ready/not ready.
 */
class IDataConnectionManager {
 public:
    /**
     * Returns current initialization status of data connection manager.
     *
     * @returns SERVICE_AVAILABLE    If data connection manager is ready for service.
     *          SERVICE_UNAVAILABLE  If data connection manager is temporarily unavailable.
     *          SERVICE_FAILED       If data connection manager encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if the data subsystem is ready.
     *
     * @returns True if Data Connection Manager is ready for service, otherwise
     * returns false.
     *
     *  @deprecated Use getServiceStatus API.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for data subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when card manager is ready.
     *
     * @deprecated Use InitResponseCb callback in factory API getDataConnectionManager.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * Set a profile as default which results in following:
    * Traffic from devices tethered to MDM via default network interfaces such as eth0, ecm0, and
    * mhi0 will be directed to rmnet_data that was brought up with default profile.
    * Traffic initiated within MDM or EAP that is destined for WAN network and is not bound to a
    * WAN interface will be routed by default to WAN network corresponding to the default profile ID
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
    * to invoke this API successfully.
    *
    * @param [in] operationType     @ref telux::data::OperationType
    * @param [in] profileId         Profile identifier to be set as default
    * @param [in] callback          optional callback to get the response setDefaultProfile
    *
    * @returns Immediate status of setDefaultProfile i.e. success or suitable status.
    *
    */
   virtual telux::common::Status setDefaultProfile(OperationType operationType, uint8_t profileId,
       telux::common::ResponseCallback callback = nullptr)  = 0;

   /**
    * Get current default profile
    *
    * @param [in] operationType     @ref telux::data::OperationType
    * @param [in] callback          callback to get the response getDefaultProfile
    *
    * @returns Immediate status of getDefaultProfile i.e. success or suitable status.
    *
    */
   virtual telux::common::Status getDefaultProfile(
       OperationType operationType, DefaultProfileIdResponseCb callback)  = 0;

    /**
     * Enable roaming mode for profile id. If disabled, any client attempt to bring up data call on
     * such profile id will be prevented by system when device is in roaming area. System will
     * report NO_NETWORK_FOUND error in such scenario.
     * if enabled, clients can bring up data call made on such profile id and slot id successfully
     * even if device is in roaming area.
     * Configuration changes will be persistant across multiple boots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] enable         enable/disable roaming mode (True: enable, False:disable).
     * @param [in] profileId      profile id on which roaming mode to be enabled/disabled.
     * @param [in] operationType  @ref telux::data::OperationType
     * @param [in] callback       optional. Callback to get response for enableRoamingMode.
     *
     * @returns Status of enableRoamingMode i.e. success or suitable status code.
     *           NO_NETWORK_FOUND error is returned if roaming is not enabled on profile id.
     *
     */
    virtual telux::common::Status setRoamingMode(bool enable, uint8_t profileId,
        OperationType operationType, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request current roaming mode for profile id.
     *
     * @param [in] profileId      profile id on which roaming mode is requested.
     * @param [in] operationType  @ref telux::data::OperationType
     * @param [in] callback       callback to get response for requestRoamingMode.
     *
     * @returns Status of requestRoamingMode i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestRoamingMode(uint8_t profileId, OperationType operationType,
        requestRoamingModeResponseCb callback) = 0;

    /**
     * Starts a data call corresponding to default or specified profile identifier.
     *
     * This will bring up data call connection based on specified profile identifier, IP family
     * type, and operation type (local/remote). Clients can also specify a network interface
     * name that will be associated with the data call to be started. This is an asynchronous API.
     * If telux::common::Status::SUCCESS is returned, client provided callback will be invoked at
     * later time with error code and DataCall object associated with requested call.
     * Clients might receive additional notification for the final data call status. For details
     * see @ref telux::data::DataCallResponseCb.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DATA_CALL_OPS
     * permission to successfully invoke this API.
     *
     * @note       if application starts data call on IPV4V6 then it's expected to stop the
     *             data call on same ip family type (i.e IPV4V6).
     *
     * @param [in] dataCallParams     Data call parameters to be specified
     *                                @ref telux::data::DataCallParams
     *
     * @param [out] callback          Optional callback to get the start data call response.
     *
     * @returns Immediate status of startDataCall() request sent i.e. success or suitable
     *          status code.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to
     *                 change and could break backwards compatibility.
     */
    virtual telux::common::Status startDataCall(const DataCallParams &dataCallParams,
        DataCallResponseCb callback = nullptr) = 0;

    /**
     * Tear down data call connection based on specified profile identifier, IP family
     * type, and operation type (local/remote). This is an asynchronous API.
     * If telux::common::Status::SUCCESS is returned, client provided callback will be invoked at
     * later time with error code and DataCall object associated with requested call.
     * Clients might receive additional notification for the final data call status. For details
     * see @ref telux::data::DataCallResponseCb.
     *
     * This will tear down specific data call connection based on profile identifier.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DATA_CALL_OPS
     * permission to successfully invoke this API.
     *
     * @note       If application starts data call on IPV4V6 then it's expected to stop the
     *             data call on same ip family type (i.e IPV4V6).
     *             Client can only stop data call it started.
     *
     * @param [in] dataCallParams     Data call parameters to be specified
     *                                @ref telux::data::DataCallParams
     *
     * @param [out] callback          Optional callback to get the stop data call response.
     *
     * @returns Immediate status of stopDataCall() request sent i.e. success or
     *          suitable status code. The client receives asynchronous notifications
     *          indicating the data call tear-down.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to
     *                 change and could break backwards compatibility.
     */
    virtual telux::common::Status stopDataCall(const DataCallParams &dataCallParams,
        DataCallResponseCb callback = nullptr) = 0;

    /**
     * Register a listener for specific events in the Connection Manager like establishment of new
     * data call, data call info change and call failure.
     *
     * @param [in] listener        Pointer of IDataConnectionListener object that processes the
     *                             notification
     * @param [in] indicationList  Optional list of specific indications to register for.
     *                             If not provided, registers for all default indications
     *                             @ref telux::data::DataConnectionIndicationsType
     *
     * @returns Status of registerListener success or suitable status code
     *
     * @note    Eval: This is a new API signature and is being evaluated. It is subject to change
     *          and could break backward compatibility.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDataConnectionListener> listener,
        DataConnectionIndications indicationList = DEFAULT_INDICATIONS) = 0;

    /**
     * Removes a previously added listener. Deregisters the listener from the list of indications
     * provided via  @ref telux::data::DataConnectionIndicationsType . If not provided, deregisters
     * from all default indications  @ref telux::data::DataConnectionIndicationsType::DEFAULT
     *
     * For example- Client registers for both DEFAULT and THROUGHPUT indication.
     * During dergisteration, if client only provides DEFAULT indication in
     * the indication list, the listener will be deregistered from the DEFAULT indication
     * but will still be registered to the THROUGHPUT indication.
     *
     * @param [in] listener        Pointer of IDataConnectionListener object that has to be removed
     * @param [in] indicationList  Optional list of specific indications to deregister from.
     *                             If not provided, deregisters from all default indications
     *                             @ref telux::data::DataConnectionIndicationsType
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     * @note    Eval: This is a new API signature and is being evaluated. It is subject to change
     *          and could break backward compatibility.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDataConnectionListener> listener,
        DataConnectionIndications indicationList = DEFAULT_INDICATIONS) = 0;

    /**
     * Get associated slot id for the Data Connection Manager.
     *
     * @returns SlotId
     *
     */
    virtual int getSlotId() = 0;

    /**
     * Request list of data calls available in the system
     *
     * @param [out] operationType    @ref telux::data::OperationType
     * @param [out] callback         Callback with list of supported data calls
     *
     */
    virtual telux::common::Status requestDataCallList(OperationType operationType,
        DataCallListResponseCb callback) = 0;

    /**
     * Starts a data call corresponding to the default or specified profile identifier.
     *
     * This brings up a data call connection based on the specified profile identifier, IP family
     * type, and operation type (local/remote). This is an asynchronous API.
     * If telux::common::Status::SUCCESS is returned, the client-provided callback is invoked
     * later with the error code and DataCall object associated with the requested call.
     * Clients might receive additional notifications for the final data call status. For
     * details see @ref telux::data::DataCallResponseCb.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DATA_CALL_OPS
     * permission to successfully invoke this API.
     *
     * @note       If an application starts a data call on IPv4v6 then it's expected to stop the
     *             data call on the same IP family type (i.e. IPv4v6).
     *
     * @param [in] profileId     Profile identifier corresponding to which data call to bring up.
     *                           Use IDataProfileManager::requestProfileList to get the list of
     *                           available profiles.
     * @param [in] ipFamilyType  Identifies IP family type
     * @param [out] callback     Optional callback to get the start data call response.
     * @param [in] operationType Optional @ref telux::data::OperationType
     * @param [in] apn           Deprecated and currently unused
     *
     * @returns The immediate status of the startDataCall() request sent, i.e., success or the
     *          suitable status code.
     *
     * @deprecated Use @ref telux::data::startDataCall(const DataCallParams&, DataCallResponseCb)
     * to start data calls.
     */
    virtual telux::common::Status startDataCall(int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::IPV4V6, DataCallResponseCb callback = nullptr,
        OperationType operationType = OperationType::DATA_LOCAL, std::string apn = "") = 0;

    /**
     * Tears down a data call connection based on the specified profile identifier, IP family
     * type, operation type (local/remote). This is an asynchronous API.
     * If telux::common::Status::SUCCESS is returned, the client-provided callback is invoked
     * later with the error code and DataCall object associated with the requested call.
     * Clients might receive additional notifications for the final data call status. For
     * details see @ref telux::data::DataCallResponseCb.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DATA_CALL_OPS
     * permission to successfully invoke this API.
     *
     * @note       If an application starts a data call on IPv4v6 then it's expected to stop the
     *             data call on same IP family type (i.e. IPv4v6).
     *             Clients can only stop data call they started.
     *
     * @param [in] profileId     Profile identifier corresponding to the data call
     *                           to tear down. Use data profile manager to get the
     *                           list of available profiles.
     * @param [in] ipFamilyType  Identifies IP family type
     * @param [out] callback     Optional callback to get the stop data call response
     * @param [in] operationType Optional @ref telux::data::OperationType
     * @param [in] apn           Deprecated and currently unused
     *
     * @returns The immediate status of the stopDataCall() request sent, i.e., success or
     *          the suitable status code. The client receives asynchronous notifications
     *          indicating the data call tear-down.
     *
     * @deprecated Use @ref telux::data::stopDataCall(const DataCallParams&, DataCallResponseCb)
     * to stop a data call.
     */
    virtual telux::common::Status stopDataCall(int profileId,
        IpFamilyType ipFamilyType = IpFamilyType::IPV4V6, DataCallResponseCb callback = nullptr,
        OperationType operationType = OperationType::DATA_LOCAL, std::string apn = "") = 0;

    /**
     * Request information about APNs that are throttled by the network
     * Any attempt to start a data call will fail if the respective APN is throttled.
     *
     * The provided callback will be invoked with information about all the APNs that are throttled.
     * It also specifies the time for which the APN is being throttled. Clients should wait until
     * that timer expires before attempting a data call on that APN. Otherwise, the data call will
     * be rejected. Any update in the throttled state will be informed via
     * @ref IDataConnectionListener::onThrottledApnInfoChanged.
     *
     * @param [out] callback         Callback with a list of APN throttle information
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual telux::common::Status requestThrottledApnInfo(ThrottleInfoCb callback) = 0;

    /**
     * Configure the interval for generating periodic uplink and downlink throughput reports.
     * Reports are delivered by @ref telux::data::IDataConnectionListener::onThroughputInfoAvailable
     *
     * @note The minimum allowed interval is 50 ms. Passing a value of 0 disables generation
     * of throughput reports. The application of this interval is a global setting.
     *
     * On platforms with Access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] reportInterval  Interval in milliseconds (ms) for throughput reporting.
     *
     * @return telux::common::ErrorCode
     *         - `SUCCESS` if the interval was set successfully.
     *         - Appropriate error code if the operation fails.
     *
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual telux::common::ErrorCode setThroughputInterval(uint32_t reportInterval) = 0;

    /**
     * Retrieves the most recent uplink and downlink throughput information
     * for all the active profiles.
     *
     * @param [out] info           A list of throughput information for all the active profiles.
     *
     * @return telux::common::ErrorCode
     *         - `SUCCESS` if the throughput info was retrieved successfully.
     *         - Appropriate error code if the operation fails.
     *
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual telux::common::ErrorCode getLastThroughputInfo(std::vector<ThroughputInfo> &info) = 0;

    /**
     * Destructor for IDataConnectionManager
     */
    virtual ~IDataConnectionManager(){};
};  // end of IDataConnectionManager

/**
 * @brief Represents single established data call on the device.
 *
 */
class IDataCall {
 public:
    /**
     * Get interface name associated with the data call.
     *
     * @returns Interface Name.
     *
     */
    virtual const std::string &getInterfaceName() = 0;

    /**
     * Get failure reason for the data call.
     *
     * @returns @ref DataCallEndReason
     *
     */
    virtual DataCallEndReason getDataCallEndReason() = 0;

    /**
     * Get data call status like connected, disconnected and IP address changes.
     *
     * @returns @ref DataCallStatus.
     *
     */
    virtual DataCallStatus getDataCallStatus() = 0;

    /**
     * Get IPv4 Family info like connected, disconnected and IP address changes.
     *
     * @returns @ref IpFamilyInfo.
     *
     */
    virtual IpFamilyInfo getIpv4Info() = 0;

    /**
     * Get IPv6 Family info like connected, disconnected and IP address changes.
     *
     * @returns @ref IpFamilyInfo.
     *
     */
    virtual IpFamilyInfo getIpv6Info() = 0;

    /**
     * Get the technology on which the call was brought up.
     *
     * @returns @ref TechPreference.
     *
     */
    virtual TechPreference getTechPreference() = 0;

    /**
     * Get list of IP address information.
     *
     * @returns List of IP address details.
     *
     */
    virtual std::list<IpAddrInfo> getIpAddressInfo() = 0;

    /**
     * Get IP Family Type i.e. IPv4, IPv6 or Both
     *
     * @returns @ref IpFamilyType.
     *
     */
    virtual IpFamilyType getIpFamilyType() = 0;

    /**
     * Get Profile Id
     *
     * @returns Profile Identifier.
     *
     */
    virtual int getProfileId() = 0;

    /**
     * Get Slot Id
     *
     * @returns Subscription Slot Identifier.
     *
     */
    virtual SlotId getSlotId() = 0;

    /**
     * Get data operation used for the DataCall.
     *
     * @returns @ref OperationType
     */
    virtual OperationType getOperationType() = 0;

    /**
     * Get the current installed QOS Traffic flow template information.
     *
     * @param [in]  ipFamilyType    - IP Family type @ref IpFamilyType. TFT's are installed per IP
     *                                Family.
     * @param [in]  callback        - callback function to get the result of API.
     *
     * @returns Status of requestTrafficFlowTemplate i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestTrafficFlowTemplate(IpFamilyType ipFamilyType,
        TrafficFlowTemplateCb callback) = 0;

    /**
     * Request the data transfer statistics for data call corresponding
     * to specified profile identifier.
     *
     * @param [in] callback    Optional callback to get the response of request Data Call
     *                         Statistics
     *
     * @returns Status of getDataCallStatistics i.e. success or suitable status code.
     */
    virtual telux::common::Status requestDataCallStatistics(StatisticsResponseCb callback = nullptr)
        = 0;

    /**
     * Reset data transfer statistics for data call corresponding to specified profile identifier.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_CALL_PROPS
     * permission to invoke this API successfully.
     *
     * @param [in] callback   optional callback to get the response of reset Data call statistics
     *
     * @returns Status of resetDataCallStatistics i.e. success or suitable status code.
     */
    virtual telux::common::Status resetDataCallStatistics(
        telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request data call bit rate in (bits/sec).
     *
     * @param [out] callback     callback to be called with bit rate results
     *                           @ref requestDataCallBitRateResponseCb
     *
     * @returns Status of requestDataCallBitRate success or suitable status code
     *
     */
    virtual telux::common::Status requestDataCallBitRate(
        requestDataCallBitRateResponseCb callback) = 0;

    /**
     * Destructor for IDataCall
     */
    virtual ~IDataCall(){};

    /**
     * Get the bearer technology on which earlier data call was brought up like LTE, WCDMA and etc.
     * This is synchronous API called by client to get bearer technology corresponding to data call.
     *
     * @returns @ref DataBearerTechnology
     *
     * @deprecated, use telux::data::IServingSystemManager::requestServiceStatus instead
     *
     */
    virtual DataBearerTechnology getCurrentBearerTech() = 0;
};

/**
 * Interface for Data connection listener object. Client needs to implement this interface to get
 * access to data services notifications like onNewDataCall, onDataCallStatusChanged and
 * onDataCallFailure.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 * The notification delivery mechanism uses the same thread to deliver all the queued notifications
 * to ensure they are delivered in order.
 * Considering this, the thread on which the notifications are delivered should not be blocked for
 * longer operations since this would result in delay in delivery of further notifications that are
 * in the queue waiting to be dispatched.
 *
 */
class IDataConnectionListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * This function is called when there is a change in the data call.
     *
     * @param [in] dataCall   Pointer to IDataCall
     *
     */
    virtual void onDataCallInfoChanged(const std::shared_ptr<IDataCall> &dataCall){};

    /**
     * This function is called when a change occur in hardware acceleration service.
     * If reported state is @ref ServiceState::INACTIVE:
     *   All existing data calls will take software path.
     * If reported state is @ref ServiceState::ACTIVE:
     *   All existing and new data calls will be hardware accelerated.
     *
     * @param [in] state   New state of hardware Acceleration service (Active/Inactive)
     *
     * @note  This is global state
     */
    virtual void onHwAccelerationChanged(const ServiceState state){};

    /**
     * This function is called when the TFT's parameters are changed for a packet data session.
     *
     * @param [in] dataCall     Pointer to IDataCall
     * @param [in] tft          vector of TftChangeInfo @ref TftChangeInfo
     *
     */
    virtual void onTrafficFlowTemplateChange(const std::shared_ptr<IDataCall> &dataCall,
        const std::vector<std::shared_ptr<TftChangeInfo>> &tft) {};

    /**
     * This function is called when WWAN backhaul connectivity config changes.
     *
     * @param [in] slotId                - Slot Id for which connectivity has changed.
     * @param [in] isConnectivityAllowed - Connectivity status allowed/disallowed.
     *
     */
    virtual void onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) {}

    /**
     * This function is called when the throttled state changes, such as when a new APN is throttled
     * or an existing throttled APN is no longer throttled after the timeout.
     * APNs that are not throttled anymore will not appear in the list of throttled APNs.
     *
     * @param [out] throttleInfoList   List of all APN throttle information.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onThrottledApnInfoChanged(const std::vector<APNThrottleInfo> &throttleInfoList){};

    /**
     * This function is invoked periodically at the interval configured via
     * @ref telux::data::IDataConnectionManager::setThroughputInterval API
     * to provide the latest uplink and downlink throughput information for all the active profiles.
     *
     * This indication will be triggered only when:
     * (1) A client registers for throughput updates using
     *     @ref telux::data::IDataConnectionManager::registerListener with
     *     @ref DataConnectionIndicationsType::THROUGHPUT bit set in the indication list.
     *
     * (2) At least one client has requested throughput reporting by calling
     *     @ref telux::data::IDataConnectionManager::setThroughputInterval.
     *
     * @param [out] info           A list of throughput information for all the active profiles.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onThroughputInfoAvailable(const std::vector<ThroughputInfo> &info) {};

    /**
     * Destructor for IDataConnectionListener
     */
    virtual ~IDataConnectionListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif // TELUX_DATA_DATACONNECTIONMANAGER_HPP
