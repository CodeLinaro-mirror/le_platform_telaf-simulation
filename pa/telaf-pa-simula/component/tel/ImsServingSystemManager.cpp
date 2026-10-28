// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ImsServingSystemManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <algorithm>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using namespace ImsSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

// telux::tel::RegistrationStatus enumerator names verbatim.
telux::tel::RegistrationStatus
wireToRegistrationStatus(const std::string& s)
{
    if (s == "NOT_REGISTERED")     return telux::tel::RegistrationStatus::NOT_REGISTERED;
    if (s == "REGISTERING")        return telux::tel::RegistrationStatus::REGISTERING;
    if (s == "REGISTERED")         return telux::tel::RegistrationStatus::REGISTERED;
    if (s == "LIMITED_REGISTERED") return telux::tel::RegistrationStatus::LIMITED_REGISTERED;
    LOG_WARN(
      "[ImsServingSystemManager] wireToRegistrationStatus: unrecognized value \"%s\" -- "
      "defaulting to UNKOWN_STATE",
      s.c_str()
    );
    return telux::tel::RegistrationStatus::UNKOWN_STATE;
}

telux::tel::RadioTechnology
wireToRadioTechnology(const std::string& s)
{
    if (s == "RADIO_TECH_GPRS")     return telux::tel::RadioTechnology::RADIO_TECH_GPRS;
    if (s == "RADIO_TECH_EDGE")     return telux::tel::RadioTechnology::RADIO_TECH_EDGE;
    if (s == "RADIO_TECH_UMTS")     return telux::tel::RadioTechnology::RADIO_TECH_UMTS;
    if (s == "RADIO_TECH_LTE")      return telux::tel::RadioTechnology::RADIO_TECH_LTE;
    if (s == "RADIO_TECH_GSM")      return telux::tel::RadioTechnology::RADIO_TECH_GSM;
    if (s == "RADIO_TECH_TD_SCDMA") return telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA;
    if (s == "RADIO_TECH_LTE_CA")   return telux::tel::RadioTechnology::RADIO_TECH_LTE_CA;
    if (s == "RADIO_TECH_NR5G")     return telux::tel::RadioTechnology::RADIO_TECH_NR5G;
    return telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN;
}

telux::tel::CellularServiceStatus
wireToCellularServiceStatus(const std::string& s)
{
    if (s == "NO_SERVICE")      return telux::tel::CellularServiceStatus::NO_SERVICE;
    if (s == "LIMITED_SERVICE") return telux::tel::CellularServiceStatus::LIMITED_SERVICE;
    if (s == "FULL_SERVICE")    return telux::tel::CellularServiceStatus::FULL_SERVICE;
    return telux::tel::CellularServiceStatus::UNKNOWN;
}

telux::tel::PdpFailureCode
wireToPdpFailureCode(const std::string& s)
{
    if (s == "OPTION_UNSUBSCRIBED")         return telux::tel::PdpFailureCode::OPTION_UNSUBSCRIBED;
    if (s == "UNKNOWN_PDP")                 return telux::tel::PdpFailureCode::UNKNOWN_PDP;
    if (s == "REASON_NOT_SPECIFIED")        return telux::tel::PdpFailureCode::REASON_NOT_SPECIFIED;
    if (s == "CONNECTION_BRINGUP_FAILURE")  return telux::tel::PdpFailureCode::CONNECTION_BRINGUP_FAILURE;
    if (s == "CONNECTION_IKE_AUTH_FAILURE") return telux::tel::PdpFailureCode::CONNECTION_IKE_AUTH_FAILURE;
    if (s == "USER_AUTH_FAILED")            return telux::tel::PdpFailureCode::USER_AUTH_FAILED;
    return telux::tel::PdpFailureCode::OTHER_FAILURE;
}

telux::common::EndReasonType
wireToEndReasonType(const std::string& s)
{
    if (s == "CE_MOBILE_IP")            return telux::common::EndReasonType::CE_MOBILE_IP;
    if (s == "CE_INTERNAL")             return telux::common::EndReasonType::CE_INTERNAL;
    if (s == "CE_CALL_MANAGER_DEFINED") return telux::common::EndReasonType::CE_CALL_MANAGER_DEFINED;
    if (s == "CE_3GPP_SPEC_DEFINED")    return telux::common::EndReasonType::CE_3GPP_SPEC_DEFINED;
    if (s == "CE_PPP")                  return telux::common::EndReasonType::CE_PPP;
    if (s == "CE_EHRPD")                return telux::common::EndReasonType::CE_EHRPD;
    if (s == "CE_IPV6")                 return telux::common::EndReasonType::CE_IPV6;
    if (s == "CE_HANDOFF")              return telux::common::EndReasonType::CE_HANDOFF;
    return telux::common::EndReasonType::CE_UNKNOWN;
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct StateIndPld
{
    Envelope env;
};

struct RequestRegistrationInfoPld
{
    telux::tel::ImsRegistrationInfoCb cb;
};

struct RequestServiceInfoPld
{
    telux::tel::ImsServiceInfoCb cb;
};

struct RequestPdpStatusPld
{
    telux::tel::ImsPdpStatusInfoCb cb;
};

struct SetInitCbPld
{
    telux::common::InitResponseCb cb;
};

std::mutex g_fanoutMutex;
std::vector<SimulaImsServingSystemManager*> g_fanoutInstances;

}  // namespace

