// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "CardManager.hpp"

#include "../common/ListenerDispatchAO.hpp"
#include "../common/Log.hpp"
#include "Signals.hpp"
#include "generated/cpp/topics.h"

#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>

namespace telux::tel::simula {

using namespace SimSignals;

namespace {

using common::simula::Envelope;

telux::tel::CardState
wireToCardState(const std::string& s)
{
    if (s == "ABSENT")
        return telux::tel::CardState::CARDSTATE_ABSENT;
    if (s == "PRESENT")
        return telux::tel::CardState::CARDSTATE_PRESENT;
    if (s == "ERROR")
        return telux::tel::CardState::CARDSTATE_ERROR;
    if (s == "RESTRICTED")
        return telux::tel::CardState::CARDSTATE_RESTRICTED;
    return telux::tel::CardState::CARDSTATE_UNKNOWN;
}

std::string
cardStateToWire(telux::tel::CardState s)
{
    switch (s)
    {
        case telux::tel::CardState::CARDSTATE_ABSENT:
            return "ABSENT";
        case telux::tel::CardState::CARDSTATE_PRESENT:
            return "PRESENT";
        case telux::tel::CardState::CARDSTATE_ERROR:
            return "ERROR";
        case telux::tel::CardState::CARDSTATE_RESTRICTED:
            return "RESTRICTED";
        default:
            return "UNKNOWN";
    }
}

telux::tel::AppState
wireToAppState(const std::string& s)
{
    if (s == "READY")
        return telux::tel::AppState::APPSTATE_READY;
    if (s == "ILLEGAL")
        return telux::tel::AppState::APPSTATE_ILLEGAL;
    return telux::tel::AppState::APPSTATE_UNKNOWN;
}

std::string
appStateToWire(telux::tel::AppState s)
{
    switch (s)
    {
        case telux::tel::AppState::APPSTATE_READY:
            return "READY";
        case telux::tel::AppState::APPSTATE_ILLEGAL:
            return "ILLEGAL";
        default:
            return "UNKNOWN";
    }
}

constexpr auto kRpcTimeout = std::chrono::seconds(30);

struct StateIndPld
{
    Envelope env;
};

struct CardPowerPld
{
    bool powerOn;
    telux::common::ResponseCallback cb;
};

template<typename T>
std::shared_ptr<T>
as(const chart::Event& e)
{
    return std::static_pointer_cast<T>(e.payload);
}

}  // namespace

chart::Status
CardNotReady_St(chart::Hsm*, chart::Event const*);
chart::Status
CardReady_St(chart::Hsm*, chart::Event const*);

SimulaCardManager::SimulaCardManager(
  int slotId,
  common::simula::IModemBridge& bridge,
  telux::common::InitResponseCb initCb
)
    : chart::ActiveObject("CardManager")
    , bridge_(bridge)
    , slotId_(slotId)
    , card_(std::make_shared<SimulaCard>(
        slotId,
        telux::tel::CardState::CARDSTATE_ABSENT,
        telux::tel::AppState::APPSTATE_UNKNOWN
      ))
{
    if (initCb)
        init_cbs_.push_back(std::move(initCb));
}

SimulaCardManager::~SimulaCardManager()
{
    //  Withdraw the event/connectivity callbacks before anything else: they
    // capture raw `this` and the bridge holds its own copies. unsubscribe_*
    // is only a queued removal, so the drain() fence inside is what actually
    // makes "no callback can reach us" true. RPC callbacks use weak ownership.
    unsubscribeFromBridge_();
    stop();
}

void
SimulaCardManager::unsubscribeFromBridge_()
{
    bridge_.unsubscribe_event(topics::sim::subsys_ready_card::ind);
    bridge_.unsubscribe_event(topics::sim::card_state::ind);
    bridge_.unsubscribe_connectivity(conn_token_);
    conn_token_ = 0;
    bridge_.drain();
}

void
SimulaCardManager::addInitCallback(telux::common::InitResponseCb cb)
{
    if (!cb)
        return;
    {
        std::lock_guard<std::mutex> lk(init_cbs_mutex_);
        if (!init_reported_)
        {
            init_cbs_.push_back(std::move(cb));
            return;
        }
    }
    cb(last_status_.load());
}

void
SimulaCardManager::fireInitCallbacks_(telux::common::ServiceStatus status)
{
    std::vector<telux::common::InitResponseCb> cbs;
    {
        std::lock_guard<std::mutex> lk(init_cbs_mutex_);
        init_reported_ = true;
        cbs.swap(init_cbs_);
    }
    for (auto& cb : cbs)
    {
        if (!cb)
            continue;
        try
        {
            cb(status);
        }
        catch (const std::future_error& e)
        {
            // A late callback may target a promise destroyed after the PA timeout.
            LOG_WARN("[CardManager] init callback fired after PA timeout (%s); dropping",
                     e.what());
        }
        catch (...)
        {
            LOG_WARN("[CardManager] init callback threw unexpectedly; dropping");
        }
    }
}

void
SimulaCardManager::start()
{
    if (running())
        return;
    // Instrumentation must be attached before start_at().
    set_instrument(std::make_unique<chart::SpyInstrument>(name()));
    start_at(CardNotReady_St);

    bridge_.subscribe_event(
      topics::sim::subsys_ready_card::ind,
      "sim.subsys_ready_card.ind",
      [this](std::string_view topic, const Envelope& env) { handleCardInd_(topic, env); }
    );
    bridge_.subscribe_event(
      topics::sim::card_state::ind,
      "sim.card_state.ind",
      [this](std::string_view topic, const Envelope& env) { handleCardInd_(topic, env); }
    );
    conn_token_ = bridge_.subscribe_connectivity([this](bool operational) {
        auto pld = std::make_shared<bool>(operational);
        post_fifo({ BridgeConnectivityChanged_Signal, pld });
    });
}

void
SimulaCardManager::handleCardInd_(std::string_view topic, const Envelope& env)
{
    // Bridge callbacks run off the AO thread; state changes are posted to it.
    auto pld = std::make_shared<StateIndPld>();
    pld->env = env;
    if (topic == topics::sim::subsys_ready_card::ind)
        post_fifo({ ReadinessEvt_Signal, pld });
    else
        post_fifo({ CardStateEvt_Signal, pld });
}

void
SimulaCardManager::resyncCardState_()
{
    // card_state is non-retained; pull current state and use the normal update path.
    nlohmann::json data = nlohmann::json::object();
    data["slot"] = slotId_;
    auto req = common::simula::makeRequestEnvelope(bridge_.currentPaId(), std::move(data));
    bridge_.send_request(
      topics::sim::get_state::req,
      "sim.get_state.rsp",
      req,
      [weak = weak_from_this()](std::optional<Envelope> rsp) {
          if (!rsp || rsp->error || !rsp->data)
              return;  // stay at last known state; card_state_ind still applies
          if (auto self = weak.lock())
          {
              auto pld = std::make_shared<StateIndPld>();
              pld->env = *rsp;
              self->post_fifo({ CardStateEvt_Signal, pld });
          }
      },
      kRpcTimeout
    );
}

void
SimulaCardManager::broadcastToListeners_(
  std::function<void(const std::shared_ptr<telux::tel::ICardListener>&)> invoke
)
{
    auto task = std::make_shared<common::simula::DispatchTask>();
    task->debug_tag = "CardManager::broadcastToListeners_";
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
        invoke(std::static_pointer_cast<telux::tel::ICardListener>(raw));
    };
    common::simula::ListenerDispatchAO::instance().enqueue(std::move(task));
}

