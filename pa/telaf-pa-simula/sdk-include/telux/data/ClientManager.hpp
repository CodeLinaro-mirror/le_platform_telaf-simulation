/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef TELUX_DATA_CLIENTMANAGER_HPP
#define TELUX_DATA_CLIENTMANAGER_HPP


#include <vector>
#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/common/SDKListener.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IClientListener;

/**
 * The event/reason that can trigger the data usage reset.
 */
enum class UsageResetReason {
    SUBSYSTEM_UNAVAILABLE = 0x00,   /**<  Subsystem is unavailable */
    BACKHAUL_SWITCHED = 0x01,       /**<  Backhaul is switched */
    DEVICE_DISCONNECTED =  0x02,    /**<  Device is disconnected */
    WLAN_DISABLED = 0x03,           /**<  WLAN is disabled  */
    WWAN_DISCONNECTED = 0x04,       /**<  WWAN is disconnected. This will be sent even if only IPv4
                                          or IPv6 goes down on an Ipv4v6 connection*/
};

/**
 * Data usage statistics.
 */
struct DataUsage {
    uint64_t bytesRx;               /**<   Bytes received by client */
    uint64_t bytesTx;               /**<   Bytes transmitted by client . */
};

/**
 * Data usage statistics for device.
 */
struct DeviceDataUsage {
    std::string macAddress;         /**<   MAC address of the client. */
    DataUsage usage;                /**<   Data usage statistics */
};

/**
 * @brief Client Manager class provides APIs related to devices and clients connected to MDM via
 * different interconnects. A device is any entity with a unique MAC address that is connected to
 * the MDM and clients are characterized by unique IP address. Clients could also be connected over
 * VLANs. Interconnects can be wired (e.g. Ethernet) or wireless (e.g. WLAN).
 */
class IClientManager {
public:

    /**
     * Checks the status of Client manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If Client manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Client manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If Client manager object encountered an irrecoverable
     *                                  failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Register listener with client manager for service status events and other notifications
     *
     * @param [in] listener    pointer of IClientListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IClientListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IClientListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IClientListener> listener) = 0;


    /**
     * Get data usage for connected devices
     *
     * This API provides the usage of a backhaul (e.g. cellular WWAN connection) on the MDM by
     * various devices. The usage does not include any traffic sent between devices within the same
     * vehicle. A device is any entity with a unique MAC address that is connected to the MDM either
     * over a wired or wireless interconnect. Device data usage monitoring should be enabled for
     * this api to work. Status of device data usage monitoring can be obtained by using
     * @ref telux::data::IDataSettingsManager::isDeviceDataUsageMonitoringEnabled.
     *
     * Statistics are reset when a backhaul switch occurs, such as switching from a WWAN interface
     * to a WLAN interface. In this case the last known statistics of the device before the reset
     * will be provided via @ref IClientListener::onDeviceDataUsageResetImminent. The statistics can
     * also be explicitly reset using @ref IClientManager::resetDataUsageStats. In this case, no
     * notification will be sent about the last known statistics before reset.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_CLIENT_INFO
     * permission to successfully invoke this API.
     *
     * @param [out] usageStats         List of data usage information, per device.
     *
     * @returns         Return code for whether the operation succeeded or failed
     *                  If usage monitoring is not enabled, INVALID_STATE is returned.
     *
     */
    virtual telux::common::ErrorCode getDeviceDataUsageStats(
        std::vector<DeviceDataUsage>& usageStats) = 0;

    /**
     * Reset data usage statistics
     *
     * The data reported via @ref IClientManager::getDeviceDataUsageStats will be reset once this
     * API is called.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_CLIENT_CONFIG
     * permission to successfully invoke this API.
     *
     * @returns     Return code for whether the operation succeeded or failed.
     *              If usage monitoring is not enabled, INVALID_STATE is returned.
     *
     */
    virtual telux::common::ErrorCode resetDataUsageStats() = 0;

};

/**
 * Interface for Client listener. Client needs to implement this interface to get
 * access to Client services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IClientListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Provides the last known statistics of a connected device, before the statistics become
     * unavailable or are reset.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_CLIENT_INFO
     * permission to successfully invoke this API.
     *
     * @param [in] usageStats            List of disconnected device(with a unique MAC) data usage
     * @param [in] reason                The event/reason that triggered the data usage reset
     *
     */
    virtual void onDeviceDataUsageResetImminent(const std::vector<DeviceDataUsage> usageStats,
        UsageResetReason reason) {}

    /**
     * Destructor for IClientListener
     */
    virtual ~IClientListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif