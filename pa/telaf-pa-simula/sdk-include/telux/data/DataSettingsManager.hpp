/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataSettingsManager.hpp
 *
 * @brief      Data Settings Manager class provides the interface to data subsystem settings.
 */

#ifndef TELUX_DATA_DATASETTINGSMANAGER_HPP
#define TELUX_DATA_DATASETTINGSMANAGER_HPP

#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataSettingsListener;

/**
 * Set priority between N79 5G and Wlan 5GHz Band
 */
enum class BandPriority {
    N79  = 0 ,              /**< N79 has higher priority  */
    WLAN = 1 ,              /**< Wlan has higher priority */
};

/**
 * N79 5G/Wlan 5GHz interference avoidance configuration
 */
struct BandInterferenceConfig {
    BandPriority priority      ;        /**< Priority settings for N79/Wlan 5G */
    uint32_t wlanWaitTimeInSec = 30 ;   /**< If Wlan 5GHz has higher priority and suffers signal
                                            drop, modem will wait for period of time specified here
                                            for Wlan signal to recover before enabeling N79 5G.  */
    uint32_t n79WaitTimeInSec  = 30 ;   /**< If N79 has higher priority and suffers signal drop,
                                            modem will wait for period of time specified here for
                                            N79 5G signal to recover before switching Wlan to
                                            5GHz. */
};

/**
 * Specifies the IP passthrough parameters.
 */
struct IpptParams {
    int profileId  = -1;             /**< Profile ID to apply the ippt configuration on */
    int16_t vlanId = -1;             /**< Vlan ID associated with network interface for @ref telux::data::IpptDeviceConfig */
    SlotId slotId = DEFAULT_SLOT_ID; /**< Slot ID on which the profile ID is available */
};

/**
 * Specifies the IP passthrough device configuration.
 */
struct IpptDeviceConfig {
    InterfaceType nwInterface = InterfaceType::UNKNOWN;  /**< Network interface on which peer device is connected */
    std::string macAddr;                                 /**< Device MAC address */
};

/**
 * IP passthrough configuration
 */
struct IpptConfig {
    Operation ipptOpr = Operation::UNKNOWN; /**< Ippt operation */
    IpptDeviceConfig devConfig;             /**< Ippt device configuration */
};

/**
 * This function is called with the response to requestBackhaulPreference API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] backhaulPref       vector of @ref telux::data::BackhaulPref which contains the
 *                                current order of backhaul preference
 *                                First element is most preferred and last element is least
 *                                preferred backhaul.
 * @param [in] error              Return code for whether the operation succeeded or failed.
 */
using RequestBackhaulPrefResponseCb = std::function<void(
    const std::vector<BackhaulType> backhaulPref, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestBandInterferenceConfig API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] isEnabled          True: interference management is enabled.
 *                                False: interference management is disabled
 * @param [in] config             Current N79 5G /Wlan 5GHz band interference configuration
 *                                Set to nullptr if interference management is disabled
 *                                @ref telux::data::BandInterferenceConfig
 * @param [in] error              Return code for whether the operation succeeded or failed.
 */
using RequestBandInterferenceConfigResponseCb = std::function<void(bool isEnabled,
    std::shared_ptr<BandInterferenceConfig> config, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestMacSecState API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] enabled          True: MacSec is enabled, False: MacSec is disabled.
 * @param [in] error            Return code for whether the operation succeeded or failed.
 */
using RequestMacSecSateResponseCb = std::function<void(bool enabled,
    telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestWwanConnectivityConfig API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] slotId           Slot id for which wwan connectivity is reported.
 * @param [in] isAllowed        True: connectivity allowed, False: connectivity disallowed.
 * @param [in] error            Return code for whether the operation succeeded or failed.
 */
using requestWwanConnectivityConfigResponseCb = std::function<void(SlotId slotId,
    bool isAllowed, telux::common::ErrorCode error)>;

/**
 * This function is called in response to requestCurrentDds API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] currentState  Provides the current DDS status @ref telux::data::DdsInfo.
 * @param [in] error         Return code for whether the operation succeeded or failed.
 *
 */
using RequestCurrentDdsResponseCb = std::function<void(DdsInfo currentState,
    telux::common::ErrorCode error)>;

/**
 * @brief Data Settings Manager class provides APIs related to the data subsystem settings.
 *        For example, ability to reset current network settings to factory settings, setting
 *        backhaul priority, and enabling roaming per PDN.
 */
