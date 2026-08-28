// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "NatManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "WireEnums.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <optional>

namespace telux::data::net::simula {

using namespace NetSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct IndPld
{
    Envelope env;
};

struct EntryPld
{
    BackhaulInfo bh;
    NatConfig cfg;
    telux::common::ResponseCallback cb;
    bool add;
};

struct QueryPld
{
    BackhaulInfo bh;
    StaticNatEntriesCb cb;
};

nlohmann::json
fromNatConfig(const BackhaulInfo& bh, const NatConfig& c)
{
    auto j = wire::fromBackhaulInfo(bh);
    j["addr"] = c.addr;
    j["port"] = c.port;
    j["globalPort"] = c.globalPort;
    // IpProtocol is a uint8_t IANA number, not an enum, so it goes on the wire
    // as an integer; 0xFF is the SDK's IP_PROT_UNKNOWN sentinel.
    j["proto"] = static_cast<int>(c.proto);
    return j;
}

NatConfig
toNatConfig(const nlohmann::json& j)
{
    NatConfig c{};
    c.addr = j.value("addr", std::string());
    c.port = static_cast<uint16_t>(j.value("port", 0));
    c.globalPort = static_cast<uint16_t>(j.value("globalPort", 0));
    c.proto = static_cast<telux::data::IpProtocol>(j.value("proto", 0xFF));
    return c;
}

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

// The deprecated profileId overloads are documented as WWAN-only, so they map
// onto a BackhaulInfo with backhaul pinned to WWAN. slotId is carried through
// because for WWAN it is part of the entry's identity (the profile ID is only
// meaningful relative to the SIM in that slot), so dropping it would silently
// retarget the operation at the default slot.
BackhaulInfo
backhaulFromProfileId(int profileId, SlotId slotId)
{
    BackhaulInfo bh{};
    bh.backhaul = telux::data::BackhaulType::WWAN;
    bh.slotId = slotId;
    bh.profileId = profileId;
    return bh;
}

}  // namespace

chart::Status NatNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status NatReady_St(chart::Hsm*, chart::Event const*);
chart::Status NatOperating_St(chart::Hsm*, chart::Event const*);

// ============================================================================

