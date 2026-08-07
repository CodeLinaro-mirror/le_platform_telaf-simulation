// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "PhoneManager.hpp"

#include "../common/EventCast.hpp"
#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Phone.hpp"
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

telux::tel::OperatingMode
wireToOperatingMode(const std::string& s)
{
    if (s == "AIRPLANE")             return telux::tel::OperatingMode::AIRPLANE;
    if (s == "FACTORY_TEST")         return telux::tel::OperatingMode::FACTORY_TEST;
    if (s == "OFFLINE")              return telux::tel::OperatingMode::OFFLINE;
    if (s == "RESETTING")            return telux::tel::OperatingMode::RESETTING;
    if (s == "SHUTTING_DOWN")        return telux::tel::OperatingMode::SHUTTING_DOWN;
    if (s == "PERSISTENT_LOW_POWER") return telux::tel::OperatingMode::PERSISTENT_LOW_POWER;
    if (s != "ONLINE")
        LOG_WARN(
          "[PhoneManager] wireToOperatingMode: unrecognized mode=%s, defaulting to ONLINE",
          s.c_str()
        );
    return telux::tel::OperatingMode::ONLINE;
}

std::string
operatingModeToWire(telux::tel::OperatingMode mode)
{
    switch (mode)
    {
        case telux::tel::OperatingMode::ONLINE:              return "ONLINE";
        case telux::tel::OperatingMode::AIRPLANE:            return "AIRPLANE";
        case telux::tel::OperatingMode::FACTORY_TEST:        return "FACTORY_TEST";
        case telux::tel::OperatingMode::OFFLINE:             return "OFFLINE";
        case telux::tel::OperatingMode::RESETTING:           return "RESETTING";
        case telux::tel::OperatingMode::SHUTTING_DOWN:       return "SHUTTING_DOWN";
        case telux::tel::OperatingMode::PERSISTENT_LOW_POWER: return "PERSISTENT_LOW_POWER";
        default:
            LOG_WARN(
              "[PhoneManager] operatingModeToWire: unrecognized OperatingMode=%d, defaulting to ONLINE",
              static_cast<int>(mode)
            );
            return "ONLINE";
    }
}

telux::tel::VoiceServiceTechnology
wireToVoiceServiceTech(const std::string& s)
{
    if (s == "VOICE_TECH_GW_CSFB") return telux::tel::VoiceServiceTechnology::VOICE_TECH_GW_CSFB;
    if (s == "VOICE_TECH_1x_CSFB") return telux::tel::VoiceServiceTechnology::VOICE_TECH_1x_CSFB;
    if (s != "VOICE_TECH_VOLTE")
        LOG_WARN(
          "[PhoneManager] wireToVoiceServiceTech: unrecognized value=%s, defaulting to VOICE_TECH_VOLTE",
          s.c_str()
        );
    return telux::tel::VoiceServiceTechnology::VOICE_TECH_VOLTE;
}

telux::tel::RATCapability
wireToRatCapability(const std::string& s)
{
    if (s == "AMPS")   return telux::tel::RATCapability::AMPS;
    if (s == "CDMA")   return telux::tel::RATCapability::CDMA;
    if (s == "HDR")    return telux::tel::RATCapability::HDR;
    if (s == "GSM")    return telux::tel::RATCapability::GSM;
    if (s == "WCDMA")  return telux::tel::RATCapability::WCDMA;
    if (s == "LTE")    return telux::tel::RATCapability::LTE;
    if (s == "TDS")    return telux::tel::RATCapability::TDS;
    if (s == "NR5G")   return telux::tel::RATCapability::NR5G;
    if (s == "NR5GSA") return telux::tel::RATCapability::NR5GSA;
    if (s != "NB1_NTN")
        LOG_WARN(
          "[PhoneManager] wireToRatCapability: unrecognized value=%s, defaulting to NB1_NTN",
          s.c_str()
        );
    return telux::tel::RATCapability::NB1_NTN;
}

telux::tel::SimRatCapability
wireToSimRatCapability(const nlohmann::json& j)
{
    telux::tel::SimRatCapability out{};
    out.slotId = j.value("slotId", 0);
    for (const auto& cap : j.value("capabilities", nlohmann::json::array()))
        out.capabilities.set(static_cast<size_t>(wireToRatCapability(cap.get<std::string>())));
    return out;
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct StateIndPld
{
    Envelope env;
};

struct RequestCellularCapabilityPld
{
    std::shared_ptr<telux::tel::ICellularCapabilityCallback> cb;
};

struct RequestOperatingModePld
{
    std::shared_ptr<telux::tel::IOperatingModeCallback> cb;
};

struct SetOperatingModePld
{
    telux::tel::OperatingMode mode;
    telux::common::ResponseCallback cb;
};

struct SetInitCbPld
{
    telux::common::InitResponseCb cb;
};

}  // namespace

