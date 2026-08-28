// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "L2tpManager.hpp"

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

std::string
fromProtocol(L2tpProtocol p)
{
    switch (p)
    {
        case L2tpProtocol::IP:  return "IP";
        case L2tpProtocol::UDP: return "UDP";
        default:                return "NONE";
    }
}

L2tpProtocol
toProtocol(const std::string& s)
{
    if (s == "IP")  return L2tpProtocol::IP;
    if (s == "UDP") return L2tpProtocol::UDP;
    return L2tpProtocol::NONE;
}

nlohmann::json
fromTunnelConfig(const L2tpTunnelConfig& c)
{
    nlohmann::json j = nlohmann::json::object();
    j["locId"] = c.locId;
    j["peerId"] = c.peerId;
    j["prot"] = fromProtocol(c.prot);
    j["localUdpPort"] = c.localUdpPort;
    j["peerUdpPort"] = c.peerUdpPort;
    j["peerIpv6Addr"] = c.peerIpv6Addr;
    j["peerIpv6GwAddr"] = c.peerIpv6GwAddr;
    j["peerIpv4Addr"] = c.peerIpv4Addr;
    j["peerIpv4GwAddr"] = c.peerIpv4GwAddr;
    j["locIface"] = c.locIface;
    j["ipType"] = wire::fromIpFamily(c.ipType);
    auto sessions = nlohmann::json::array();
    for (const auto& s : c.sessionConfig)
        sessions.push_back({ { "locId", s.locId }, { "peerId", s.peerId } });
    j["sessions"] = std::move(sessions);
    return j;
}

L2tpTunnelConfig
toTunnelConfig(const nlohmann::json& j)
{
    L2tpTunnelConfig c{};
    c.locId = j.value("locId", 0u);
    c.peerId = j.value("peerId", 0u);
    c.prot = toProtocol(j.value("prot", std::string()));
    c.localUdpPort = j.value("localUdpPort", 0u);
    c.peerUdpPort = j.value("peerUdpPort", 0u);
    c.peerIpv6Addr = j.value("peerIpv6Addr", std::string());
    c.peerIpv6GwAddr = j.value("peerIpv6GwAddr", std::string());
    c.peerIpv4Addr = j.value("peerIpv4Addr", std::string());
    c.peerIpv4GwAddr = j.value("peerIpv4GwAddr", std::string());
    c.locIface = j.value("locIface", std::string());
    c.ipType = wire::toIpFamily(j.value("ipType", std::string()));
    for (const auto& s : j.value("sessions", nlohmann::json::array()))
    {
        L2tpSessionConfig sc{};
        sc.locId = s.value("locId", 0u);
        sc.peerId = s.value("peerId", 0u);
        c.sessionConfig.push_back(sc);
    }
    return c;
}

nlohmann::json
fromBindConfig(const L2tpSessionBindConfig& c)
{
    auto j = wire::fromBackhaulInfo(c.bhInfo);
    j["locId"] = c.locId;
    return j;
}

L2tpSessionBindConfig
toBindConfig(const nlohmann::json& j)
{
    L2tpSessionBindConfig c{};
    c.locId = j.value("locId", 0u);
    c.bhInfo = wire::toBackhaulInfo(j);
    return c;
}

// ---- chart payloads -------------------------------------------------------

struct IndPld
{
    Envelope env;
};

struct SetConfigPld
{
    bool enable;
    bool enableMss;
    bool enableMtu;
    uint32_t mtuSize;
    telux::common::ResponseCallback cb;
};

struct RequestConfigPld
{
    L2tpConfigCb cb;
};

struct AddTunnelPld
{
    L2tpTunnelConfig config;
    telux::common::ResponseCallback cb;
};

struct TunnelIdPld
{
    uint32_t tunnelId;
    telux::common::ResponseCallback cb;
};

struct SessionPld
{
    uint32_t tunnelId;
    uint32_t sessionId;
    uint32_t peerId;
    telux::common::ResponseCallback cb;
};

struct BindPld
{
    L2tpSessionBindConfig config;
    telux::common::ResponseCallback cb;
    bool bind;
};

struct QueryBindingsPld
{
    BackhaulType backhaul;
    L2tpSessionBindingsResponseCb cb;
};

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

chart::Status L2tpNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status L2tpReady_St(chart::Hsm*, chart::Event const*);
chart::Status L2tpOperating_St(chart::Hsm*, chart::Event const*);

// ============================================================================

