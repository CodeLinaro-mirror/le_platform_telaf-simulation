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
 * @file       L2tpManager.hpp
 *
 * @brief      L2tpManager is a primary interface for configuring L2TP feature.
 *             Currently only un-managed tunnels are supported
 *
 */

#ifndef TELUX_DATA_NET_L2TPMANAGER_HPP
#define TELUX_DATA_NET_L2TPMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_data_net
 * @{ */

// Forward declarations
class IL2tpListener;


/**
 * L2TP session binding to backhaul configuration.
 */
struct L2tpSessionBindConfig {
    uint32_t locId;                 /**< Local ID of session to be bound to the specified backhaul */
    BackhaulInfo bhInfo;            /**< Configuration of backhaul to bind L2TP session*/
};

/**
 * L2TP encapsulation protocols
 */
enum class L2tpProtocol {
    NONE = 0,
    IP   = 0x01,   /**< IP Protocol used for encapsulation */
    UDP  = 0x02    /**< UDP Protocol used for encapsulation */
};

/**
 * L2TP tunnel sessions configuration
 */
struct L2tpSessionConfig {
    uint32_t locId;      /**< Local session id */
    uint32_t peerId;     /**< Peer session id  */
};

/**
 * L2TP tunnel configuration
 */
struct L2tpTunnelConfig {
    L2tpProtocol                prot;          /**< Encapsulation protocols */
    uint32_t                    locId;         /**< Local tunnel id */
    uint32_t                    peerId;        /**< Peer tunnel id */
    uint32_t                    localUdpPort;  /**< Local udp port - if UDP encapsulation is used */
    uint32_t                    peerUdpPort;   /**< Peer udp port - if IP encapsulation is used   */
    std::string                 peerIpv6Addr;  /**< Peer IPv6 Address - for Ipv6 tunnels          */
    std::string                 peerIpv6GwAddr;/**< Peer IPv6 Gateway Address - for Ipv6 tunnels  */
    std::string                 peerIpv4Addr;  /**< Peer IPv4 Address - for Ipv4 tunnels          */
    std::string                 peerIpv4GwAddr;/**< Peer IPv4 Gateway Address - for Ipv4 tunnels  */
    std::string                 locIface;      /**< interface name to create L2TP tunnel on       */
    telux::data::IpFamilyType   ipType;        /**< Ip family type @ref telux::data::IpFamilyType */
    std::vector<L2tpSessionConfig> sessionConfig;  /**< List of L2tp tunnel sessions              */
};

/**
 * L2TP Configuration
 */
struct L2tpSysConfig {
    std::vector<L2tpTunnelConfig>configList;       /**< List of L2tp tunnel configurations */
    bool enableMtu;   /**< Enable MTU size setting on underlying interfaces to avoid segmentation */
    bool enableTcpMss;   /**< Enable TCP MSS clampping on L2TP interfaces to avoid segmentation */
    uint32_t mtuSize; /**< Current MTU size in bytes */
};

/**
 * This function is called as a response to @ref requestConfig()
 *
 * @param [in] l2tpSysConfig       Current L2TP configuration
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode
 *
 */
using L2tpConfigCb
    = std::function<void(const L2tpSysConfig &l2tpSysConfig, telux::common::ErrorCode error)>;


/**
 * This function is called as a response to
 * @ref telux::data::net::IL2tpManager::querySessionToBackhaulBindings().
 *
 * @param [in] bindings        List of L2TP session binding configurations
 *                             @ref telux::data::net::L2tpSessionBindConfig
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref telux::common::ErrorCode
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change
 *          and could break backwards compatibility.
 */
using L2tpSessionBindingsResponseCb = std::function<void(
    const std::vector<L2tpSessionBindConfig> bindings, telux::common::ErrorCode error)>;

/**
 *@brief    L2tpManager is a primary interface for configuring L2TP Service.
 *          It also provides interface to Subsystem Restart events by registering as listener.
 *          Notifications will be received when modem is ready/not ready.
 */
class IL2tpManager {
 public:
    /**
     * Checks the status of L2tp manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If L2tp manager is ready for service.
     *          SERVICE_UNAVAILABLE    If L2tp manager is temporarily unavailable.
     *          SERVICE_FAILED       - If L2tp manager encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if the L2tp manager subsystem is ready.
     *
     * @returns True if L2tp Manager is ready for service, otherwise
     *          returns false.
     *
     * @note    This API will be deprecated. getServiceStatus API is recommended as an alternative
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for L2tp manager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     *          when L2tp manager is ready.
     *
     * @note    This API will be deprecated. Callback of type InitResponseCb argument in data
     *          factory API getL2tpManager is recommended as an alternative.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Enable L2TP for unmanaged Tunnel State
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] enable         Enable/Disable L2TP for unmanaged tunnels.
     * @param [in] enableMss      Enable/Disable TCP MSS to be clamped on L2TP interfaces to
     *                            avoid Segmentation
     * @param [in] enableMtu      Enable/Disable MTU size to be set on underlying interfaces to
     *                            avoid fragmentation
     * @param [in] callback       optional callback to get the response setConfig
     *
     * @param [in] mtuSize        optional MTU size in bytes. If not set, MTU size will be set to
     *                            default 1422 bytes
     * @returns Status of setConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setConfig(bool enable, bool enableMss, bool enableMtu,
        telux::common::ResponseCallback callback = nullptr, uint32_t mtuSize = 0) = 0;

    /**
     * Set L2TP Configuration for one tunnel
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] l2tpTunnelConfig     Configuration to be set @ref telux::data::net::L2tpTunnelConfig
     * @param [in] callback             Optional callback to get the response addTunnel
     *
     * @returns Status of addTunnel i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status addTunnel(const L2tpTunnelConfig &l2tpTunnelConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Get Current L2TP Configuration
     *
     * @param [in] l2tpConfigCb      Asynchronous callback to get current L2TP configurations
     *
     * @returns Status of requestConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestConfig(L2tpConfigCb l2tpConfigCb) = 0;

    /**
     * Remove L2TP Tunnel
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] tunnelId          Tunnel ID to be removed
     * @param [in] callback          optional callback to get the response removeConfig
     *
     * @returns Status of removeTunnel i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status removeTunnel(
        uint32_t tunnelId, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Adds L2TP session to the specified tunnel.
     * Adds the L2TP session to a pre-existing tunnel at run time. Existing tunnel configurations
     * and sessions are not changed by this API. This API only adds a new session to the tunnel.
     * This setting is persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] tunnelId          Tunnel ID to add the session to.
     * @param [in] sessionConfig     Configuration of added session.
     * @param [in] callback          Callback to get the addSession response; optional.
     *
     * @returns Status of addSession, i.e., success or applicable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status addSession(uint32_t tunnelId,
        L2tpSessionConfig sessionConfig, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Removes L2TP Session from specified tunnel.
     * Removes L2TP session from a pre-existing tunnel at run time. Existing tunnel configurations
     * and sessions will not change by this API. This API only removes a session from the tunnel.
     * This setting is persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] tunnelId          Tunnel ID to remove the session from
     * @param [in] sessionId         Session ID to be removed.
     * @param [in] callback          Callback to get the removeSession response; optional.
     *
     * @returns Status of removeSession, i.e. success or applicable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status removeSession(uint32_t tunnelId,
        uint32_t sessionId, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Binds L2TP Session to the specified backhaul.
     * For WWAN backhaul, sessions can be bound to both default bridge (bridge0) and on-demand
     * bridges associated with VLANs.
     * This setting is persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] sessionBindConfig   Backhaul information to bind session ID to.
     *                                 @ref telux::data::net::L2tpSessionBindConfig
     * @param [in] callback            Callback to get the bindSessionToBackhaul response; optional
     *
     * @returns Status of bindSessionToBackhaul(), i.e. success or applicable status code.
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     *
     */
    virtual telux::common::Status bindSessionToBackhaul(L2tpSessionBindConfig sessionBindConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Unbind L2TP session from the specified backhaul. This API will stop L2TP session traffic flow
     * to/from specified backhaul type.
     * This setting is persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] sessionBindConfig     Backhaul information to unbind VLAN ID from.
     *                                   @ref telux::data::net::L2tpSessionBindConfig
     * @param [in] callback              Callback to get the unbindSessionFromBackhaul response;
     *                                   optional.
     *
     * @returns Status of unbindSessionFromBackhaul(), i.e. success or applicable status code
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status unbindSessionFromBackhaul(L2tpSessionBindConfig sessionBindConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Queries L2TP session bindings to the specified backhaul.
     *
     * @param [in] backhaul    Backhaul to query L2TP session binding for.
     * @param [in] callback    callback to get the querySessionToBackhaulBindings response; optional
     *
     * @returns Status of querySessionToBackhaulBindings(), i.e. success or applicable status code
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status querySessionToBackhaulBindings(
        BackhaulType backhaul, L2tpSessionBindingsResponseCb callback) = 0;

    /**
     * Register L2TP Manager as listener for Data Service heath events like data service available
     * or data service not available.
     *
     * @param [in] listener    pointer of IL2tpListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IL2tpListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IL2tpListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IL2tpListener> listener) = 0;

    /**
     * Destructor for IL2tpManager
     */
    virtual ~IL2tpManager(){};
};  // end of IL2tpManager

/**
 * Interface for L2TP listener object. Client needs to implement this interface to get
 * access to L2TP services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IL2tpListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for IL2tpListener
     */
    virtual ~IL2tpListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}
}
}

#endif // TELUX_DATA_NET_L2TPMANAGER_HPP
