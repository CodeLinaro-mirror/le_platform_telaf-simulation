/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanDeviceManager.hpp
 *
 * @brief      WlanDeviceManager is a primary interface for configuring WLAN (Wireless Local
 *             Area Network). it provide APIs for configuring WLAN connectivity.
 *
 */

#ifndef TELUX_WLAN_WLANDEVICEMANAGER_HPP
#define TELUX_WLAN_WLANDEVICEMANAGER_HPP

#include <memory>
#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>


namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

//Forward declaration
class IWlanListener;

/**
 * Wlan Interface State
 */
enum class InterfaceState {
    INACTIVE  = 0x00,   /**<  Interface is Inactive  */
    ACTIVE    = 0x01,   /**<  Interface is Active    */
};

/**
 * Wlan Interface Device
 */
enum class HwDeviceType {
    UNKNOWN   = 0,                  /**<  Wlan device is Unknown   */
    QCA6574   = 1,                  /**<  Wlan device is QCA6574   */
    QCA6696   = 2,                  /**<  Wlan device is QCA6696   */
    QCA6595   = 3,                  /**<  Wlan device is QCA6595   */
    QCA6797   = 4,                  /**<  Wlan device is QCA6797   */
};

/**
 * Wlan Device Performance State
 */
enum class DevicePerfState {
    UNKNOWN   = 0,                  /**<  Device is in Unknown performance state   */
    FULL      = 1,                  /**<  Device is in full performance state      */
    REDUCED   = 2,                  /**<  Device is in reduced performance state   */
    SHUTDOWN  = 3,                  /**<  Device is shutdown                       */
};

/**
 * Wlan Interface status
 */
struct InterfaceStatus {
    HwDeviceType     device;              /**> WiFi hardware type           */
    std::vector<ApStatus>  apStatus;      /**< Vector of active APs status  */
    std::vector<StaStatus> staStatus;     /**< Vector of active Sta status  */
};

/**
 * Wlan Regulatory Setting
 */
struct RegulatoryParams {
    std::string         country;     /**< Country code according to ISO 3166 alpha-2 standard     */
    float               opChannel;   /**< Operating channel according to IEEE 802.11 Standards    */
    std::vector<float>  opClass;     /**< Operating class according to IEEE 802.11 Standards      */
    uint32_t            txPowerMw;   /**< Transmit power in multiple of 100 MilliWatts Actual transmit power = value set here * 100 milliwatts */
};

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief     WlanDeviceManager is a primary interface for configuring Wireless LAN.
 *           it provide APIs to enable, configure, activate, and modify modes
 */