class IDataSettingsManager {
public:
    /**
     * Checks the status of Data Settings manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If Data Settings manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Data Settings manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If Data Settings manager object encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Resets current network settings to initial setting configured in factory.
     * Factory settings are the initial network settings generated during manufacturing process.
     * For the factory settings to take effect a reboot is required. Clients can choose if this API
     * invocation should reboot the system or the client would take responsibility of rebooting it.
     *
     * @param [in] operationType    @ref telux::data::OperationType
     * @param [in] callback         callback to get the response to restoreFactorySettings
     * @param [in] isRebootNeeded   true: System is automatically rebooted after reverting
     *                                    to factory settings
     *                              false: System is not rebooted after successful reset
     *
     * @returns Immediate status of restoreFactorySettings i.e. success or suitable status.
     *
     */
    virtual telux::common::Status restoreFactorySettings(OperationType operationType,
        telux::common::ResponseCallback callback = nullptr, bool isRebootNeeded = true) = 0;

    /**
     * Set backhaul preference for bridge0 (default bridge) traffic. Bridge0 Traffic routing to
     * backhaul will be attempted on first to least preferred.
     * For instance if backhaul vector contains ETH, USB, and WWAN, bridge0 traffic routing will be
     * attempted on ETH first, then USB and finally WWAN backhaul.
     * Configuration changes will be persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] backhaulPref     vector of @ref telux::data::BackhaulType which contains the
     *                              order of backhaul preference to be used when connecting to
     *                              external network.
     *                              First element is most preferred and last element is least
     *                              preferred backhaul.
     * @param [in] callback         callback to get response for setBackhaulPreference.
     *
     * @returns Status of setBackhaulPreference i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setBackhaulPreference(std::vector<BackhaulType> backhaulPref,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request current backhaul preference for bridge0 (default bridge) traffic.
     *
     * @param [in] callback         callback to get response for requestBackhaulPreference.
     *
     * @returns Status of requestBackhaulPreference i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestBackhaulPreference(
        RequestBackhaulPrefResponseCb callback) = 0;

    /**
     * Configure N79 5G and Wlan 5GHz band priority.
     * Sets priority for modem to use either 5GHz Wlan or N79 5G band when they are both available
     * to avoid interference.
     * In case N79 5G is configured as higher priority:
     *    If N79 5G becomes available while 5G Wlan is enabled, Wlan (AP/Sta) will be moved to
     *    2.4 GHz.
     *    If N79 5G becomes unavailable for
     *    @ref telux::data::BandInterferenceConfig::n79WaitTimeInSec time period, Wlan will be
     *    moved to 5GHz.
     * In case Wlan 5GHz is configured as higher priority:
     *    If Wlan 5GHz (AP/Sta) becomes available while N79 5G is enabled, N79 5G will be disabled.
     *    If Wlan 5GHz becomes unavailable for
     *    @ref telux::data::BandInterferenceConfig::wlanWaitTimeInSec period and N79 5G is
     *    available, N79 will be enabled.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] enable           True: enable interference management.
     *                              False: disable interference management
     * @param [in] config           N79 5G /Wlan 5GHz band interference configuration
     *                              @ref telux::data::BandInterferenceConfig
     * @param [in] callback         callback to get response for setBandInterferenceConfig.
     *
     * @returns Status of setBandInterferenceConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setBandInterferenceConfig(bool enable,
        std::shared_ptr<BandInterferenceConfig> config = nullptr ,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request N79 5G and Wlan 5GHz band priority settings.
     * Request the configurations set by telux::data::setBandInterferenceConfig
     *
     * @param [in] callback         callback to get response for requestBandInterferenceConfig.
     *
     * @returns Status of requestBandInterferenceConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestBandInterferenceConfig(
        RequestBandInterferenceConfigResponseCb callback) = 0;

    /**
     * Allow/Disallow WWAN connectivity.
     * Controls whether system should allow/disallow WWAN connectivity to cellular network.
     * Default setting is allow WWAN connectivity to cellular network.
     * - If client selects to disallow WWAN connectivity, any further attempts to start data
     *   calls using @ref telux::data::IDataConnectionManager::startDataCall will fail with
     *   @ref telux::common::ErrorCode::NOT_SUPPORTED.
     *   Data calls can be connected again only if client selects to allow WWAN connectivity.
     * - If client selects to disallow WWAN connectivity while data calls are already connected,
     *   all WWAN data calls will also be disconnected.
     *   Client will also receive @ref telux::data::IDataConnectionListener::onDataCallInfoChanged
     *   notification with @ref telux::data::IDataCall object status
     *   @ref telux::data::DataCallStatus::NET_NO_NET for all impacted data calls.
     * Configuration changes will be persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] slotId           Slot id on which WWAN connectivity to be allowed/disallowed
     * @param [in] allow            True: allow connectivity, False: disallow connectivity
     * @param [in] callback         optional callback to get response for setWwanConnectivityConfig.
     *
     * @returns Status of setWwanConnectivityConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setWwanConnectivityConfig(SlotId slotId, bool allow,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request current WWAN connectivity Configuration.
     *
     * @param [in] slotId           Slot id for which WWAN connectivity to be reported.
     * @param [in] callback         callback to get response for requestWwanConnectivityConfig.
     *
     * @returns Status of requestWwanConnectivityConfig i.e. success or suitable status code.
     *
     *
     */
    virtual telux::common::Status requestWwanConnectivityConfig(SlotId slotId,
        requestWwanConnectivityConfigResponseCb callback) = 0;