// ---------------------------------------------------------------------------
// telux::tel::ICardManager

bool
SimulaCardManager::isSubsystemReady()
{
    return ready_flag_.load();
}

std::future<bool>
SimulaCardManager::onSubsystemReady()
{
    std::promise<bool> p;
    p.set_value(ready_flag_.load());
    return p.get_future();
}

telux::common::ServiceStatus
SimulaCardManager::getServiceStatus()
{
    return last_status_.load();
}

telux::common::Status
SimulaCardManager::getSlotCount(int& count)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    count = 1;
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCardManager::getSlotIds(std::vector<int>& slotIds)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    slotIds = { slotId_ };
    return telux::common::Status::SUCCESS;
}

std::shared_ptr<telux::tel::ICard>
SimulaCardManager::getCard(int slotId, telux::common::Status* status)
{
    if (!ready_flag_.load())
    {
        if (status)
            *status = telux::common::Status::NOTREADY;
        return nullptr;
    }
    if (slotId != slotId_ && slotId != DEFAULT_SLOT_ID)
    {
        if (status)
            *status = telux::common::Status::NOSUCH;
        return nullptr;
    }
    if (status)
        *status = telux::common::Status::SUCCESS;

    // The PA represents ABSENT and UNKNOWN cards as null pointers.
    if (!cardIsPresent_())
        return nullptr;
    return card_;
}

