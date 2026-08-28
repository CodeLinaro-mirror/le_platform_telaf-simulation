// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "VlanManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
// std::list is required by VlanMappingResponseCb's signature and std::optional
// by QueryBindingsPld; both happen to arrive transitively via the telux
// headers, but depending on that is fragile.
#include <list>
#include <optional>

namespace telux::data::net::simula {

using namespace NetSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

constexpr auto kRpcTimeout = std::chrono::seconds(30);

// ---------------------------------------------------------------------------
// Wire <-> telux enum mapping. Kept exhaustive over the wire strings the
// net_vlan schemas allow; anything unrecognised maps to the enum's own
// UNKNOWN member rather than silently picking a real interface.

std::string
ifaceTypeToWire(InterfaceType t)
{
    switch (t)
    {
        case InterfaceType::WLAN:          return "WLAN";
        case InterfaceType::ETH:           return "ETH";
        case InterfaceType::ECM:           return "ECM";
        case InterfaceType::RNDIS:         return "RNDIS";
        case InterfaceType::MHI:           return "MHI";
        case InterfaceType::VMTAP0:        return "VMTAP0";
        case InterfaceType::VMTAP1:        return "VMTAP1";
        case InterfaceType::ETH2:          return "ETH2";
        case InterfaceType::AP_PRIMARY:    return "AP_PRIMARY";
        case InterfaceType::AP_SECONDARY:  return "AP_SECONDARY";
        case InterfaceType::AP_TERTIARY:   return "AP_TERTIARY";
        case InterfaceType::AP_QUATERNARY: return "AP_QUATERNARY";
        default:                           return "UNKNOWN";
    }
}

InterfaceType
wireToIfaceType(const std::string& s)
{
    if (s == "WLAN")          return InterfaceType::WLAN;
    if (s == "ETH")           return InterfaceType::ETH;
    if (s == "ECM")           return InterfaceType::ECM;
    if (s == "RNDIS")         return InterfaceType::RNDIS;
    if (s == "MHI")           return InterfaceType::MHI;
    if (s == "VMTAP0")        return InterfaceType::VMTAP0;
    if (s == "VMTAP1")        return InterfaceType::VMTAP1;
    if (s == "ETH2")          return InterfaceType::ETH2;
    if (s == "AP_PRIMARY")    return InterfaceType::AP_PRIMARY;
    if (s == "AP_SECONDARY")  return InterfaceType::AP_SECONDARY;
    if (s == "AP_TERTIARY")   return InterfaceType::AP_TERTIARY;
    if (s == "AP_QUATERNARY") return InterfaceType::AP_QUATERNARY;
    return InterfaceType::UNKNOWN;
}

std::string
backhaulToWire(BackhaulType t)
{
    switch (t)
    {
        case BackhaulType::ETH:  return "ETH";
        case BackhaulType::USB:  return "USB";
        case BackhaulType::WLAN: return "WLAN";
        case BackhaulType::WWAN: return "WWAN";
        case BackhaulType::BLE:  return "BLE";
        // MAX_SUPPORTED is a count sentinel, not a real backhaul; the schema
        // has no wire string for it, so treat it like ETH's absence and let
        // the MPSS-side schema validation reject the request.
        default:                 return "";
    }
}

BackhaulType
wireToBackhaul(const std::string& s)
{
    if (s == "USB")  return BackhaulType::USB;
    if (s == "WLAN") return BackhaulType::WLAN;
    if (s == "WWAN") return BackhaulType::WWAN;
    if (s == "BLE")  return BackhaulType::BLE;
    return BackhaulType::ETH;
}

std::string
nwTypeToWire(NetworkType t)
{
    switch (t)
    {
        case NetworkType::LAN: return "LAN";
        case NetworkType::WAN: return "WAN";
        default:               return "UNKNOWN";
    }
}

NetworkType
wireToNwType(const std::string& s)
{
    if (s == "WAN") return NetworkType::WAN;
    if (s == "LAN") return NetworkType::LAN;
    return NetworkType::UNKNOWN;
}

// Builds the shared bind/unbind request body from a VlanBindConfig. slotId and
// profileId are emitted only for WWAN: the SDK documents them as don't-care
// for every other backhaul, and the MPSS side discards them there anyway, so
// sending them would imply a meaning they don't have.
nlohmann::json
bindConfigToJson(const VlanBindConfig& cfg)
{
    nlohmann::json data = nlohmann::json::object();
    data["vlanId"] = cfg.vlanId;
    data["backhaul"] = backhaulToWire(cfg.bhInfo.backhaul);
    if (cfg.bhInfo.backhaul == BackhaulType::WWAN)
    {
        data["slot"] = static_cast<int>(cfg.bhInfo.slotId);
        data["profileId"] = cfg.bhInfo.profileId;
    }
    // vlanId == -1 is BackhaulInfo's documented "unset" default; only forward
    // a real VLAN-as-backhaul id.
    if (cfg.bhInfo.vlanId >= 0)
        data["bhVlanId"] = cfg.bhInfo.vlanId;
    return data;
}

VlanConfig
jsonToVlanConfig(const nlohmann::json& j)
{
    VlanConfig cfg{};
    cfg.vlanId = static_cast<int16_t>(j.value("vlanId", 0));
    cfg.iface = wireToIfaceType(j.value("ifaceType", std::string()));
    cfg.isAccelerated = j.value("isAccelerated", false);
    cfg.priority = static_cast<uint8_t>(j.value("priority", 0));
    cfg.nwType = wireToNwType(j.value("nwType", std::string("LAN")));
    cfg.createBridge = j.value("createBridge", true);
    return cfg;
}

VlanBindConfig
jsonToBindConfig(const nlohmann::json& j)
{
    VlanBindConfig cfg{};
    cfg.vlanId = j.value("vlanId", 0);
    cfg.bhInfo.backhaul = wireToBackhaul(j.value("backhaul", std::string()));
    if (j.contains("slot"))
        cfg.bhInfo.slotId = static_cast<SlotId>(j.value("slot", 1));
    if (j.contains("profileId"))
        cfg.bhInfo.profileId = j.value("profileId", -1);
    if (j.contains("bhVlanId"))
        cfg.bhInfo.vlanId = j.value("bhVlanId", -1);
    return cfg;
}

// ---------------------------------------------------------------------------
// Chart payloads.

struct IndPld
{
    Envelope env;
};

struct CreateVlanPld
{
    VlanConfig config;
    CreateVlanCb cb;
};

struct RemoveVlanPld
{
    int16_t vlanId;
    InterfaceType ifaceType;
    telux::common::ResponseCallback cb;
};

struct QueryVlanInfoPld
{
    QueryVlanResponseCb cb;
};

struct BindVlanPld
{
    VlanBindConfig config;
    telux::common::ResponseCallback cb;
    bool bind;  // true = bindToBackhaul, false = unbindFromBackhaul
};

struct QueryBindingsPld
{
    // Empty optional == "every backhaul", which queryVlanMappingList needs
    // (it projects the whole table down to (profileId, vlanId) pairs).
    std::optional<BackhaulType> backhaul;
    SlotId slotId;
    VlanBindingsResponseCb bindingsCb;
    VlanMappingResponseCb mappingCb;
};

// Shared tail for the ResponseCallback-shaped RPCs (remove/bind/unbind):
// timeout -> OPERATION_TIMEOUT, wire error -> its parsed code, else SUCCESS.
void
completeResponseCb(const telux::common::ResponseCallback& cb, std::optional<Envelope> rsp)
{
    if (!cb)
        return;
    if (!rsp)
    {
        cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
        return;
    }
    if (rsp->error)
    {
        cb(common::simula::parseErrorCode(rsp->error->value("code", std::string())));
        return;
    }
    cb(telux::common::ErrorCode::SUCCESS);
}

}  // namespace

