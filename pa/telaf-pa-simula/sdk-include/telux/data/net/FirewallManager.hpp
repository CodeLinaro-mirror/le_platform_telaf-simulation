/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file       FirewallManager.hpp
 *
 * @brief      FirewallManager is a primary interface that filters and controls the network
 *             traffic on a pre-configured set of rules.
 *
 */

#ifndef TELUX_DATA_NET_FIREWALLMANAGER_HPP
#define TELUX_DATA_NET_FIREWALLMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>
#include <telux/data/IpFilter.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_data_net
 * @{ */

// Forward declarations
class IFirewallEntry;
class IFirewallListener;

/**
 * Firewall configuration parameters
 */
struct FirewallConfig {
    BackhaulInfo bhInfo;        /**< Backhaul Information to apply firewal settings on       */
    bool   enable;              /**< True: Firewall enabled. False: Firewall disabled        */
    bool   allowPackets;        /**< True: Packets that match rules will be allowed.         */
                                /**< False: Packets that match rules will be dropped         */
};

/**
 * DMZ configuration parameters
 */
struct DmzConfig {
    BackhaulInfo bhInfo;        /**< Backhaul Information to apply firewal settings on       */
    std::string ipAddr;         /**< IP address for which DMZ will be enabled                */
};

/**
 * Firewall rules parameters
 */
struct FirewallEntryInfo {
    std::shared_ptr<IFirewallEntry> fwEntry;
                                 /**< Shared pointer to firewall rules for the backhaul       */
    BackhaulInfo bhInfo;       /**< Backhaul Information to add firewal rules on       */
};

/**
 * This function is called as a response to @ref requestFirewallConfig()
 *
 * @param [in] config            Firewall configuration status for specific backhaul
 *                               @ref telux::data::FirewallConfig.
 * @param [in] error             Return code which indicates whether the operation
 *                               succeeded or not. @ref telux::common::ErrorCode
 *
*/
using FirewallConfigCb
    = std::function<void(FirewallConfig status, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestFirewallEntries()
 *
 * @param [in] entries           Vector of firewall entries for specific backhaul
 * @param [in] error             Return code which indicates whether the operation
 *                               succeeded or not. @ref telux::common::ErrorCode
 *
 */
using FirewallEntryInfoCb = std::function<void(
    std::vector<FirewallEntryInfo> entry, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestDmzEntries()
 *
 * @param [in] dmzEntries     list of dmz entries configurations
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not. @ref telux::common::ErrorCode
 *
 */
using DmzEntryInfoCb
    = std::function<void(std::vector<DmzConfig> dmzEntries, telux::common::ErrorCode error)>;


/**
 * This function is called as a response to @ref requestFirewallStatus()
 *
 * @param [in] enable            Indicates whether the firewall is enabled
 * @param [in] allowPackets      Indicates whether to accept or drop packets
 *                               matching the rules
 * @param [in] error       -     Return code which indicates whether the operation
 *                               succeeded or not. @ref telux::common::ErrorCode
 *
 */
using FirewallStatusCb
    = std::function<void(bool enable, bool allowPackets, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to
 * @ref IFirewallManager::requestFirewallEntries() and
 * @ref IFirewallManager::requestHwAccelerationFirewallEntries()
 *
 * @param [in] entries           list of firewall entries
 * @param [in] error       -     Return code which indicates whether the operation
 *                               succeeded or not. @ref telux::common::ErrorCode
 *
 */
using FirewallEntriesCb = std::function<void(
    std::vector<std::shared_ptr<IFirewallEntry>> entries, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestDmzEntries()
 *
 * @param [in] dmzEntries     list of dmz entries
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not. @ref telux::common::ErrorCode
 *
 */
using DmzEntriesCb
    = std::function<void(std::vector<std::string> dmzEntries, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref addHwAccelerationFirewallEntry()
 *
 * @param [in] handle         handle of the firewall entry. It can be used to remove firewall entry
 *                            @ref IFirewallManager::removeFirewallEntry.
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not. @ref telux::common::ErrorCode
 *
 */
using AddFirewallEntryCb
    = std::function<void(const uint32_t handle, telux::common::ErrorCode error)>;

/**
 *@brief    FirewallManager is a primary interface that filters and controls the network
 *          traffic on a pre-configured set of rules.
 *          It also provides interface to Subsystem Restart events by registering as listener.
 *          Notifications will be received when modem is ready/not ready.
 */
class IFirewallManager {
 public:
    /**
     * Checks the status of Firewall manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    If Firewall manager object is ready for service.
     *          SERVICE_UNAVAILABLE  If Firewall manager object is temporarily unavailable.
     *          SERVICE_FAILED       If Firewall manager object encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if the Firewall manager subsystem is ready.
     *
     * @returns True if Firewall Manager is ready for service, otherwise
     * returns false.
     *
     * @deprecated Use getServiceStatus API.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for Firewall manager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when firewall manager is ready.
     *
     * @deprecated Use InitResponseCb callback in factory API getFirewallManager.
     */
    virtual std::future<bool> onSubsystemReady() = 0;
    /**
     * Sets firewall configuration to enable or disable firewall and update configuration to
     * drop or accept the packets matching the rules on slot ID, profile ID and backhaul type.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] config            Firewall configuration @ref telux::data::FirewallConfig.
     *
     * @returns Status of setFirewallConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setFirewallConfig(FirewallConfig fwConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request status of firewall settings on specific backhaul
     *
     * @param [in] bhInfo            Backhaul Information to request firewall status for.
     * @param [in] callback          callback to get the response of requestFirewallConfig
     *
     * @returns Status of requestFirewallConfig i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestFirewallConfig(BackhaulInfo bhInfo,
        FirewallConfigCb callback) = 0;

    /**
     * Adds the firewall rule to specific backhaul
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] entries          Firewall rules entries settings.
     * @param [in] callback         optional callback to get the response addFirewallEntry
     *
     * @returns Status of addFirewallEntry i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status addFirewallEntry(FirewallEntryInfo entry,
        AddFirewallEntryCb callback = nullptr) = 0;

    /**
     * Request Firewall rules for specific backhaul
     *
     * @param [in] bhInfo          Backhaul Information to request firewall entries for.
     * @param [in] callback          callback to get the response requestFirewallEntries.
     *
     * @returns Status of requestFirewallEntries i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestFirewallEntries(BackhaulInfo bhInfo,
        FirewallEntryInfoCb callback) = 0;

    /**
     * Add Hardware Acceleration Rule
     * Adds a firewall rule which will direct all traffic that matches the rule to bypass hardware
     * acceleration, and take the software path.
     *
     * These rules are per PDN. If the same rule applies to more than one PDN then this API needs to
     * be invoked per PDN by specifying the corresponding profile ID of the PDN.
     * @ref setFirewall is not required for hw acceleration firewall rules to have an effect, which
     * means that as soon as the rule is added successfully, packets matching the firewall rule will
     * not be hw accelerated.
     * Irrespective of whether firewall rules are set via @ref addFirewallEntry and the type of
     * firewall set (blacklist/whitelist) via @ref setFirewall, any packet matching rule added by
     * @ref addHwAccelerationFirewallEntry will not be hw accelerated and this packet will be routed
     * by the S/w stack.
     * On successful execution, a firewall handle will be provided in the callback which can be
     * used to remove the firewall entry @ref removeFirewallEntry().
     *
     * @param [in] entries          Firewall rules entries settings.
     * @param [in] callback         optional callback to get the response addFirewallEntry
     *
     * @returns Status of addHwAccelerationFirewallEntry i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status addHwAccelerationFirewallEntry(FirewallEntryInfo entry,
        AddFirewallEntryCb callback = nullptr) = 0;

    /**
     * Request Hardware Acceleration rules
     * Returns a list of hardware acceleration firewall entries.
     *
     * @param [in] bhInfo          Backhaul Information to request firewall entries for.
     * @param [in] callback          callback to get the response requestFirewallEntries.
     *
     * @returns Status of requestHwAccelerationFirewallEntries i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestHwAccelerationFirewallEntries(BackhaulInfo bhInfo,
        FirewallEntryInfoCb callback) = 0;

    /**
     * Remove firewall entry set on particular backhaul
     *
     * @param [in] bhInfo           Backhaul information to remove firewall entries from.
     * @param[in] handle            handle of Firewall entry to be removed. To retrieve the handle,
     *                              first use requestFirewallEntries() to get the list of entries
     *                              added in the system. And then use IFirewallEntry::getHandle()
     * @param[in] callback          callback to get the response removeFirewallEntry
     *
     * @returns Status of removeFirewallEntry i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status removeFirewallEntry(BackhaulInfo bhInfo, uint32_t handle,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Enable demilitarized zone (DMZ) on particular backhaul
     *
     * @param [in] config        DMZ configuration to be enabled
     * @param [in] callback      optional callback to get the response addDmz
     *
     * @returns Status of enableDmz i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status enableDmz(DmzConfig config,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Disable demilitarized zone (DMZ) on particular backhaul
     *
     * @param [in] bhInfo        Backhaul on which DMZ will be disabled.
     * @param [in] ipType        Specify IP type of the DMZ to be disabled
     * @param [in] callback      optional callback to get the response removeDmz
     *
     * @returns Status of disableDmz i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status disableDmz(BackhaulInfo bhInfo, const IpFamilyType ipType,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request DMZ entry on particulat backhaul that was previously set using enableDmz API
     *
     * @param [in] bhInfo          Backhaul info on which DMZ entries are requested.
     * @param [in] callback        callback to get the response requestDmzEntry
     *
     * @returns Status of requestDmzEntry i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status requestDmzEntry(BackhaulInfo bhInfo, DmzEntryInfoCb callback) = 0;

    /**
     * Register Firewall Manager as listener for Data Service heath events like data service
     * available or data service not available.
     *
     * @param [in] listener    pointer of IFirewallListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IFirewallListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IFirewallListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IFirewallListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Sets firewall configuration to enable or disable and update configuration to
     * drop or accept the packets matching the rules.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] profileId         Profile identifier on which firewall will be set.
     * @param [in] enable            Indicates whether the firewall is enabled
     * @param [in] allowPackets      Indicates whether to accept or drop packets
     *                               matching the rules
     * @param [in] callback          optional callback to get the response setFirewall
     * @param [in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of setFirewall i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::setFirewallConfig API to set firewall on any backhaul
     *
     */
    virtual telux::common::Status setFirewall(int profileId, bool enable, bool allowPackets,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Request status of firewall
     *
     * @param [in] profileId         Profile identifier for which firewall status is requested.
     * @param [in] callback          callback to get the response of requestFirewallStatus
     * @param [in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of requestFirewallStatus i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::requestFirewallConfig API to request firewall status
     *             on any backhaul
     */
    virtual telux::common::Status requestFirewallStatus(int profileId,
        FirewallStatusCb callback, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Adds the firewall rule
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] profileId        Profile identifier on which firewall rule will be added.
     * @param [in] entry            Firewall entry based on protocol type
     * @param [in] callback         optional callback to get the response addFirewallEntry
     * @param [in] slotId           Specify slot id which has the sim that contains profile id
     *
     * @returns Status of addFirewallEntry i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::addFirewallEntry API to add firewall rule on any backhaul
     */
    virtual telux::common::Status addFirewallEntry(int profileId,
        std::shared_ptr<IFirewallEntry> entry, telux::common::ResponseCallback callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Add Hardware Acceleration Rule
     * Adds a firewall rule which will direct all traffic that matches the rule to bypass hardware
     * acceleration, and take the software path.
     *
     * These rules are per PDN. If the same rule applies to more than one PDN then this API needs to
     * be invoked per PDN by specifying the corresponding profile ID of the PDN.
     * @ref setFirewall is not required for hw acceleration firewall rules to have an effect, which
     * means that as soon as the rule is added successfully, packets matching the firewall rule will
     * not be hw accelerated.
     * Irrespective of whether firewall rules are set via @ref addFirewallEntry and the type of
     * firewall set (blacklist/whitelist) via @ref setFirewall, any packet matching rule added by
     * @ref addHwAccelerationFirewallEntry will not be hw accelerated and this packet will be routed
     * by the S/w stack.
     * On successful execution, a firewall handle will be provided in the callback which can be
     * used to remove the firewall entry @ref removeFirewallEntry().
     *
     * @param [in] profileId        Profile identifier on which firewall rule will be added.
     * @param [in] entry            Firewall entry based on protocol type
     * @param [in] callback         optional callback to get the response
     *                              @ref addHwAccelerationFirewallEntry
     * @param [in] slotId           Specify slot id which has the sim that contains profile id
     *
     * @returns Status of addHwAccelerationFirewallEntry i.e. success or suitable status code.
     *
     * @deprecated  Use @ref telux::data::addHwAccelerationFirewallEntry API to add hardware
     *              acceleration firewall rule on any backhaul
     */
    virtual telux::common::Status addHwAccelerationFirewallEntry(int profileId,
        std::shared_ptr<IFirewallEntry> entry, AddFirewallEntryCb callback = nullptr,
        SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Request Hardware Acceleration rules
     * Returns a list of hardware acceleration firewall entries.
     *
     * @param [in] profileId       Profile identifier on which firewall entries are retrieved
     * @param [in] callback        callback to get the response requestHwAccelerationFirewallEntries
     * @param [in] slotId          Specify slot id which has the sim that contains profile id
     *
     * @returns Status of requestHwAccelerationFirewallEntries i.e. success or suitable status code.
     *
     * @deprecated  Use @ref telux::data::requestHwAccelerationFirewallEntries API to request
     *              hardware acceleration firewall rule on any backhaul
     */
    virtual telux::common::Status requestHwAccelerationFirewallEntries(int profileId,
        FirewallEntriesCb callback, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Request Firewall rules
     *
     * @param [in] profileId         Profile identifier on which firewall entries are retrieved.
     * @param [in] callback          callback to get the response requestFirewallEntries.
     * @param [in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of requestFirewallEntries i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::requestFirewallEntries API to request firewall rules on
     *             any backhaul
     */
    virtual telux::common::Status requestFirewallEntries(int profileId,
        FirewallEntriesCb callback, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Remove firewall entry
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param[in] profileId         Profile identifier on which firewall entry will be removed.
     * @param[in] handle            handle of Firewall entry to be removed. To retrieve the handle,
     *                              first use requestFirewallEntries() to get the list of entries
     *                              added in the system. And then use IFirewallEntry::getHandle().
     *                              Handle is also returned when hardware acceleration rule is added
     *                              using @ref addHwAccelerationFirewallEntry
     * @param[in] callback          callback to get the response removeFirewallEntry
     * @param[in] slotId            Specify slot id which has the sim that contains profile id
     *
     * @returns Status of removeFirewallEntry i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::removeFirewallEntry API to remove firewall rule
     *             from any backhaul
     */
    virtual telux::common::Status removeFirewallEntry(int profileId, uint32_t handle,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Enable demilitarized zone (DMZ)
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] profileId     Profile identifier on which DMZ will be enabled.
     * @param [in] ipAddr        IP address for which DMZ will be enabled
     * @param [in] callback      optional callback to get the response addDmz
     * @param [in] slotId        Specify slot id which has the sim that contains profile id
     *
     * @returns Status of enableDmz i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::enableDmz API to enable DMZ on any backhaul
     */
    virtual telux::common::Status enableDmz(int profileId, const std::string ipAddr,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Disable demilitarized zone (DMZ)
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] profileId     Profile identifier on which DMZ will be disabled.
     * @param [in] ipType        Specify IP type of the DMZ to be disabled
     * @param [in] callback      optional callback to get the response removeDmz
     * @param [in] slotId        Specify slot id which has the sim that contains profile id
     *
     * @returns Status of disableDmz i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::disableDmz API to Disable DMZ on any backhaul
     */
    virtual telux::common::Status disableDmz(int profileId, const telux::data::IpFamilyType ipType,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Request DMZ entry that was previously set using enableDmz API
     *
     * @param [in] profileId     Profile identifier on which DMZ entries are requested.
     * @param [in] dmzCb         callback to get the response requestDmzEntry
     * @param [in] slotId        Specify slot id which has the sim that contains profile id
     *
     * @returns Status of requestDmzEntry i.e. success or suitable status code.
     *
     * @deprecated Use @ref telux::data::requestDmzEntry API to request DMZ on any backhaul
     */
    virtual telux::common::Status requestDmzEntry(int profileId,
        DmzEntriesCb dmzCb, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Destructor for IFirewallManager
     */
    virtual ~IFirewallManager(){};
};  // end of IFirewallManager

/**
 * @brief   Firewall entry class is used for configuring firewall rules
 */
class IFirewallEntry {
 public:
    static const uint32_t INVALID_HANDLE = 0;

    /**
     * Get IProtocol filter type
     *
     * @returns @ref telux::data::IIpFilter.
     *
     */
    virtual std::shared_ptr<IIpFilter> getIProtocolFilter() = 0;

    /**
     * Get firewall direction
     *
     * @returns @ref telux::data::Direction.
     *
     */
    virtual telux::data::Direction getDirection() = 0;

    /**
     * Get Ip FamilyType
     *
     * @returns @ref telux::data::IpFamilyType.
     *
     */
    virtual telux::data::IpFamilyType getIpFamilyType() = 0;

    /**
     * Get the unique handle identifying this Firewall entry in the system
     *
     * @returns uint32_t handle if initialized or INVALID_HANDLE otherwise
     *
     */
    virtual uint32_t getHandle() = 0;

    /**
     * Destructor for IFirewallEntry
     */
    virtual ~IFirewallEntry(){};
};

/**
 * Interface for Firewall listener object. Client needs to implement this interface to get
 * access to Firewall services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IFirewallListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for IFirewallListener
     */
    virtual ~IFirewallListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}
}
}
#endif // TELUX_DATA_NET_FIREWALLMANAGER_HPP
