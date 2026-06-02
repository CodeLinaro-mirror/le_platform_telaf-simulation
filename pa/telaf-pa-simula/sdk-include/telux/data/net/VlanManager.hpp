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
 * @file       VlanManager.hpp
 *
 * @brief      VlanManager is a primary interface for configuring VLAN (Virtual Local Area Network).
 *             it provide APIs for create, query, remove VLAN interfaces and associate or
               disassociate with profile IDs
 *
 */

#ifndef TELUX_DATA_NET_VLANMANAGER_HPP
#define TELUX_DATA_NET_VLANMANAGER_HPP

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
class IVlanListener;

struct VlanBindConfig {
    int vlanId;                         /**< VLAN ID to be bound to the specified backhaul     */
    BackhaulInfo bhInfo;                /**< Configuration of Backhaul to bind VLAN to        */
};

/**
 * This function is called as a response to @ref createVlan()
 *
 * @param [in] isAccelerated              Offload status returned by server
 * @param [in] error                      Return code which indicates whether the operation
 *                                        succeeded or not @ref telux::common::ErrorCode
 *
 */
using CreateVlanCb = std::function<void(bool isAccelerated, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref queryVlanInfo()
 *
 * @param [in] configs         List of VLAN configs
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref telux::common::ErrorCode
 *
 */
using QueryVlanResponseCb
    = std::function<void(const std::vector<VlanConfig> &configs, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref queryVlanMappingList()
 *
 * @param [in] mapping         List of profile Id and VLAN id map
 *                             Key is Profile Id and value is VLAN id
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref telux::common::ErrorCode
 *
 */
using VlanMappingResponseCb = std::function<void(
    const std::list<std::pair<int, int>> &mapping, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref queryVlanToBackhaulBindings()
 *
 * @param [in] bindings        list of Vlan binding configurations
 *                             @ref telux::data::net::VlanBindConfig
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref telux::common::ErrorCode
 *
 */
using VlanBindingsResponseCb = std::function<void(
    const std::vector<VlanBindConfig> bindings, telux::common::ErrorCode error)>;


/**
 *@brief       VlanManager is a primary interface for configuring VLAN (Virtual Local Area Network).
 *             it provide APIs for create, query, remove VLAN interfaces and associate or
 *             disassociate with profile IDs.
 *             It also provides interface to Subsystem Restart events by registering as listener.
 *             Notifications will be received when modem is ready/not ready.
 */
class IVlanManager {
 public:
    /**
     * Checks the status of VLAN manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If VLAN manager object is ready for service.
     *          SERVICE_UNAVAILABLE    If VLAN manager object is temporarily unavailable.
     *          SERVICE_FAILED       - If VLAN manager object encountered an irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if the VLAN manager subsystem is ready.
     *
     * @returns True if VLAN Manager is ready for service, otherwise
     * returns false.
     *
     * @deprecated Use getServiceStatus API.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for VLAN manager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when VLAN manager is ready.
     *
     * @deprecated Use InitResponseCb callback in factory API getVlanManager.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Create a VLAN associated with multiple interfaces
     * Creates VLAN on hardware interface @ref telux::data::InterfaceType, assigns VLAN id, assigns
     * VLAN priority level (according to IEEE 802.1p priority code point-PCP), assigns network type,
     * sets whether traffic on this VLAN needs to be accelerated and sets the option to create the
     * VLAN with bridge or not.
     * 
     * The creation of VLANs with bridge is not allowed for @ref telux::data::NetworkType::WAN.
     *
     * If platform does not support assigning priorities to VLANs and priority is set to value
     * other than 0, @ref telux::common::Status::NOTSUPPORTED is returned.
     * If platform supports Vlan priority, all traffic coming from WWAN or LAN are stamped with
     * priority before sending traffic to tethered client.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @note       if interface configured as VLAN for the first time, it may trigger auto reboot.
     *
     * @param [in] vlanConfig       VLAN configuration
     * @param [out] callback        optional callback to get the response createVlan
     *
     * @returns Immediate status of createVlan() request sent i.e. success or suitable status
     * code.
     *
     *
     */
    virtual telux::common::Status createVlan(
        const VlanConfig &vlanConfig, CreateVlanCb callback = nullptr)
        = 0;

    /**
     * Remove VLAN configuration
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @note   This will delete all clients associated with interface
     *
     * @param [in] vlanId          VLAN ID
     * @param [in] ifaceType       @ref telux::data::InterfaceType
     * @param [out] callback       optional callback to get the response removeVlan
     *
     * @returns Immediate status of removeVlan() request sent i.e. success or suitable status
     * code.
     *
     */
    virtual telux::common::Status removeVlan(
        int16_t vlanId, InterfaceType ifaceType, telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Query information about all the VLANs in the system
     *
     * @param [out] callback        Response callback with list of configured VLANs
     *
     * @returns Immediate status of queryVlanInfo() request sent i.e. success or suitable status
     * code.
     *
     */
    virtual telux::common::Status queryVlanInfo(QueryVlanResponseCb callback) = 0;

    /**
     * Bind Vlan to a particular backhaul. When network interface associated with specified
     * backhaul is brought up, VLAN traffic will be forwarded to specified backhaul via the
     * network interface.
     *
     * @note    Slot ID and profile ID are relevant only for WWAN backhaul. For all other
     * backhauls types, values are don't care.
     *
     * The behavior of this API is dependent on platform/system configuration.
     * For WWAN backhaul, if the platform is configured to allow multiple VLANs to be
     * bound to the same profile id then:
     *   - Binding multiple VLANs to any profile id can be achieved by calling this
     *     API with each VLAN id. Each VLAN will be associated with it's own bridge.
     *   - Reboot is not triggered with any bind operation.
     * For WWAN backhaul, if the platform is not configured to allow multiple VLANs
     * to be bound to the same profile id then:
     *   - Binding VLAN to default profile id will associate it with bridge0 and
     *     trigger automatic reboot.
     *   - Binding VLAN to any other profile id will associate it with own bridge.
     *   - Multiple VLAN binding attempt to any profile id will result in error
     *     telux::common::ErrorCode::INVALID_OPERATION
     * This setting will be persistent across multiple boots.
     *
     * @param [in] vlanBindConfig       Backhaul information and vlan id to bind it to.
     *                                  @ref telux::data::net::VlanBindConfig
     * @param [out] callback            Callback to get the response of bindToBackhaul API
     *
     * @returns Immediate status of bindToBackhaul() request sent i.e. success or
     * suitable status code.
     *
     */
    virtual telux::common::Status bindToBackhaul(VlanBindConfig vlanBindConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Unbind VLAN from particular backhaul. This API will stop vlan traffic flow to/from specified
     * backhaul type.
     * @note    Slot ID and profile ID are relevant only for WWAN backhaul. For all other backhauls
     * types, values are don't care.
     *
     * @param [in] vlanBindConfig       Backhaul information and vlan id to unbind it from.
     *                                  @ref telux::data::net::VlanBindConfig
     * @param [in] callback             Callback to get the response of unbindFromBackhaul API
     *
     * @returns Immediate status of unbindFromBackhaul() request sent i.e. success or
     * suitable status code
     *
     */
    virtual telux::common::Status unbindFromBackhaul(VlanBindConfig vlanBindConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Query VLAN to backhaul binding configurations
     *
     * @param [in] backhaulType     Backhaul to query vlan binding for.
     * @param [in] callback         callback to get the response of queryVlanToBackhaulBindings API
     * @param [in] slotId           Specify slot id which has the sim that contains profile id
     *                              mapping to VLAN id.
     *
     * @returns Immediate status of queryVlanToBackhaulBindings() request sent i.e. success or
     * suitable status code
     *
     */
    virtual telux::common::Status queryVlanToBackhaulBindings(
        BackhaulType backhaulType, VlanBindingsResponseCb callback,
        SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Register VLAN Manager as a listener for Data Service health events like data service
     * available or data service not available.
     *
     * @param [in] listener    pointer of IVlanListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IVlanListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IVlanListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IVlanListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Bind a VLAN with a particular profile id and slot id. When a WWAN network interface is
     * brought up using IDataConnectionManager::startDataCall on that profile id and slot id,
     * that interface will be accessible from this VLAN
     * The behavior of this API is dependent on platform/system configuration.
     * If the platform is configured to allow multiple VLANs to be bound to the same
     * profile id - slot id pair then:
     *   - Binding multiple VLANs to any profile id - slot id pair can be achieved by calling this
     *     API with each VLAN id. Each VLAN will be associated with it's own bridge.
     *   - Reboot is not triggered with any bind operation.
     * If the platform is not configured to allow multiple VLANs to be bound to the same
     * profile id - slot id pair then:
     *   - Binding VLAN to default profile id and slot id will associate it with bridge0 and
     *     trigger automatic reboot.
     *   - Binding VLAN to any other profile id and slot id will associate it with own bridge.
     *   - Multiple VLAN binding attempt to any profile id or slot id will result in error
     *     telux::common::ErrorCode::INVALID_OPERATION
     * This setting will be persistant across multiple boots.
     *
     * @param [in] profileId    profile id for VLAN association
     * @param [in] vlanId       VLAN ID to be bound to the data call brought up on the profile id
     * @param [out] callback    callback to get the response of associateWithProfileId API
     * @param [in] slotId       Specify slot id which has the sim that contains profile id.
     *
     * @returns Immediate status of associateWithProfileId() request sent i.e. success or
     * suitable status code.
     *
     * @deprecated Use bindToBackhaul() API below to bind VLAN to backhaul
     */
    virtual telux::common::Status bindWithProfile(int profileId, int vlanId,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Unbind VLAN id from given slot id and profile id
     * This setting will be persistant across multiple boots.
     *
     * @param [in] profileId    profile id for VLAN association
     * @param [in] vlanId       VLAN ID to be unbound to the data call brought up on the profile id
     * @param [in] callback     callback to get the response of associateWithProfileId API
     * @param [in] slotId       Specify slot id which has the sim that contains profile id .
     *
     * @returns Immediate status of disassociateFromProfileId() request sent i.e. success or
     * suitable status code
     *
     * @deprecated Use unbindFromBackhaul() API below to unbind VLAN to backhaul
     */
    virtual telux::common::Status unbindFromProfile(int profileId, int vlanId,
        telux::common::ResponseCallback callback = nullptr, SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Query VLAN mapping of profile id and VLAN id on specified sim
     *
     * @param [in] callback    callback to get the response of queryVlanMappingList API
     * @param [in] slotId      Specify slot id which has the sim that contains profile id
     *                         mapping to VLAN id.
     *
     * @returns Immediate status of queryVlanMappingList() request sent i.e. success or
     * suitable status code
     *
     * @deprecated Use queryVlanToBackhaulBindings() API below to request VLAN to backhaul mapping
     */
    virtual telux::common::Status queryVlanMappingList(VlanMappingResponseCb callback,
        SlotId slotId = DEFAULT_SLOT_ID) = 0;

    /**
     * Destructor for IVlanManager
     */
    virtual ~IVlanManager(){};
};  // end of IVlanManager

/**
 * Interface for VLAN listener object. Client needs to implement this interface to get
 * access to Socks services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IVlanListener : public telux::common::ISDKListener{
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when there is a change in IPA Connection Manager daemon state.
     *
     * @param [in] state   New state of IPA connection Manager daemon Active/Inactive
     *
     * @note  This is global state
     */
    virtual void onHwAccelerationChanged(const ServiceState state){};

    /**
     * Destructor for IVlanListener
     */
    virtual ~IVlanListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}
}
}
#endif // TELUX_DATA_NET_VLANMANAGER_HPP
