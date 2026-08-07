// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "NetworkSelectionManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using namespace TelSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

telux::tel::NetworkSelectionMode
wireToNetworkSelectionMode(const std::string& s)
{
    if (s == "AUTOMATIC") return telux::tel::NetworkSelectionMode::AUTOMATIC;
    if (s == "MANUAL")    return telux::tel::NetworkSelectionMode::MANUAL;
    LOG_WARN("[NetworkSelectionManager] wireToNetworkSelectionMode: unrecognized mode=%s", s.c_str());
    return telux::tel::NetworkSelectionMode::UNKNOWN;
}

std::string
networkSelectionModeToWire(telux::tel::NetworkSelectionMode mode)
{
    switch (mode)
    {
        case telux::tel::NetworkSelectionMode::AUTOMATIC: return "AUTOMATIC";
        case telux::tel::NetworkSelectionMode::MANUAL:    return "MANUAL";
        default:                                            return "UNKNOWN";
    }
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct StateIndPld
{
    Envelope env;
};

struct SetNetworkSelectionModePld
{
    telux::tel::NetworkSelectionMode mode;
    std::string mcc;
    std::string mnc;
    telux::common::ResponseCallback cb;
};

struct RequestModeInfoPld
{
    telux::tel::SelectionModeInfoCb cb;
};

struct RequestModePld
{
    telux::tel::SelectionModeResponseCallback cb;
};

struct SetInitCbPld
{
    telux::common::InitResponseCb cb;
};

}  // namespace

chart::Status
NetSelNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
NetSelReady_St(chart::Hsm*, chart::Event const*);

