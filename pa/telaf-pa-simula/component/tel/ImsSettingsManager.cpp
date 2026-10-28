// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ImsSettingsManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <array>
#include <future>
#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using namespace ImsSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

constexpr auto kRpcTimeout = std::chrono::seconds(30);
constexpr size_t kImsSettingsVoims = 1;       // ImsServiceConfigType::IMSSETTINGS_VOIMS
constexpr size_t kImsSettingsImsService = 2;  // IMSSETTINGS_IMS_SERVICE
constexpr size_t kImsSettingsSms = 3;         // IMSSETTINGS_SMS
constexpr size_t kImsSettingsRtt = 4;         // IMSSETTINGS_RTT

struct StateIndPld
{
    Envelope env;
};

struct RunOnAoPld
{
    std::function<void()> fn;
    telux::common::ResponseCallback fail;
};

struct SetInitCbPld
{
    telux::common::InitResponseCb cb;
};

}  // namespace

chart::Status
ImsSettingsNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
ImsSettingsReady_St(chart::Hsm*, chart::Event const*);

SimulaImsSettingsManager::SimulaImsSettingsManager(
  common::simula::IModemBridge& bridge, telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("ImsSettingsManager")
    , bridge_(bridge)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaImsSettingsManager::~SimulaImsSettingsManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaImsSettingsManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::radio::subsys_ready_ims_settings::ind);
    bridge_.unsubscribe_event(topics::radio::ims_service_config::ind);
    bridge_.unsubscribe_event(topics::radio::ims_sip_user_agent::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaImsSettingsManager::start()
{
    if (running())
        return;
    LOG_INFO("[ImsSettingsManager] start()");
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(ImsSettingsNotReady_St);

    bridge_.subscribe_event(
      topics::radio::subsys_ready_ims_settings::ind,
      "radio.subsys_ready_ims_settings.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::ims_service_config::ind,
      "radio.ims_service_config.ind",
      [this](std::string_view topic, const Envelope& env) { handleServiceConfigInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::ims_sip_user_agent::ind,
      "radio.ims_sip_user_agent.ind",
      [this](std::string_view topic, const Envelope& env) { handleSipUserAgentInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaImsSettingsManager::setInitCallback(telux::common::InitResponseCb cb)
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
SimulaImsSettingsManager::handleReadinessInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaImsSettingsManager::handleServiceConfigInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ServiceConfigEvt_Signal, pld });
}

void
SimulaImsSettingsManager::handleSipUserAgentInd_(std::string_view /*topic*/, const Envelope& env)
{
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ SipUserAgentEvt_Signal, pld });
}

void
SimulaImsSettingsManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::IImsSettingsListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "ImsSettingsManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::tel::IImsSettingsListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

bool
SimulaImsSettingsManager::isReadyDerived_() const
{
    return const_cast<SimulaImsSettingsManager*>(this)->current_state() == ImsSettingsReady_St;
}

telux::common::ServiceStatus
SimulaImsSettingsManager::getServiceStatus()
{
    return last_status_.load();
}


void
SimulaImsSettingsManager::doSetService_(
  int slotId, std::string service, bool enable, telux::common::ResponseCallback cb
)
{
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = slotId;
    data["service"] = service;
    data["enable"] = enable;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[ImsSettingsManager] doSetService_ slot=%d service=%s enable=%d corrId=%s",
      slotId, service.c_str(), enable, req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::set_ims_service_config::req, "radio.set_ims_service_config.rsp", req,
      [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!cb)
              return;
          if (!rsp || rsp->error)
          {
              LOG_WARN(
                "[ImsSettingsManager] set_ims_service_config failed corrId=%s -- timeout/error",
                corrId.c_str()
              );
              cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          LOG_DEBUG("[ImsSettingsManager] set_ims_service_config success corrId=%s", corrId.c_str());
          cb(telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
}

void
SimulaImsSettingsManager::doRequestService_(
  int slotId, std::string service, std::function<void(bool, telux::common::ErrorCode)> cb
)
{
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = slotId;
    data["service"] = service;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[ImsSettingsManager] doRequestService_ slot=%d service=%s corrId=%s",
      slotId, service.c_str(), req.corrId.c_str()
    );
    bridge_.send_request(
      topics::radio::request_ims_service_config::req, "radio.request_ims_service_config.rsp", req,
      [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!cb)
              return;
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN(
                "[ImsSettingsManager] request_ims_service_config failed corrId=%s -- timeout/error",
                corrId.c_str()
              );
              cb(false, telux::common::ErrorCode::OPERATION_TIMEOUT);
              return;
          }
          bool enable = rsp->data->value("enable", false);
          LOG_DEBUG(
            "[ImsSettingsManager] request_ims_service_config success corrId=%s enable=%d",
            corrId.c_str(), enable
          );
          cb(enable, telux::common::ErrorCode::SUCCESS);
      },
      kRpcTimeout
    );
}