SimulaNatManager::SimulaNatManager(
  telux::data::OperationType opType, common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("NatManager")
    , op_type_(opType)
    , bridge_(bridge)
    , init_gate_(std::move(initCb))
{}

void
SimulaNatManager::addInitCallback(telux::common::InitResponseCb cb)
{
    init_gate_.add(std::move(cb));
}

SimulaNatManager::~SimulaNatManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaNatManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::net_nat::subsys_ready_net_nat::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaNatManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(NatNotReady_St);

    bridge_.subscribe_event(
      topics::net_nat::subsys_ready_net_nat::ind, "net_nat.subsys_ready_net_nat.ind",
      [this](std::string_view topic, const Envelope& env) { handleInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaNatManager::handleInd_(std::string_view, const Envelope& env)
{
    auto pld = std::make_shared<IndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaNatManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<INatListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "NatManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<INatListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaNatManager::isReadyDerived_() const
{
    return const_cast<SimulaNatManager*>(this)->current_state() == NatReady_St;
}

void
SimulaNatManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::ServiceStatus
SimulaNatManager::getServiceStatus()
{
    return last_status_.load();
}

telux::data::OperationType
SimulaNatManager::getOperationType()
{
    return op_type_;
}

bool
SimulaNatManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaNatManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::Status
SimulaNatManager::postEntryOp_(
  bool add, const BackhaulInfo& bh, const NatConfig& cfg, telux::common::ResponseCallback cb
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<EntryPld>();
    pld->bh = bh;
    pld->cfg = cfg;
    pld->cb = std::move(cb);
    pld->add = add;
    post_fifo({ add ? AddNatEntry_Signal : RemoveNatEntry_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNatManager::addStaticNatEntry(
  const BackhaulInfo& bhInfo, const NatConfig& snatConfig,
  telux::common::ResponseCallback callback
)
{
    return postEntryOp_(true, bhInfo, snatConfig, std::move(callback));
}

telux::common::Status
SimulaNatManager::removeStaticNatEntry(
  const BackhaulInfo& bhInfo, const NatConfig& snatConfig,
  telux::common::ResponseCallback callback
)
{
    return postEntryOp_(false, bhInfo, snatConfig, std::move(callback));
}

telux::common::Status
SimulaNatManager::addStaticNatEntry(
  int profileId, const NatConfig& snatConfig, telux::common::ResponseCallback callback,
  SlotId slotId
)
{
    return postEntryOp_(
      true, backhaulFromProfileId(profileId, slotId), snatConfig, std::move(callback)
    );
}

telux::common::Status
SimulaNatManager::removeStaticNatEntry(
  int profileId, const NatConfig& snatConfig, telux::common::ResponseCallback callback,
  SlotId slotId
)
{
    return postEntryOp_(
      false, backhaulFromProfileId(profileId, slotId), snatConfig, std::move(callback)
    );
}

telux::common::Status
SimulaNatManager::requestStaticNatEntries(const BackhaulInfo& bhInfo, StaticNatEntriesCb cb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<QueryPld>();
    pld->bh = bhInfo;
    pld->cb = std::move(cb);
    post_fifo({ RequestNatEntries_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNatManager::requestStaticNatEntries(
  int profileId, StaticNatEntriesCb cb, SlotId slotId
)
{
    return requestStaticNatEntries(backhaulFromProfileId(profileId, slotId), std::move(cb));
}

telux::common::Status
SimulaNatManager::registerListener(std::weak_ptr<INatListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNatManager::deregisterListener(std::weak_ptr<INatListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    for (auto it = listeners_.begin(); it != listeners_.end();)
    {
        auto sp = it->lock();
        if (!sp || (target && sp == target))
            it = listeners_.erase(it);
        else
            ++it;
    }
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
NatOperating_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaNatManager*>(h);
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
NatNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaNatManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(NatReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        case AddNatEntry_Signal:
        case RemoveNatEntry_Signal:
        {
            auto pld = event_cast<EntryPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case RequestNatEntries_Signal:
        {
            auto pld = event_cast<QueryPld>(*e);
            if (pld->cb)
                pld->cb({}, telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(NatOperating_St);
    }
}

chart::Status
NatReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaNatManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_gate_.markReadyAndFire(
                  telux::common::ServiceStatus::SERVICE_AVAILABLE))
            {
                self->broadcastToListeners_([](const std::shared_ptr<INatListener>& l) {
                    l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                });
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_([](const std::shared_ptr<INatListener>& l) {
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
                return self->to(NatNotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(NatNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(NatNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case AddNatEntry_Signal:
        case RemoveNatEntry_Signal:
        {
            auto pld = event_cast<EntryPld>(*e);
            auto req = common::simula::makeRequestEnvelope(
              self->bridge_.currentPaId(), fromNatConfig(pld->bh, pld->cfg)
            );
            auto cb = pld->cb;
            const char* topic = pld->add ? topics::net_nat::add_nat_entry::req
                                         : topics::net_nat::remove_nat_entry::req;
            const char* schema =
              pld->add ? "net_nat.add_nat_entry.rsp" : "net_nat.remove_nat_entry.rsp";
            self->bridge_.send_request(
              topic, schema, req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestNatEntries_Signal:
        {
            auto pld = event_cast<QueryPld>(*e);
            auto req = common::simula::makeRequestEnvelope(
              self->bridge_.currentPaId(), wire::fromBackhaulInfo(pld->bh)
            );
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_nat::request_nat_entries::req, "net_nat.request_nat_entries.rsp", req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb({}, !rsp ? telux::common::ErrorCode::OPERATION_TIMEOUT
                                  : telux::common::ErrorCode::GENERIC_FAILURE);
                      return;
                  }
                  std::vector<NatConfig> out;
                  for (const auto& n : rsp->data->value("entries", nlohmann::json::array()))
                      out.push_back(toNatConfig(n));
                  cb(out, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(NatOperating_St);
    }
}

CHART_NAMED_STATE(NatNotReady_St,  "NatManager::NotReady");
CHART_NAMED_STATE(NatReady_St,     "NatManager::Ready");
CHART_NAMED_STATE(NatOperating_St, "NatManager::Operating");

}  // namespace telux::data::net::simula
