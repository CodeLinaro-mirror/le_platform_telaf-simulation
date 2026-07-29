// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// DataFactory.hpp - entry point implementing telux::data::DataFactory.
//
// Real telux::data::DataFactory is a process-wide singleton (getInstance())
// exposing 19 getters spanning the whole data domain (connection/profile/
// serving-system managers, NAT/firewall/VLAN/SOCKS/bridge/L2TP managers,
// QoS, dual-data, data control, keep-alive, data link, ...). This
// reimplementation covers only the connection, profile, and serving-system
// managers -- the other 16 getters return nullptr without invoking their
// InitResponseCb (nullptr already signals "not implemented"; synthesizing a
// callback for a feature that doesn't exist would be misleading, not
// helpful).

#ifndef TELUX_DATA_SIMULA_DATA_FACTORY_HPP
#define TELUX_DATA_SIMULA_DATA_FACTORY_HPP

#include "../common/IModemBridge.hpp"
#include "DataConnectionManager.hpp"
#include "DataProfileManager.hpp"
#include "ServingSystemManager.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <telux/data/DataFactory.hpp>

namespace telux::data::simula {

class SimulaDataFactory final : public telux::data::DataFactory
{
public:
    // No getInstance() override here -- telux::data::DataFactory::
    // getInstance() (the real SDK's non-virtual static method) is defined
    // out-of-line in DataFactory.cpp to return a SimulaDataFactory
    // singleton, matching the ABI real client code links against.
    //
    // Ctor/dtor are public (not private+friend) because getInstance()'s
    // function-local static lives in the enclosing telux::data namespace,
    // not as a member of this class -- there's no single friend
    // declaration that would reach it.
    SimulaDataFactory();
    ~SimulaDataFactory() override;

    SimulaDataFactory(const SimulaDataFactory&) = delete;
    SimulaDataFactory& operator=(const SimulaDataFactory&) = delete;

    std::shared_ptr<telux::data::IDataConnectionManager> getDataConnectionManager(
      SlotId slotId = DEFAULT_SLOT_ID,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IDataProfileManager> getDataProfileManager(
      SlotId slotId = DEFAULT_SLOT_ID,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IServingSystemManager> getServingSystemManager(
      SlotId slotId = DEFAULT_SLOT_ID,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;

    // Out of scope (see class-level comment) -- always nullptr, callback
    // never invoked.
    std::shared_ptr<telux::data::IDataFilterManager> getDataFilterManager(
      SlotId slotId = DEFAULT_SLOT_ID,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::INatManager> getNatManager(
      telux::data::OperationType oprType,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::IFirewallManager> getFirewallManager(
      telux::data::OperationType oprType,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::IFirewallEntry> getNewFirewallEntry(
      telux::data::IpProtocol proto,
      telux::data::Direction direction,
      telux::data::IpFamilyType ipFamilyType
    ) override;
    std::shared_ptr<telux::data::IIpFilter> getNewIpFilter(telux::data::IpProtocol proto) override;
    std::shared_ptr<telux::data::net::IVlanManager> getVlanManager(
      telux::data::OperationType oprType,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::ISocksManager> getSocksManager(
      telux::data::OperationType oprType,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::IBridgeManager> getBridgeManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::IL2tpManager> getL2tpManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IDataSettingsManager> getDataSettingsManager(
      telux::data::OperationType oprType,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IClientManager> getClientManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IDualDataManager> getDualDataManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IDataControlManager> getDataControlManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::net::IQoSManager> getQoSManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IKeepAliveManager> getKeepAliveManager(
      SlotId slotId = DEFAULT_SLOT_ID,
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;
    std::shared_ptr<telux::data::IDataLinkManager> getDataLinkManager(
      telux::common::InitResponseCb clientCallback = nullptr
    ) override;

private:
    std::mutex mutex_;
    common::simula::IModemBridge& bridge_;
    std::map<SlotId, std::shared_ptr<SimulaDataConnectionManager>> connection_managers_;
    std::map<SlotId, std::shared_ptr<SimulaDataProfileManager>> profile_managers_;
    std::map<SlotId, std::shared_ptr<SimulaServingSystemManager>> serving_managers_;
};

}  // namespace telux::data::simula

#endif  // TELUX_DATA_SIMULA_DATA_FACTORY_HPP