// ---------------------------------------------------------------------------
// telux::tel::IImsSettingsManager

telux::common::Status
SimulaImsSettingsManager::toggleVonr(
  SlotId slotId, bool isEnable, telux::common::ResponseCallback callback
)
{
    LOG_DEBUG("[ImsSettingsManager] toggleVonr slot=%d isEnable=%d", static_cast<int>(slotId), isEnable);
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fail = callback;
    pld->fn = [this, slotId, isEnable, callback]() {
        doSetService_(static_cast<int>(slotId), "VONR", isEnable, callback);
    };
    post_fifo({ RunImsSettingsOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::requestVonrStatus(
  SlotId slotId, telux::tel::ImsVonrStatusCb callback
)
{
    LOG_DEBUG("[ImsSettingsManager] requestVonrStatus slot=%d", static_cast<int>(slotId));
    if (!isReadyDerived_())
    {
        if (callback)
            callback(slotId, false, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fail = [slotId, callback](telux::common::ErrorCode err) {
        if (callback)
            callback(slotId, false, err);
    };
    pld->fn = [this, slotId, callback]() {
        doRequestService_(static_cast<int>(slotId), "VONR", [slotId, callback](bool enable, telux::common::ErrorCode err) {
            if (callback)
                callback(slotId, enable, err);
        });
    };
    post_fifo({ RunImsSettingsOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::setServiceConfig(
  SlotId slotId, telux::tel::ImsServiceConfig config,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG("[ImsSettingsManager] setServiceConfig slot=%d", static_cast<int>(slotId));
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto ops = std::make_shared<std::vector<std::pair<std::string, bool>>>();
    if (config.configValidityMask.test(kImsSettingsImsService))
        ops->emplace_back("IMS_REG", config.imsServiceEnabled);
    if (config.configValidityMask.test(kImsSettingsSms))
        ops->emplace_back("SMS", config.smsEnabled);
    if (config.configValidityMask.test(kImsSettingsVoims))
        ops->emplace_back("VOIP", config.voImsEnabled);
    if (config.configValidityMask.test(kImsSettingsRtt))
        ops->emplace_back("RTT", config.rttEnabled);

    auto pld = std::make_shared<RunOnAoPld>();
    pld->fail = callback;
    pld->fn = [this, slotId, ops, callback]() {
        auto step = std::make_shared<std::function<void(size_t)>>();
        // Only the pending response owns the continuation; it must not own itself.
        std::weak_ptr<std::function<void(size_t)>> weakStep = step;
        *step = [this, slotId, ops, callback, weakStep](size_t idx) {
            if (idx >= ops->size())
            {
                if (callback)
                    callback(telux::common::ErrorCode::SUCCESS);
                return;
            }
            const auto& [service, enable] = (*ops)[idx];
            doSetService_(
              static_cast<int>(slotId), service, enable,
              [step = weakStep.lock(), idx, callback](telux::common::ErrorCode err) {
                  if (err != telux::common::ErrorCode::SUCCESS)
                  {
                      if (callback)
                          callback(err);
                      return;
                  }
                  (*step)(idx + 1);
              }
            );
        };
        (*step)(0);
    };
    post_fifo({ RunImsSettingsOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::requestServiceConfig(
  SlotId slotId, telux::tel::ImsServiceConfigCb callback
)
{
    LOG_DEBUG("[ImsSettingsManager] requestServiceConfig slot=%d", static_cast<int>(slotId));
    if (!isReadyDerived_())
    {
        if (callback)
            callback(slotId, telux::tel::ImsServiceConfig{}, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fail = [slotId, callback](telux::common::ErrorCode err) {
        if (callback)
            callback(slotId, telux::tel::ImsServiceConfig{}, err);
    };
    pld->fn = [this, slotId, callback]() {
        static const std::array<std::pair<const char*, size_t>, 4> kFields{ {
          { "IMS_REG", kImsSettingsImsService },
          { "SMS", kImsSettingsSms },
          { "VOIP", kImsSettingsVoims },
          { "RTT", kImsSettingsRtt },
        } };
        auto config = std::make_shared<telux::tel::ImsServiceConfig>();
        auto step = std::make_shared<std::function<void(size_t)>>();
        std::weak_ptr<std::function<void(size_t)>> weakStep = step;
        *step = [this, slotId, config, callback, weakStep](size_t idx) {
            if (idx >= kFields.size())
            {
                if (callback)
                    callback(slotId, *config, telux::common::ErrorCode::SUCCESS);
                return;
            }
            const auto& [service, bit] = kFields[idx];
            doRequestService_(
              static_cast<int>(slotId), service,
              [config, bit, step = weakStep.lock(), idx, callback, slotId](bool enable, telux::common::ErrorCode err) {
                  if (err != telux::common::ErrorCode::SUCCESS)
                  {
                      if (callback)
                          callback(slotId, telux::tel::ImsServiceConfig{}, err);
                      return;
                  }
                  config->configValidityMask.set(bit);
                  if (bit == kImsSettingsImsService)
                      config->imsServiceEnabled = enable;
                  else if (bit == kImsSettingsSms)
                      config->smsEnabled = enable;
                  else if (bit == kImsSettingsVoims)
                      config->voImsEnabled = enable;
                  else if (bit == kImsSettingsRtt)
                      config->rttEnabled = enable;
                  (*step)(idx + 1);
              }
            );
        };
        (*step)(0);
    };
    post_fifo({ RunImsSettingsOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::setSipUserAgent(
  SlotId slotId, std::string userAgent, telux::common::ResponseCallback callback
)
{
    LOG_DEBUG("[ImsSettingsManager] setSipUserAgent slot=%d", static_cast<int>(slotId));
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fail = callback;
    pld->fn = [this, slotId, userAgent, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["slot"] = static_cast<int>(slotId);
        data["userAgent"] = userAgent;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        LOG_DEBUG(
          "[ImsSettingsManager] setSipUserAgent send_request slot=%d corrId=%s userAgent=%s",
          static_cast<int>(slotId), req.corrId.c_str(), userAgent.c_str()
        );
        bridge_.send_request(
          topics::radio::set_ims_sip_user_agent::req, "radio.set_ims_sip_user_agent.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error)
              {
                  LOG_WARN(
                    "[ImsSettingsManager] set_ims_sip_user_agent failed corrId=%s -- timeout/error",
                    corrId.c_str()
                  );
                  callback(telux::common::ErrorCode::OPERATION_TIMEOUT);
                  return;
              }
              LOG_DEBUG("[ImsSettingsManager] set_ims_sip_user_agent success corrId=%s", corrId.c_str());
              callback(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunImsSettingsOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::requestSipUserAgent(
  SlotId slotId, telux::tel::ImsSipUserAgentConfigCb callback
)
{
    LOG_DEBUG("[ImsSettingsManager] requestSipUserAgent slot=%d", static_cast<int>(slotId));
    if (!isReadyDerived_())
    {
        if (callback)
            callback(slotId, std::string(), telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fail = [slotId, callback](telux::common::ErrorCode err) {
        if (callback)
            callback(slotId, std::string(), err);
    };
    pld->fn = [this, slotId, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["slot"] = static_cast<int>(slotId);
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        LOG_DEBUG(
          "[ImsSettingsManager] requestSipUserAgent send_request slot=%d corrId=%s",
          static_cast<int>(slotId), req.corrId.c_str()
        );
        bridge_.send_request(
          topics::radio::request_ims_sip_user_agent::req, "radio.request_ims_sip_user_agent.rsp", req,
          [slotId, callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error || !rsp->data)
              {
                  LOG_WARN(
                    "[ImsSettingsManager] request_ims_sip_user_agent failed corrId=%s -- timeout/error",
                    corrId.c_str()
                  );
                  callback(slotId, std::string(), telux::common::ErrorCode::OPERATION_TIMEOUT);
                  return;
              }
              std::string userAgent = rsp->data->value("userAgent", std::string());
              LOG_DEBUG(
                "[ImsSettingsManager] request_ims_sip_user_agent success corrId=%s userAgent=%s",
                corrId.c_str(), userAgent.c_str()
              );
              callback(slotId, userAgent, telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunImsSettingsOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::registerListener(std::weak_ptr<telux::tel::IImsSettingsListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaImsSettingsManager::deregisterListener(std::weak_ptr<telux::tel::IImsSettingsListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::IImsSettingsListener>& w) {
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
ImsSettingsNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaImsSettingsManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            LOG_INFO("[ImsSettingsManager] -> NotReady");
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(ImsSettingsReady_St);
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
        case RunImsSettingsOp_Signal:
        {
            auto pld = event_cast<RunOnAoPld>(*e);
            LOG_WARN("[ImsSettingsManager] stale RunImsSettingsOp_Signal -- NotReady before response");
            if (pld->fail)
                pld->fail(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        case ServiceConfigEvt_Signal:
        case SipUserAgentEvt_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
ImsSettingsReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaImsSettingsManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            LOG_INFO("[ImsSettingsManager] -> Ready");
            self->last_status_.store(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            {
                std::vector<telux::common::InitResponseCb> cbs;
                cbs.swap(self->init_cbs_);
                for (auto& cb : cbs)
                    if (cb)
                        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::IImsSettingsListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
              }
            );
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->last_status_.store(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::IImsSettingsListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            if (pld->cb)
                pld->cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            return chart::Status::HANDLED;
        }

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "UNAVAILABLE")
                return self->to(ImsSettingsNotReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
                return self->to(ImsSettingsNotReady_St);
            return chart::Status::HANDLED;
        }

        case RunImsSettingsOp_Signal:
        {
            auto pld = event_cast<RunOnAoPld>(*e);
            if (pld->fn)
                pld->fn();
            return chart::Status::HANDLED;
        }

        case ServiceConfigEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int slot = pld->env.data->value("slot", 0);
            std::string service = pld->env.data->value("service", std::string());
            bool enable = pld->env.data->value("enable", false);

            if (service == "VONR")
            {
                LOG_DEBUG(
                  "[ImsSettingsManager] ServiceConfigEvt_Signal service=VONR -- no "
                  "ImsServiceConfig field maps to VoNR, not broadcasting onImsServiceConfigsChange"
                );
                return chart::Status::HANDLED;
            }

            telux::tel::ImsServiceConfig config{};
            if (service == "IMS_REG")
            {
                config.configValidityMask.set(kImsSettingsImsService);
                config.imsServiceEnabled = enable;
            }
            else if (service == "SMS")
            {
                config.configValidityMask.set(kImsSettingsSms);
                config.smsEnabled = enable;
            }
            else if (service == "VOIP")
            {
                config.configValidityMask.set(kImsSettingsVoims);
                config.voImsEnabled = enable;
            }
            else if (service == "RTT")
            {
                config.configValidityMask.set(kImsSettingsRtt);
                config.rttEnabled = enable;
            }
            else
            {
                LOG_WARN(
                  "[ImsSettingsManager] ServiceConfigEvt_Signal unrecognized service \"%s\" -- dropping",
                  service.c_str()
                );
                return chart::Status::HANDLED;
            }

            auto slotId = static_cast<SlotId>(slot);
            LOG_DEBUG(
              "[ImsSettingsManager] ServiceConfigEvt_Signal slot=%d service=%s enable=%d",
              slot, service.c_str(), enable
            );
            self->broadcastToListeners_(
              [slotId, config](const std::shared_ptr<telux::tel::IImsSettingsListener>& l) {
                  l->onImsServiceConfigsChange(slotId, config);
              }
            );
            return chart::Status::HANDLED;
        }

        case SipUserAgentEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            std::string userAgent = pld->env.data->value("userAgent", std::string());
            LOG_DEBUG(
              "[ImsSettingsManager] SipUserAgentEvt_Signal userAgent=%s", userAgent.c_str()
            );
            self->broadcastToListeners_(
              [userAgent](const std::shared_ptr<telux::tel::IImsSettingsListener>& l) {
                  l->onImsSipUserAgentChange(DEFAULT_SLOT_ID, userAgent);
              }
            );
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(ImsSettingsNotReady_St, "ImsSettingsManager::NotReady");
CHART_NAMED_STATE(ImsSettingsReady_St,    "ImsSettingsManager::Ready");

}  // namespace telux::tel::simula
