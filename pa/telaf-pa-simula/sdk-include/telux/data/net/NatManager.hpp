/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file       NatManager.hpp
 *
 * @brief      NatManager is a primary interface for configuring static network
 *             address translation(SNAT) and DMZ (demilitarized zone)
 *
 */

#ifndef TELUX_DATA_NET_NATMANAGER_HPP
#define TELUX_DATA_NET_NATMANAGER_HPP

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
class INatListener;

/**
 * Structure represents Network Address Translation (NAT) configuration
 */
struct NatConfig {
    std::string addr;    /**< Private IP address */
    uint16_t port;       /**< Private port */
    uint16_t globalPort; /**< Global port */
    IpProtocol proto;    /**< IP protocol @ref telux::data::IpProtocol */
};

/**
 * This function is called as a response to @ref requestStaticNatEntries()
 *
 * @param [in] snatEntries    list of static Network Address Translation (NAT)
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode
 *
 */
using StaticNatEntriesCb
    = std::function<void(const std::vector<NatConfig> &snatEntries, telux::common::ErrorCode error)>;

/**
 *@brief    NatManager is a primary interface for configuring static network address
 *          translation(SNAT) and DMZ (demilitarized zone).
 *          It also provides interface to Subsystem Restart events by registering as listener.
 *          Notifications will be received when modem is ready/not ready.
 */
class INatManager {
 public:
    /**
     * Checks the status of NAT manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If Nat manager object is ready for service.
     *          SERVICE_UNAVAILABLE    If Nat manager object is temporarily unavailable.
     *          SERVICE_FAILED       - If Nat manager object encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Adds a static Network Address Translation (NAT) entry in the NAT table. These
     * entries are persistent across object, connection and reboot lifetimes. To remove
     * an entry it needs an explicit call to removeStaticNatEntry() API. It supports both
     * IPv4 and IPv6
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] bhInfo            Backhaul on which static entry will be mapped to.
     * @param [in] snatConfig        snatConfiguration @ref telux::data::net::NatConfig
     * @param [in] callback          optional callback to get the response addStaticNatEntry
     *
     * @returns Status of addStaticNatEntry i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status addStaticNatEntry(const BackhaulInfo &bhInfo, const NatConfig
            &snatConfig, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Removes a static Network Address Translation (NAT) entry in the NAT table,
     * it supports both IPv4 and IPv6
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] bhInfo           Backhaul on which static entry will be removed from.
     * @param [in] snatConfig       snatConfiguration @ref telux::data::net::NatConfig
     * @param [in] callback         optional callback to get the response removeStaticNatEntry
     *
     * @returns Status of removeStaticNatEntry i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status removeStaticNatEntry(const BackhaulInfo &bhInfo, const NatConfig
            &snatConfig, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request list of static NAT entries available in the NAT table
     *
     * @param [in] bhInfo            Backhaul on which static entries will be retrieved.
     * @param [in] snatEntriesCb     Asynchronous callback to get the list of static
     *                               Network Address Translation (NAT) entries
     *
     * @returns Status of requestStaticNatEntries i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestStaticNatEntries(const BackhaulInfo &bhInfo,
        StaticNatEntriesCb snatEntriesCb) = 0;

    /**
     * Register Nat Manager as listener for Data Service health events like data service available
     * or data service not available.
     *
     * @param [in] listener    pointer of INatListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<INatListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of INatListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<INatListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Checks if the NAT manager subsystem is ready.
     *
     * @returns True if NAT Manager is ready for service, otherwise
     * returns false.
     *
     * @deprecated Use getServiceStatus API.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for NAT manager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when NAT manager is ready.
     *
     * @deprecated Use InitResponseCb callback in factory API getNatManager.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Adds a static Network Address Translation (NAT) entry in the NAT table, these
     * entries are persistent across object, connection and reboot lifetimes. To remove
     * an entry it needs a explicit call to removeStaticNatEntry() API, it supports both
     * IPv4 and IPv6
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] profileId         Profile identifier to which static entry will be mapped to.
     * @param [in] snatConfig        snatConfiguration @ref telux::data::net::NatConfig
     * @param [in] callback          optional callback to get the response addStaticNatEntry
     * @param [in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of addStaticNatEntry i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::addStaticNatEntryconst BackhaulInfo&, const NatConfig&,
     * telux::common::ResponseCallback) API to add a static Network Address Translation entry (NAT)
     * in the NAT table.
     */
    virtual telux::common::Status addStaticNatEntry(int profileId, const NatConfig &snatConfig,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Removes a static Network Address Translation (NAT) entry in the NAT table,
     * it supports both IPv4 and IPv6
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] profileId         Profile identifier to which static entry will be removed from.
     * @param [in] snatConfig        snatConfiguration @ref telux::data::net::NatConfig
     * @param [in] callback          optional callback to get the response removeStaticNatEntry
     * @param [in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of removeStaticNatEntry i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::removeStaticNatEntry(const BackhaulInfo&, const NatConfig&,
     * telux::common::ResponseCallback) API to removes a static Network Address Translation (NAT)
     * entry in the NAT table.
     */
    virtual telux::common::Status removeStaticNatEntry(int profileId, const NatConfig &snatConfig,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Request list of static nat entries available in the NAT table
     *
     * @param [in] profileId         Profile identifier to which static entries will be retrieved.
     * @param [in] snatEntriesCb     Asynchronous callback to get the list of static
     *                               Network Address Translation (NAT) entries
     * @param [in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of requestStaticNatEntries i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::requestStaticNatEntries(const BackhaulInfo&,
     * StaticNatEntriesCb) API to request list of static nat entries in the NAT table.
     */
    virtual telux::common::Status requestStaticNatEntries(int profileId,
        StaticNatEntriesCb snatEntriesCb, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Destructor for INatManager
     */
    virtual ~INatManager(){};
};  // end of INatManager

/**
 * Interface for Nat listener object. Client needs to implement this interface to get
 * access to Nat services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class INatListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for INatListener
     */
    virtual ~INatListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}
}
}

#endif // TELUX_DATA_NET_NATMANAGER_HPP