SimulaL2tpManager::SimulaL2tpManager(
  common::simula::IModemBridge& bridge, telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("L2tpManager")
    , bridge_(bridge)
    , init_gate_(std::move(initCb))
{}

SimulaL2tpManager::~SimulaL2tpManager()
{
    // Withdraw from the bridge before any member teardown: the callbacks
    // registered in start() capture raw `this`. unsubscribe_* only queues the
    // removal, so the drain() fence inside is mandatory.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaL2tpManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::net_l2tp::subsys_ready_net_l2tp::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaL2tpManager::addInitCallback(telux::common::InitResponseCb cb)
{
    init_gate_.add(std::move(cb));
}

void
SimulaL2tpManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(L2tpNotReady_St);

    bridge_.subscribe_event(
      topics::net_l2tp::subsys_ready_net_l2tp::ind,
      "net_l2tp.subsys_ready_net_l2tp.ind",
      [this](std::string_view topic, const Envelope& env) { handleInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaL2tpManager::handleInd_(std::string_view, const Envelope& env)
{
    auto pld = std::make_shared<IndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaL2tpManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<IL2tpListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "L2tpManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<IL2tpListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaL2tpManager::isReadyDerived_() const
{
    return const_cast<SimulaL2tpManager*>(this)->current_state() == L2tpReady_St;
}

void
SimulaL2tpManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::ServiceStatus
SimulaL2tpManager::getServiceStatus()
{
    return last_status_.load();
}

bool
SimulaL2tpManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaL2tpManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

// ---------------------------------------------------------------------------
// Public API

telux::common::Status
SimulaL2tpManager::setConfig(
  bool enable, bool enableMss, bool enableMtu,
  telux::common::ResponseCallback callback, uint32_t mtuSize
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SetConfigPld>();
    pld->enable = enable;
    pld->enableMss = enableMss;
    pld->enableMtu = enableMtu;
    pld->mtuSize = mtuSize;
    pld->cb = std::move(callback);
    post_fifo({ SetL2tpConfig_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::addTunnel(
  const L2tpTunnelConfig& l2tpTunnelConfig, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<AddTunnelPld>();
    pld->config = l2tpTunnelConfig;
    pld->cb = std::move(callback);
    post_fifo({ AddTunnel_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::requestConfig(L2tpConfigCb l2tpConfigCb)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestConfigPld>();
    pld->cb = std::move(l2tpConfigCb);
    post_fifo({ RequestL2tpConfig_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::removeTunnel(uint32_t tunnelId, telux::common::ResponseCallback callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<TunnelIdPld>();
    pld->tunnelId = tunnelId;
    pld->cb = std::move(callback);
    post_fifo({ RemoveTunnel_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::addSession(
  uint32_t tunnelId, L2tpSessionConfig sessionConfig, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SessionPld>();
    pld->tunnelId = tunnelId;
    pld->sessionId = sessionConfig.locId;
    pld->peerId = sessionConfig.peerId;
    pld->cb = std::move(callback);
    post_fifo({ AddSession_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::removeSession(
  uint32_t tunnelId, uint32_t sessionId, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SessionPld>();
    pld->tunnelId = tunnelId;
    pld->sessionId = sessionId;
    pld->peerId = 0;
    pld->cb = std::move(callback);
    post_fifo({ RemoveSession_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::bindSessionToBackhaul(
  L2tpSessionBindConfig sessionBindConfig, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<BindPld>();
    pld->config = std::move(sessionBindConfig);
    pld->cb = std::move(callback);
    pld->bind = true;
    post_fifo({ BindSession_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::unbindSessionFromBackhaul(
  L2tpSessionBindConfig sessionBindConfig, telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<BindPld>();
    pld->config = std::move(sessionBindConfig);
    pld->cb = std::move(callback);
    pld->bind = false;
    post_fifo({ UnbindSession_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::querySessionToBackhaulBindings(
  BackhaulType backhaul, L2tpSessionBindingsResponseCb callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<QueryBindingsPld>();
    pld->backhaul = backhaul;
    pld->cb = std::move(callback);
    post_fifo({ QuerySessionBindings_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::registerListener(std::weak_ptr<IL2tpListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaL2tpManager::deregisterListener(std::weak_ptr<IL2tpListener> listener)
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
L2tpOperating_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaL2tpManager*>(h);
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
L2tpNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaL2tpManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<IndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(L2tpReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        // Only reachable if a call raced the transition out of Ready. Each
        // still fires its callback rather than being dropped: a caller awaiting
        // one would otherwise hang forever.
        case SetL2tpConfig_Signal:
        {
            auto pld = event_cast<SetConfigPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case AddTunnel_Signal:
        {
            auto pld = event_cast<AddTunnelPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case RemoveTunnel_Signal:
        {
            auto pld = event_cast<TunnelIdPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case AddSession_Signal:
        case RemoveSession_Signal:
        {
            auto pld = event_cast<SessionPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case BindSession_Signal:
        case UnbindSession_Signal:
        {
            auto pld = event_cast<BindPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case RequestL2tpConfig_Signal:
        {
            auto pld = event_cast<RequestConfigPld>(*e);
            if (pld->cb)
                pld->cb(L2tpSysConfig{}, telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        case QuerySessionBindings_Signal:
        {
            auto pld = event_cast<QueryBindingsPld>(*e);
            if (pld->cb)
                pld->cb({}, telux::common::ErrorCode::DEVICE_NOT_READY);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(L2tpOperating_St);
    }
}

chart::Status
L2tpReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaL2tpManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_gate_.markReadyAndFire(
                  telux::common::ServiceStatus::SERVICE_AVAILABLE))
            {
                self->broadcastToListeners_([](const std::shared_ptr<IL2tpListener>& l) {
                    l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                });
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_([](const std::shared_ptr<IL2tpListener>& l) {
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
                return self->to(L2tpNotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(L2tpNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(L2tpNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case SetL2tpConfig_Signal:
        {
            auto pld = event_cast<SetConfigPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["enable"] = pld->enable;
            data["enableMss"] = pld->enableMss;
            data["enableMtu"] = pld->enableMtu;
            // Forward 0 as-is: MPSS resolves it to the platform default (1422)
            // so requestConfig reports the MTU actually in force.
            data["mtuSize"] = pld->mtuSize;
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::set_l2tp_config::req, "net_l2tp.set_l2tp_config.rsp", req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestL2tpConfig_Signal:
        {
            auto pld = event_cast<RequestConfigPld>(*e);
            auto req = common::simula::makeRequestEnvelope(
              self->bridge_.currentPaId(), nlohmann::json::object()
            );
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::request_l2tp_config::req, "net_l2tp.request_l2tp_config.rsp", req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb(L2tpSysConfig{},
                         !rsp ? telux::common::ErrorCode::OPERATION_TIMEOUT
                              : telux::common::ErrorCode::GENERIC_FAILURE);
                      return;
                  }
                  L2tpSysConfig cfg{};
                  cfg.enableMtu = rsp->data->value("enableMtu", false);
                  cfg.enableTcpMss = rsp->data->value("enableTcpMss", false);
                  cfg.mtuSize = rsp->data->value("mtuSize", 0u);
                  for (const auto& t : rsp->data->value("tunnels", nlohmann::json::array()))
                      cfg.configList.push_back(toTunnelConfig(t));
                  cb(cfg, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case AddTunnel_Signal:
        {
            auto pld = event_cast<AddTunnelPld>(*e);
            auto req = common::simula::makeRequestEnvelope(
              self->bridge_.currentPaId(), fromTunnelConfig(pld->config)
            );
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::add_tunnel::req, "net_l2tp.add_tunnel.rsp", req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RemoveTunnel_Signal:
        {
            auto pld = event_cast<TunnelIdPld>(*e);
            nlohmann::json data = { { "tunnelId", pld->tunnelId } };
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::remove_tunnel::req, "net_l2tp.remove_tunnel.rsp", req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case AddSession_Signal:
        {
            auto pld = event_cast<SessionPld>(*e);
            nlohmann::json data = { { "tunnelId", pld->tunnelId },
                                    { "locId", pld->sessionId },
                                    { "peerId", pld->peerId } };
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::add_session::req, "net_l2tp.add_session.rsp", req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RemoveSession_Signal:
        {
            auto pld = event_cast<SessionPld>(*e);
            nlohmann::json data = { { "tunnelId", pld->tunnelId },
                                    { "sessionId", pld->sessionId } };
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::remove_session::req, "net_l2tp.remove_session.rsp", req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case BindSession_Signal:
        case UnbindSession_Signal:
        {
            auto pld = event_cast<BindPld>(*e);
            auto req = common::simula::makeRequestEnvelope(
              self->bridge_.currentPaId(), fromBindConfig(pld->config)
            );
            auto cb = pld->cb;
            const char* topic = pld->bind ? topics::net_l2tp::bind_session::req
                                          : topics::net_l2tp::unbind_session::req;
            const char* schema =
              pld->bind ? "net_l2tp.bind_session.rsp" : "net_l2tp.unbind_session.rsp";
            self->bridge_.send_request(
              topic, schema, req,
              [cb](std::optional<Envelope> rsp) { completeResponseCb(cb, std::move(rsp)); },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case QuerySessionBindings_Signal:
        {
            auto pld = event_cast<QueryBindingsPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            auto bh = wire::fromBackhaul(pld->backhaul);
            if (!bh.empty())
                data["backhaul"] = bh;
            auto req =
              common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::net_l2tp::query_session_bindings::req,
              "net_l2tp.query_session_bindings.rsp", req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb({}, !rsp ? telux::common::ErrorCode::OPERATION_TIMEOUT
                                  : telux::common::ErrorCode::GENERIC_FAILURE);
                      return;
                  }
                  std::vector<L2tpSessionBindConfig> out;
                  for (const auto& b : rsp->data->value("bindings", nlohmann::json::array()))
                      out.push_back(toBindConfig(b));
                  cb(out, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(L2tpOperating_St);
    }
}

CHART_NAMED_STATE(L2tpNotReady_St,  "L2tpManager::NotReady");
CHART_NAMED_STATE(L2tpReady_St,     "L2tpManager::Ready");
CHART_NAMED_STATE(L2tpOperating_St, "L2tpManager::Operating");

}  // namespace telux::data::net::simula