chart::Status
VlanNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
VlanReady_St(chart::Hsm*, chart::Event const*);
chart::Status
VlanOperating_St(chart::Hsm*, chart::Event const*);

// ============================================================================
// SimulaVlanManager

SimulaVlanManager::SimulaVlanManager(
  telux::data::OperationType opType,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("VlanManager")
    , bridge_(bridge)
    , opType_(opType)
    , init_gate_(std::move(initCb))
{}

SimulaVlanManager::~SimulaVlanManager()
{
    // Withdraw from the bridge FIRST: every callback registered in start()
    // captures raw `this`, and the bridge holds its own copies that would
    // otherwise run against freed memory. unsubscribe_* only *queues* the
    // removal, so the drain() inside unsubscribeFromBridge_() is mandatory --
    // it fences against the bridge worker's FIFO.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaVlanManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::net_vlan::subsys_ready_net_vlan::ind);
    bridge_.unsubscribe_event(topics::net_vlan::hw_accel_state::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaVlanManager::addInitCallback(telux::common::InitResponseCb cb)
{
    init_gate_.add(std::move(cb));
}

void
SimulaVlanManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(VlanNotReady_St);

    bridge_.subscribe_event(
      topics::net_vlan::subsys_ready_net_vlan::ind,
      "net_vlan.subsys_ready_net_vlan.ind",
      [this](std::string_view topic, const Envelope& env) { handleInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::net_vlan::hw_accel_state::ind,
      "net_vlan.hw_accel_state.ind",
      [this](std::string_view topic, const Envelope& env) { handleInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaVlanManager::handleInd_(std::string_view topic, const Envelope& env)
{
    // Runs on the bridge's worker thread -- forward only, touch no members.
    auto pld = std::make_shared<IndPld>();
    pld->env = env;
    if (topic == topics::net_vlan::subsys_ready_net_vlan::ind)
        post_fifo({ ReadinessEvt_Signal, pld });
    else
        post_fifo({ VlanHwAccelEvt_Signal, pld });
}

void
SimulaVlanManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<IVlanListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "VlanManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& weak : listeners_)
        {
            if (auto sp = weak.lock())
                task->listeners.push_back(sp);
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<IVlanListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaVlanManager::isReadyDerived_() const
{
    return const_cast<SimulaVlanManager*>(this)->current_state() == VlanReady_St;
}

void
SimulaVlanManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::ServiceStatus
SimulaVlanManager::getServiceStatus()
{
    return last_status_.load();
}

bool
SimulaVlanManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaVlanManager::onSubsystemReady()
{
    // Deprecated in the real SDK in favour of the factory's InitResponseCb --
    // not worth plumbing a real promise/future path for.
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::data::OperationType
SimulaVlanManager::getOperationType()
{
    return opType_;
}

// ---------------------------------------------------------------------------
// Public API -- each gates on readiness, then posts to the AO thread.

telux::common::Status
SimulaVlanManager::createVlan(const VlanConfig& vlanConfig, CreateVlanCb callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    // The SDK documents NOTSUPPORTED for a non-zero priority on platforms
    // without VLAN-priority support. This simulation *does* support it, so
    // the only local rejection is an out-of-range value: priority is a 3-bit
    // PCP field, so anything above 7 could never reach the wire intact.
    if (vlanConfig.priority > 7)
        return telux::common::Status::INVALIDPARAM;
    auto pld = std::make_shared<CreateVlanPld>();
    pld->config = vlanConfig;
    pld->cb = std::move(callback);
    post_fifo({ CreateVlan_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::removeVlan(
  int16_t vlanId, InterfaceType ifaceType, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RemoveVlanPld>();
    pld->vlanId = vlanId;
    pld->ifaceType = ifaceType;
    pld->cb = std::move(callback);
    post_fifo({ RemoveVlan_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::queryVlanInfo(QueryVlanResponseCb callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<QueryVlanInfoPld>();
    pld->cb = std::move(callback);
    post_fifo({ QueryVlanInfo_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::bindToBackhaul(
  VlanBindConfig vlanBindConfig, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<BindVlanPld>();
    pld->config = std::move(vlanBindConfig);
    pld->cb = std::move(callback);
    pld->bind = true;
    post_fifo({ BindVlan_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::unbindFromBackhaul(
  VlanBindConfig vlanBindConfig, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<BindVlanPld>();
    pld->config = std::move(vlanBindConfig);
    pld->cb = std::move(callback);
    pld->bind = false;
    post_fifo({ UnbindVlan_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::queryVlanToBackhaulBindings(
  BackhaulType backhaulType, VlanBindingsResponseCb callback, SlotId slotId
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<QueryBindingsPld>();
    pld->backhaul = backhaulType;
    pld->slotId = slotId;
    pld->bindingsCb = std::move(callback);
    post_fifo({ QueryVlanBindings_Signal, pld });
    return telux::common::Status::SUCCESS;
}

// --- Deprecated profile-based API: same operations, backhaul pinned to WWAN.

telux::common::Status
SimulaVlanManager::bindWithProfile(
  int profileId, int vlanId, telux::common::ResponseCallback callback, SlotId slotId
)
{
    VlanBindConfig cfg{};
    cfg.vlanId = vlanId;
    cfg.bhInfo.backhaul = BackhaulType::WWAN;
    cfg.bhInfo.slotId = slotId;
    cfg.bhInfo.profileId = profileId;
    return bindToBackhaul(std::move(cfg), std::move(callback));
}

telux::common::Status
SimulaVlanManager::unbindFromProfile(
  int profileId, int vlanId, telux::common::ResponseCallback callback, SlotId slotId
)
{
    VlanBindConfig cfg{};
    cfg.vlanId = vlanId;
    cfg.bhInfo.backhaul = BackhaulType::WWAN;
    cfg.bhInfo.slotId = slotId;
    cfg.bhInfo.profileId = profileId;
    return unbindFromBackhaul(std::move(cfg), std::move(callback));
}

telux::common::Status
SimulaVlanManager::queryVlanMappingList(VlanMappingResponseCb callback, SlotId slotId)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    // Deliberately leaves `backhaul` unset: queryVlanMappingList is documented
    // as "VLAN mapping of profile id and VLAN id on specified sim", i.e. every
    // WWAN binding on that slot. Filtering happens in the response handler,
    // which projects the table down to (profileId, vlanId) pairs and drops
    // bindings with no profile id (non-WWAN backhauls have none).
    auto pld = std::make_shared<QueryBindingsPld>();
    pld->backhaul = BackhaulType::WWAN;
    pld->slotId = slotId;
    pld->mappingCb = std::move(callback);
    post_fifo({ QueryVlanBindings_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::registerListener(std::weak_ptr<IVlanListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaVlanManager::deregisterListener(std::weak_ptr<IVlanListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    for (auto it = listeners_.begin(); it != listeners_.end();)
    {
        auto sp = it->lock();
        // Also reaps already-expired weak_ptrs while walking.
        if (!sp || (target && sp == target))
            it = listeners_.erase(it);
        else
            ++it;
    }
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

// Composite parent for {NotReady, Ready}. Exists so readiness-agnostic
// bookkeeping has one home; currently that is nothing, but keeping the shell
// symmetrical with the data domain's Operating_St means a later addition
// doesn't have to restructure the chart.
chart::Status
VlanOperating_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaVlanManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
VlanNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaVlanManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(VlanReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            // Already NotReady; nothing to degrade from.
            return chart::Status::HANDLED;
        // These only land here if a call raced the transition out of Ready --
        // every public method returns NOTREADY synchronously before posting.
        // Each still fires its callback with DEVICE_NOT_READY rather than
        // being dropped: a caller awaiting a callback would otherwise hang.
        case CreateVlan_Signal:
        {
            auto pld = event_cast<CreateVlanPld>(*e);
            if (pld->cb)
                pld->cb(false, telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case RemoveVlan_Signal:
        {
            auto pld = event_cast<RemoveVlanPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case BindVlan_Signal:
        case UnbindVlan_Signal:
        {
            auto pld = event_cast<BindVlanPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case QueryVlanInfo_Signal:
        {
            auto pld = event_cast<QueryVlanInfoPld>(*e);
            if (pld->cb)
                pld->cb({}, telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case QueryVlanBindings_Signal:
        {
            auto pld = event_cast<QueryBindingsPld>(*e);
            if (pld->bindingsCb)
                pld->bindingsCb({}, telux::common::ErrorCode::DEVICE_NOT_READY);
            if (pld->mappingCb)
                pld->mappingCb({}, telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case VlanHwAccelEvt_Signal:
            // Not Ready: no listener has a meaningful VLAN view to update.
            return chart::Status::HANDLED;
        default:
            return self->super(VlanOperating_St);
    }
}

chart::Status
VlanReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaVlanManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_gate_.markReadyAndFire(
                  telux::common::ServiceStatus::SERVICE_AVAILABLE))
            {
                self->broadcastToListeners_(
                  [](const std::shared_ptr<IVlanListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_([](const std::shared_ptr<IVlanListener>& l) {
                l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
            });
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(VlanNotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(VlanNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(VlanNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case CreateVlan_Signal:
        {
            auto pld = event_cast<CreateVlanPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["vlanId"] = pld->config.vlanId;
            data["ifaceType"] = ifaceTypeToWire(pld->config.iface);
            data["isAccelerated"] = pld->config.isAccelerated;
            data["priority"] = static_cast<int>(pld->config.priority);
            data["nwType"] = nwTypeToWire(pld->config.nwType);
            data["createBridge"] = pld->config.createBridge;
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_vlan::create_vlan::req,
              "net_vlan.create_vlan.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp)
                  {
                      cb(false, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  if (rsp->error)
                  {
                      cb(false,
                         common::simula::parseErrorCode(rsp->error->value("code", std::string())));
                      return;
                  }
                  // The offload status the modem actually applied, which need
                  // not equal what was requested.
                  bool accel = rsp->data ? rsp->data->value("isAccelerated", false) : false;
                  cb(accel, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RemoveVlan_Signal:
        {
            auto pld = event_cast<RemoveVlanPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["vlanId"] = pld->vlanId;
            data["ifaceType"] = ifaceTypeToWire(pld->ifaceType);
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_vlan::remove_vlan::req,
              "net_vlan.remove_vlan.rsp",
              req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case QueryVlanInfo_Signal:
        {
            auto pld = event_cast<QueryVlanInfoPld>(*e);
            auto req = common::simula::makeRequestEnvelope(
              self->bridge_.currentPaId(), nlohmann::json::object()
            );
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_vlan::query_vlan_info::req,
              "net_vlan.query_vlan_info.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb({}, !rsp ? telux::common::ErrorCode::OPERATION_TIMEOUT
                                  : telux::common::ErrorCode::GENERIC_FAILURE);
                      return;
                  }
                  std::vector<VlanConfig> configs;
                  for (const auto& v : rsp->data->value("vlans", nlohmann::json::array()))
                      configs.push_back(jsonToVlanConfig(v));
                  cb(configs, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case BindVlan_Signal:
        case UnbindVlan_Signal:
        {
            auto pld = event_cast<BindVlanPld>(*e);
            auto data = bindConfigToJson(pld->config);
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            const char* topic = pld->bind ? topics::net_vlan::bind_vlan::req
                                          : topics::net_vlan::unbind_vlan::req;
            const char* schema = pld->bind ? "net_vlan.bind_vlan.rsp"
                                           : "net_vlan.unbind_vlan.rsp";
            self->bridge_.send_request(
              topic,
              schema,
              req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case QueryVlanBindings_Signal:
        {
            auto pld = event_cast<QueryBindingsPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            if (pld->backhaul)
            {
                auto wire = backhaulToWire(*pld->backhaul);
                if (!wire.empty())
                    data["backhaul"] = wire;
            }
            data["slot"] = static_cast<int>(pld->slotId);
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto bindingsCb = pld->bindingsCb;
            auto mappingCb = pld->mappingCb;
            self->bridge_.send_request(
              topics::net_vlan::query_vlan_bindings::req,
              "net_vlan.query_vlan_bindings.rsp",
              req,
              [bindingsCb, mappingCb](std::optional<Envelope> rsp) {
                  auto error = !rsp ? telux::common::ErrorCode::OPERATION_TIMEOUT
                                    : telux::common::ErrorCode::GENERIC_FAILURE;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      if (bindingsCb)
                          bindingsCb({}, error);
                      if (mappingCb)
                          mappingCb({}, error);
                      return;
                  }
                  const auto& arr = rsp->data->value("bindings", nlohmann::json::array());
                  if (bindingsCb)
                  {
                      std::vector<VlanBindConfig> bindings;
                      for (const auto& b : arr)
                          bindings.push_back(jsonToBindConfig(b));
                      bindingsCb(bindings, telux::common::ErrorCode::SUCCESS);
                  }
                  if (mappingCb)
                  {
                      // queryVlanMappingList's contract is a (profileId,
                      // vlanId) list. Bindings without a profileId (every
                      // non-WWAN backhaul) have no meaningful pair, so they're
                      // omitted rather than reported with a -1 profile.
                      std::list<std::pair<int, int>> mapping;
                      for (const auto& b : arr)
                      {
                          if (!b.contains("profileId"))
                              continue;
                          mapping.emplace_back(b.value("profileId", -1), b.value("vlanId", 0));
                      }
                      mappingCb(mapping, telux::common::ErrorCode::SUCCESS);
                  }
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case VlanHwAccelEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("state", std::string()) == "ACTIVE"
              ? ServiceState::ACTIVE
              : ServiceState::INACTIVE;
            self->broadcastToListeners_([state](const std::shared_ptr<IVlanListener>& l) {
                l->onHwAccelerationChanged(state);
            });
            return chart::Status::HANDLED;
        }

        default:
            return self->super(VlanOperating_St);
    }
}

CHART_NAMED_STATE(VlanNotReady_St,  "VlanManager::NotReady");
CHART_NAMED_STATE(VlanReady_St,     "VlanManager::Ready");
CHART_NAMED_STATE(VlanOperating_St, "VlanManager::Operating");

}  // namespace telux::data::net::simula