chart::Status
PhoneMgrNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
PhoneMgrReady_St(chart::Hsm*, chart::Event const*);

SimulaPhoneManager::SimulaPhoneManager(
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("PhoneManager")
    , bridge_(bridge)
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaPhoneManager::~SimulaPhoneManager()
{
    // Withdraw from the bridge before anything else: the callbacks below
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaPhoneManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::radio::subsys_ready_phone::ind);
    bridge_.unsubscribe_event(topics::radio::op_mode::ind);
    bridge_.unsubscribe_event(topics::radio::signal_strength::ind);
    bridge_.unsubscribe_event(topics::radio::cell_info::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaPhoneManager::start()
{
    if (running())
        return;
    LOG_INFO("[PhoneManager] start()");
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(PhoneMgrNotReady_St);

    bridge_.subscribe_event(
      topics::radio::subsys_ready_phone::ind,
      "radio.subsys_ready_phone.ind",
      [this](std::string_view topic, const Envelope& env) { handleReadinessInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::op_mode::ind,
      "radio.op_mode.ind",
      [this](std::string_view topic, const Envelope& env) { handleOpModeInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::signal_strength::ind,
      "radio.signal_strength.ind",
      [this](std::string_view topic, const Envelope& env) { handleSignalStrengthInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::radio::cell_info::ind,
      "radio.cell_info.ind",
      [this](std::string_view topic, const Envelope& env) { handleCellInfoInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaPhoneManager::setInitCallback(telux::common::InitResponseCb cb)
{
    LOG_DEBUG("[PhoneManager] setInitCallback called hasCb=%d", cb ? 1 : 0);
    if (!cb)
        return;
    // Already Ready: PhoneMgrReady_St's Entry handler has fired and will not
    // fire again for this caller, so queuing into init_cbs_ would strand it.
    // Satisfy synchronously instead.
    if (isReadyDerived_())
    {
        cb(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        return;
    }
    // Not Ready yet -- hand off through the AO. init_cbs_ is appended to and
    // read only on the worker thread, so it must not be written from the
    // caller's thread.
    auto pld = std::make_shared<SetInitCbPld>();
    pld->cb = std::move(cb);
    post_fifo({ SetInitCb_Signal, pld });
}

void
SimulaPhoneManager::handleReadinessInd_(std::string_view topic, const Envelope& env)
{
    LOG_DEBUG(
      "[PhoneManager] handleReadinessInd_ topic=%.*s corrId=%s",
      static_cast<int>(topic.size()),
      topic.data(),
      env.corrId.c_str()
    );
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ ReadinessEvt_Signal, pld });
}
void
SimulaPhoneManager::handleOpModeInd_(std::string_view topic, const Envelope& env)
{
    LOG_DEBUG(
      "[PhoneManager] handleOpModeInd_ topic=%.*s corrId=%s",
      static_cast<int>(topic.size()),
      topic.data(),
      env.corrId.c_str()
    );
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ OpModeEvt_Signal, pld });
}

void
SimulaPhoneManager::handleSignalStrengthInd_(std::string_view topic, const Envelope& env)
{
    LOG_DEBUG(
      "[PhoneManager] handleSignalStrengthInd_ topic=%.*s corrId=%s",
      static_cast<int>(topic.size()),
      topic.data(),
      env.corrId.c_str()
    );
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ SignalStrengthEvt_Signal, pld });
}

void
SimulaPhoneManager::handleCellInfoInd_(std::string_view topic, const Envelope& env)
{
    LOG_DEBUG(
      "[PhoneManager] handleCellInfoInd_ topic=%.*s corrId=%s",
      static_cast<int>(topic.size()),
      topic.data(),
      env.corrId.c_str()
    );
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    post_fifo({ CellInfoEvt_Signal, pld });
}

void
SimulaPhoneManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::IPhoneListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "PhoneManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::tel::IPhoneListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// telux::tel::IPhoneManager

bool
SimulaPhoneManager::isSubsystemReady()
{
    return isReadyDerived_();
}

std::future<bool>
SimulaPhoneManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(isReadyDerived_());
    return p.get_future();
}

telux::common::ServiceStatus
SimulaPhoneManager::getServiceStatus()
{
    return last_status_.load();
}

bool
SimulaPhoneManager::isReadyDerived_() const
{
    return const_cast<SimulaPhoneManager*>(this)->current_state() == PhoneMgrReady_St;
}

void
SimulaPhoneManager::publishStatus_(telux::common::ServiceStatus s)
{
    last_status_.store(s);
}

telux::common::Status
SimulaPhoneManager::getPhoneIds(std::vector<int>& phoneIds)
{
    phoneIds = { DEFAULT_PHONE_ID };
    return telux::common::Status::SUCCESS;
}

int
SimulaPhoneManager::getPhoneIdFromSlotId(int slotId)
{
    return slotId;
}

int
SimulaPhoneManager::getSlotIdFromPhoneId(int phoneId)
{
    return phoneId;
}

std::shared_ptr<telux::tel::IPhone>
SimulaPhoneManager::getPhone(int phoneId)
{
    LOG_DEBUG("[PhoneManager] getPhone phoneId=%d", phoneId);
    std::lock_guard<std::mutex> lk(phones_mutex_);
    auto it = phones_.find(phoneId);
    if (it != phones_.end())
        return it->second;
    auto phone = std::make_shared<SimulaPhone>(phoneId, bridge_);
    phones_.emplace(phoneId, phone);
    return phone;
}

telux::common::Status
SimulaPhoneManager::requestCellularCapabilityInfo(
  std::shared_ptr<telux::tel::ICellularCapabilityCallback> callback
)
{
    LOG_DEBUG(
      "[PhoneManager] requestCellularCapabilityInfo called hasCallback=%d",
      callback ? 1 : 0
    );
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestCellularCapabilityPld>();
    pld->cb = callback;
    post_fifo({ RequestCellularCapability_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhoneManager::requestOperatingMode(std::shared_ptr<telux::tel::IOperatingModeCallback> callback)
{
    LOG_DEBUG("[PhoneManager] requestOperatingMode called hasCallback=%d", callback ? 1 : 0);
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
    auto pld = std::make_shared<RequestOperatingModePld>();
    pld->cb = callback;
    post_fifo({ RequestOperatingMode_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhoneManager::setOperatingMode(
  telux::tel::OperatingMode operatingMode,
  telux::common::ResponseCallback callback
)
{
    LOG_DEBUG(
      "[PhoneManager] setOperatingMode called mode=%d hasCallback=%d",
      static_cast<int>(operatingMode),
      callback ? 1 : 0
    );
    if (!isReadyDerived_())
        return telux::common::Status::NOTREADY;
 
    if (static_cast<int>(operatingMode) < static_cast<int>(telux::tel::OperatingMode::ONLINE) ||
        static_cast<int>(operatingMode) >
          static_cast<int>(telux::tel::OperatingMode::PERSISTENT_LOW_POWER))
    {
        if (callback)
            callback(telux::common::ErrorCode::INVALID_ARGUMENTS);
        return telux::common::Status::SUCCESS;
    }
    auto pld = std::make_shared<SetOperatingModePld>();
    pld->mode = operatingMode;
    pld->cb = std::move(callback);
    post_fifo({ SetOperatingMode_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhoneManager::resetWwan(telux::common::ResponseCallback callback)
{
    LOG_DEBUG("[PhoneManager] resetWwan called hasCallback=%d", callback ? 1 : 0);

    if (callback)
        callback(telux::common::ErrorCode::SUCCESS);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhoneManager::registerListener(std::weak_ptr<telux::tel::IPhoneListener> listener)
{
    LOG_DEBUG("[PhoneManager] registerListener called listenerAlive=%d", listener.lock() ? 1 : 0);
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(std::move(listener));
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaPhoneManager::removeListener(std::weak_ptr<telux::tel::IPhoneListener> listener)
{
    LOG_DEBUG("[PhoneManager] removeListener called listenerAlive=%d", listener.lock() ? 1 : 0);
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    auto target = listener.lock();
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::IPhoneListener>& w) {
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
PhoneMgrNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaPhoneManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            LOG_INFO("[PhoneManager] -> NotReady");
            return chart::Status::HANDLED;
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            auto status = pld->env.data ? pld->env.data->value("status", std::string())
                                         : std::string("<no-data>");
            LOG_DEBUG("[PhoneManager] ReadinessEvt_Signal (NotReady) status=%s", status.c_str());
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(PhoneMgrReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
        case OpModeEvt_Signal:
        case SignalStrengthEvt_Signal:
        case CellInfoEvt_Signal:
            return chart::Status::HANDLED;
 
        case SetInitCb_Signal:
        {
            auto pld = event_cast<SetInitCbPld>(*e);
            self->init_cbs_.push_back(pld->cb);
            return chart::Status::HANDLED;
        }
     
        case RequestCellularCapability_Signal:
        {
            auto pld = event_cast<RequestCellularCapabilityPld>(*e);
            LOG_WARN(
              "[PhoneManager] stale RequestCellularCapability_Signal failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
            {
                telux::tel::CellularCapabilityInfo info{};
                pld->cb->cellularCapabilityResponse(
                  info, telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE
                );
            }
            return chart::Status::HANDLED;
        }
        case RequestOperatingMode_Signal:
        {
            auto pld = event_cast<RequestOperatingModePld>(*e);
            LOG_WARN(
              "[PhoneManager] stale RequestOperatingMode_Signal failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
                pld->cb->operatingModeResponse(
                  telux::tel::OperatingMode::ONLINE,
                  telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE
                );
            return chart::Status::HANDLED;
        }
        case SetOperatingMode_Signal:
        {
            auto pld = event_cast<SetOperatingModePld>(*e);
            LOG_WARN(
              "[PhoneManager] stale SetOperatingMode_Signal failed with "
              "SUBSYSTEM_UNAVAILABLE -- NotReady before response"
            );
            if (pld->cb)
                pld->cb(telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE);
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
PhoneMgrReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaPhoneManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            LOG_INFO("[PhoneManager] -> Ready");
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
                self->broadcastToListeners_([](const std::shared_ptr<telux::tel::IPhoneListener>& l) {
                    l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                });
            }
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->broadcastToListeners_([](const std::shared_ptr<telux::tel::IPhoneListener>& l) {
                l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
            });
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            LOG_DEBUG("[PhoneManager] ReadinessEvt_Signal (Ready) status=%s", state.c_str());
            if (state == "UNAVAILABLE")
            {
                self->publishStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(PhoneMgrNotReady_St);
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
                return self->to(PhoneMgrNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case OpModeEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto mode = wireToOperatingMode(pld->env.data->value("mode", std::string()));
            LOG_DEBUG("[PhoneManager] OpModeEvt_Signal mode=%d", static_cast<int>(mode));
            self->broadcastToListeners_([mode](const std::shared_ptr<telux::tel::IPhoneListener>& l) {
                l->onOperatingModeChanged(mode);
            });
            return chart::Status::HANDLED;
        }

        case SignalStrengthEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            const auto& data = *pld->env.data;
            const auto& g = data.at("gsm");
            auto gsm = std::make_shared<telux::tel::GsmSignalStrengthInfo>(
              g.value("gsmSignalStrength", 0), g.value("gsmBitErrorRate", 0), 0
            );
            const auto& w = data.at("wcdma");
            auto wcdma = std::make_shared<telux::tel::WcdmaSignalStrengthInfo>(
              w.value("signalStrength", 0), w.value("bitErrorRate", 0),
              w.value("ecio", 0), w.value("rscp", 0)
            );
            const auto& l = data.at("lte");
            LOG_DEBUG(
              "[PhoneManager] SignalStrengthEvt_Signal gsmSignalStrength=%d lteRsrp=%d",
              g.value("gsmSignalStrength", 0),
              l.value("lteRsrp", 0)
            );
            auto lte = std::make_shared<telux::tel::LteSignalStrengthInfo>(
              l.value("lteSignalStrength", 0), l.value("lteRsrp", 0), l.value("lteRsrq", 0),
              l.value("lteRssnr", 0), l.value("lteCqi", 0), l.value("timingAdvance", 0)
            );
            const auto& n = data.at("nr5g");
            auto nr5g = std::make_shared<telux::tel::Nr5gSignalStrengthInfo>(
              n.value("rsrp", 0), n.value("rsrq", 0), n.value("rssnr", 0)
            );
            auto signalStrength = std::make_shared<telux::tel::SignalStrength>(
              lte, gsm, nullptr, wcdma, nullptr, nr5g, nullptr
            );
            self->broadcastToListeners_(
              [signalStrength](const std::shared_ptr<telux::tel::IPhoneListener>& l) {
                  l->onSignalStrengthChanged(DEFAULT_PHONE_ID, signalStrength);
              }
            );
            return chart::Status::HANDLED;
        }

        case CellInfoEvt_Signal:
        {
            auto pld = event_cast<StateIndPld>(*e);
            std::vector<std::shared_ptr<telux::tel::CellInfo>> cells;
            if (pld->env.data)
            {
                for (const auto& c : pld->env.data->value("cells", nlohmann::json::array()))
                {
                    auto cell = wireToCellInfo(c);
                    if (cell)
                        cells.push_back(cell);
                }
            }
            else
            {
                LOG_WARN("[PhoneManager] CellInfoEvt_Signal indication had no data payload");
            }
            LOG_DEBUG(
              "[PhoneManager] CellInfoEvt_Signal received, notifying listeners phoneId=%d cells=%zu",
              DEFAULT_PHONE_ID,
              cells.size()
            );
            self->broadcastToListeners_([cells](const std::shared_ptr<telux::tel::IPhoneListener>& l) {
                l->onCellInfoListChanged(DEFAULT_PHONE_ID, cells);
            });
            return chart::Status::HANDLED;
        }

        case RequestCellularCapability_Signal:
        {
            auto pld = event_cast<RequestCellularCapabilityPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[PhoneManager] RequestCellularCapability_Signal send_request corrId=%s",
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_cellular_capability::req,
              "radio.request_cellular_capability.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  telux::tel::CellularCapabilityInfo info{};
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[PhoneManager] RequestCellularCapability response failed corrId=%s",
                        corrId.c_str()
                      );
                      cb->cellularCapabilityResponse(info, telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  LOG_DEBUG(
                    "[PhoneManager] RequestCellularCapability response ok corrId=%s",
                    corrId.c_str()
                  );
                  for (const auto& t : rsp->data->value("voiceServiceTechs", nlohmann::json::array()))
                      info.voiceServiceTechs.set(
                        static_cast<size_t>(wireToVoiceServiceTech(t.get<std::string>()))
                      );
                  info.simCount = rsp->data->value("simCount", 0);
                  info.maxActiveSims = rsp->data->value("maxActiveSims", 0);
                  for (const auto& c : rsp->data->value("simRatCapabilities", nlohmann::json::array()))
                      info.simRatCapabilities.push_back(wireToSimRatCapability(c));
                  for (const auto& c : rsp->data->value("deviceRatCapability", nlohmann::json::array()))
                      info.deviceRatCapability.push_back(wireToSimRatCapability(c));
                  cb->cellularCapabilityResponse(info, telux::common::ErrorCode::SUCCESS);
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case RequestOperatingMode_Signal:
        {
            auto pld = event_cast<RequestOperatingModePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = DEFAULT_PHONE_ID;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[PhoneManager] RequestOperatingMode_Signal send_request slot=%d corrId=%s",
              DEFAULT_PHONE_ID,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::request_operating_mode::req,
              "radio.request_operating_mode.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error || !rsp->data)
                  {
                      LOG_WARN(
                        "[PhoneManager] RequestOperatingMode response failed corrId=%s",
                        corrId.c_str()
                      );
                      cb->operatingModeResponse(
                        telux::tel::OperatingMode::ONLINE, telux::common::ErrorCode::OPERATION_TIMEOUT
                      );
                      return;
                  }
                  LOG_DEBUG(
                    "[PhoneManager] RequestOperatingMode response ok corrId=%s",
                    corrId.c_str()
                  );
                  cb->operatingModeResponse(
                    wireToOperatingMode(rsp->data->value("mode", std::string())),
                    telux::common::ErrorCode::SUCCESS
                  );
              },
              kRpcTimeout
            );
            return chart::Status::HANDLED;
        }

        case SetOperatingMode_Signal:
        {
            auto pld = event_cast<SetOperatingModePld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["mode"] = operatingModeToWire(pld->mode);
            data["slot"] = DEFAULT_PHONE_ID;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            LOG_DEBUG(
              "[PhoneManager] SetOperatingMode_Signal send_request mode=%d slot=%d corrId=%s",
              static_cast<int>(pld->mode),
              DEFAULT_PHONE_ID,
              req.corrId.c_str()
            );
            self->bridge_.send_request(
              topics::radio::set_operating_mode::req,
              "radio.set_operating_mode.rsp",
              req,
              [cb, corrId = req.corrId](std::optional<Envelope> rsp) {
                  if (!cb)
                      return;
                  if (!rsp || rsp->error)
                  {
                      LOG_WARN(
                        "[PhoneManager] SetOperatingMode response failed corrId=%s",
                        corrId.c_str()
                      );
                      cb(telux::common::ErrorCode::OPERATION_TIMEOUT);
                      return;
                  }
                  LOG_DEBUG("[PhoneManager] SetOperatingMode response ok corrId=%s", corrId.c_str());
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

CHART_NAMED_STATE(PhoneMgrNotReady_St, "PhoneManager::NotReady");
CHART_NAMED_STATE(PhoneMgrReady_St,    "PhoneManager::Ready");

}  // namespace telux::tel::simula