SimulaNetworkSelectionManager::SimulaNetworkSelectionManager(
  int slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("NetworkSelectionManager")
    , bridge_(bridge)
    , slotId_(slotId)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaNetworkSelectionManager::~SimulaNetworkSelectionManager()
{
    // Withdraw from the bridge before anything else: the callbacks below
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaNetworkSelectionManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::radio::subsys_ready_netsel::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaNetworkSelectionManager::start()
{
    if (running())
        return;
    LOG_INFO("[NetworkSelectionManager] start() slot=%d", slotId_);
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(NetSelNotReady_St);

    bridge_.subscribe_event(
      topics::radio::subsys_ready_netsel::ind,
      "radio.subsys_ready_netsel.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaNetworkSelectionManager::setInitCallback(telux::common::InitResponseCb cb)
{
    LOG_DEBUG("[NetworkSelectionManager] setInitCallback slot=%d hasCb=%d", slotId_, cb ? 1 : 0);
    if (!cb)
        return;
    // Already Ready: NetSelReady_St's Entry has fired and will not fire again
    // for this caller, so queuing into init_cbs_ would strand it.
    if (isReadyDerived_())
    {
        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return;
    }
    // Not Ready -- hand off through the AO; init_cbs_ is appended to and read
    // only on the worker thread.
    auto pld = std::make_shared<SetInitCbPld>();
    pld->cb = std::move(cb);
    post_fifo({ SetInitCb_Signal, pld });
}

void
SimulaNetworkSelectionManager::handleReadinessInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto status = env.data ? env.data->value("status", std::string()) : std::string();
    LOG_DEBUG("[NetworkSelectionManager] handleReadinessInd_ status=%s", status.c_str());
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaNetworkSelectionManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::INetworkSelectionListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "NetworkSelectionManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::tel::INetworkSelectionListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// Readiness

bool
SimulaNetworkSelectionManager::isReadyDerived_() const
{
    return const_cast<SimulaNetworkSelectionManager*>(this)->current_state() == NetSelReady_St;
}

void
SimulaNetworkSelectionManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

bool
SimulaNetworkSelectionManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaNetworkSelectionManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::ServiceStatus
SimulaNetworkSelectionManager::getServiceStatus()
{
    return last_status_.load();
}

// ---------------------------------------------------------------------------
// telux::tel::INetworkSelectionManager

telux::common::Status
SimulaNetworkSelectionManager::requestNetworkSelectionMode(telux::tel::SelectionModeInfoCb callback)
{
    LOG_DEBUG("[NetworkSelectionManager] requestNetworkSelectionMode(info) slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestModeInfoPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestNetworkSelectionModeInfo_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNetworkSelectionManager::requestNetworkSelectionMode(
  telux::tel::SelectionModeResponseCallback callback
)
{
    LOG_DEBUG("[NetworkSelectionManager] requestNetworkSelectionMode(deprecated) slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestModePld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestNetworkSelectionMode_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNetworkSelectionManager::setNetworkSelectionMode(
  telux::tel::NetworkSelectionMode selectMode,
  std::string mcc,
  std::string mnc,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[NetworkSelectionManager] setNetworkSelectionMode slot=%d mode=%s mcc=%s mnc=%s",
      slotId_,
      networkSelectionModeToWire(selectMode).c_str(),
      mcc.c_str(),
      mnc.c_str()
    );
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<SetNetworkSelectionModePld>();
    pld->mode = selectMode;
    pld->mcc = std::move(mcc);
    pld->mnc = std::move(mnc);
    pld->cb = std::move(callback);
    post_fifo({ SetNetworkSelectionMode_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNetworkSelectionManager::requestPreferredNetworks(telux::tel::PreferredNetworksCallback callback)
{
    // Preferred-network list is out of scope for the requested radio API
    // list (see registry/radio.yaml scope note) -- no wire RPC exists.
    LOG_DEBUG("[NetworkSelectionManager] requestPreferredNetworks slot=%d -- NOT_SUPPORTED", slotId_);
    if (callback)
        callback({}, {}, telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaNetworkSelectionManager::setPreferredNetworks(
  std::vector<telux::tel::PreferredNetworkInfo>,
  bool,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG("[NetworkSelectionManager] setPreferredNetworks slot=%d -- NOT_SUPPORTED", slotId_);
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaNetworkSelectionManager::performNetworkScan(telux::tel::NetworkScanCallback callback)
{
    // PLMN scan is out of scope for the requested radio API list.
    LOG_DEBUG("[NetworkSelectionManager] performNetworkScan slot=%d -- NOT_SUPPORTED", slotId_);
    if (callback)
        callback({}, telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaNetworkSelectionManager::performNetworkScan(
  telux::tel::NetworkScanInfo,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG("[NetworkSelectionManager] performNetworkScan(info) slot=%d -- NOT_SUPPORTED", slotId_);
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode
SimulaNetworkSelectionManager::abortNetworkScan()
{
    LOG_DEBUG("[NetworkSelectionManager] abortNetworkScan slot=%d -- NOT_SUPPORTED", slotId_);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode
SimulaNetworkSelectionManager::setLteDubiousCell(const std::vector<telux::tel::LteDubiousCell>&)
{
    LOG_DEBUG("[NetworkSelectionManager] setLteDubiousCell slot=%d -- NOT_SUPPORTED", slotId_);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode
SimulaNetworkSelectionManager::setNrDubiousCell(const std::vector<telux::tel::NrDubiousCell>&)
{
    LOG_DEBUG("[NetworkSelectionManager] setNrDubiousCell slot=%d -- NOT_SUPPORTED", slotId_);
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::Status
SimulaNetworkSelectionManager::registerListener(
  std::weak_ptr<telux::tel::INetworkSelectionListener> listener
)
{
    LOG_DEBUG("[NetworkSelectionManager] registerListener slot=%d", slotId_);
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaNetworkSelectionManager::deregisterListener(
  std::weak_ptr<telux::tel::INetworkSelectionListener> listener
)
{
    LOG_DEBUG("[NetworkSelectionManager] deregisterListener slot=%d", slotId_);
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::INetworkSelectionListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
NetSelNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaNetworkSelectionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            LOG_INFO("[NetworkSelectionManager] -> NotReady");
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            auto status = pld->env.data ? pld->env.data->value("status", std::string()) : std::string();
            LOG_DEBUG("[NetworkSelectionManager] NotReady ReadinessEvt_Signal status=%s", status.c_str());
            if (status == "AVAILABLE")
            {
                LOG_INFO("[NetworkSelectionManager] -> Ready");
                return self->to(NetSelReady_St);
            }
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        // Arm the callback so the transition into Ready fires it. Every
        // caller's callback is queued -- see init_cbs_'s header comment --
        // so a repeat cache-hit caller from the factory never strands an
        // earlier one still waiting on its own callback.
        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            self->init_cbs_.push_back(pld->cb);
            return chart::Status::HANDLED;
        }
   
        case SetNetworkSelectionMode_Signal:
        {
            auto pld = event_cast<SetNetworkSelectionModePld>(*e);
            LOG_WARN(
              "[NetworkSelectionManager] stale SetNetworkSelectionMode request failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response (mode=%s)",
              networkSelectionModeToWire(pld->mode).c_str()
            );
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case RequestNetworkSelectionModeInfo_Signal:
        {
            auto pld = event_cast<RequestModeInfoPld>(*e);
            LOG_WARN(
              "[NetworkSelectionManager] stale RequestNetworkSelectionModeInfo request failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
            {
                telux::tel::NetworkModeInfo info{};
                pld->cb(info, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            }
            return chart::Status::HANDLED;
        }
        case RequestNetworkSelectionMode_Signal:
        {
            auto pld = event_cast<RequestModePld>(*e);
            LOG_WARN(
              "[NetworkSelectionManager] stale RequestNetworkSelectionMode request failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
                pld->cb(
                  telux::tel::NetworkSelectionMode::AUTOMATIC,
                  telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE
                );
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
NetSelReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaNetworkSelectionManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            LOG_INFO("[NetworkSelectionManager] -> Ready");
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_cbs_.empty())
            {
                std::vector<telux::common::InitResponseCb> cbs;
                cbs.swap(self->init_cbs_);
                for (auto& cb : cbs)
                    if (cb)
                        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            else
            {
                self->broadcastToListeners_(
                  [](const std::shared_ptr<telux::tel::INetworkSelectionListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            LOG_INFO("[NetworkSelectionManager] -> NotReady");
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::INetworkSelectionListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            LOG_DEBUG("[NetworkSelectionManager] Ready ReadinessEvt_Signal status=%s", state.c_str());
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(NetSelNotReady_St);
            }
            return chart::Status::HANDLED;
        }


        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            LOG_DEBUG(
              "[NetworkSelectionManager] Ready BridgeConnectivityChanged_Signal operational=%d",
              (pld && *pld) ? 1 : 0
            );
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(NetSelNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case SetNetworkSelectionMode_Signal:
        {
            auto pld = event_cast<SetNetworkSelectionModePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["mode"] = networkSelectionModeToWire(pld->mode);
            data["mcc"] = pld->mcc;
            data["mnc"] = pld->mnc;
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[NetworkSelectionManager] send_request set_network_selection_mode slot=%d corrId=%s mode=%s",
              self->slotId_,
              req.corrId.c_str(),
              networkSelectionModeToWire(pld->mode).c_str()
            );
            self->bridge_.send_request(
              topics::radio::set_network_selection_mode::req,
              "radio.set_network_selection_mode.rsp",
              req,
              [cb, slotId = self->slotId_, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error)
                  {
                      LOG_WARN(
                        "[NetworkSelectionManager] set_network_selection_mode failed slot=%d corrId=%s "
                        "-- timeout/error",
                        slotId,
                        corrId.c_str()
                      );
                      cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  LOG_DEBUG(
                    "[NetworkSelectionManager] set_network_selection_mode success slot=%d corrId=%s",
                    slotId,
                    corrId.c_str()
                  );
                  cb(telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestNetworkSelectionModeInfo_Signal:
        {
            auto pld = event_cast<RequestModeInfoPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[NetworkSelectionManager] send_request request_network_selection_mode(info) slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_network_selection_mode::req,
              "radio.request_network_selection_mode.rsp",
              req,
              [cb, slotId = self->slotId_, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  telux::tel::NetworkModeInfo info{};
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[NetworkSelectionManager] request_network_selection_mode(info) failed slot=%d "
                        "corrId=%s -- timeout/error",
                        slotId,
                        corrId.c_str()
                      );
                      cb(info, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  info.mode = wireToNetworkSelectionMode(rsp->data->value("mode", std::string()));
                  info.mcc = rsp->data->value("mcc", std::string());
                  info.mnc = rsp->data->value("mnc", std::string());
                  LOG_DEBUG(
                    "[NetworkSelectionManager] request_network_selection_mode(info) success slot=%d "
                    "corrId=%s mode=%d",
                    slotId,
                    corrId.c_str(),
                    static_cast<int>(info.mode)
                  );
                  cb(info, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestNetworkSelectionMode_Signal:
        {
            auto pld = event_cast<RequestModePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[NetworkSelectionManager] send_request request_network_selection_mode slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_network_selection_mode::req,
              "radio.request_network_selection_mode.rsp",
              req,
              [cb, slotId = self->slotId_, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[NetworkSelectionManager] request_network_selection_mode failed slot=%d "
                        "corrId=%s -- timeout/error",
                        slotId,
                        corrId.c_str()
                      );
                      cb(telux::tel::NetworkSelectionMode::UNKNOWN,
                          telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  auto mode = wireToNetworkSelectionMode(rsp->data->value("mode", std::string()));
                  LOG_DEBUG(
                    "[NetworkSelectionManager] request_network_selection_mode success slot=%d "
                    "corrId=%s mode=%d",
                    slotId,
                    corrId.c_str(),
                    static_cast<int>(mode)
                  );
                  cb(mode, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(NetSelNotReady_St, "NetworkSelectionManager::NotReady");
CHART_NAMED_STATE(NetSelReady_St,    "NetworkSelectionManager::Ready");

}  // namespace telux::tel::simula
