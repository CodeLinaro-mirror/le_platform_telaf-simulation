// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ServingSystemManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <nlohmann/json.hpp>

namespace telux::data::simula {

using namespace DataSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

telux::data::DataServiceState
wireToServiceState(const std::string& s)
{
    if (s == "IN_SERVICE")     return telux::data::DataServiceState::IN_SERVICE;
    if (s == "OUT_OF_SERVICE") return telux::data::DataServiceState::OUT_OF_SERVICE;
    return telux::data::DataServiceState::UNKNOWN;
}

telux::data::NetworkRat
wireToNetworkRat(const std::string& s)
{
    if (s == "GSM")       return telux::data::NetworkRat::GSM;
    if (s == "WCDMA")     return telux::data::NetworkRat::WCDMA;
    if (s == "LTE")       return telux::data::NetworkRat::LTE;
    if (s == "CDMA_1X")   return telux::data::NetworkRat::CDMA_1X;
    if (s == "CDMA_EVDO") return telux::data::NetworkRat::CDMA_EVDO;
    return telux::data::NetworkRat::UNKNOWN;
}

telux::data::DrbStatus
wireToDrbStatus(const std::string& s)
{
    if (s == "ACTIVE")  return telux::data::DrbStatus::ACTIVE;
    if (s == "DORMANT") return telux::data::DrbStatus::DORMANT;
    return telux::data::DrbStatus::UNKNOWN;
}

telux::data::RoamingType
wireToRoamingType(const std::string& s)
{
    if (s == "DOMESTIC")      return telux::data::RoamingType::DOMESTIC;
    if (s == "INTERNATIONAL") return telux::data::RoamingType::INTERNATIONAL;
    return telux::data::RoamingType::UNKNOWN;
}

telux::data::NrIconType
wireToNrIconType(const std::string& s)
{
    if (s == "BASIC") return telux::data::NrIconType::BASIC;
    if (s == "UWB")   return telux::data::NrIconType::UWB;
    return telux::data::NrIconType::NONE;
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct RpcResultPld
{
    std::optional<Envelope> rsp;
};

struct StateIndPld
{
    Envelope env;
};

struct RequestServiceStatusPld
{
    telux::data::RequestServiceStatusResponseCb cb;
};

struct RequestRoamingStatusPld
{
    telux::data::RequestRoamingStatusResponseCb cb;
};

struct RequestNrIconTypePld
{
    telux::data::RequestNrIconTypeResponseCb cb;
};

struct MakeDormantPld
{
    telux::common::ResponseCallback cb;
};

}  // namespace

chart::Status
ServNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
ServReady_St(chart::Hsm*, chart::Event const*);

SimulaServingSystemManager::SimulaServingSystemManager(
  SlotId slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("ServingSystemManager")
    , bridge_(bridge)
    , slotId_(slotId)
    , init_cb_(std::move(initCb))
{}

SimulaServingSystemManager::~SimulaServingSystemManager()
{
    // Withdraw from the bridge before anything else: the callbacks below
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaServingSystemManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::data::subsys_ready_serv::ind);
    bridge_.unsubscribe_event(topics::data::serv_state::ind);
    bridge_.unsubscribe_event(topics::data::serv_roaming::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaServingSystemManager::start()
{
    if (running())
        return;
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(ServNotReady_St);

    bridge_.subscribe_event(
      topics::data::subsys_ready_serv::ind,
      "data.subsys_ready_serv.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::serv_state::ind,
      "data.serv_state.ind",
      [this](std::string_view topic, const Envelope& env) { handleServStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::data::serv_roaming::ind,
      "data.serv_roaming.ind",
      [this](std::string_view topic, const Envelope& env) { handleServRoamingInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaServingSystemManager::handleReadinessInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaServingSystemManager::handleServStateInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ServStateEvt_Signal, pld });
}

void
SimulaServingSystemManager::handleServRoamingInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ServRoamingEvt_Signal, pld });
}

void
SimulaServingSystemManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::data::IServingSystemListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "ServingSystemManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::data::IServingSystemListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// telux::data::IServingSystemManager

telux::common::ServiceStatus
SimulaServingSystemManager::getServiceStatus()
{
    return last_status_.load();
}

telux::data::DrbStatus
SimulaServingSystemManager::getDrbStatus()
{
    return drb_status_.load();
}

bool
SimulaServingSystemManager::isReadyDerived_() const
{
    return const_cast<SimulaServingSystemManager*>(this)->current_state() == ServReady_St;
}

void
SimulaServingSystemManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

void
SimulaServingSystemManager::publishDrb_(telux::data::DrbStatus d)
{
    drb_status_.store(d);
}

telux::common::Status
SimulaServingSystemManager::requestServiceStatus(telux::data::RequestServiceStatusResponseCb callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestServiceStatusPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestServiceStatus_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaServingSystemManager::requestRoamingStatus(telux::data::RequestRoamingStatusResponseCb callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestRoamingStatusPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestRoamingStatus_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaServingSystemManager::requestNrIconType(telux::data::RequestNrIconTypeResponseCb callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestNrIconTypePld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestNrIconType_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaServingSystemManager::makeDormant(telux::common::ResponseCallback callback)
{
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<MakeDormantPld>();
    pld->cb = std::move(callback);
    post_fifo({ MakeDormant_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaServingSystemManager::registerListener(
  std::weak_ptr<telux::data::IServingSystemListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaServingSystemManager::deregisterListener(
  std::weak_ptr<telux::data::IServingSystemListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::data::IServingSystemListener>& w) {
            auto sp = w.lock();
            return !sp || (target && sp == target);
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

SlotId
SimulaServingSystemManager::getSlotId()
{
    // IServingSystemManager::getSlotId() returns SlotId (unlike
    // IDataProfileManager's, which returns plain int) -- verified against
    // the real header, not a typo.
    return slotId_;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
ServNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaServingSystemManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(ServReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        // API calls landed here synchronously return NOTREADY before ever
        // posting -- reachable only if stale-queued during a transition.
        case RequestServiceStatus_Signal:
        case RequestRoamingStatus_Signal:
        case RequestNrIconType_Signal:
        case MakeDormant_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
ServReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaServingSystemManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!self->init_cb_fired_)
            {
                self->init_cb_fired_ = true;
                if (self->init_cb_)
                    self->init_cb_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            else
            {
                self->broadcastToListeners_(
                  [](const std::shared_ptr<telux::data::IServingSystemListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::data::IServingSystemListener>& l) {
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
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(ServNotReady_St);
            }
            if (state == "FAILED")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(ServNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(ServNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case ServStateEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto drb = wireToDrbStatus(pld->env.data->value("drbStatus", std::string()));
            self->publishDrb_(drb);
            telux::data::ServiceStatus status{};
            status.serviceState = wireToServiceState(pld->env.data->value("serviceState", std::string()));
            status.networkRat = wireToNetworkRat(pld->env.data->value("networkRat", std::string()));
            self->broadcastToListeners_(
              [status, drb](const std::shared_ptr<telux::data::IServingSystemListener>& l) {
                  l->onDrbStatusChanged(drb);
                  l->onServiceStateChanged(status);
              }
            );
            return chart::Status::HANDLED;
        }

        case ServRoamingEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::data::RoamingStatus roaming{};
            roaming.isRoaming = pld->env.data->value("isRoaming", false);
            roaming.type = wireToRoamingType(pld->env.data->value("roamingType", std::string()));
            self->broadcastToListeners_(
              [roaming](const std::shared_ptr<telux::data::IServingSystemListener>& l) {
                  l->onRoamingStatusChanged(roaming);
              }
            );
            return chart::Status::HANDLED;
        }

        case RequestServiceStatus_Signal:
        {
            auto pld = event_cast<RequestServiceStatusPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::request_service_status::req,
              "data.request_service_status.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  telux::data::ServiceStatus status{};
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb(status, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  status.serviceState =
                    wireToServiceState(rsp->data->value("serviceState", std::string()));
                  status.networkRat = wireToNetworkRat(rsp->data->value("networkRat", std::string()));
                  cb(status, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestRoamingStatus_Signal:
        {
            auto pld = event_cast<RequestRoamingStatusPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::request_roaming_status::req,
              "data.request_roaming_status.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  telux::data::RoamingStatus roaming{};
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb(roaming, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  roaming.isRoaming = rsp->data->value("isRoaming", false);
                  roaming.type = wireToRoamingType(rsp->data->value("roamingType", std::string()));
                  cb(roaming, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestNrIconType_Signal:
        {
            auto pld = event_cast<RequestNrIconTypePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::request_nr_icon_type::req,
              "data.request_nr_icon_type.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      cb(telux::data::NrIconType::NONE, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb(
                    wireToNrIconType(rsp->data->value("iconType", std::string())),
                    telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case MakeDormant_Signal:
        {
            auto pld = event_cast<MakeDormantPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = static_cast<int>(self->slotId_);
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::data::make_dormant::req,
              "data.make_dormant.rsp",
              req,
              [cb](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error)
                  {
                      cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  cb(telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(ServNotReady_St,  "ServingSystemManager::NotReady");
CHART_NAMED_STATE(ServReady_St,     "ServingSystemManager::Ready");

}  // namespace telux::data::simula
