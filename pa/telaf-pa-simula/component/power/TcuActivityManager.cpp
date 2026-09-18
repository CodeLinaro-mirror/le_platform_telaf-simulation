// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "TcuActivityManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <algorithm>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <cstdlib>
#include <fstream>
#include <future>

namespace telux::power::simula {

using common::simula::Envelope;
using common::simula::event_cast;

namespace {

constexpr auto kRpcTimeout = std::chrono::seconds(30);

// Local machine identity, decoupled from ClientInstanceConfig::machineName
// (which is the slave's *listen scope*, e.g. ALL_MACHINES -- not "who am I").
// Mirrors ModemBridge.cpp's readWhoami_() convention: a one-line file under
// $HOME, read at runtime so it can be set without rebuilding.
std::string
readLocalMachineName_()
{
    const char* home = std::getenv("HOME");
    if (!home)
        return "mdm";
    std::ifstream f(std::string(home) + "/.machine_name");
    if (!f)
        return "mdm";
    std::string name;
    std::getline(f, name);
    while (!name.empty() && (name.back() == '\n' || name.back() == '\r' || name.back() == ' '))
    {
        name.pop_back();
    }
    return name.empty() ? "mdm" : name;
}

std::string
tcuActivityStateToWire(telux::power::TcuActivityState state)
{
    switch (state)
    {
        case telux::power::TcuActivityState::SUSPEND:
            return "SUSPEND";
        case telux::power::TcuActivityState::RESUME:
            return "RESUME";
        case telux::power::TcuActivityState::SHUTDOWN:
            return "SHUTDOWN";
        default:
            return "UNKNOWN";
    }
}

telux::power::TcuActivityState
wireToTcuActivityState(const std::string& wire)
{
    if (wire == "SUSPEND")
        return telux::power::TcuActivityState::SUSPEND;
    if (wire == "RESUME")
        return telux::power::TcuActivityState::RESUME;
    if (wire == "SHUTDOWN")
        return telux::power::TcuActivityState::SHUTDOWN;
    return telux::power::TcuActivityState::UNKNOWN;
}

std::string
stateChangeResponseToWire(telux::power::StateChangeResponse ack)
{
    return ack == telux::power::StateChangeResponse::ACK ? "ACK" : "NACK";
}

telux::power::MachineEvent
wireToMachineEvent(const std::string& wire)
{
    return wire == "AVAILABLE" ? telux::power::MachineEvent::AVAILABLE
                                : telux::power::MachineEvent::UNAVAILABLE;
}

// power.slave_ack_status.ind carries a 2-value wire `status`
// (COMPLETE/TIMEOUT) plus `unresponsive`/`nack` arrays; ITcuActivityListener::
// onSlaveAckStatusUpdate wants a telux::common::Status summarizing the
// group's outcome. NACK takes priority over unresponsive per
// TcuActivityManager.hpp's "most number of clients" doc comment being a
// tie-break, not an ordering rule -- any NACK at all means at least one
// client explicitly rejected the transition, which is a stronger signal
// than one that merely never answered.
telux::common::Status
slaveAckStatusToCommonStatus(const nlohmann::json& data)
{
    bool hasNack = data.contains("nack") && !data["nack"].empty();
    bool hasUnresponsive = data.contains("unresponsive") && !data["unresponsive"].empty();
    if (hasNack)
        return telux::common::Status::NOTREADY;
    if (hasUnresponsive)
        return telux::common::Status::EXPIRED;
    return telux::common::Status::SUCCESS;
}

std::vector<telux::power::ClientInfo>
decodeClientInfos(const nlohmann::json& arr)
{
    std::vector<telux::power::ClientInfo> out;
    for (auto& item : arr)
        out.emplace_back(
          item.value("clientName", std::string()), item.value("machineName", std::string())
        );
    return out;
}

struct EnvPld
{
    Envelope env;
};

struct SetActivityStatePld
{
    telux::power::TcuActivityState state;
    std::string machineName;
    telux::common::ResponseCallback cb;
};

struct GetAllMachineNamesPld
{
    std::shared_ptr<std::promise<std::pair<telux::common::Status, std::vector<std::string>>>>
      result;
};

struct SendActivityStateAckPld
{
    telux::power::StateChangeResponse ack;
    telux::power::TcuActivityState state;
};

}  // namespace

chart::Status
NotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
Ready_St(chart::Hsm*, chart::Event const*);

SimulaTcuActivityManager::SimulaTcuActivityManager(
  common::simula::IModemBridge& bridge,
  telux::power::ClientInstanceConfig config,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("TcuActivityManager")
    , bridge_(bridge)
    , config_(std::move(config))
    , init_cb_(std::move(initCb))
{}

SimulaTcuActivityManager::~SimulaTcuActivityManager()
{
    // Same rationale as SimulaDataConnectionManager's dtor
    // (DataConnectionManager.cpp:1159-1179): withdraw from the bridge and
    // drain BEFORE any member teardown, since every subscribed callback
    // captures raw `this`.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaTcuActivityManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::power::subsys_ready_tcu::ind);
    bridge_.unsubscribe_event(topics::power::state_update::ind);
    bridge_.unsubscribe_event(topics::power::machine_update::ind);
    bridge_.unsubscribe_event(topics::power::slave_ack_status::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaTcuActivityManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(NotReady_St);

    bridge_.subscribe_event(
      topics::power::subsys_ready_tcu::ind,
      "power.subsys_ready_tcu.ind",
      [this](std::string_view topic, const Envelope& env) { handleStateUpdateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::power::state_update::ind,
      "power.state_update.ind",
      [this](std::string_view topic, const Envelope& env) { handleStateUpdateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::power::machine_update::ind,
      "power.machine_update.ind",
      [this](std::string_view topic, const Envelope& env) { handleMachineUpdateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::power::slave_ack_status::ind,
      "power.slave_ack_status.ind",
      [this](std::string_view topic, const Envelope& env) { handleSlaveAckStatusInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ PowerSignals::BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaTcuActivityManager::handleStateUpdateInd_(std::string_view topic, const Envelope& env)
{
    auto pld = std::make_shared<EnvPld>();
    pld->env = env;
    if (topic == topics::power::subsys_ready_tcu::ind)
        post_fifo({ PowerSignals::ReadinessEvt_Signal, pld });
    else
        post_fifo({ PowerSignals::StateUpdateInd_Signal, pld });
}

void
SimulaTcuActivityManager::handleMachineUpdateInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<EnvPld>();
    pld->env = env;
    post_fifo({ PowerSignals::MachineUpdateInd_Signal, pld });
}

void
SimulaTcuActivityManager::handleSlaveAckStatusInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<EnvPld>();
    pld->env = env;
    post_fifo({ PowerSignals::SlaveAckStatusInd_Signal, pld });
}

void
SimulaTcuActivityManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::power::ITcuActivityListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "TcuActivityManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::power::ITcuActivityListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

void
SimulaTcuActivityManager::dispatchResponse_(
  telux::common::ResponseCallback cb,
  telux::common::ErrorCode error
)
{
    // Never invoke a client ResponseCallback on this AO's own worker thread:
    // tafPmsPa's master path blocks inside its callback until the local slave
    // ack goes out, and that ack is driven by the state_update.ind that this
    // very thread has to dispatch. Hand it to ListenerDispatchAO instead --
    // the same reason broadcastToListeners_ does (see ListenerDispatchAO.hpp).
    if (!cb)
        return;
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "TcuActivityManager::dispatchResponse_";
    // DispatchTask drives everything off its (weak) listener list, so the
    // callback needs a live owner to hang onto. This shared_ptr<ResponseCallback>
    // is that owner: kept alive by the capture below, handed back as the
    // "listener", and dropped once the task is done.
    auto holder = std::make_shared<telux::common::ResponseCallback>(std::move(cb));
    task->listeners.push_back(std::weak_ptr<void>(holder));
    task->invoker = [holder, error](std::shared_ptr<void> raw) {
        (void)raw;
        (*holder)(error);
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

void
SimulaTcuActivityManager::broadcastServiceState_(telux::common::ServiceStatus status)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "TcuActivityManager::broadcastServiceState_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& weak : service_listeners_)
        {
            if (auto sp = weak.lock())
                task->listeners.push_back(sp);
        }
    }
    if (task->listeners.empty())
        return;
    task->invoker = [status](std::shared_ptr<void> raw) {
        std::static_pointer_cast<telux::common::IServiceStatusListener>(raw)
          ->onServiceStatusChange(status);
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaTcuActivityManager::isReadyDerived_() const
{
    return const_cast<SimulaTcuActivityManager*>(this)->current_state() == Ready_St;
}

// ---------------------------------------------------------------------------
// telux::power::ITcuActivityManager

telux::common::ServiceStatus
SimulaTcuActivityManager::getServiceStatus()
{
    return isReadyDerived_() ? telux::common::ServiceStatus::SERVICE_AVAILABLE
                              : telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

telux::common::Status
SimulaTcuActivityManager::registerListener(std::weak_ptr<telux::power::ITcuActivityListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::deregisterListener(std::weak_ptr<telux::power::ITcuActivityListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(), listeners_.end(),
        [&target](const std::weak_ptr<telux::power::ITcuActivityListener>& w) {
            auto sp = w.lock();
            return !sp || sp == target;
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::registerServiceStateListener(
  std::weak_ptr<telux::common::IServiceStatusListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    service_listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::deregisterServiceStateListener(
  std::weak_ptr<telux::common::IServiceStatusListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    service_listeners_.erase(
      std::remove_if(
        service_listeners_.begin(), service_listeners_.end(),
        [&target](const std::weak_ptr<telux::common::IServiceStatusListener>& w) {
            auto sp = w.lock();
            return !sp || sp == target;
        }
      ),
      service_listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::getMachineName(std::string& machineName)
{
    machineName = readLocalMachineName_();
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::getAllMachineNames(std::vector<std::string>& machineNames)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<GetAllMachineNamesPld>();
    pld->result = std::make_shared<
      std::promise<std::pair<telux::common::Status, std::vector<std::string>>>>();
    auto future = pld->result->get_future();
    post_fifo({ PowerSignals::GetAllMachineNames_Signal, pld });
    if (future.wait_for(kRpcTimeout) != std::future_status::ready)
        return telux::common::Status::EXPIRED;
    auto [status, names] = future.get();
    machineNames = std::move(names);
    return status;
}

telux::common::Status
SimulaTcuActivityManager::setActivityState(
  telux::power::TcuActivityState state,
  std::string machineName,
  telux::common::ResponseCallback callback
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SetActivityStatePld>();
    pld->state = state;
    pld->machineName = std::move(machineName);
    pld->cb = std::move(callback);
    post_fifo({ PowerSignals::SetActivityState_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode
SimulaTcuActivityManager::getActivityState(
  std::string /*machineName*/,
  telux::power::TcuActivityState& state
)
{
    // No dedicated get-state RPC on the wire (only state_update.ind pushes
    // changes) -- report the last indication this Manager observed, same
    // shape as the deprecated getActivityState() below.
    state = local_state_;
    return telux::common::ErrorCode::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::sendActivityStateAck(
  telux::power::StateChangeResponse ack,
  telux::power::TcuActivityState state
)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SendActivityStateAckPld>();
    pld->ack = ack;
    pld->state = state;
    post_fifo({ PowerSignals::SendActivityStateAck_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaTcuActivityManager::setModemActivityState(telux::power::TcuActivityState /*state*/)
{
    // No wire equivalent for a modem-local activity state distinct from
    // setActivityState's machine-scoped transition -- simula has no separate
    // modem-vs-application power domain to model.
    return telux::common::Status::NOTSUPPORTED;
}

bool
SimulaTcuActivityManager::isReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaTcuActivityManager::onReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::Status
SimulaTcuActivityManager::setActivityState(
  telux::power::TcuActivityState state,
  telux::common::ResponseCallback callback
)
{
    return setActivityState(state, telux::power::LOCAL_MACHINE, std::move(callback));
}

telux::common::Status
SimulaTcuActivityManager::sendActivityStateAck(telux::power::TcuActivityStateAck ack)
{
    telux::power::TcuActivityState mapped;
    switch (ack)
    {
        case telux::power::TcuActivityStateAck::SUSPEND_ACK:
            mapped = telux::power::TcuActivityState::SUSPEND;
            break;
        default:
            mapped = telux::power::TcuActivityState::SHUTDOWN;
            break;
    }
    return sendActivityStateAck(telux::power::StateChangeResponse::ACK, mapped);
}

telux::power::TcuActivityState
SimulaTcuActivityManager::getActivityState()
{
    return local_state_;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
NotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaTcuActivityManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case PowerSignals::ReadinessEvt_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(Ready_St);
            return chart::Status::HANDLED;
        }
        case PowerSignals::BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        case PowerSignals::SetActivityState_Signal:
        {
            auto pld = event_cast<SetActivityStatePld>(*e);
            if (pld->cb)
                self->dispatchResponse_(pld->cb, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case PowerSignals::GetAllMachineNames_Signal:
        {
            auto pld = event_cast<GetAllMachineNamesPld>(*e);
            pld->result->set_value({ telux::common::Status::NOTREADY, {} });
            return chart::Status::HANDLED;
        }
        case PowerSignals::SendActivityStateAck_Signal:
            // No promise/callback attached -- just drop.
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
Ready_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaTcuActivityManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            if (!self->init_cb_fired_)
            {
                self->init_cb_fired_ = true;
                if (self->init_cb_)
                    self->init_cb_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            else
            {
                self->broadcastServiceState_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastServiceState_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
            return chart::Status::HANDLED;

        case PowerSignals::ReadinessEvt_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "UNAVAILABLE")
                return self->to(NotReady_St);
            return chart::Status::HANDLED;
        }

        case PowerSignals::BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
                return self->to(NotReady_St);
            return chart::Status::HANDLED;
        }

        case PowerSignals::SetActivityState_Signal:
        {
            auto pld = event_cast<SetActivityStatePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["state"] = tcuActivityStateToWire(pld->state);
            data["machineName"] = pld->machineName;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            // QCPMD parity: the master's ResponseCallback answers "request
            // accepted", NOT "transition finished". Real QCPMD's
            // send_cmd_msg_resp() (power_qmi.cpp) enqueues the event and
            // sends PWR_MGR_CMD_RESP immediately; the consolidated outcome
            // arrives much later as a separate PWR_MGR_STATE_CHANGE_IND_EXT
            // from its sendind_waitforack() thread -- which maps to
            // slave_ack_status.ind -> onSlaveAckStatusUpdate() here.
            //
            // So the callback fires as soon as the request is on the wire,
            // and is NOT bound to set_state.rsp. Binding it there would
            // deadlock a master that is also a slave on this machine: MP
            // withholds set_state.rsp until every awaited machine acks,
            // including this one, but a master blocking its dispatch thread
            // inside the callback can never drain the state_update.ind that
            // would make it ack.
            //
            // set_state.req is still a req/rsp method per the contract
            // (registry/power.yaml), so the RPC is kept -- MP always answers
            // it. The response is consumed here only to retire the in-flight
            // entry and log a mismatch; the client already heard SUCCESS.
            self->bridge_.send_request(
              topics::power::set_state::req,
              "power.set_state.rsp",
              req,
              [](std::optional<Envelope> rsp) {
                  if (!rsp || rsp->error || !(rsp->data && rsp->data->value("ok", false)))
                  {
                      // Not surfaced to the client: onSlaveAckStatusUpdate is
                      // the SDK-sanctioned channel for a failed round.
                      LOG_WARN("[TcuActivityManager] set_state round did not complete ok");
                  }
              },
              kRpcTimeout
            );
            if (pld->cb)
                self->dispatchResponse_(pld->cb, telux::common::ErrorCode::SUCCESS);
            return chart::Status::HANDLED;
        }

        case PowerSignals::GetAllMachineNames_Signal:
        {
            auto pld = event_cast<GetAllMachineNamesPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto result = pld->result;
            self->bridge_.send_request(
              topics::power::get_machine_names::req,
              "power.get_machine_names.rsp",
              req,
              [result](std::optional<Envelope> rsp) {
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      result->set_value({ telux::common::Status::FAILED, {} });
                      return;
                  }
                  std::vector<std::string> names;
                  for (auto& n : rsp->data->value("machineNames", nlohmann::json::array()))
                      names.push_back(n.get<std::string>());
                  result->set_value({ telux::common::Status::SUCCESS, std::move(names) });
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case PowerSignals::SendActivityStateAck_Signal:
        {
            auto pld = event_cast<SendActivityStateAckPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["response"] = stateChangeResponseToWire(pld->ack);
            data["state"] = tcuActivityStateToWire(pld->state);
            data["machineName"] = readLocalMachineName_();
            auto msg = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            self->bridge_.publish_oneway(topics::power::slave_ack::req, "power.slave_ack.ff", msg);
            return chart::Status::HANDLED;
        }

        case PowerSignals::StateUpdateInd_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = wireToTcuActivityState(pld->env.data->value("state", std::string()));
            auto machineName = pld->env.data->value("machineName", std::string());
            self->local_state_ = state;
            self->broadcastToListeners_(
              [state, machineName](const std::shared_ptr<telux::power::ITcuActivityListener>& l) {
                  l->onTcuActivityStateUpdate(state, machineName);
                  l->onTcuActivityStateUpdate(state);
              }
            );
            return chart::Status::HANDLED;
        }

        case PowerSignals::MachineUpdateInd_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto machineName = pld->env.data->value("machineName", std::string());
            auto machineEvent = wireToMachineEvent(pld->env.data->value("machineEvent", std::string()));
            self->broadcastToListeners_(
              [machineName, machineEvent](const std::shared_ptr<telux::power::ITcuActivityListener>& l) {
                  l->onMachineUpdate(machineName, machineEvent);
              }
            );
            return chart::Status::HANDLED;
        }

        case PowerSignals::SlaveAckStatusInd_Signal:
        {
            auto pld = event_cast<EnvPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto& data = *pld->env.data;
            auto status = slaveAckStatusToCommonStatus(data);
            auto machineName = data.value("machineName", std::string());
            auto unresponsive = decodeClientInfos(data.value("unresponsive", nlohmann::json::array()));
            auto nack = decodeClientInfos(data.value("nack", nlohmann::json::array()));
            self->broadcastToListeners_(
              [status, machineName, unresponsive, nack](
                const std::shared_ptr<telux::power::ITcuActivityListener>& l
              ) {
                  l->onSlaveAckStatusUpdate(status, machineName, unresponsive, nack);
                  l->onSlaveAckStatusUpdate(status);
              }
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

}  // namespace telux::power::simula
