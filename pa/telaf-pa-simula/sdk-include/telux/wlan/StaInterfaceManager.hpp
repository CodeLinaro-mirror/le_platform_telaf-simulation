/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       StaInterfaceManager.hpp
 *
 * @brief      Primary interface for Wi-Fi Station Mode.
 *             it provide APIs for Wi-Fi Station mode configurations and management.
 *
 */

#ifndef TELUX_WLAN_STAINTERFACEMANAGER_HPP
#define TELUX_WLAN_STAINTERFACEMANAGER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>
#include "WlanDeviceManager.hpp"

namespace telux {
namespace wlan {

//Forward declaration
class IStaListener;

/** @addtogroup telematics_wlan_station
 * @{ */

/**
 * Network identifier is a unique serial number assigned to each configured network,
 * It serves as an index to reference and manage the persistent network settings.
 * Network identifiers remain consistent across operations as long as the wpa_supplicant daemon is
 * active. If the daemon is restarted, these identifiers may be reassigned, which can change their
 * mapping to specific network configurations.
 */
using NetworkId = uint16_t;

/**
 * Priority is used to determine the order in which persistent network
 * configurations are selected when multiple network entries are present and
 * network with higher priority value is preferred over those with lower
 * values, it should be set to 0 if you do not want specify priority.
 */
using Priority = uint16_t;

/**
 * Station Connection IP Type.
 */
enum class StaIpConfig {
    DYNAMIC_IP   = 1,   /**< Station is configured with dynamic IP */
    STATIC_IP    = 2,   /**< Station is configured with Static IP  */
};

/**
 * Bridge/Router Mode.
 */
enum class StaBridgeMode {
    ROUTER = 0,    /**<  Station is in Router Mode      */
    BRIDGE = 1     /**<  Station is in Bridge Mode      */
};

/**
 * Static IP Configuration.
 */
struct StaStaticIpConfig {
    std::string ipAddr;       /**<   IPv4 address to be assigned. */
    std::string gwIpAddr;     /**<   IPv4 address of the gateway. */
    std::string netMask;      /**<   Subnet mask.                 */
    std::string dnsAddr;      /**<   DNS IPv4 address.            */
};

/**
 * Station base network configuration.
 */
struct StaNetworkConfig {
    std::string ssid;               /**< SSID of external Access point               */
    Priority    priority;           /**< Priority to determine the preferred network */
    BandType    band;               /**< Operation band type                         */
    std::string bssid;              /**< BSSID/MAC address of external access point  */
};

/**
 * Input entry for station network configuration, containing details needed by WLAN
 * station to connect to an external access point.
 */
struct StaNetworkConfigEntry : StaNetworkConfig {
    std::string passPhrase;     /**< Passphrase of external Access point    */
    bool        enable;        /**< Flag to control connection behavior.
                                    When set to true, initiates a connection to this configured
                                    network if the station is not already connected, regardless
                                    of its priority, and also updates the network configuration
                                    to persistent settings.
                                    When set to false, only updates the network configuration to
                                    persistent settings without attempting a connection. */
};

/**
 * Query listing from WLAN station network configurations.
 */
struct StaNetworkConfigInfo : StaNetworkConfig {
    NetworkId   networkId;      /**< Identifier associated with network     */
    bool        isCurrent;      /**< Indicates whether this is the active
                                     network configuration                  */
};

/**
 * Station Configuration
 */
struct StaConfig {
    Id                  staId;            /**< Id of station backhaul                 */
    StaIpConfig         ipConfig;         /**< IP configuration of station backhaul   */
    StaStaticIpConfig   staticIpConfig;   /**< Static IP configuration if selected    */
    StaBridgeMode       bridgeMode;       /**< Station configuration as Router/bridge */
};


/**
 * Details of an individual external access point (AP) discovered during a WLAN station scan.
 * An external AP refers to a Wi-Fi network that the WLAN station can connect to.
 */
struct ExternalApInfo{
    std::string ssid;               /**< SSID of external AP                          */
    std::string bssid;              /**< BSSID/MAC address of external AP             */
    BandType    band;               /**< Operation band type                          */
    std::string securityFlags;      /**< Describes the authentication, key management, and encryption schemes supported by the external access point. Below is an example format of the string, indicating that the external AP network utilizes WPA2 for authentication. PSK denotes the key management method, while CCMP specifies the encryption protocol employed. ESS signifies that the network operates in standard infrastructure mode. Example: [WPA2-PSK-CCMP][ESS]                */
    int16_t    signalStrength;      /**< The detected signal level, measured in dBm and referred to as RSSI, ranges from -100 dBm for the weakest signal to 0 dBm for the strongest possible signal strength  */
};


/**
 * Station Scan result
 */
struct StaScanResult {
    Id                             staId;            /**< Id of station backhaul                 */
    std::vector<ExternalApInfo>    externalApList;   /**< List of scanned External Access point details                                */
    uint8_t                        batchIndex;       /**< This value serves as the order index for the batched scan results. Batching is performed when the number of APs in the scan results exceeds the capacity of a single batch, the details are communicated through multiple indications. i.e. in batches           */
    bool                           isScanComplete;   /**< Indicates this scan result is the last of the batches.                        */
};

/** @addtogroup telematics_wlan_station
 * @{ */

/**
 * @brief  Manager class for configuring Wlan Station Mode
 */
class IStaInterfaceManager {
 public:
    /**
     * Set Station IP Configurations: Set Station IP configuration dynamic/static and static IP
     * address if selected. If API is called when WLAN is disabled, changes will take effect when
     * WLAN is enabled using @ref telux::wlan::IWlanDeviceManager::enable API.
     * If API is called when WLAN is enabled, changes will take effect after restarting
     * wpa_supplicant by calling @ref telux::wlan::IStaInterfaceManager::manageStaService
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     * @param [in] ipConfig                Static/Dynamic IP configuration
     *                                     @ref telux::wlan::StaIpConfig.
     * @param [in] staticIpConfig          Static IP configuration, not used if station was
     *                                     configured to use dynamic IP.
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setIpConfig(Id staId, StaIpConfig ipConfig,
        StaStaticIpConfig staticIpConfig) = 0;

    /**
     * Set Station backhaul to act as router or bridge: Sets Station to act as router or bridge
     * where station internal clients get public IP addresses.
     * If API is called when WLAN is disabled, changes will take effect when WLAN is enabled using
     * @ref telux::wlan::IWlanDeviceManager::enable API.
     * If API is called when WLAN is enabled, changes will take effect after restarting
     * wpa_supplicant by calling @ref telux::wlan::IStaInterfaceManager::manageStaService
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     * @param [in] bridgeMode              bridgeMode @ref telux::wlan::StaBridgeMode
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setBridgeMode(Id staId, StaBridgeMode bridgeMode) = 0;

    /**
     * Enable Hotspot 2.0 Support
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     * @param [in] enable                  True: enable Hotspot support, False disable support
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode enableHotspot2(Id staId, bool enable) = 0;

    /**
     * Request current station configurations: Returns configurations set by
     * @ref telux::wlan::IStaInterfaceManager::setIpConfig and
     * @ref telux::wlan::IStaInterfaceManager::setBridgeMode
     *
     * @param [in] config         Station configurations @ref telux::wlan::StaConfig
      *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getConfig(std::vector<StaConfig>& config) = 0;

    /**
     * Request current station status: Returns current Sta interface status such as network
     * interface name and IP address.
     *
     * @param [in] status         Station Status @ref telux::wlan::StaStatus
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getStatus(std::vector<StaStatus>& status) = 0;

    /**
     * Initiates a scan for available Wi-Fi access points in the vicinity.
     * Scan results are notified via @ref telux::wlan::IStaListener::onScanResultUpdated.
     * This API should be called only after WLAN is enabled using
     * @ref telux::wlan::IWlanDeviceManager::enable API and required number of STAs are
     * configured using @ref telux::wlan::IDeviceManager::setMode
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId                   Station Identifier @ref telux::wlan::Id
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode startScan(Id staId) = 0;

    /**
     * Add a network configuration entry to configure various parameters associated with a
     * specific network and store the saved network settings persistently. This request will
     * internally attempt to connect to the SSID tied to the configured network if there is no
     * active connection, regardless of its priority compared to other configured networks,
     * provided that the network is not disabled. The connection status is notified via
     * @ref telux::wlan::IStaListener::onStationStatusChanged.
     * This API should be called only after WLAN is enabled using
     * @ref telux::wlan::IWlanDeviceManager::enable API and required number of STAs are
     * configured using @ref telux::wlan::IDeviceManager::setMode
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId          Station Identifier @ref telux::wlan::Id
     * @param [in] network        Station network config entry
     *                            @ref telux::wlan::StaNetworkConfigEntry
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode addNetworkConfig(Id staId,
        const StaNetworkConfigEntry &network) = 0;

    /**
     * Remove the specified network configuration entry from the saved network configurations.
     * Subsequently, disconnect from the external AP if it is the active connection.
     * Upon disconnection, an attempt is made to connect to another saved network, based on
     * priority or availability.
     * Connection status is notified via @ref telux::wlan::IStaListener::onStationStatusChanged.
     * This API should be called only after WLAN is enabled using
     * @ref telux::wlan::IWlanDeviceManager::enable API and required number of STAs are
     * configured using @ref telux::wlan::IDeviceManager::setMode
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId          Station Identifier @ref telux::wlan::Id
     * @param [in] networkId      Station network Id @ref telux::wlan::NetworkId
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode removeNetworkConfig(Id staId, NetworkId networkId) = 0;

    /**
     * Retrieve the list of network block entries stored in the saved network configurations.
     * This API should be called only after WLAN is enabled using
     * @ref telux::wlan::IWlanDeviceManager::enable API and required number of STAs are
     * configured using @ref telux::wlan::IDeviceManager::setMode
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId          Station Identifier @ref telux::wlan::Id
     * @param [in] network        Vector of Station network configs from persistent
     *                            configuration @ref telux::wlan::StaNetworkConfigInfo
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode getNetworkConfigs(Id staId,
        std::vector<StaNetworkConfigInfo>& network) = 0;

    /**
     * Connect to the specified network ID from the saved network configurations. Details on
     * the connection status are notified via @ref telux::wlan::IStaListener::onStationStatusChanged.
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId          Station Identifier @ref telux::wlan::Id
     * @param [in] networkId      Station network Id @ref telux::wlan::NetworkId
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode connect(Id staId, NetworkId networkId) = 0;

    /**
     * Disconnect from the active station connection. Details on the disconnection status are
     * notified via @ref telux::wlan::IStaListener::onStationStatusChanged.
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId          Station Identifier @ref telux::wlan::Id
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode disconnect(Id staId) = 0;

    /**
     * Execute an operation on wpa_supplicant service. Provides ability for client to either
     * stop/start or restart wpa_supplicant service for selected station.
     * Restarting wpa_supplicant service is required for any changes made to wpa_supplicant.conf
     * file to take effect.
     * Station selected to execute operation on, will temporarily go out of service when this
     * API is called.
     * This API should be called only after station mode is configured through
     * @ref telux::wlan::IDeviceManager::setMode
     *
     * On platforms with Access control enabled, caller needs to have TELUX_WLAN_STA_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] staId         Station identifier to execute operation on. @ref telux::wlan::Id
     * @param [in] opr           Operation to be performed on wpa_supplicant
     *                           @ref telux::wlan::ServiceOperation
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode manageStaService(Id staId, ServiceOperation opr) = 0;

    /**
     * Register as a listener for specific events defined in telux::wlan::IStaListener
     *
     * @param [in] listener    pointer of IStaListener object that processes the
     * notification
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IStaListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IStaListener object that needs to be removed
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IStaListener> listener) = 0;

    virtual ~IStaInterfaceManager(){};
};

class IStaListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when Station Status Changes
     *
     * @param [in] status     List of station state @ref telux::wlan::StaStatus
     */
    virtual void onStationStatusChanged(std::vector<StaStatus> staStatus) {}

    /**
     * This function is triggered upon receiving the station scan results. The results may be
     * received in batches. The final indication in the sequence is identified by the flag
     * @ref telux::wlan::StaScanResult::isScanComplete being set to true.
     * This indication is sent exclusively to the client that initiated the scan request via
     * @ref telux::wlan::IStaInterfaceManager::startScan
     *
     * @param [in] staScanResult     Station scan result @ref telux::wlan::StaScanResult
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */

    virtual void onScanResultUpdated(const StaScanResult &staScanResult) {}

    /**
     * This function is called when Station switch to different operation band
     *
     * @param [in] band        New Station operation band @ref telux::wlan::BandType
     */
    virtual void onStationBandChanged(BandType band) {}

    virtual ~IStaListener() {}
};

/** @} */ /* end_addtogroup telematics_wlan_station */
}
}
#endif // TELUX_WLAN_STAINTERFACEMANAGER_HPP