chart::Status
ImsServNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
ImsServReady_St(chart::Hsm*, chart::Event const*);

SimulaImsServingSystemManager::SimulaImsServingSystemManager(
  int slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("ImsServingSystemManager")
    , bridge_(bridge)
    , slotId_(slotId)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaImsServingSystemManager::~SimulaImsServingSystemManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaImsServingSystemManager::unsubscribeFromBridge_()
{
    bool lastInstance;
    {
        std::lock_guard<std::mutex> lk(g_fanoutMutex);
        g_fanoutInstances.erase(
          std::remove(g_fanoutInstances.begin(), g_fanoutInstances.end(), this),
          g_fanoutInstances.end()
        );
        lastInstance = g_fanoutInstances.empty();
    }
    if (lastInstance)
    {
        bridge_.unsubscribe_event(topics::radio::subsys_ready_ims::ind);
        bridge_.unsubscribe_event(topics::radio::ims_registration_info::ind);
        bridge_.unsubscribe_event(topics::radio::ims_service_info::ind);
        bridge_.unsubscribe_event(topics::radio::ims_pdp_status::ind);
    }
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaImsServingSystemManager::start()
{
    if (running())
        return;
    LOG_INFO("[ImsServingSystemManager] start() slot=%d", slotId_);
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(ImsServNotReady_St);

    bool firstInstance;
    {
        std::lock_guard<std::mutex> lk(g_fanoutMutex);
        firstInstance = g_fanoutInstances.empty();
        g_fanoutInstances.push_back(this);
    }
    if (firstInstance)
    {
        bridge_.subscribe_event(
          topics::radio::subsys_ready_ims::ind,
          "radio.subsys_ready_ims.ind",
          [](std::string_view topic, const Envelope& env) {
              std::lock_guard<std::mutex> lk(g_fanoutMutex);
              for (auto* inst : g_fanoutInstances)
                  inst->handleReadinessInd_(topic, env);
          }
        );
        bridge_.subscribe_event(
          topics::radio::ims_registration_info::ind,
          "radio.ims_registration_info.ind",
          [](std::string_view topic, const Envelope& env) {
              std::lock_guard<std::mutex> lk(g_fanoutMutex);
              for (auto* inst : g_fanoutInstances)
                  inst->handleRegStatusInd_(topic, env);
          }
        );
        bridge_.subscribe_event(
          topics::radio::ims_service_info::ind,
          "radio.ims_service_info.ind",
          [](std::string_view topic, const Envelope& env) {
              std::lock_guard<std::mutex> lk(g_fanoutMutex);
              for (auto* inst : g_fanoutInstances)
                  inst->handleServiceInfoInd_(topic, env);
          }
        );
        bridge_.subscribe_event(
          topics::radio::ims_pdp_status::ind,
          "radio.ims_pdp_status.ind",
          [](std::string_view topic, const Envelope& env) {
              std::lock_guard<std::mutex> lk(g_fanoutMutex);
              for (auto* inst : g_fanoutInstances)
                  inst->handlePdpStatusInd_(topic, env);
          }
        );
    }
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaImsServingSystemManager::setInitCallback(telux::common::InitResponseCb cb)
{
    if (!cb)
        return;
    if (isReadyDerived_())
    {
        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return;
    }
    auto pld = std::make_shared<SetInitCbPld>();
    pld->cb = std::move(cb);
    post_fifo({ SetInitCb_Signal, pld });
}

void
SimulaImsServingSystemManager::handleReadinessInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaImsServingSystemManager::handleRegStatusInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ RegStatusEvt_Signal, pld });
}

void
SimulaImsServingSystemManager::handleServiceInfoInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ServiceInfoEvt_Signal, pld });
}

void
SimulaImsServingSystemManager::handlePdpStatusInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ PdpStatusEvt_Signal, pld });
}

