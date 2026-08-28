// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "CallManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Call.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <algorithm>
#include <future>
#include <nlohmann/json.hpp>
#include <utility>

namespace telux::tel::simula {

using namespace EcallSignals;

namespace {

using common::simula::Envelope;
using common::simula::event_cast;

constexpr auto kRpcTimeout = std::chrono::seconds(30);

std::string
ecallModeToWire(telux::tel::ECallMode mode)
{
    switch (mode)
    {
        case telux::tel::ECallMode::NORMAL:      return "NORMAL";
        case telux::tel::ECallMode::ECALL_ONLY:  return "ECALL_ONLY";
        default:                                 return "NONE";
    }
}

std::string
hlapTimerTypeToWire(telux::tel::HlapTimerType type)
{
    switch (type)
    {
        case telux::tel::HlapTimerType::T2_TIMER:  return "T2";
        case telux::tel::HlapTimerType::T5_TIMER:  return "T5";
        case telux::tel::HlapTimerType::T6_TIMER:  return "T6";
        case telux::tel::HlapTimerType::T7_TIMER:  return "T7";
        case telux::tel::HlapTimerType::T9_TIMER:  return "T9";
        case telux::tel::HlapTimerType::T10_TIMER: return "T10";
        default:                                    return "UNKNOWN_TIMER";
    }
}

telux::tel::HlapTimerStatus
wireToHlapTimerStatus(const std::string& s)
{
    if (s == "ACTIVE")   return telux::tel::HlapTimerStatus::ACTIVE;
    if (s == "INACTIVE") return telux::tel::HlapTimerStatus::INACTIVE;
    return telux::tel::HlapTimerStatus::UNKNOWN;
}

telux::tel::HlapTimerEvent
wireToHlapTimerEvent(const std::string& s)
{
    if (s == "STARTED") return telux::tel::HlapTimerEvent::STARTED;
    if (s == "STOPPED") return telux::tel::HlapTimerEvent::STOPPED;
    if (s == "EXPIRED") return telux::tel::HlapTimerEvent::EXPIRED;
    if (s == "RESUMED") return telux::tel::HlapTimerEvent::RESUMED;
    if (s == "UNCHANGED") return telux::tel::HlapTimerEvent::UNCHANGED;
    return telux::tel::HlapTimerEvent::UNKNOWN;
}

telux::tel::ECallMsdTransmissionStatus
wireToMsdTransmissionStatus(const std::string& s)
{
    if (s == "MSD_TRANSMISSION_SUCCESS" || s == "OUTBAND_MSD_TRANSMISSION_SUCCESS")
        return telux::tel::ECallMsdTransmissionStatus::SUCCESS;
    if (s == "MSD_TRANSMISSION_FAILURE" || s == "OUTBAND_MSD_TRANSMISSION_FAILURE")
        return telux::tel::ECallMsdTransmissionStatus::FAILURE;
    if (s == "MSD_TRANSMISSION_STARTED" || s == "OUTBAND_MSD_TRANSMISSION_STARTED")
        return telux::tel::ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED;
    if (s == "START_RECEIVED")
        return telux::tel::ECallMsdTransmissionStatus::START_RECEIVED;
    return telux::tel::ECallMsdTransmissionStatus::SUCCESS;
}

telux::tel::CallState
wireToCallState(const std::string& s)
{
    if (s == "DIALING")  return telux::tel::CallState::CALL_DIALING;
    if (s == "ALERTING") return telux::tel::CallState::CALL_ALERTING;
    if (s == "ACTIVE")   return telux::tel::CallState::CALL_ACTIVE;
    if (s == "INCOMING") return telux::tel::CallState::CALL_INCOMING;
    if (s == "WAITING")  return telux::tel::CallState::CALL_WAITING;
    if (s == "HOLD")     return telux::tel::CallState::CALL_ON_HOLD;
    if (s == "ENDED")    return telux::tel::CallState::CALL_ENDED;
    return telux::tel::CallState::CALL_IDLE;
}

telux::tel::CallDirection
wireToCallDirection(const std::string& s)
{
    return s == "MT" ? telux::tel::CallDirection::INCOMING : telux::tel::CallDirection::OUTGOING;
}

telux::tel::ReasonType
wireToRedialReason(const std::string& s)
{
    if (s == "CALL_ORIG_FAILURE")    return telux::tel::ReasonType::CALL_ORIG_FAILURE;
    if (s == "CALL_DROP")            return telux::tel::ReasonType::CALL_DROP;
    if (s == "MAX_REDIAL_ATTEMPTED") return telux::tel::ReasonType::MAX_REDIAL_ATTEMPTED;
    if (s == "CALL_CONNECTED")       return telux::tel::ReasonType::CALL_CONNECTED;
    return telux::tel::ReasonType::NONE;
}

nlohmann::json
ecallConfigToWire(const telux::tel::EcallConfig& config)
{
    nlohmann::json data = nlohmann::json::object();
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_MUTE_RX_AUDIO))
    {
        data["isMuteRxAudioValid"] = true;
        data["muteRxAudio"] = config.muteRxAudio;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_NUM_TYPE))
    {
        data["isNumTypeValid"] = true;
        data["numType"] = config.numType == telux::tel::ECallNumType::OVERRIDDEN
                             ? "OVERRIDDEN"
                             : "DEFAULT";
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_OVERRIDDEN_NUM))
    {
        data["isOverriddenNumValid"] = true;
        data["overriddenNum"] = config.overriddenNum;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_USE_CANNED_MSD))
    {
        data["isUseCannedMsdValid"] = true;
        data["useCannedMsd"] = config.useCannedMsd;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_GNSS_UPDATE_INTERVAL))
    {
        data["isGnssUpdateIntervalValid"] = true;
        data["gnssUpdateInterval"] = config.gnssUpdateInterval;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_T2_TIMER))
    {
        data["isT2TimerValid"] = true;
        data["t2Timer"] = config.t2Timer;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_T7_TIMER))
    {
        data["isT7TimerValid"] = true;
        data["t7Timer"] = config.t7Timer;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_T9_TIMER))
    {
        data["isT9TimerValid"] = true;
        data["t9Timer"] = config.t9Timer;
    }
    if (config.configValidityMask.test(telux::tel::ECALL_CONFIG_MSD_VERSION))
    {
        data["isMsdVersionValid"] = true;
        data["msdVersion"] = config.msdVersion;
    }
    return data;
}