    /**
     * Request device data usage monitoring status
     *
     * This function can be used to obtain the current status of device data usage monitoring.
     *
     * @returns    Returns true if data usage monitoring is enabled, else false.
     *
     */
    virtual bool isDeviceDataUsageMonitoringEnabled() = 0;

    /**
     * Allows the client to set the MacSec state.
     *
     * - If client enables the MacSec, post that the packets over the ethernet link
     *   will be encrypted.
     * - If client disables the MacSec, post that the packets over the ethernet link
     *   will not be encrypted.
     *
     * @param [in] enable          True: enable the MacSec, False: disable the MacSec.
     * @param [in] callback        Callback to get the setMacSecState response.
     *
     * @returns Status of setMacSecState, i.e., success or suitable status code.
     *
     */
    virtual telux::common::Status setMacSecState(bool enable,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Requests the current MacSec state.
     *
     * @param [in] callback    callback to get response for requestMacSecState.
     *
     * @returns Status of requestMacSecState, i.e., success or suitable status code.
     *
     *
     */
    virtual telux::common::Status requestMacSecState(RequestMacSecSateResponseCb callback) = 0;

    /**
     * Switch backhaul to be used by traffic.
     * Provides the ability to re-route clients traffic from one backhaul to another.
     * Clients must call this API for each backhaul switch. For instance, if the default bridge
     * (bridge0) and the on-demand bridge (bridges created by VLANs) need to be re-routed to WLAN,
     * this API must be called twice for the default profile ID and the on-demand profile ID..
     * If destination backhaul is WLAN (WLAN in Station Mode):
     * - Traffic associated with the default and on-demand bridges will be re-routed to WLAN
     *   backhaul.
     * - Client traffic can only be re-routed to WLAN backhaul if the station is connected to an
     *   external access point.
     * - VLANs mapped to WWAN backhaul will be automatically mapped to WLAN backhaul.
     * - Firewall and DMZ rules configured on WLAN backhaul (if configured before calling this API)
     *   will be automatically activated.
     * If destination backhaul is WWAN:
     *  - Any VLAN profile ID mapping configured in the destination backhaul prior to calling this
     *    API will be applied automatically.
     *  - Any firewall or DMZ rule configured on WWAN backhaul before calling this API will be
     *    activated automatically.
     *
     * @param [in] source         Backhaul @ref telux::data::BackhaulInfo to re-route traffic from
     * @param [in] dest           Backhaul @ref telux::data::BackhaulType to re-route traffic to
     * @param [in] applyToAll     Traffic on all source backhauls will be routed to dest backhauls
     *                            if the source backhaul type is
     *                            @ref telux::data::BackhaulType::WWAN, traffic on all WWAN
     *                            backhauls (default and on-demand) will be routed to dest backhaul.
     *                            if dest backhaul type is @ref telux::data::BackhaulType::WWAN
     *                            traffic on source backhaul will be routed to WWAN backhauls
     *                            (default and on-demand) based on vlan-backhaul binding set by
     *                            telux::data::net::IVlanManager::bindToBackhaul
     * @param [in] callback       Optional callback to get the response for switchBackHaul.
     *
     * @returns Status of switchBackHaul, i.e., success or applicable status code
     *
     */
    virtual telux::common::Status switchBackHaul(BackhaulInfo source, BackhaulInfo dest,
        bool applyToAll = false, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Allows the client to set the IP passthrough configuration for a specific profile and vlan ID.
     *
     * When @ref telux::data::IpptConfig ipptOpr is set to ENABLE, the client can add a new
     * @ref telux::data::IpptDeviceConfig or modify an existing configuration.
     *
     * The @ref telux::data::IpAddrInfo gwMask is not required for this API.
     *
     * If @ref telux::data::IpptDeviceConfig is not provided, the system will perform an IP
     * passthrough operation on the existing configuration.
     *
     * The system cannot add or modify the @ref telux::data::IpptDeviceConfig if the @ref
     * telux::data::IpptConfig ipptOpr is set to DISABLE.
     *
     * Configuration changes will be persistent across reboots.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] ipptParms  IP passthrough parameters, @ref telux::data::IpptParams
     * @param [in] config     IP passthrough configuration, @ref telux::data::IpptConfig
     *
     * @returns               @ref telux::common::ErrorCode as appropriate.
     *
     * @note   @ref telux::data::IpAddrInfo gwMask is not required for this API.
     *         Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode setIpPassThroughConfig(const IpptParams &ipptParms,
            const IpptConfig &config) = 0;

    /**
     * Allows the client to configure the Network Address Translation (NAT) for IP passthrough
     * feature.
     *
     * Configuration changes will be persistent across reboots.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] enableNat  Set to false if NAT is disabled, default is true.
     *
     * @returns               @ref telux::common::ErrorCode as appropriate.
     *
     * @note   All active data calls must be disconnected before invoking this API.
     *         Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode setIpPassThroughNatConfig(bool enableNat = true) = 0;

    /**
     * Allows the client to get the IP passthrough feature configuration which includes whether the
     * Network Address Translation (NAT) is enabled or not.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [out] isNatEnabled  true if NAT is enabled, otherwise false.
     *
     * @returns               @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode getIpPassThroughNatConfig(bool &isNatEnabled) = 0;

    /**
     * Get the current IP passthrough configuration for a specific profile ID and vlan ID.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] ipptParms  IP passthrough parameters, @ref telux::data::IpptParams
     * @param [out] config    IP passthrough configuration, @ref telux::data::IpptConfig
     *
     * @returns              @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode getIpPassThroughConfig(const IpptParams &ipptParms,
            IpptConfig &config) = 0;

    /**
     * Set the IP configuration for an interface.
     * Provides the ability to configure @ref telux::data::IpAssignType::STATIC_IP or
     * @ref telux::data::IpAssignType::DYNAMIC_IP to a specified @ref telux::data::InterfaceType.
     *
     * Currently, @ref telux::data::IpAssignType::STATIC_IP support is only available for
     * @ref telux::data::IpFamilyType::IPV4.
     *
     * To change the @ref telux::data::IpAssignType from STATIC_IP to DYNAMIC_IP (or vice versa),
     * the client must first configure the @ref telux::data::IpConfig ipOpr to DISABLE using this
     * API.
     *
     * This API does not support @ref telux::data::IpFamilyType::IPV4V6. The client must
     * invoke this API multiple times to configure @ref telux::data::IpAssignType::STATIC_IP or
     * @ref telux::data::IpAssignType::DYNAMIC_IP for @ref telux::data::IpFamilyType::IPV4 and
     * @ref telux::data::IpFamilyType::IPV6 separately.
     *
     * Prior to invoking this API, the data call should be up and running.
     * If the data call status changes, the clients will be notified using
     * @ref telux::data::IDataConnectionListener::onDataCallInfoChanged and this API must be
     * invoked again as described below.
     *
     * When @ref telux::data::DataCallStatus, whose IP address is being passed through to this
     * NAD, changes to NET_NO_NET, this API must be invoked again with @ref telux::data::IpConfig
     * ipOpr to DISABLE.
     * When @ref telux::data::DataCallStatus, whose IP address is being passed through to this
     * NAD, changes to NET_CONNECTED, this API must be invoked again with @ref telux::data::IpConfig
     * ipOpr to ENABLE.
     * When @ref telux::data::DataCallStatus, whose IP address is being passed through to this
     * NAD, changes to NET_RECONFIGURED, this API must be invoked again with
     * @ref telux::data::IpConfig ipOpr to RECONFIG.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] ipConfigParams     @ref telux::data:IpConfigParams
     * @param [in] ipConfig           @ref telux::data:IpConfig
     *
     * @returns                  @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode setIpConfig(const IpConfigParams &ipConfigParams,
            const IpConfig &ipConfig) = 0;

    /**
     * Get the IP configuration for an interface.
     * Provides the ability to get the configuration for @ref telux::data::IpAssignType::STATIC_IP
     * or @ref telux::data::IpAssignType::DYNAMIC_IP for a specific @ref telux::data::InterfaceType
     * and @ref telux::data::IpFamilyType.
     *
     * This API does not support @ref telux::data::IpFamilyType::IPV4V6. The client must invoke this
     * API multiple times to get IP configuration for @ref telux::data::IpFamilyType::IPV4 and
     * @ref telux::data::IpFamilyType::IPV6.
     *
     * The @ref telux::data::IpAddrInfo only provides @ref telux::data::IpAssignType::STATIC_IP
     * configuration.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in]  ipConfigParams     @ref telux::data:IpConfigParams
     * @param [out] ipConfig           @ref telux::data:IpConfig
     *
     * @returns                @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode getIpConfig(const IpConfigParams &ipConfigParams,
            IpConfig &ipConfig) = 0;

    /**
     * Register Data Settings Manager as listener for Data Service heath events like data service
     * available or data service not available.
     *
     * @param [in] listener    pointer of IDataSettingsListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDataSettingsListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IDataSettingsListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDataSettingsListener> listener) = 0;

    /**
     * Allows the client to perform the DDS switch. Client has the option
     * to either select permanent or temporary switch.
     *
     * @param [in] request          Client has to provide the request
     *                              @ref telux::data::DdsInfo.
     *
     * @param [in] callback         Callback to get response for requestDdsSwitch.
     *                              Possible ErrorCode in @ref telux::common::ResponseCallback:
     *                              - If the DDS switch is performed successfully
     *                                @ref telux::common::ErrorCode::SUCCESS
     *                              - If the DDS switch request is rejected
     *                                @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
     *                                The following scenarios are examples of when a switch
     *                                request will be rejected:
     *                                    1. Slot1 is permanent DDS and the client attempts to
     *                                       trigger a permanent DDS switch on slot 1.
     *                                    2. During an MT/MO voice call and the client attempts
     *                                       to trigger a permanent DDS switch.
     *                              - If the DDS switch is allowed but due to some reason DDS
     *                                switch failed @ref telux::common::ErrorCode::GENERIC_FAILURE
     *
     * @returns Status of requestDdsSwitch, i.e., success or suitable status code.
     *
     * @deprecated Use IDualDataManager::requestDdsSwitch API.
     */
    virtual telux::common::Status requestDdsSwitch(DdsInfo request,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request the current DDS slot information.
     *
     * @param [in] callback      Callback to get response for requestCurrentDds.
     *
     * @returns Status of requestCurrentDds, i.e., success or suitable status code.
     *
     * @deprecated Use IDualDataManager::requestCurrentDds API.
     */
    virtual telux::common::Status requestCurrentDds(RequestCurrentDdsResponseCb callback) = 0;
};

/**
 * Interface for Data Settings listener object. Client needs to implement this interface to get
 * access to Data Settings services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IDataSettingsListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when WWAN backhaul connectivity config changes.
     *
     * @param [in] slotId                - Slot Id for which connectivity has changed.
     * @param [in] isConnectivityAllowed - Connectivity status allowed/disallowed.
     *
     */
    virtual void onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) {}

    /**
     * Provides the current DDS state and is called whenever a DDS switch occurs.
     *
     * @param [in] currentState      Provides the current DDS status.
     *                               - Slot ID on which the DDS switch occured.
     *                               - DDS switch type @ref telux::data::DdsType.
     *
     * @deprecated Use IDualDataListener::onDdsChange indication.
     */
    virtual void onDdsChange(DdsInfo currentState) {}

    /**
     * Destructor for IDataSettingsListener
     */
    virtual ~IDataSettingsListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif // TELUX_DATA_DATASETTINGSMANAGER_HPP
