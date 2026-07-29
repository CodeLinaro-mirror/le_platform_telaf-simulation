// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DataFactory.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/ModemBridge.hpp"

namespace telux::data::simula {

SimulaDataFactory::SimulaDataFactory()
    : bridge_(common::simula::ModemBridge::instance())
{
    bridge_.start();
    // Boot the shared listener-dispatch worker. Every manager delivers its
    // async callbacks (onDataCallInfoChanged, serving-system/profile
    // indications) by enqueueing onto this AO; if its worker thread isn't
    // running, post_fifo() silently drops the event (see
    // chart::ActiveObject::post_fifo), so a data call would connect on the
    // wire yet the client's SessionState listener would never see CONNECTED.
    common::simula::ListenerDispatchAO::instance().start();
}

SimulaDataFactory::~SimulaDataFactory() = default;

std::shared_ptr<telux::data::IDataConnectionManager>
SimulaDataFactory::getDataConnectionManager(SlotId slotId, telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = connection_managers_.find(slotId);
    if (it != connection_managers_.end())
    {
        // Manager already exists and boots asynchronously; a second caller
        // asking for the same slot doesn't get its own InitResponseCb fired
        // (mirrors the real SDK's factory-getter contract: InitResponseCb
        // fires once per underlying subsystem becoming ready, not once per
        // caller) -- if it's already Ready, report that synchronously so
        // the caller isn't left hanging forever.
        if (clientCallback && it->second->isSubsystemReady())
            clientCallback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return it->second;
    }
    auto mgr =
      std::make_shared<SimulaDataConnectionManager>(slotId, bridge_, std::move(clientCallback));
    connection_managers_.emplace(slotId, mgr);
    mgr->start();
    return mgr;
}

std::shared_ptr<telux::data::IDataProfileManager>
SimulaDataFactory::getDataProfileManager(SlotId slotId, telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = profile_managers_.find(slotId);
    if (it != profile_managers_.end())
    {
        if (clientCallback && it->second->isSubsystemReady())
            clientCallback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return it->second;
    }
    auto mgr =
      std::make_shared<SimulaDataProfileManager>(slotId, bridge_, std::move(clientCallback));
    profile_managers_.emplace(slotId, mgr);
    mgr->start();
    return mgr;
}

std::shared_ptr<telux::data::IServingSystemManager>
SimulaDataFactory::getServingSystemManager(SlotId slotId, telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = serving_managers_.find(slotId);
    if (it != serving_managers_.end())
    {
        if (clientCallback && it->second->getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE)
            clientCallback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return it->second;
    }
    auto mgr =
      std::make_shared<SimulaServingSystemManager>(slotId, bridge_, std::move(clientCallback));
    serving_managers_.emplace(slotId, mgr);
    mgr->start();
    return mgr;
}

// ---------------------------------------------------------------------------
// Out of scope -- see DataFactory.hpp's class-level comment.

std::shared_ptr<telux::data::IDataFilterManager>
SimulaDataFactory::getDataFilterManager(SlotId, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::INatManager>
SimulaDataFactory::getNatManager(telux::data::OperationType, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::IFirewallManager>
SimulaDataFactory::getFirewallManager(telux::data::OperationType, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::IFirewallEntry>
SimulaDataFactory::getNewFirewallEntry(
  telux::data::IpProtocol,
  telux::data::Direction,
  telux::data::IpFamilyType
)
{
    return nullptr;
}

std::shared_ptr<telux::data::IIpFilter>
SimulaDataFactory::getNewIpFilter(telux::data::IpProtocol)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::IVlanManager>
SimulaDataFactory::getVlanManager(telux::data::OperationType, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::ISocksManager>
SimulaDataFactory::getSocksManager(telux::data::OperationType, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::IBridgeManager>
SimulaDataFactory::getBridgeManager(telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::IL2tpManager>
SimulaDataFactory::getL2tpManager(telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::IDataSettingsManager>
SimulaDataFactory::getDataSettingsManager(telux::data::OperationType, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::IClientManager>
SimulaDataFactory::getClientManager(telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::IDualDataManager>
SimulaDataFactory::getDualDataManager(telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::IDataControlManager>
SimulaDataFactory::getDataControlManager(telux::common::InitResponseCb)
{
    return nullptr;
}

// net::IQoSManager is a different QoS concept from what the wire's
// qos_status indication carries: it's traffic-class/bandwidth-shaping
// config (createTrafficClass/addQoSFilter, net/QoSManager.hpp), not
// per-flow TFT state. The real PA (telaf-pa's
// tafDataTeluxDataConnectionImplPa.cpp PaAddQosTftEventsCallback ->
// tafDataTeluxDataConnectionListenerImplPa.cpp
// TafPaTeluxDataConnectionListener::onTrafficFlowTemplateChange, confirmed
// by tracing taf_dcs_AddQosStatusHandler's call chain through
// tafDcsProfileManagerImpl.cpp's paQosTftEvtHandler) backs the DCS test's
// QoS handler entirely through IDataConnectionManager::registerListener's
// onTrafficFlowTemplateChange callback -- it never calls getQoSManager()
// or touches net::IQoSManager at all. So getQoSManager() returning nullptr
// here is NOT a gap; the QoS fan-out this design targets is wired
// entirely through DataConnectionManager.cpp's QosStatusEvt_Signal handler
// instead.
std::shared_ptr<telux::data::net::IQoSManager>
SimulaDataFactory::getQoSManager(telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::IKeepAliveManager>
SimulaDataFactory::getKeepAliveManager(SlotId, telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::IDataLinkManager>
SimulaDataFactory::getDataLinkManager(telux::common::InitResponseCb)
{
    return nullptr;
}

}  // namespace telux::data::simula

// =============================================================================
// telux::data::DataFactory::getInstance()
//
// This symbol must live in the telux::data namespace to satisfy the ABI
// that client code built against the real SDK header links against.
// telux::data::DataFactory's ctor/dtor are protected in the real header
// (public/include/telux/data/DataFactory.hpp) with no out-of-line
// definition shipped anywhere else in this build, so they're defined here
// too.
// =============================================================================

namespace telux::data {

DataFactory&
DataFactory::getInstance()
{
    static simula::SimulaDataFactory instance;
    return instance;
}

DataFactory::DataFactory() = default;
DataFactory::~DataFactory() = default;

}  // namespace telux::data