void
SimulaImsServingSystemManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::IImsServingSystemListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "ImsServingSystemManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::tel::IImsServingSystemListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaImsServingSystemManager::isReadyDerived_() const
{
    return const_cast<SimulaImsServingSystemManager*>(this)->current_state() == ImsServReady_St;
}

void
SimulaImsServingSystemManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::ServiceStatus
SimulaImsServingSystemManager::getServiceStatus()
{
    return last_status_.load();
}

// ---------------------------------------------------------------------------
// telux::tel::IImsServingSystemManager

telux::common::Status
SimulaImsServingSystemManager::requestRegistrationInfo(telux::tel::ImsRegistrationInfoCb callback)
{
    LOG_DEBUG("[ImsServingSystemManager] requestRegistrationInfo slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestRegistrationInfoPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestRegistrationInfo_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsServingSystemManager::requestServiceInfo(telux::tel::ImsServiceInfoCb callback)
{
    LOG_DEBUG("[ImsServingSystemManager] requestServiceInfo slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestServiceInfoPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestServiceInfo_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsServingSystemManager::requestPdpStatus(telux::tel::ImsPdpStatusInfoCb callback)
{
    LOG_DEBUG("[ImsServingSystemManager] requestPdpStatus slot=%d", slotId_);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestPdpStatusPld>();
    pld->cb = std::move(callback);
    post_fifo({ RequestPdpStatus_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsServingSystemManager::registerListener(
  std::weak_ptr<telux::tel::IImsServingSystemListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsServingSystemManager::deregisterListener(
  std::weak_ptr<telux::tel::IImsServingSystemListener> listener
)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::IImsServingSystemListener>& w) {
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
ImsServNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaImsServingSystemManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            LOG_INFO("[ImsServingSystemManager] -> NotReady");
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(ImsServReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            self->init_cbs_.push_back(pld->cb);
            return chart::Status::HANDLED;
        }
        case RequestRegistrationInfo_Signal:
        {
            auto pld = event_cast<RequestRegistrationInfoPld>(*e);
            LOG_WARN("[ImsServingSystemManager] stale requestRegistrationInfo -- NotReady before response");
            if (pld->cb)
                pld->cb(telux::tel::ImsRegistrationInfo{}, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case RequestServiceInfo_Signal:
        {
            auto pld = event_cast<RequestServiceInfoPld>(*e);
            LOG_WARN("[ImsServingSystemManager] stale requestServiceInfo -- NotReady before response");
            if (pld->cb)
                pld->cb(telux::tel::ImsServiceInfo{}, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case RequestPdpStatus_Signal:
        {
            auto pld = event_cast<RequestPdpStatusPld>(*e);
            LOG_WARN("[ImsServingSystemManager] stale requestPdpStatus -- NotReady before response");
            if (pld->cb)
                pld->cb(telux::tel::ImsPdpStatusInfo{}, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case RegStatusEvt_Signal:
        case ServiceInfoEvt_Signal:
        case PdpStatusEvt_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
ImsServReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaImsServingSystemManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            LOG_INFO("[ImsServingSystemManager] -> Ready");
            self->publishStatus_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            {
                std::vector<telux::common::InitResponseCb> cbs;
                cbs.swap(self->init_cbs_);
                for (auto& cb : cbs)
                    if (cb)
                        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::IImsServingSystemListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
              }
            );
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::IImsServingSystemListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            if (pld->env.data->value("status", std::string()) == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(ImsServNotReady_St);
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
            if (pld && !*pld)
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(ImsServNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case RequestRegistrationInfo_Signal:
        {
            auto pld = event_cast<RequestRegistrationInfoPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[ImsServingSystemManager] RequestRegistrationInfo_Signal send_request slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_ims_registration_info::req,
              "radio.request_ims_registration_info.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[ImsServingSystemManager] request_ims_registration_info failed corrId=%s -- timeout/error",
                        corrId.c_str()
                      );
                      cb(telux::tel::ImsRegistrationInfo{}, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  telux::tel::ImsRegistrationInfo info{};
                  info.imsRegStatus = wireToRegistrationStatus(rsp->data->value("imsRegStatus", std::string()));
                  info.rat = wireToRadioTechnology(rsp->data->value("rat", std::string()));
                  info.errorCode = rsp->data->value("errorCode", 0);
                  info.errorString = rsp->data->value("errorString", std::string());
                  LOG_DEBUG(
                    "[ImsServingSystemManager] request_ims_registration_info success corrId=%s regStatus=%d rat=%d",
                    corrId.c_str(),
                    static_cast<int>(info.imsRegStatus),
                    static_cast<int>(info.rat)
                  );
                  cb(info, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestServiceInfo_Signal:
        {
            auto pld = event_cast<RequestServiceInfoPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[ImsServingSystemManager] RequestServiceInfo_Signal send_request slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_ims_service_info::req,
              "radio.request_ims_service_info.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[ImsServingSystemManager] request_ims_service_info failed corrId=%s -- timeout/error",
                        corrId.c_str()
                      );
                      cb(telux::tel::ImsServiceInfo{}, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  telux::tel::ImsServiceInfo info{};
                  info.sms = wireToCellularServiceStatus(rsp->data->value("sms", std::string()));
                  info.voice = wireToCellularServiceStatus(rsp->data->value("voice", std::string()));
                  LOG_DEBUG(
                    "[ImsServingSystemManager] request_ims_service_info success corrId=%s sms=%d voice=%d",
                    corrId.c_str(),
                    static_cast<int>(info.sms),
                    static_cast<int>(info.voice)
                  );
                  cb(info, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestPdpStatus_Signal:
        {
            auto pld = event_cast<RequestPdpStatusPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[ImsServingSystemManager] RequestPdpStatus_Signal send_request slot=%d corrId=%s",
              self->slotId_,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_ims_pdp_status::req,
              "radio.request_ims_pdp_status.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[ImsServingSystemManager] request_ims_pdp_status failed corrId=%s -- timeout/error",
                        corrId.c_str()
                      );
                      cb(telux::tel::ImsPdpStatusInfo{}, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  telux::tel::ImsPdpStatusInfo info{};
                  info.isPdpConnected = rsp->data->value("isPdpConnected", false);
                  info.failureCode = wireToPdpFailureCode(rsp->data->value("failureCode", std::string()));
                  info.failureReason.type = wireToEndReasonType(rsp->data->value("failureReasonType", std::string()));
                  info.apnName = rsp->data->value("apnName", std::string());
                  LOG_DEBUG(
                    "[ImsServingSystemManager] request_ims_pdp_status success corrId=%s connected=%d apnName=%s",
                    corrId.c_str(),
                    info.isPdpConnected,
                    info.apnName.c_str()
                  );
                  cb(info, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RegStatusEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::tel::ImsRegistrationInfo info{};
            info.imsRegStatus = wireToRegistrationStatus(pld->env.data->value("imsRegStatus", std::string()));
            info.rat = wireToRadioTechnology(pld->env.data->value("rat", std::string()));
            info.errorCode = pld->env.data->value("errorCode", 0);
            info.errorString = pld->env.data->value("errorString", std::string());
            LOG_DEBUG(
              "[ImsServingSystemManager] ims_registration_info ind regStatus=%d rat=%d",
              static_cast<int>(info.imsRegStatus),
              static_cast<int>(info.rat)
            );
            self->broadcastToListeners_(
              [info](const std::shared_ptr<telux::tel::IImsServingSystemListener>& l) {
                  l->onImsRegStatusChange(info);
              }
            );
            return chart::Status::HANDLED;
        }

        case ServiceInfoEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::tel::ImsServiceInfo info{};
            info.sms = wireToCellularServiceStatus(pld->env.data->value("sms", std::string()));
            info.voice = wireToCellularServiceStatus(pld->env.data->value("voice", std::string()));
            LOG_DEBUG(
              "[ImsServingSystemManager] ims_service_info ind sms=%d voice=%d",
              static_cast<int>(info.sms),
              static_cast<int>(info.voice)
            );
            self->broadcastToListeners_(
              [info](const std::shared_ptr<telux::tel::IImsServingSystemListener>& l) {
                  l->onImsServiceInfoChange(info);
              }
            );
            return chart::Status::HANDLED;
        }

        case PdpStatusEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            telux::tel::ImsPdpStatusInfo info{};
            info.isPdpConnected = pld->env.data->value("isPdpConnected", false);
            info.failureCode = wireToPdpFailureCode(pld->env.data->value("failureCode", std::string()));
            info.failureReason.type = wireToEndReasonType(pld->env.data->value("failureReasonType", std::string()));
            info.apnName = pld->env.data->value("apnName", std::string());
            LOG_DEBUG(
              "[ImsServingSystemManager] ims_pdp_status ind connected=%d apnName=%s",
              info.isPdpConnected,
              info.apnName.c_str()
            );
            self->broadcastToListeners_(
              [info](const std::shared_ptr<telux::tel::IImsServingSystemListener>& l) {
                  l->onImsPdpStatusInfoChange(info);
              }
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(ImsServNotReady_St, "ImsServingSystemManager::NotReady");
CHART_NAMED_STATE(ImsServReady_St,    "ImsServingSystemManager::Ready");

}  // namespace telux::tel::simula