bool
SimulaCardManager::cardIsPresent_() const
{
    if (!card_)
        return false;
    telux::tel::CardState cs = telux::tel::CardState::CARDSTATE_UNKNOWN;
    card_->getState(cs);
    return cs != telux::tel::CardState::CARDSTATE_ABSENT
           && cs != telux::tel::CardState::CARDSTATE_UNKNOWN;
}

void
SimulaCardManager::setPresenceChangedCb(PresenceChangedCb cb)
{
    std::lock_guard<std::mutex> lk(presence_cb_mutex_);
    presence_cb_ = std::move(cb);
}

void
SimulaCardManager::notifyIfPresenceChanged_()
{
    const bool now = cardIsPresent_();
    if (now == last_present_)
        return;
    last_present_ = now;

    PresenceChangedCb cb;
    {
        std::lock_guard<std::mutex> lk(presence_cb_mutex_);
        cb = presence_cb_;
    }
    if (cb)
    {
        LOG_INFO("[CardManager] card presence -> %s; requesting PA slot-status refresh",
                 now ? "present" : "not present");
        cb(now);
    }
}

telux::common::Status
SimulaCardManager::cardPowerUp(SlotId slotId, telux::common::ResponseCallback callback)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    if (slotId != slotId_ && slotId != DEFAULT_SLOT_ID)
        return telux::common::Status::NOSUCH;
    auto pld = std::make_shared<CardPowerPld>();
    pld->powerOn = true;
    pld->cb = std::move(callback);
    post_fifo({ CardPowerUp_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCardManager::cardPowerDown(SlotId slotId, telux::common::ResponseCallback callback)
{
    if (!ready_flag_.load())
        return telux::common::Status::NOTREADY;
    if (slotId != slotId_ && slotId != DEFAULT_SLOT_ID)
        return telux::common::Status::NOSUCH;
    auto pld = std::make_shared<CardPowerPld>();
    pld->powerOn = false;
    pld->cb = std::move(callback);
    post_fifo({ CardPowerDown_Signal, pld });
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCardManager::setupRefreshConfig(
  SlotId, bool, bool, std::vector<telux::tel::IccFile>, telux::tel::RefreshParams,
  telux::common::ResponseCallback
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardManager::allowCardRefresh(SlotId, bool, telux::tel::RefreshParams, telux::common::ResponseCallback)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardManager::confirmRefreshHandlingCompleted(
  SlotId, bool, telux::tel::RefreshParams, telux::common::ResponseCallback
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardManager::requestLastRefreshEvent(
  SlotId, telux::tel::RefreshParams, telux::tel::refreshLastEventResponseCallback
)
{
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
SimulaCardManager::registerListener(std::shared_ptr<telux::tel::ICardListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.push_back(listener);
    return telux::common::Status::SUCCESS;
}

telux::common::Status
SimulaCardManager::removeListener(std::shared_ptr<telux::tel::ICardListener> listener)
{
    std::lock_guard<std::mutex> lk(listeners_mutex_);
    listeners_.erase(
      std::remove_if(
        listeners_.begin(),
        listeners_.end(),
        [&](const std::weak_ptr<telux::tel::ICardListener>& w) {
            auto sp = w.lock();
            return !sp || sp == listener;
        }
      ),
      listeners_.end()
    );
    return telux::common::Status::SUCCESS;
}

// ---------------------------------------------------------------------------
// State handlers

chart::Status
CardNotReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaCardManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case ReadinessEvt_Signal:
        {
            auto pld = as<StateIndPld>(*e);
            if (pld->env.data && pld->env.data->value("status", std::string()) == "AVAILABLE")
                return self->to(CardReady_St);
            return chart::Status::HANDLED;
        }
        case BridgeConnectivityChanged_Signal:
            return chart::Status::HANDLED;
        case CardPowerUp_Signal:
        case CardPowerDown_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
CardReady_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<SimulaCardManager*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->ready_flag_.store(true);
            self->last_status_.store(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            bool first_report;
            {
                std::lock_guard<std::mutex> lk(self->init_cbs_mutex_);
                first_report = !self->init_reported_;
            }
            self->fireInitCallbacks_(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            if (!first_report)
            {
                self->broadcastToListeners_(
                  [](const std::shared_ptr<telux::tel::ICardListener>& l) {
                      l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                  }
                );
            }
            self->resyncCardState_();
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->ready_flag_.store(false);
            self->broadcastToListeners_(
              [](const std::shared_ptr<telux::tel::ICardListener>& l) {
                  l->onServiceStatusChange(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
              }
            );
            return chart::Status::HANDLED;

        case ReadinessEvt_Signal:
        {
            auto pld = as<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto state = pld->env.data->value("status", std::string());
            if (state == "UNAVAILABLE")
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(CardNotReady_St);
            }
            if (state == "FAILED")
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_FAILED);
                return self->to(CardNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case BridgeConnectivityChanged_Signal:
        {
            auto pld = as<bool>(*e);
            if (pld && !*pld)
            {
                self->last_status_.store(telux::common::ServiceStatus::SERVICE_UNAVAILABLE);
                return self->to(CardNotReady_St);
            }
            return chart::Status::HANDLED;
        }

        case CardStateEvt_Signal:
        {
            auto pld = as<StateIndPld>(*e);
            if (!pld->env.data)
                return chart::Status::HANDLED;
            auto cardState = wireToCardState(pld->env.data->value("cardState", std::string()));
            auto appState = wireToAppState(pld->env.data->value("appState", std::string()));
            self->card_->setState(cardState);
            self->card_->usimApp()->setAppState(appState);
            // Refresh the PA card cache before notifying card listeners.
            self->notifyIfPresenceChanged_();
            self->broadcastToListeners_(
              [slotId = self->slotId_](const std::shared_ptr<telux::tel::ICardListener>& l) {
                  l->onCardInfoChanged(slotId);
              }
            );
            return chart::Status::HANDLED;
        }

        case CardPowerUp_Signal:
        case CardPowerDown_Signal:
        {
            auto pld = as<CardPowerPld>(*e);
            nlohmann::json data = nlohmann::json::object();
            data["slot"] = self->slotId_;
            data["powerOn"] = pld->powerOn;
            auto req = common::simula::makeRequestEnvelope(self->bridge_.currentPaId(), std::move(data));
            auto cb = pld->cb;
            self->bridge_.send_request(
              topics::sim::set_power::req,
              "sim.set_power.rsp",
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

// State names for chart::Spy output.
CHART_NAMED_STATE(CardNotReady_St, "CardManager::NotReady");
CHART_NAMED_STATE(CardReady_St,    "CardManager::Ready");

}  // namespace telux::tel::simula