class IWlanDeviceManager {
 public:
    /**
     * Checks the readiness status of wlan manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If wlan manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If wlan manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If wlan manager encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Enable or Disable Wlan Service.
     * Configurations set by @ref telux::wlan::IWlanDeviceManager::setMode must be completed before
     * enabling Wlan.
     * If any of configurations need to be changed after Wlan is enabled, this API must be called
     * with enable set to false followed by a call with enable set to true for the new
     * configurations to take effect.
     * Calling this API with enable, will start hostapd and wpa_supplicant daemons.
     * Further changes to hostapd and wpa_supplicant will require calling
     * @ref telux::wlan::IApInterfaceManager::manageApService and
     * @ref telux::wlan::IStaInterfaceManager::manageStaService respectively.
     * Client shall wait for @ref IWlanListener::onEnableChanged indication to confirm WLAN was
     * enabled/disabled successfully
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] enable            true : Enable Wlan, false: Disable Wlan.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual  telux::common::ErrorCode enable(bool enable) = 0;

    /**
     * Set Wlan mode - number of supported APs, and stations.
     * This API shall be called when wlan is disabled. On enablement, wlan will enable APs and
     * Stations set in this API.
     *
     * @param [in] numOfAp           Num of Access Points to be enabled. If no Access Point is
     *                               enabled, this argument should be set to 0.
     *                               Configuration of each AP is accomplished through
     *                               telux::data::wlan::IApManager instance requested from factory.
     * @param [in] numOfSta          Num of Stations to be enabled. If no station is enabled,
     *                               this argument should be set to 0.
     *                               Configuration of each Station is accomplished through
     *                               telux::data::wlan::IStaManager instance requested from factory.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual  telux::common::ErrorCode setMode(int numOfAp, int numOfSta) = 0;

    /**
     * Request Wlan configuration: Returns the configuration that was set using
     * @ref telux::wlan::IWlanDeviceManager::setMode.
     * This might differ from the configuration that is actually enabled in the system, for
     * instance, when the hardware cannot fully support the configuration that was set.
     * To get the status of the current configuration on Wlan enablement,
     * @ref telux::wlan::IWlanDeviceManager::getStatus should be used.
     *
     * @param [in] numAp                Num of configured APs
     * @param [in] numSta               Num of configured Stations
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode getConfig(int& numAp, int& numSta) = 0;

    /**
     * Request Wlan status: Return Wlan enablement status and Interface status of APs and Station
     * such as active/inactive,
     * network interface name and hardware device they are mapped to.
     * Results are valid only if Wlan is enabled.
     *
     * @param [in] isEnabled            true: Wlan is enabled. false: Wlan is Disabled.
     * @param [in] status               vector of interface status @ref InterfaceStatus.
      *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode getStatus(
        bool& isEnabled, std::vector<InterfaceStatus>& status) = 0;

    /**
     * Set the country in which the device is operating. The country code will be used to make the
     * device operate using the regulatory parameters pertaining to the active country
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] country              Active country code according to ISO 3166 alpha-2 standard
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode setActiveCountry(std::string country) = 0;
    /**
     * Request Regulatory Parameters
     *
     * This API can only be successfully invoked when at least one WLAN interface is in operational
     * state. Specifically, when at least one of the configured access point is active, or the
     * configured station interface is connected to an external access point.
     *
     * @param [out] regulatoryParams    Current Regulatory Settings @ref RegulatoryParams.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode getRegulatoryParams(RegulatoryParams& regulatoryParams) = 0;

    /**
     * Set Transmit Power
     *
     * Immediately changes WLAN transmit power for the current session. This setting is not
     * persistent across power cycles or service restarts.
     *
     * This API can only be successfully invoked when at least one WLAN interface is in operational
     * state. Specifically, when at least one of the configured access point is active, or the
     * configured station interface is connected to an external access point.
     *
     * This requirement exists because the underlying driver or firmware applies transmit
     * power settings only when the interface is fully initialized and actively handling traffic.
     *
     * To restore the default transmit power as defined by the regulatory domain associated with
     * the active country set by telux::wlan::IWlanDeviceManager::setActiveCountry, either hostapd or
     * wpa_supplicant daemons need to be restarted via telux::wlan::IApInterfaceManager::manageApService or
     * telux::wlan::IStaInterfaceManager::manageStaService
     *
     * @param [in] txPower              Transmit Power to be set in mutiple of 100 milliwatts.
     *                                  For instance, if txPower equals 15, transmit power will be
     *                                  set to 1500 milliwatts.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode setTxPower(uint32_t txPowerMw) = 0;
    /**
     * Request Transmit Power
     *
     * This API can only be successfully invoked when at least one WLAN interface is in operational
     * state. Specifically, when at least one of the configured access point is active, or the
     * configured station interface is connected to an external access point.
     *
     * This restriction exists because the driver or firmware provides transmit power information
     * only when an interface is fully initialized and actively transmitting or receiving data.
     *
     * @param [out] txPowerMw           Current Transmit Power in mutiple of 100 milliwatts.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode getTxPower(uint32_t& txPowerMw) = 0;

    /**
     * Register a listener for specific events in the Wlan Manager
     *
     * @param [in] listener    pointer of IWlanListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IWlanListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IWlanListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IWlanListener> listener) = 0;

    /**
     * Destructor for IWlanDeviceManager
     */
    virtual ~IWlanDeviceManager(){};
};  // end of IWlanDeviceManager


class IWlanListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when temperature has crossed threshold
     *
     * @param [in] temperature - current device temperature in Fahrenheit
     * @param [in] perfState   - current performance state of device due to device temperature
     */

    virtual void onTempCrossed(float temperature, DevicePerfState perfState) {}
    /**
     * This function is called when Wlan enablement has changed
     *
     * @param [in] enable     True: Wlan is enabled, False: Wlan is disabled
     */
    virtual void onEnableChanged(bool enable) {}

    virtual ~IWlanListener() {}
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}
#endif // TELUX_WLAN_WLANDEVICEMANAGER_HPP
