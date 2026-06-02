/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * @file       BridgeManager.hpp
 *
 * @brief      BridgeManager is a primary interface to enable packet acceleration for non-standard
 *             WLAN and Ethernet physical interfaces. It can configure software bridge between the
 *             the interface and Hardware accelerator. It provide APIs to enable/disable and
 *             set/get/delete software bridges for various interfaces.
 *
 */

#ifndef TELUX_DATA_NET_BRIDGEMANAGER_HPP
#define TELUX_DATA_NET_BRIDGEMANAGER_HPP

#include <future>
#include <vector>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_data_net
 * @{ */

// Forward declarations
class IBridgeListener;

/**
 * Interface types supported for bridge configuration
 */
enum class BridgeIFaceType {
    UNKNOWN = 0,
    WLAN_AP = 1,    /**< Wireless Local Area Network (WLAN) in AP mode */
    WLAN_STA = 2,   /**< Wireless Local Area Network (WLAN) in STA mode */
    ETH = 3,        /**< Ethernet (ETH) */
};

/**
 * Structure to configure a software bridge for an interface
 */
struct BridgeInfo {
    std::string  ifaceName;     /**< Interface name */
    BridgeIFaceType ifaceType;  /**< Interface type */
    uint32_t bandwidth;         /**< Bandwidth(in Mbps) required for software bridge */
};

/**
 * This function is called as a response to @ref requestBridgeInfo
 *
 * @param [in] infos        List of the software bridge configurations in the system
 * @param [in] error        Return code which indicates whether the operation is succeeded or not
 *                          @ref telux::common::ErrorCode
 *
 */
using BridgeInfoResponseCb
    = std::function<void(const std::vector<BridgeInfo> &infos, telux::common::ErrorCode error)>;

/**
 * @brief      IBridgeManager provides APIs to enable/disable and set/get/delete software bridges
 *             for various WLAN and Ethernet interfaces.
 *             It also provides interface to Subsystem Restart events by registering as listener.
 *             Notifications will be received when modem is ready/not ready.
 */
class IBridgeManager {
 public:
    /**
     * Checks the status of Bridge manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE     If Bridge manager object is ready for service.
     *          SERVICE_UNAVAILABLE   If Bridge manager object is temporarily unavailable.
     *          SERVICE_FAILED        If Bridge manager object encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if the Bridge manager subsystem is ready.
     *
     * @returns True if the Bridge Manager is ready for service, otherwise returns false.
     *
     *  @deprecated Use getServiceStatus API.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for Bridge manager subsystem to be ready.
     *
     * @returns A future that caller can wait until the Bridge Manager succeed/fail to be ready.
     *
     * @deprecated Use InitResponseCb callback in factory API getBridgeManager.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Enable/Disable the software bridge in the system.
     * It will affect all the configured software bridges for various interfaces.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] enable    TRUE to enable, FALSE to disable the bridge
     * @param [in] callback  Optional callback to get the response for enableBridge
     *
     * @returns Status of enableBridge request i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status enableBridge( bool enable,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Add software bridge configuration for an interface.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] config    configuration for an interface
     * @param [in] callback  Optional callback to get the response for addBridge
     *
     * @returns Status of addBridge request i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status addBridge( BridgeInfo config,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request information about all the software bridge configurations in the system
     *
     * @param [in] callback    Response callback with list of bridge configurations
     *
     * @returns Status of requestBridgeInfo request i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestBridgeInfo(BridgeInfoResponseCb callback) = 0;

    /**
     * Delete a software bridge configuration for an interface.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] ifaceName   Name of the interface whose configuration needs to be deleted
     * @param [in] callback    Optional callback to get the response for removeBridge
     *
     * @returns Status of removeBridge request i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status removeBridge( std::string ifaceName,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Sets the bridge associated with the hardware interface @ref telux::data::InterfaceType.
     *
     * Tethered traffic always flows through the default route (bridge0). This API provides the
     * ability to use a non-default route for data traffic, using on-demand bridges instead.
     *
     * The client shall specify the interface type on which the operation has to be executed and
     * provide the bridgeId that needs to be associated with that particular interface type.
     * If the client provides a bridgeId for a bridge that doesn't exist, the bridge is created
     * and then the interface is mapped to it.
     *
     * Mapping is persistent across reboots.
     * In case of WLAN_AP, this API should be used before the AP is configured.
     * If this API is invoked after the WLAN is enabled, the changes will be reflected only when
     * WLAN is re-enabled.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to successfully invoke this API.
     *
     * @param [in] ifaceType      The @ref telux::data::InterfaceType to be mapped
     * @param [in] bridgeId       Bridge ID that will be mapped to the hardware interface
     *
     * @returns @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode setInterfaceBridge(InterfaceType ifaceType,
        uint32_t bridgeId) = 0;

    /**
     * Gets the bridge ID mapped to a given interface type.
     *
     * @param [in] ifaceType      @ref telux::data::InterfaceType
     * @param [out] bridgeId      Bridge ID mapped to the hardware interface
     *
     * @returns @ref telux::common::ErrorCode as appropriate.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode getInterfaceBridge(InterfaceType ifaceType,
        uint32_t& bridgeId) = 0;

    /**
     * Register Bridge Manager as listener for Data Service heath events like data service available
     * or data service not available.
     *
     * @param [in] listener    pointer of IBridgeListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IBridgeListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IBridgeListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IBridgeListener> listener) = 0;

    /**
     * Destructor for IBridgeManager
     */
    virtual ~IBridgeManager(){};
};  // end of IBridgeManager

/**
 * Interface for Bridge listener object. Client needs to implement this interface to get
 * access to Bridge services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IBridgeListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for IBridgeListener
     */
    virtual ~IBridgeListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}
}
}
#endif // TELUX_DATA_NET_BRIDGEMANAGER_HPP
