/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ApInterfaceManager.hpp
 *
 * @brief      Primary interface for Wi-Fi Access Points.
 *             It provide APIs for Access Points configurations and management.
 *
 */

#ifndef TELUX_WLAN_APINTERFACEMANAGER_HPP
#define TELUX_WLAN_APINTERFACEMANAGER_HPP

#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>

namespace telux {
namespace wlan {

class IApListener;

/** @addtogroup telematics_wlan_ap
 * @{ */

#define INVALID_AP_ID    0

/**
 * AP Interworking Information
 */
enum class ApInterworking {
    INTERNET_ACCESS     = 0,   /**<  AP with internet access only - No LAN access   */
    FULL_ACCESS         = 1    /**<  AP Can Access LAN and Internet                 */
};

/**
 * AP Client Connection Status
 */
enum class ApDeviceConnectionEvent {
    CONNECTED    = 0,
    DISCONNECTED = 1,
    IPV4_UPDATED = 2,
    IPV6_UPDATED = 3,
};

/**
 * Wlan Security Mode
 */
enum class SecMode {
    OPEN ,                         /**<  Open System Architecture    */
    WEP  ,                         /**<  Wired Equivalent Privacy    */
    WPA  ,                         /**<  Wi-Fi Protected Access      */
    WPA2 ,                         /**<  Wi-Fi Protected Access II   */
    WPA3 ,                         /**<  Wi-Fi Protected Access III  */
};

/**
 * Wlan Authentication Method
 */
enum class SecAuth {
    NONE    ,                      /**<  No Authentication - Open System                      */
    PSK     ,                      /**<  Pre-Shared Key                                       */
    EAP_SIM,                       /**<  EAP - Subscriber Identity Module                     */
    EAP_AKA,                       /**<  EAP - Authentication and Key Agreement               */
    EAP_LEAP,                      /**<  EAP - Lightweight Extensible Authentication Protocol */
    EAP_TLS ,                      /**<  EAP - Transport Layer Security                       */
    EAP_TTLS,                      /**<  EAP - Tunneled Transport Layer Security              */
    EAP_PEAP,                      /**<  EAP - Protected EAP                                  */
    EAP_FAST,                      /**<  EAP - Flexible Authentication via Secure Tunneling   */
    EAP_PSK ,                      /**<  EAP - Pre-Shared Key                                 */
    SAE,                           /**< Simultaneous Authentication of Equals                 */
};

/**
 * Wlan Encryption Method
 */
enum class SecEncrypt {
    RC4 ,                          /**<  Rivest Cipher 4                   */
    TKIP,                          /**<  Temporal Key Integrity Protocol   */
    AES ,                          /**<  Advanced Encryption Standard      */
    GCMP,                          /**<  Galois/Counter Mode Protocol      */
};

/**
 * AP Network Access Type
 */
enum class NetAccessType {
    PRIVATE = 0            ,      /**< Private Network                    */
    PRIVATE_WITH_GUEST     ,      /**< Private network with guest access  */
    CHARGEABLE_PUBLIC      ,      /**< Chargeable public network          */
    FREE_PUBLIC            ,      /**< Free public network                */
    PERSONAL_DEVICE        ,      /**< Personal device network            */
    EMERGENCY_SERVICES_ONLY,      /**< Emergency services only network    */
    TEST_OR_EXPERIMENTAL   ,      /**< Test or experimental               */
    WILDCARD               ,      /**< Wildcard                           */
};

/**
 * Wlan AP Venue Info as defined in IEEE Std 802.11u-2011, 7.3.1.34
 */
struct ApVenueInfo {
    int type;                   /**< Venue Type      */
    int group;                  /**< Venue Group     */
};

/**
 * AP Security
 */
struct ApSecurity {
    SecMode     mode;               /**< Security mode          */
    SecAuth     auth;               /**< Authorization method   */
    SecEncrypt   encrypt;           /**< Encryption method      */
};

/**
 * AP Element Info
 */
struct ApElementInfoConfig {
    bool    isEnabled;
    bool isInterworkingEnabled;      /**< Interworking Service enablement                      */
    NetAccessType netAccessType;     /**< Network Access Type                                  */
    bool internet;                   /**< Whether network provide connectivity to internet     */
    bool asra;                       /**< Additional step required for access                  */
    bool esr;                        /**< Emergency services reachable                         */
    bool uesa;                       /**< Unauthenticated emergency service accessible         */
    uint8_t venueGroup;              /**< Venue group                                          */
    uint8_t venueType;               /**< Venue type                                           */
    std::string hessid;              /**< Homogeneous ESS identifier                           */
    std::string vendorElements;      /**< Vendor elements for Beacon and Probe Response frames */
    std::string assocRespElements;   /**< Vendor elements for (Re)Association Response frames  */
};

/**
 * Ap Network Configuration
 */
struct ApNetConfig {
    ApInfo               info;              /**< AP type                                          */
    std::string          ssid;              /**< SSID for AP                                      */
    bool                 isVisible;         /**< AP broadcast SSID                                */
    ApElementInfoConfig  elementInfoConfig; /**< AP broadcast it's capabilities (Such as CarPlay) */
    ApInterworking       interworking;      /**< AP network access (internet/local)               */
    ApSecurity           apSecurity;        /**< AP Security settings                             */
    std::string          passPhrase;        /**< Passphrase for SSID used                         */
};

/**
 * Ap Configuration
 */
struct ApConfig {
    Id          id;                        /**< AP id                           */
    ApVenueInfo venue;                     /**< AP venue info                   */
    std::vector<ApNetConfig> network;      /**< Configurations supported by AP  */
};

/**
 * Wlan Client Device Indication Info
 */
struct DeviceIndInfo {
    Id           id;                   /**<  AP id device is connected to                      */
    std::string  macAddress;           /**<  MAC Address of Wi-Fi device                       */
};

/**
 * Wlan Client Device Info
 */
struct DeviceInfo {
    Id           id;                      /**< AP id device is connected to                      */
    std::string  name;                    /**< User friendly string that identifies Wi-Fi device */
    std::string  ipv4Address;             /**< IPv4 Address of Wi-Fi device                      */
    std::vector<std::string> ipv6Address; /**< List of IPv6 Addresses of Wi-Fi device            */
    std::string  macAddress;              /**< MAC Address of Wi-Fi device                       */
};

/** @addtogroup telematics_wlan_ap
 * @{ */

/**
 * @brief   Manager class for configuring Wlan Access Points
 */
class IApInterfaceManager {
 public:
    /**
     * Set Access Point config: Used to fully configure access points including venue type,
     * radio type (2.4/5/6 GHz), private/guest network and all other related settings.
     * Configurations will take effect after hostapd service is restarted by calling
     * @ref telux::wlan::IApInterfaceManager::manageApService.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_AP_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] config       AP configuration parameters @ref telux::wlan::ApConfig
     *
     * @returns  operation error code (if any). @ref telux::common::ErrorCode
     *           telux::common::Status::NOTALLOWED is returned if AP to be configured was not
     *           enabled in @ref telux::wlan::WlanDeviceManager::setMode.
     */
     virtual telux::common::ErrorCode setConfig(ApConfig config) = 0;

