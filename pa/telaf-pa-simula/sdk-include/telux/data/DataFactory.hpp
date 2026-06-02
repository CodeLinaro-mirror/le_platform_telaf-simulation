/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 * @file       DataFactory.hpp
 *
 * @brief      DataFactory is the central factory to create all data instances
 *
 */

#ifndef TELUX_DATA_DATAFACTORY_HPP
#define TELUX_DATA_DATAFACTORY_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/DataSettingsManager.hpp>
#include <telux/data/IpFilter.hpp>
#include <telux/data/DataLinkManager.hpp>

#include <telux/data/net/FirewallManager.hpp>
#include <telux/data/net/NatManager.hpp>
#include <telux/data/net/VlanManager.hpp>
#include <telux/data/net/SocksManager.hpp>
#include <telux/data/net/BridgeManager.hpp>
#include <telux/data/net/L2tpManager.hpp>
#include <telux/data/ClientManager.hpp>
#include <telux/data/DualDataManager.hpp>
#include <telux/data/DataControlManager.hpp>
#include <telux/data/net/QoSManager.hpp>
#include <telux/data/KeepAliveManager.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 *@brief DataFactory is the central factory to create all data classes
 *
 */
class DataFactory {
 public:
    /**
     * Get Data Factory instance.
     */
    static DataFactory &getInstance();

    /**
     * Get Data Connection Manager
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              DataConnectionManager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataConnectionManager
     *
     */
    virtual std::shared_ptr<IDataConnectionManager> getDataConnectionManager(
        SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Data Profile Manager
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              DataProfileManager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataProfileManager
     *
     */
    virtual std::shared_ptr<IDataProfileManager> getDataProfileManager(
        SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Serving System Manager
     *
     * @param [in] slotId            Unique identifier for the SIM slot
     * @param [in] clientCallback    Optional callback to get the initialization status of
     *                               ServingSystemManager @ref telux::common::InitResponseCb
     *
     * @returns instance of IServingSystemManager
     *
     */
    virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
        SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Data Filter Manager instance
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Serving System Manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataFilterManager.
     *
     */
    virtual std::shared_ptr<IDataFilterManager> getDataFilterManager(
        SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Network Address Translation(NAT) Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              NAT manager @ref telux::common::InitResponseCb
     *
     * @returns instance of INatManager or nullptr if NAT management is not supported
     *
     */
    virtual std::shared_ptr<telux::data::net::INatManager> getNatManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Firewall Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Firewall manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IFirewallManager or nullptr if Firewall management is not supported
     *
     */
    virtual std::shared_ptr<telux::data::net::IFirewallManager> getFirewallManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Firewall entry based on IP protocol and set respective filter (i.e. TCP or UDP)
     *
     * @param [in] proto            @ref telux::data::IpProtocol
     * @param [in] direction        @ref telux::data::Direction
     * @param [in] ipFamilyType     Identifies IP family type @ref telux::data::IpFamilyType
     *
     * @returns instance of IFirewallEntry
     *
     */
    virtual std::shared_ptr<telux::data::net::IFirewallEntry> getNewFirewallEntry(
        IpProtocol proto, Direction direction, IpFamilyType ipFamilyType) = 0;

    /**
     * Get IIpFilter instance based on IP Protocol, This can be used in Firewall Manager and
     * Data Filter Manager
     *
     * @param [in] proto    @ref telux::data::IpProtocol
     *                      Some sample protocol values are
     *                      ICMP = 1    # Internet Control Message Protocol - RFC 792
     *                      IGMP = 2    # Internet Group Management Protocol - RFC 1112
     *                      TCP = 6     # Transmission Control Protocol - RFC 793
     *                      UDP = 17    # User Datagram Protocol - RFC 768
     *                      ESP = 50    # Encapsulating Security Payload - RFC 4303
     *
     * @returns instance of IIpFilter based on IpProtocol filter (i.e TCP, UDP)
     *
     */
    virtual std::shared_ptr<IIpFilter> getNewIpFilter(IpProtocol proto) = 0;

    /**
     * Get VLAN Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Vlan manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IVlanManager
     *
     */
    virtual std::shared_ptr<telux::data::net::IVlanManager> getVlanManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Socks Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Socks manager @ref telux::common::InitResponseCb
     *
     * @returns instance of ISocksManager or nullptr if Socks management is not supported
     *
     */
    virtual std::shared_ptr<telux::data::net::ISocksManager> getSocksManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Software Bridge Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Bridge manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IBridgeManager
     *
     */
    virtual std::shared_ptr<telux::data::net::IBridgeManager> getBridgeManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get L2TP Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              L2TP manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IL2tpManager
     *
     */
    virtual std::shared_ptr<telux::data::net::IL2tpManager> getL2tpManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get Data Settings Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Data Settings manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataSettingsManager
     *
     */
    virtual std::shared_ptr<telux::data::IDataSettingsManager> getDataSettingsManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr)
        = 0;

    /**
     * Get Client Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              ClientManager @ref telux::common::InitResponseCb.
     *
     * @returns instance of IClientManager
     *
     */
    virtual std::shared_ptr<IClientManager> getClientManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get DualData Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              DuaData manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDualDataManager
     *
     */
    virtual std::shared_ptr<telux::data::IDualDataManager> getDualDataManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Get DataControl Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              DataControl manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataControlManager
     *
     */
    virtual std::shared_ptr<telux::data::IDataControlManager> getDataControlManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Gets the QoS manager instance.
     *
     *  @param [in] clientCallback   Optional callback to get the initialization status of
     *                               IQoSManager @ref telux::common::InitResponseCb
     *
     * @returns IQoSManager instance.
     *
     */
    virtual std::shared_ptr<telux::data::net::IQoSManager> getQoSManager(
            telux::common::InitResponseCb clientCallback = nullptr) = 0;

    /**
     * Gets the KeepAlive manager instance.
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              KeepAlive manager @ref telux::common::InitResponseCb
     *
     * @returns IKeepAliveManager instance.
     *
     */
    virtual std::shared_ptr<telux::data::IKeepAliveManager> getKeepAliveManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) = 0;
   /**
     * Get Data Link Manager
     * For hypervisor-based platforms, IDataLinkManager is supported only in the primary/host VM.
     *
     *  @param [in] clientCallback    Optional callback to get the initialization status of
     *                               IDataLinkManager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataLinkManager
     *
     */
    virtual std::shared_ptr<IDataLinkManager> getDataLinkManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    DataFactory();
    virtual ~DataFactory();
#endif

 private:
    DataFactory(const DataFactory &) = delete;
    DataFactory &operator=(const DataFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_data */
}  // namespace data
}  // namespace telux

#endif // TELUX_DATA_DATAFACTORY_HPP