telux::tel::EcallConfig
wireToEcallConfig(const nlohmann::json& data)
{
    telux::tel::EcallConfig config{};
    if (data.value("isMuteRxAudioValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_MUTE_RX_AUDIO);
        config.muteRxAudio = data.value("muteRxAudio", false);
    }
    if (data.value("isNumTypeValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_NUM_TYPE);
        config.numType = data.value("numType", std::string()) == "OVERRIDDEN"
                            ? telux::tel::ECallNumType::OVERRIDDEN
                            : telux::tel::ECallNumType::DEFAULT;
    }
    if (data.value("isOverriddenNumValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_OVERRIDDEN_NUM);
        config.overriddenNum = data.value("overriddenNum", std::string());
    }
    if (data.value("isUseCannedMsdValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_USE_CANNED_MSD);
        config.useCannedMsd = data.value("useCannedMsd", false);
    }
    if (data.value("isGnssUpdateIntervalValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_GNSS_UPDATE_INTERVAL);
        config.gnssUpdateInterval = data.value("gnssUpdateInterval", 0);
    }
    if (data.value("isT2TimerValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_T2_TIMER);
        config.t2Timer = data.value("t2Timer", 0);
    }
    if (data.value("isT7TimerValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_T7_TIMER);
        config.t7Timer = data.value("t7Timer", 0);
    }
    if (data.value("isT9TimerValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_T9_TIMER);
        config.t9Timer = data.value("t9Timer", 0);
    }
    if (data.value("isMsdVersionValid", false))
    {
        config.configValidityMask.set(telux::tel::ECALL_CONFIG_MSD_VERSION);
        config.msdVersion = static_cast<uint8_t>(data.value("msdVersion", 0));
    }
    return config;
}

struct RunOnAoPld
{
    std::function<void()> fn;
};

struct StateIndPld
{
    Envelope env;
};

struct SetInitCbPld
{
    telux::common::InitResponseCb cb;
};

template <typename T>
std::optional<T>
waitForEcallOp(std::future<T>& future)
{
    if (future.wait_for(kRpcTimeout + std::chrono::seconds(1)) != std::future_status::ready)
    {
        LOG_WARN("[CallManager] waitForEcallOp: timed out waiting for the AO response");
        return std::nullopt;
    }
    return future.get();
}

}  // namespace

chart::Status
CallMgrNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
CallMgrReady_St(chart::Hsm*, chart::Event const*);

// ---------------------------------------------------------------------------
// Construction / lifecycle

SimulaCallManager::SimulaCallManager(
  common::simula::IModemBridge& bridge, telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("CallManager")
    , bridge_(bridge)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaCallManager::~SimulaCallManager()
{
    unsubscribeFromBridge_();
    stop();
}

void
SimulaCallManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::ecall::subsys_ready_call::ind);
    bridge_.unsubscribe_event(topics::ecall::call_state::ind);
    bridge_.unsubscribe_event(topics::ecall::msd_transmission_status::ind);
    bridge_.unsubscribe_event(topics::ecall::hlap_timer_event::ind);
    bridge_.unsubscribe_event(topics::ecall::redial::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaCallManager::start()
{
    if (running())
        return;
    LOG_INFO("[CallManager] start()");
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(CallMgrNotReady_St);

    bridge_.subscribe_event(
      topics::ecall::subsys_ready_call::ind, "ecall.subsys_ready_call.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::ecall::call_state::ind, "ecall.call_state.ind",
      [this](std::string_view topic, const Envelope& env) { handleCallStateInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::ecall::msd_transmission_status::ind, "ecall.msd_transmission_status.ind",
      [this](std::string_view topic, const Envelope& env) { handleMsdStatusInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::ecall::hlap_timer_event::ind, "ecall.hlap_timer_event.ind",
      [this](std::string_view topic, const Envelope& env) { handleHlapTimerEventInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::ecall::redial::ind, "ecall.redial.ind",
      [this](std::string_view topic, const Envelope& env) { handleRedialInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

bool
SimulaCallManager::isReadyDerived_() const
{
    return const_cast<SimulaCallManager*>(this)->current_state() == CallMgrReady_St;
}

telux::common::ServiceStatus
SimulaCallManager::getServiceStatus()
{
    return isReadyDerived_() ? telux::common::ServiceStatus::SERVICE_AVAILABLE
                              : telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

void
SimulaCallManager::setInitCallback(telux::common::InitResponseCb cb)
{
    LOG_DEBUG("[CallManager] setInitCallback called hasCb=%d", cb ? 1 : 0);
    if (!cb)
        return;
    ensureStarted_();
    // Already Ready: satisfy synchronously.
    if (isReadyDerived_())
    {
        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return;
    }
    auto pld = std::make_shared<SetInitCbPld>();
    pld->cb = std::move(cb);
    post_fifo({ SetInitCb_Signal, pld });
}

// ---------------------------------------------------------------------------
// Indication handlers -- post into the AO fifo; state handlers do the work.

void
SimulaCallManager::handleReadinessInd_(std::string_view, const Envelope& env)
{
    LOG_DEBUG("[CallManager] handleReadinessInd_ received");
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}

void
SimulaCallManager::handleCallStateInd_(std::string_view, const Envelope& env)
{
    LOG_DEBUG("[CallManager] handleCallStateInd_ received");
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ CallStateEvt_Signal, pld });
}

void
SimulaCallManager::handleMsdStatusInd_(std::string_view, const Envelope& env)
{
    LOG_DEBUG("[CallManager] handleMsdStatusInd_ received");
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ MsdStatusEvt_Signal, pld });
}

void
SimulaCallManager::handleHlapTimerEventInd_(std::string_view, const Envelope& env)
{
    LOG_DEBUG("[CallManager] handleHlapTimerEventInd_ received");
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ HlapTimerEventEvt_Signal, pld });
}

void
SimulaCallManager::handleRedialInd_(std::string_view, const Envelope& env)
{
    LOG_DEBUG("[CallManager] handleRedialInd_ received");
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ RedialEvt_Signal, pld });
}

void
SimulaCallManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::ICallListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "CallManager::broadcastToListeners_";
    {
        std::lock_guard<std::mutex> lk(listeners_mutex_);
        for (auto& l : listeners_)
            task->listeners.push_back(l);
    }
    if (task->listeners.empty())
        return;
    task->invoker = [invoke](std::shared_ptr<void> raw) {
        invoke(std::static_pointer_cast<telux::tel::ICallListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

std::shared_ptr<SimulaCall>
SimulaCallManager::findOrCreateCall_(
  int phoneId, int callIndex, telux::tel::CallDirection direction,
  const std::string& remotePartyNumber
)
{
    std::shared_ptr<SimulaCall> stale;
    std::shared_ptr<SimulaCall> call;
    {
        std::lock_guard<std::mutex> lk(calls_mutex_);
        auto it = calls_.find(phoneId);
        if (it != calls_.end() && it->second->getCallIndex() == callIndex)
        {
            LOG_DEBUG(
              "[CallManager] findOrCreateCall_ phoneId=%d callIndex=%d: cache hit", phoneId, callIndex
            );
            return it->second;
        }
        if (it != calls_.end() && it->second->getCallState() != telux::tel::CallState::CALL_ENDED)
            stale = it->second;
        call = std::make_shared<SimulaCall>(phoneId, callIndex, direction, remotePartyNumber, bridge_);
        calls_[phoneId] = call;
        LOG_DEBUG(
          "[CallManager] findOrCreateCall_ phoneId=%d callIndex=%d: created new call", phoneId,
          callIndex
        );
    }
    if (stale)
    {
        LOG_WARN(
          "[CallManager] findOrCreateCall_ phoneId=%d: replacing still-active callIndex=%d with "
          "new callIndex=%d -- ending the stale call first",
          phoneId, stale->getCallIndex(), callIndex
        );
        stale->setCallState(telux::tel::CallState::CALL_ENDED);
        broadcastToListeners_([stale](const std::shared_ptr<telux::tel::ICallListener>& l) {
            l->onCallInfoChange(stale);
        });
    }
    return call;
}

// ---------------------------------------------------------------------------
// makeECall overloads -- all funnel into doMakeECall_ via RunEcallOp_Signal.

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const telux::tel::ECallMsdData&, int, int,
  std::shared_ptr<telux::tel::IMakeCallCallback> callback
)
{
    LOG_DEBUG("[CallManager] makeECall(msd,category,variant) phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback->makeCallResponse(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, callback]() {
        doMakeECall_(
          phoneId, "AUTOMATIC", true, "", "",
          [callback](telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> call) {
              if (callback)
                  callback->makeCallResponse(error, call);
          }
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const std::string dialNumber, const telux::tel::ECallMsdData&, int,
  std::shared_ptr<telux::tel::IMakeCallCallback> callback
)
{
    LOG_DEBUG("[CallManager] makeECall(dialNumber,msd,category) phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback->makeCallResponse(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, dialNumber, callback]() {
        doMakeECall_(
          phoneId, "AUTOMATIC", true, dialNumber, "",
          [callback](telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> call) {
              if (callback)
                  callback->makeCallResponse(error, call);
          }
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const std::string dialNumber, const std::vector<uint8_t>& msdPdu,
  telux::tel::CustomSipHeader, telux::tel::MakeCallCallback callback
)
{
    LOG_DEBUG("[CallManager] makeECall(dialNumber,msdPdu,header) [PRIVATE] phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    bool isMsdTransmitted = !msdPdu.empty();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, dialNumber, isMsdTransmitted, callback]() {
        doMakeECall_(phoneId, "PRIVATE", isMsdTransmitted, "", dialNumber, callback);
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const std::vector<uint8_t>& msdPdu, int, int, telux::tel::MakeCallCallback callback
)
{
    LOG_DEBUG("[CallManager] makeECall(msdPdu,category,variant) phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    bool isMsdTransmitted = !msdPdu.empty();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, isMsdTransmitted, callback]() {
        doMakeECall_(phoneId, "AUTOMATIC", isMsdTransmitted, "", "", callback);
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const std::string dialNumber, const std::vector<uint8_t>& msdPdu, int,
  telux::tel::MakeCallCallback callback
)
{
    LOG_DEBUG("[CallManager] makeECall(dialNumber,msdPdu,category) phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    bool isMsdTransmitted = !msdPdu.empty();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, dialNumber, isMsdTransmitted, callback]() {
        doMakeECall_(phoneId, "AUTOMATIC", isMsdTransmitted, dialNumber, "", callback);
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(int phoneId, int, int, telux::tel::MakeCallCallback callback)
{
    LOG_DEBUG("[CallManager] makeECall(category,variant) [no MSD] phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, callback]() {
        doMakeECall_(phoneId, "AUTOMATIC", false, "", "", callback);
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const std::string dialNumber, int, telux::tel::MakeCallCallback callback
)
{
    LOG_DEBUG("[CallManager] makeECall(dialNumber,category) [no MSD] phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, dialNumber, callback]() {
        doMakeECall_(phoneId, "AUTOMATIC", false, dialNumber, "", callback);
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeECall(
  int phoneId, const std::string dialNumber, const std::vector<uint8_t>& msdPdu,
  telux::tel::MakeCallCallback callback
)
{
    LOG_DEBUG("[CallManager] makeECall(dialNumber,msdPdu) [PRIVATE, no header] phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, nullptr);
        return telux::common::Status::NOTREADY;
    }
    bool isMsdTransmitted = !msdPdu.empty();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, dialNumber, isMsdTransmitted, callback]() {
        doMakeECall_(phoneId, "PRIVATE", isMsdTransmitted, "", dialNumber, callback);
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

void
SimulaCallManager::doMakeECall_(
  int phoneId, const std::string& type, bool isMsdTransmitted,
  const std::string& remotePartyNumber, const std::string& dialNumber,
  std::function<void(telux::common::ErrorCode, std::shared_ptr<telux::tel::ICall>)> notify
)
{
    nlohmann::json data = nlohmann::json::object();
    data["phoneId"] = phoneId;
    data["type"] = type;
    data["isMsdTransmitted"] = isMsdTransmitted;
    if (!remotePartyNumber.empty())
        data["remotePartyNumber"] = remotePartyNumber;
    if (!dialNumber.empty())
        data["dialNumber"] = dialNumber;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    LOG_DEBUG(
      "[CallManager] doMakeECall_ phoneId=%d type=%s corrId=%s", phoneId, type.c_str(),
      req.corrId.c_str()
    );
    bridge_.send_request(
      topics::ecall::start_ecall::req, "ecall.start_ecall.rsp", req,
      [this, phoneId, notify, corrId = req.corrId](std::optional<Envelope> rsp) {
          if (!rsp || rsp->error || !rsp->data)
          {
              LOG_WARN("[CallManager] start_ecall response failed corrId=%s", corrId.c_str());
              telux::common::ErrorCode error = rsp && rsp->error
                                                  ? common::simula::parseErrorCode(
                                                      rsp->error->value("code", std::string())
                                                    )
                                                  : telux::common::ErrorCode::OPERATION_TIMEOUT;
              if (notify)
                  notify(error, nullptr);
              return;
          }
          const auto& call = rsp->data->at("call");
          auto simCall = findOrCreateCall_(
            phoneId, call.value("callIndex", 0), wireToCallDirection(call.value("direction", std::string())),
            call.value("remotePartyNumber", std::string())
          );
          simCall->setCallState(wireToCallState(call.value("callState", std::string())));
          LOG_DEBUG("[CallManager] start_ecall response ok corrId=%s", corrId.c_str());
          if (notify)
              notify(telux::common::ErrorCode::SUCCESS, simCall);
      },
      kRpcTimeout
    );
}

// ---------------------------------------------------------------------------
// updateECallMsd / requestECallHlapTimerStatus / getInProgressCalls

telux::common::Status
SimulaCallManager::updateECallMsd(
  int phoneId, const telux::tel::ECallMsdData&,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    LOG_DEBUG("[CallManager] updateECallMsd(struct) phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback->commandResponse(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["phoneId"] = phoneId;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::send_msd::req, "ecall.send_msd.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error)
              {
                  LOG_WARN("[CallManager] send_msd response failed corrId=%s", corrId.c_str());
                  callback->commandResponse(telux::common::ErrorCode::OPERATION_TIMEOUT);
                  return;
              }
              callback->commandResponse(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::updateECallMsd(
  int phoneId, const std::vector<uint8_t>&, telux::common::ResponseCallback callback
)
{
    LOG_DEBUG("[CallManager] updateECallMsd(pdu) phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["phoneId"] = phoneId;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::send_msd::req, "ecall.send_msd.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error)
              {
                  LOG_WARN("[CallManager] send_msd response failed corrId=%s", corrId.c_str());
                  callback(telux::common::ErrorCode::OPERATION_TIMEOUT);
                  return;
              }
              callback(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::requestECallHlapTimerStatus(
  int phoneId, telux::tel::ECallHlapTimerStatusCallback callback
)
{
    LOG_DEBUG("[CallManager] requestECallHlapTimerStatus phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, phoneId, {});
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["phoneId"] = phoneId;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::request_hlap_timer_status::req, "ecall.request_hlap_timer_status.rsp", req,
          [callback, phoneId, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error || !rsp->data)
              {
                  LOG_WARN(
                    "[CallManager] request_hlap_timer_status response failed corrId=%s",
                    corrId.c_str()
                  );
                  callback(telux::common::ErrorCode::OPERATION_TIMEOUT, phoneId, {});
                  return;
              }
              const auto& d = *rsp->data;
              telux::tel::ECallHlapTimerStatus status{};
              status.t2 = wireToHlapTimerStatus(d.value("t2", std::string()));
              status.t5 = wireToHlapTimerStatus(d.value("t5", std::string()));
              status.t6 = wireToHlapTimerStatus(d.value("t6", std::string()));
              status.t7 = wireToHlapTimerStatus(d.value("t7", std::string()));
              status.t9 = wireToHlapTimerStatus(d.value("t9", std::string()));
              status.t10 = wireToHlapTimerStatus(d.value("t10", std::string()));
              callback(telux::common::ErrorCode::SUCCESS, phoneId, status);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

std::vector<std::shared_ptr<telux::tel::ICall>>
SimulaCallManager::getInProgressCalls()
{
    // Pure local cache read -- no RPC.
    std::lock_guard<std::mutex> lk(calls_mutex_);
    std::vector<std::shared_ptr<telux::tel::ICall>> out;
    out.reserve(calls_.size());
    for (auto& [phoneId, call] : calls_)
        out.push_back(call);
    return out;
}

// ---------------------------------------------------------------------------
// requestNetworkDeregistration / updateEcallHlapTimer / requestEcallHlapTimer

telux::common::Status
SimulaCallManager::requestNetworkDeregistration(int phoneId, telux::common::ResponseCallback callback)
{
    LOG_DEBUG("[CallManager] requestNetworkDeregistration phoneId=%d", phoneId);
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["phoneId"] = phoneId;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::request_network_deregistration::req,
          "ecall.request_network_deregistration.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error)
              {
                  LOG_WARN(
                    "[CallManager] request_network_deregistration failed corrId=%s", corrId.c_str()
                  );
                  callback(
                    rsp && rsp->error
                      ? common::simula::parseErrorCode(rsp->error->value("code", std::string()))
                      : telux::common::ErrorCode::OPERATION_TIMEOUT
                  );
                  return;
              }
              callback(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::updateEcallHlapTimer(
  int phoneId, telux::tel::HlapTimerType type, uint32_t timeDuration,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[CallManager] updateEcallHlapTimer phoneId=%d type=%d duration=%u", phoneId,
      static_cast<int>(type), timeDuration
    );
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, type, timeDuration, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["phoneId"] = phoneId;
        data["type"] = hlapTimerTypeToWire(type);
        data["timeDuration"] = timeDuration;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::update_hlap_timer::req, "ecall.update_hlap_timer.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error)
              {
                  LOG_WARN("[CallManager] update_hlap_timer failed corrId=%s", corrId.c_str());
                  callback(
                    rsp && rsp->error
                      ? common::simula::parseErrorCode(rsp->error->value("code", std::string()))
                      : telux::common::ErrorCode::OPERATION_TIMEOUT
                  );
                  return;
              }
              callback(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::requestEcallHlapTimer(
  int phoneId, telux::tel::HlapTimerType type, telux::tel::ECallHlapTimerCallback callback
)
{
    LOG_DEBUG("[CallManager] requestEcallHlapTimer phoneId=%d type=%d", phoneId, static_cast<int>(type));
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE, 0);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, phoneId, type, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["phoneId"] = phoneId;
        data["type"] = hlapTimerTypeToWire(type);
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::request_hlap_timer::req, "ecall.request_hlap_timer.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!callback)
                  return;
              if (!rsp || rsp->error || !rsp->data)
              {
                  LOG_WARN("[CallManager] request_hlap_timer failed corrId=%s", corrId.c_str());
                  callback(
                    rsp && rsp->error
                      ? common::simula::parseErrorCode(rsp->error->value("code", std::string()))
                      : telux::common::ErrorCode::OPERATION_TIMEOUT,
                    0
                  );
                  return;
              }
              callback(telux::common::ErrorCode::SUCCESS, rsp->data->value("timeDuration", 0));
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::setECallConfig(telux::tel::EcallConfig config)
{
    LOG_DEBUG("[CallManager] setECallConfig");
    ensureStarted_();
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto promise = std::make_shared<std::promise<telux::common::ErrorCode>>();
    auto future = promise->get_future();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, config, promise]() {
        auto req = common::simula::makeRequestEnvelope(
          bridge_.currentPaId(), ecallConfigToWire(config)
        );
        bridge_.send_request(
          topics::ecall::set_config::req, "ecall.set_config.rsp", req,
          [promise, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!rsp || rsp->error)
              {
                  LOG_WARN("[CallManager] set_config failed corrId=%s", corrId.c_str());
                  promise->set_value(telux::common::ErrorCode::OPERATION_TIMEOUT);
                  return;
              }
              promise->set_value(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    auto result = waitForEcallOp(future);
    if (!result)
        return telux::common::Status::FAILED;
    return *result == telux::common::ErrorCode::SUCCESS ? telux::common::Status::SUCCESS
                                                          : telux::common::Status::FAILED;
}

telux::common::Status
SimulaCallManager::getECallConfig(telux::tel::EcallConfig& config)
{
    LOG_DEBUG("[CallManager] getECallConfig");
    ensureStarted_();
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto promise =
      std::make_shared<std::promise<std::pair<telux::common::ErrorCode, telux::tel::EcallConfig>>>();
    auto future = promise->get_future();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, promise]() {
        nlohmann::json data = nlohmann::json::object();
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::get_config::req, "ecall.get_config.rsp", req,
          [promise, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!rsp || rsp->error || !rsp->data)
              {
                  LOG_WARN("[CallManager] get_config failed corrId=%s", corrId.c_str());
                  promise->set_value({ telux::common::ErrorCode::OPERATION_TIMEOUT, {} });
                  return;
              }
              promise->set_value({ telux::common::ErrorCode::SUCCESS, wireToEcallConfig(*rsp->data) });
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    auto result = waitForEcallOp(future);
    if (!result)
        return telux::common::Status::FAILED;
    auto [error, resultConfig] = *result;
    if (error != telux::common::ErrorCode::SUCCESS)
        return telux::common::Status::FAILED;
    config = resultConfig;
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// registerListener / removeListener

telux::common::Status
SimulaCallManager::registerListener(std::shared_ptr<telux::tel::ICallListener> listener)
{
    LOG_DEBUG("[CallManager] registerListener listenerSet=%d", listener ? 1 : 0);
    if (!listener)
        return telux::common::Status::INVALIDPARAM;
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(listener);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::removeListener(std::shared_ptr<telux::tel::ICallListener> listener)
{
    LOG_DEBUG("[CallManager] removeListener listenerSet=%d", listener ? 1 : 0);
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.erase(
      std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::makeCall(int, const std::string&, std::shared_ptr<telux::tel::IMakeCallCallback> callback)
{
    if (callback)
        callback->makeCallResponse(telux::common::ErrorCode::NOT_SUPPORTED, nullptr);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::makeRttCall(
  int, const std::string&, std::shared_ptr<telux::tel::IMakeCallCallback> callback
)
{
    if (callback)
        callback->makeCallResponse(telux::common::ErrorCode::NOT_SUPPORTED, nullptr);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::conference(
  std::shared_ptr<telux::tel::ICall>, std::shared_ptr<telux::tel::ICall>,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::swap(
  std::shared_ptr<telux::tel::ICall>, std::shared_ptr<telux::tel::ICall>,
  std::shared_ptr<telux::common::ICommandResponseCallback> callback
)
{
    if (callback)
        callback->commandResponse(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::hangupForegroundResumeBackground(int, telux::common::ResponseCallback callback)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::hangupWaitingOrBackground(int, telux::common::ResponseCallback callback)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::requestEcbm(int, telux::tel::EcbmStatusCallback callback)
{
    if (callback)
        callback({}, telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::exitEcbm(int, telux::common::ResponseCallback callback)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode
SimulaCallManager::encodeECallMsd(telux::tel::ECallMsdData, std::vector<uint8_t>&)
{
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::Status
SimulaCallManager::encodeEuroNcapOptionalAdditionalData(telux::tel::ECallOptionalEuroNcapData, std::vector<uint8_t>&)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::sendRtt(int, std::string, telux::common::ResponseCallback callback)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCallManager::configureECallRedial(
  telux::tel::RedialConfigType config, const std::vector<int>& timeGap,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[CallManager] configureECallRedial config=%d size=%zu", static_cast<int>(config), timeGap.size()
    );
    size_t maxAttempts = config == telux::tel::RedialConfigType::CALL_ORIG ? 10 : 2;
    if (timeGap.empty() || timeGap.size() > maxAttempts)
    {
        LOG_ERROR(
          "[CallManager] configureECallRedial invalid timeGap size=%zu for config=%d (must be 1-%zu)",
          timeGap.size(), static_cast<int>(config), maxAttempts
        );
        if (callback)
            callback(telux::common::ErrorCode::INVALID_ARGUMENTS);
        return telux::common::Status::INVALIDPARAM;
    }
    ensureStarted_();
    if (!isReadyDerived_())
    {
        if (callback)
            callback(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
        return telux::common::Status::NOTREADY;
    }
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, config, timeGap, callback]() {
        nlohmann::json data = nlohmann::json::object();
        data["config"] = config == telux::tel::RedialConfigType::CALL_ORIG ? "CALLORIG" : "CALLDROP";
        data["timeGap"] = timeGap;
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::configure_ecall_redial::req, "ecall.configure_ecall_redial.rsp", req,
          [callback, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!rsp || rsp->error)
              {
                  LOG_WARN("[CallManager] configure_ecall_redial failed corrId=%s", corrId.c_str());
                  if (callback)
                      callback(
                        rsp && rsp->error
                          ? common::simula::parseErrorCode(rsp->error->value("code", std::string()))
                          : telux::common::ErrorCode::OPERATION_TIMEOUT
                      );
                  return;
              }
              LOG_DEBUG("[CallManager] configure_ecall_redial response ok corrId=%s", corrId.c_str());
              if (callback)
                  callback(telux::common::ErrorCode::SUCCESS);
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCallManager::restartECallHlapTimer(
  int, telux::tel::EcallHlapTimerId, int, telux::common::ResponseCallback callback
)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode
SimulaCallManager::getECallRedialConfig(std::vector<int>& callOrigTimeGap, std::vector<int>& callDropTimeGap)
{
    LOG_DEBUG("[CallManager] getECallRedialConfig");
    ensureStarted_();
    if (!isReadyDerived_())
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    auto promise = std::make_shared<
      std::promise<std::tuple<telux::common::ErrorCode, std::vector<int>, std::vector<int>>>>();
    auto future = promise->get_future();
    auto pld = std::make_shared<RunOnAoPld>();
    pld->fn = [this, promise]() {
        nlohmann::json data = nlohmann::json::object();
        auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
        bridge_.send_request(
          topics::ecall::get_ecall_redial_config::req, "ecall.get_ecall_redial_config.rsp", req,
          [promise, corrId = req.corrId](std::optional<Envelope> rsp) {
              if (!rsp || rsp->error || !rsp->data)
              {
                  LOG_WARN("[CallManager] get_ecall_redial_config failed corrId=%s", corrId.c_str());
                  promise->set_value({ telux::common::ErrorCode::OPERATION_TIMEOUT, {}, {} });
                  return;
              }
              promise->set_value({
                telux::common::ErrorCode::SUCCESS,
                rsp->data->value("callOrigTimeGap", std::vector<int>{}),
                rsp->data->value("callDropTimeGap", std::vector<int>{}),
              });
          },
          kRpcTimeout
        );
    };
    post_fifo({ RunEcallOp_Signal, pld });
    auto result = waitForEcallOp(future);
    if (!result)
        return telux::common::ErrorCode::OPERATION_TIMEOUT;
    auto [error, origGap, dropGap] = *result;
    if (error == telux::common::ErrorCode::SUCCESS)
    {
        callOrigTimeGap = std::move(origGap);
        callDropTimeGap = std::move(dropGap);
    }
    return error;
}

telux::common::Status
SimulaCallManager::updateECallPostTestRegistrationTimer(int, uint32_t, telux::common::ResponseCallback callback)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::ErrorCode
SimulaCallManager::getECallPostTestRegistrationTimer(int, uint32_t&)
{
    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::Status
SimulaCallManager::setEmergencyMode(int, bool, bool, telux::common::ResponseCallback callback)
{
    if (callback)
        callback(telux::common::ErrorCode::NOT_SUPPORTED);
    return telux::common::Status::NOTSUPPORTED;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
CallMgrNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaCallManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            LOG_INFO("[CallManager] -> NotReady");
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            auto status = pld->env.data ? pld->env.data->value("status", std::string()) : std::string();
            LOG_DEBUG("[CallManager] NotReady: ReadinessEvt_Signal status=%s", status.c_str());
            if (status == "AVAILABLE")
                return self->to(CallMgrReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            LOG_DEBUG(
              "[CallManager] NotReady: BridgeConnectivityChanged_Signal operational=%d",
              pld ? (*pld ? 1 : 0) : -1
            );
            return chart::Status::HANDLED;
        }
        case CallStateEvt_Signal:
        case MsdStatusEvt_Signal:
        case HlapTimerEventEvt_Signal:
        case RedialEvt_Signal:
            return chart::Status::HANDLED;
        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            self->init_cbs_.push_back(pld->cb);
            return chart::Status::HANDLED;
        }
        case RunEcallOp_Signal:
            LOG_WARN("[CallManager] stale RunEcallOp_Signal dropped -- NotReady before response");
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
CallMgrReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaCallManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            LOG_INFO("[CallManager] -> Ready");
            if (!self->init_cbs_.empty())
            {
                std::vector<telux::common::InitResponseCb> cbs;
                cbs.swap(self->init_cbs_);
                for (auto& cb : cbs)
                    if (cb)
                        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
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
            auto status = pld->env.data ? pld->env.data->value("status", std::string()) : std::string();
            LOG_DEBUG("[CallManager] Ready: ReadinessEvt_Signal status=%s", status.c_str());
            if (status == "UNAVAILABLE")
                return self->to(CallMgrNotReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        {
            auto pld = event_cast<bool>(*e);
            if (pld && !*pld)
            {
                LOG_WARN("[CallManager] Ready: bridge connectivity lost -- transitioning to NotReady");
                return self->to(CallMgrNotReady_St);
            }
            LOG_DEBUG("[CallManager] Ready: BridgeConnectivityChanged_Signal operational=%d", pld ? 1 : 0);
            return chart::Status::HANDLED;
        }

        case RunEcallOp_Signal:
        {
            auto pld = event_cast<RunOnAoPld>(*e);
            if (pld->fn)
                pld->fn();
            return chart::Status::HANDLED;
        }

        case CallStateEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            const auto& d = *pld->env.data;
            int phoneId = d.value("phoneId", 0);
            auto call = self->findOrCreateCall_(
              phoneId, d.value("callIndex", 0), wireToCallDirection(d.value("direction", std::string())),
              d.value("remotePartyNumber", std::string())
            );
            call->setCallState(wireToCallState(d.value("callState", std::string())));
            LOG_DEBUG(
              "[CallManager] CallStateEvt_Signal phoneId=%d callState=%s", phoneId,
              d.value("callState", std::string()).c_str()
            );
            self->broadcastToListeners_([call](const std::shared_ptr<telux::tel::ICallListener>& l) {
                l->onCallInfoChange(call);
            });
            return chart::Status::HANDLED;
        }

        case MsdStatusEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int phoneId = pld->env.data->value("phoneId", 0);
            auto status = wireToMsdTransmissionStatus(pld->env.data->value("status", std::string()));
            LOG_DEBUG("[CallManager] MsdStatusEvt_Signal phoneId=%d status=%d", phoneId, static_cast<int>(status));
            self->broadcastToListeners_([phoneId, status](const std::shared_ptr<telux::tel::ICallListener>& l) {
                l->onECallMsdTransmissionStatus(phoneId, status);
            });
            return chart::Status::HANDLED;
        }

        case RedialEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int phoneId = pld->env.data->value("phoneId", 0);
            telux::tel::ECallRedialInfo info;
            info.willECallRedial = pld->env.data->value("willRedial", false);
            info.reason = wireToRedialReason(pld->env.data->value("reason", std::string()));
            LOG_DEBUG(
              "[CallManager] RedialEvt_Signal phoneId=%d willRedial=%d reason=%d", phoneId,
              info.willECallRedial, static_cast<int>(info.reason)
            );
            self->broadcastToListeners_([phoneId, info](const std::shared_ptr<telux::tel::ICallListener>& l) {
                l->onECallRedial(phoneId, info);
            });
            return chart::Status::HANDLED;
        }

        case HlapTimerEventEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            int phoneId = pld->env.data->value("phoneId", 0);
            auto timer = pld->env.data->value("timer", std::string());
            auto action = wireToHlapTimerEvent(pld->env.data->value("action", std::string()));
            telux::tel::ECallHlapTimerEvents events;
            {
                std::lock_guard<std::mutex> lk(self->hlap_events_mutex_);
                events = self->hlapEvents_[phoneId];
                if (timer == "T2")       events.t2 = action;
                else if (timer == "T5")  events.t5 = action;
                else if (timer == "T6")  events.t6 = action;
                else if (timer == "T7")  events.t7 = action;
                else if (timer == "T9")  events.t9 = action;
                else if (timer == "T10") events.t10 = action;
                self->hlapEvents_[phoneId] = events;
            }
            LOG_DEBUG(
              "[CallManager] HlapTimerEventEvt_Signal phoneId=%d timer=%s action=%d", phoneId,
              timer.c_str(), static_cast<int>(action)
            );
            self->broadcastToListeners_([phoneId, events](const std::shared_ptr<telux::tel::ICallListener>& l) {
                l->onECallHlapTimerEvent(phoneId, events);
            });
            return chart::Status::HANDLED;
        }

        default:
            return self->super(&chart::Hsm::top);
    }
}

CHART_NAMED_STATE(CallMgrNotReady_St, "CallManager::NotReady");
CHART_NAMED_STATE(CallMgrReady_St,    "CallManager::Ready");

}  // namespace telux::tel::simula
