// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "DataFactory.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
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
        LOG_DEBUG("[DataFactory] getDataConnectionManager slot=%d already exists -- returning cached instance", static_cast<int>(slotId));
        // Manager already exists and boots asynchronously. Register the
        // caller's InitResponseCb on the live manager rather than dropping it:
        // a caller that re-fetches an existing manager to supply a callback
        // (the pattern the net PA uses -- see getNatManager) must still be
        // notified. The gate invokes it immediately if already Ready.
        it->second->addInitCallback(std::move(clientCallback));
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
        LOG_DEBUG("[DataFactory] getDataProfileManager slot=%d already exists -- returning cached instance", static_cast<int>(slotId));
        it->second->addInitCallback(std::move(clientCallback));
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
        LOG_DEBUG("[DataFactory] getServingSystemManager slot=%d already exists -- returning cached instance", static_cast<int>(slotId));
        it->second->addInitCallback(std::move(clientCallback));
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
SimulaDataFactory::getNatManager(
  telux::data::OperationType opType, telux::common::InitResponseCb clientCallback
)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = nat_managers_.find(opType);
    if (it != nat_managers_.end())
    {
        // Existing manager: the target PA calls this getter a second time with
        // the callback that backs its 30s promise wait (see
        // telaf-pa/component/taf_pa_net/tafNatPa.cpp initialize()), so the
        // callback must be registered on the live manager -- dropping it here
        // is what stranded the PA until PA_TIMEOUT. The gate fires it
        // immediately if readiness has already been reported.
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr = std::make_shared<telux::data::net::simula::SimulaNatManager>(
      opType, bridge_, std::move(clientCallback)
    );
    nat_managers_.emplace(opType, mgr);
    mgr->start();
    return mgr;
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
SimulaDataFactory::getVlanManager(
  telux::data::OperationType opType, telux::common::InitResponseCb clientCallback
)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = vlan_managers_.find(opType);
    if (it != vlan_managers_.end())
    {
        // See getNatManager: the second getter call carries the callback the
        // PA actually waits on, so hand it to the live manager's gate.
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr = std::make_shared<telux::data::net::simula::SimulaVlanManager>(
      opType, bridge_, std::move(clientCallback)
    );
    vlan_managers_.emplace(opType, mgr);
    mgr->start();
    return mgr;
}

std::shared_ptr<telux::data::net::ISocksManager>
SimulaDataFactory::getSocksManager(
  telux::data::OperationType opType, telux::common::InitResponseCb clientCallback
)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = socks_managers_.find(opType);
    if (it != socks_managers_.end())
    {
        // See getNatManager.
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr = std::make_shared<telux::data::net::simula::SimulaSocksManager>(
      opType, bridge_, std::move(clientCallback)
    );
    socks_managers_.emplace(opType, mgr);
    mgr->start();
    return mgr;
}

std::shared_ptr<telux::data::net::IBridgeManager>
SimulaDataFactory::getBridgeManager(telux::common::InitResponseCb)
{
    return nullptr;
}

std::shared_ptr<telux::data::net::IL2tpManager>
SimulaDataFactory::getL2tpManager(telux::common::InitResponseCb clientCallback)
{
    std::lock_guard<std::mutex> lk(mutex_);
    if (l2tp_manager_)
    {
        // See getNatManager. tafL2tpPa.cpp returns PA_FAULT (-6) on timeout
        // rather than PA_TIMEOUT, which is why L2TP logged a different code
        // for the same underlying cause.
        l2tp_manager_->addInitCallback(std::move(clientCallback));
        return l2tp_manager_;
    }
    l2tp_manager_ = std::make_shared<telux::data::net::simula::SimulaL2tpManager>(
      bridge_, std::move(clientCallback)
    );
    l2tp_manager_->start();
    return l2tp_manager_;
}

std::shared_ptr<telux::data::IDataSettingsManager>
SimulaDataFactory::getDataSettingsManager(
  telux::data::OperationType opType, telux::common::InitResponseCb clientCallback
)
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = settings_managers_.find(opType);
    if (it != settings_managers_.end())
    {
        // See getNatManager. tafVlanPa.cpp's initialize() waits on this one
        // too, after the VLAN manager comes up.
        it->second->addInitCallback(std::move(clientCallback));
        return it->second;
    }
    auto mgr = std::make_shared<SimulaDataSettingsManager>(
      opType, bridge_, std::move(clientCallback)
    );
    settings_managers_.emplace(opType, mgr);
    mgr->start();
    return mgr;
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