    /**
     * Set Wlan Security Configuration: Used to change security settings of selected network.
     *
     * @param [in] apId             AP identifier to set security for. @ref telux::wlan::Id
     * @param [in] apSecurity       AP security settings. @ref telux::wlan::ApSecurity

     * @returns operation error code (if any). @ref telux::common::ErrorCode

     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode setSecurityConfig(Id apId, ApSecurity apSecurity) = 0;

    /**
     * Set Access Point SSID: Used to change SSID of selected network.
     *
     * @param [in] apId                AP identifier to set SSID for. @ref telux::wlan::Id
     * @param [in] ssid                new SSID to be set
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode setSsid(Id apId, std::string ssid) = 0;

    /**
     * Set Access Point visibility: Used to change SSID broadcast of selected network.
     *
     * @param [in] apId           AP identifier to set SSID visibility for. @ref telux::wlan::Id
     * @param [in] isVisible      Visibility to be set
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode setVisibility(Id apId, bool isVisible) = 0;

    /**
     * Configure Element Info: Used to change element info configurations of selected network.
     *
     * A successful response from this API indicates only that the input configuration has been
     * stored.  It does not validate the correctness or compliance of the provided values with
     * IEEE 802.11 specifications.
     *
     * The caller is responsible for ensuring that the values provided in
     * @ref telux::wlan::ApElementInfoConfig comply with applicable constraints and
     * interdependencies as defined in the IEEE 802.11 standard.
     *
     * @param [in] apId            AP identifier to enable element info on. @ref telux::wlan::Id
     * @param [in] config          Element Info configurations.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_AP_DEVICES
     * permission to invoke this API successfully.
	 *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode setElementInfoConfig(Id apId, ApElementInfoConfig config) = 0;

    /**
     * Set Passphrase for Access Point: Used to change passphrase of selected network.
     *
     * @param [in] apId            AP identifier to set passphrase for. @ref telux::wlan::Id
     * @param [in] passPhrase      new passPhrase string
     *
     * @returns Immediate status of setPassPhrase() request i.e. success or suitable status.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode setPassPhrase(Id apId, std::string passPhrase) = 0;

    /**
     * Request Access Point Configurations
     *
     * If @ref telux::wlan::IApInterfaceManager::setConfig was previously used to update the
     * access point configuration, it is required to wait for the
     * @ref telux::wlan::IApConfigListener::onApConfigChanged notification before calling
     * getConfig(). This ensures that the configuration changes have been fully applied
     * and synchronized internally.
     *
     * @param [in] config         Vector of AP configurations @ref telux::wlan::ApConfig as set by
     *                            @ref telux::wlan::IApInterfaceManager::setConfig
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
     virtual telux::common::ErrorCode getConfig(std::vector<ApConfig>& config) = 0;

    /**
     * Request AP Status
     *
     * @param [in] status         Vector of AP network Status @ref telux::wlan::ApStatus
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
     virtual telux::common::ErrorCode getStatus(std::vector<ApStatus>& status) = 0;

    /**
     * Request Connected Devices to all enabled access points.
     * Each entry in returned list will contain information about a device such as access point
     * it is connected to and IP and MAC address as defined in @ref telux::wlan::DeviceInfo
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_AP_DEVICES
     * permission to invoke this API successfully.
     *
     * @param [in] clientsInfo     List of connected devices Info @ref telux::wlan::DeviceInfo
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode getConnectedDevices(std::vector<DeviceInfo>& clientsInfo) = 0;

    /**
     * Execute an operation on hostapd service. Provides ability for client to either stop/start or
     * restart hostapd service for selected access point. Restarting hostapd service is required
     * for any changes made to hostapd.conf file and changes made by
     * @ref telux::wlan::IApInterfaceManager::setConfig to take effect.
     * Stop/Start operation @ref telux::wlan::ServiceOperation will Stop/Start WiFi service for
     * access point.
     * Access points selected to execute operation on, will temporarily go out of service when this
     * API is called.
     * This API should be called only when access point is configured through
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_AP_CONFIG
     * permission to invoke this API successfully.
     *
     * @ref telux::wlan::IDeviceManager::setMode
     *
     * @param [in] apId          AP identifier to execute operation on. @ref telux::wlan::Id
     * @param [in] opr           Operation to be performed on hostapd
     *                           @ref telux::wlan::ServiceOperation
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode manageApService(Id apId, ServiceOperation opr) = 0;

    /**
     * Register a listener for specific events in Access Point Manager
     *
     * @param [in] listener    pointer of IApListener object that processes the
     * notification
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IApListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IApListener object that needs to be removed
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IApListener> listener) = 0;

     virtual ~IApInterfaceManager(){};
};

class IApListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when AP device status has changed
     *
     * @param [in] event       Event detected on device @ref telux::wlan::ApDeviceConnectionEvent
     * @param [in] info        Info about devices @ref telux::wlan::DeviceIndInfo
     */
    virtual void onApDeviceStatusChanged(ApDeviceConnectionEvent event,
        std::vector<DeviceIndInfo> info) {}

    /**
     * This function is called when AP switch to different operation band
     *
     * @param [in] radio        New AP operation band @ref telux::wlan::BandType
     */
    virtual void onApBandChanged(BandType radio) {}

    /**
     * This function is called when AP configuration has changed
     *
     * @param [in] apid        @ref telux::wlan::Id of Ap it's configuration has changed
     */
    virtual void onApConfigChanged(Id apId) {}

    virtual ~IApListener() {}
};

/** @} */ /* end_addtogroup telematics_wlan_ap */
}
}
#endif // TELUX_WLAN_APINTERFACEMANAGER_HPP
