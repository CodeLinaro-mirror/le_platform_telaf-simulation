// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ModemBridge.hpp"

#include "EventCast.hpp"
#include "Log.hpp"
#include "Signals.hpp"

#include <algorithm>
#include <chart/assert.hpp>
#include <chart/event.hpp>
#include <chart/hsm.hpp>
#include <chart/spy.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <future>
#include "generated/cpp/validators.h"
#include <mosquitto.h>
#include <nlohmann/json.hpp>
#include <random>
#include <thread>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

namespace telux::common::simula {

using namespace CommonSignals;

namespace {

// =============================================================================
// Payload types used internally between libmosquitto callbacks, public API, and
// the state handlers. All are owned via std::shared_ptr because chart::Event
// payload is std::shared_ptr<void>.
// =============================================================================

struct LinkUpPld
{
    int rc;
};
struct LinkDownPld
{
    int rc;
};
struct MessagePld
{
    std::string topic;
    std::string body;
};

struct SendReqPld
{
    std::string topic;
    std::string rsp_schema_id;
    Envelope envelope;
    IModemBridge::RpcCallback cb;
    std::chrono::steady_clock::time_point deadline;
};

struct RspTimeoutPld
{
    std::string corrId;
};

struct SubscribeEventPld
{
    std::string topic;
    std::string schema_id;
    IModemBridge::EventCallback cb;
};

struct UnsubscribeEventPld
{
    std::string topic;
};

struct SubscribeConnectivityPld
{
    IModemBridge::ConnectivityToken token;
    IModemBridge::ConnectivityCallback cb;
};

struct UnsubscribeConnectivityPld
{
    IModemBridge::ConnectivityToken token;
};

// drain() fence payload. The waiter holds the same shared_ptr and blocks on
// the future; whichever state handler the event lands in fulfils the promise
// (ShuttingDown included -- see ShuttingDown_St). If the AO is stopped before
// the event is reached, post_fifo drops it and the shared_ptr dies with the
// queue, so DrainPld's dtor completes the promise instead of leaving the
// waiter blocked forever.
struct DrainPld
{
    std::promise<void> done;
    bool fulfilled{ false };

    void fulfil()
    {
        if (!fulfilled)
        {
            fulfilled = true;
            done.set_value();
        }
    }
    ~DrainPld() { fulfil(); }
};

// Helpers ---------------------------------------------------------------------

std::string
readWhoami_()
{
    const char* home = std::getenv("HOME");
    if (!home)
        return "unknown";
    std::ifstream f(std::string(home) + "/.whoami");
    if (!f)
        return "unknown";
    std::string role;
    std::getline(f, role);
    while (!role.empty() && (role.back() == '\n' || role.back() == '\r' || role.back() == ' '))
    {
        role.pop_back();
    }
    return role.empty() ? "unknown" : role;
}

std::string
readProcessName_()
{
    std::ifstream f("/proc/self/comm");
    if (!f)
        return "pa";
    std::string name;
    std::getline(f, name);
    return name.empty() ? "pa" : name;
}

std::string
drawShortId_()
{
    // 4 hex chars, seeded from std::random_device (typically /dev/urandom).
    std::random_device rd;
    uint32_t r = rd();
    char buf[5];
    std::snprintf(buf, sizeof(buf), "%04x", r & 0xFFFFu);
    return std::string(buf, 4);
}

using telux::common::simula::event_cast;

// Mirrors sml/mpss/data/envelope.py's resolve_schema_id() -- PA's outbound
// req topics follow the fixed "ap/req/<domain>/<method>" ->
// "<domain>.<method>.req" rule (unlike inbound rsp/ind, which has no such
// rule and needs a per-call-site schema_id -- see rsp_schema_id/schema_id
// params on send_request/subscribe_event). Returns "" for a topic outside
// that prefix (PA never publishes on ctrl/cmd/** per invariant (f), so
// ap/req/** is the only outbound shape this bridge needs to resolve).
std::string
resolveReqSchemaId_(const std::string& topic)
{
    constexpr std::string_view kPrefix = "ap/req/";
    if (topic.rfind(kPrefix, 0) != 0)
        return {};
    std::string rest = topic.substr(kPrefix.size());
    std::replace(rest.begin(), rest.end(), '/', '.');
    return rest + ".req";
}

// Fixed rsp subscription wildcard: shared per-domain-per-method rsp topics,
// `dest` field filters misdelivery. ModemBridge itself is domain-agnostic,
// so it subscribes to every domain's
// rsp namespace with one wildcard.
constexpr std::string_view kRspWildcard = "mp/rsp/#";

}  // anonymous namespace

// =============================================================================
// PA id construction
// =============================================================================

std::string
buildPaId()
{
    return readProcessName_() + "-" + readWhoami_() + "-" + drawShortId_();
}

// =============================================================================
// Forward decl of state handlers; defined below.
// =============================================================================

chart::Status
NotOperational_St(chart::Hsm*, chart::Event const*);
chart::Status
Disconnected_St(chart::Hsm*, chart::Event const*);
chart::Status
Connecting_St(chart::Hsm*, chart::Event const*);
chart::Status
Connected_St(chart::Hsm*, chart::Event const*);
chart::Status
Subscribing_St(chart::Hsm*, chart::Event const*);
chart::Status
Operational_St(chart::Hsm*, chart::Event const*);
chart::Status
Backoff_St(chart::Hsm*, chart::Event const*);
chart::Status
ShuttingDown_St(chart::Hsm*, chart::Event const*);

// =============================================================================
// Singleton + lifecycle
// =============================================================================

ModemBridge&
ModemBridge::instance()
{
    static ModemBridge inst;
    return inst;
}

ModemBridge::ModemBridge()
    : chart::ActiveObject("ModemBridge")
    , pa_id_(buildPaId())
    , connect_timeout_(this, ConnectTimeout_Signal)
{
    // Process-global libmosquitto init. ModemBridge is a singleton, so its
    // ctor runs exactly once -- the matching cleanup lives in the dtor.
    mosquitto_lib_init();
    LOG_INFO("[ModemBridge] ctor paId=%s default_uds=%d", pa_id_.c_str(), SIMULA_MQTT_DEFAULT_UDS);
}

ModemBridge::~ModemBridge()
{
    stop();
    // client_ (unique_ptr with mosquitto_destroy deleter) is released before
    // the global cleanup by member-destruction order below; force it now so
    // mosquitto_lib_cleanup() never races a live handle.
    client_.reset();
    mosquitto_lib_cleanup();
}

std::string
ModemBridge::computeBrokerUri_() const
{
    // libmosquitto has no URI parser; the "uri" here is a log-friendly label
    // only. The actual transport is chosen by (host,port) in Connecting_St:
    // UDS => host = socket path, port = 0; TCP => host, port.
    if (cfg_.use_uds)
    {
        const std::string& sock =
          cfg_.socket_path.empty() ? std::string(SIMULA_MQTT_UDS_SOCKET) : cfg_.socket_path;
        return "unix://" + sock;
    }
    return "tcp://" + cfg_.host + ":" + std::to_string(cfg_.port);
}

void
ModemBridge::setBrokerConfig(BrokerConfig cfg)
{
    CHART_REQUIRE(!running());  // disallowed once start() has been called
    cfg_ = std::move(cfg);
}

void
ModemBridge::start()
{
    if (running())
        return;
    auto uri = computeBrokerUri_();
    LOG_INFO("[ModemBridge] start() uri=%s", uri.c_str());

    // mosquitto_new(id, clean_session, userdata=this).
    client_.reset(mosquitto_new(pa_id_.c_str(), /*clean_session*/ true, this));
    if (!client_)
    {
        // Only fails on ENOMEM / EINVAL; nothing we can do but refuse to
        // start -- keep a bad transport from taking the whole service down.
        LOG_ERROR("[ModemBridge] mosquitto_new failed -- bridge NOT started");
        return;
    }
    mosquitto_connect_callback_set(client_.get(), &ModemBridge::onConnect_);
    mosquitto_disconnect_callback_set(client_.get(), &ModemBridge::onDisconnect_);
    mosquitto_message_callback_set(client_.get(), &ModemBridge::onMessage_);

    // Delegate reconnect to libmosquitto's background loop: exponential
    // backoff between reconnect_delay and reconnect_delay_max seconds. Map
    // the existing ms-granularity BrokerConfig onto its second-granularity
    // API (min 1s), preserving the exponential-backoff intent.
    unsigned int rd = static_cast<unsigned int>(
      std::max<long long>(1, cfg_.backoff_initial.count() / 1000));
    unsigned int rd_max = static_cast<unsigned int>(
      std::max<long long>(rd, cfg_.backoff_max.count() / 1000));
    mosquitto_reconnect_delay_set(client_.get(), rd, rd_max, /*exponential*/ true);

    start_at(Disconnected_St);
    post_fifo({ Start_Signal, nullptr });
}

void
ModemBridge::stop()
{
    if (!running())
        return;
    post_fifo({ Stop_Signal, nullptr });
    // Give the AO worker a moment to process Stop_Signal and run
    // ShuttingDown's entry (which fails in-flight/deferred RPC callbacks
    // before we tear down).
    for (int i = 0; i < 200; ++i)
    {
        if (currentState() == State::ShuttingDown)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    chart::ActiveObject::stop();
}

std::string
ModemBridge::currentPaId() const
{
    return pa_id_;
}

ModemBridge::State
ModemBridge::currentState() const
{
    // Chart-derived: no shadow atomic. Map current_state() (a StateFn
    // pointer) back to the domain enum via a switch. Cross-thread pointer
    // read on x86-64 is atomic when aligned; between dispatches the AO
    // worker is the sole writer, and a reader sees either the pre- or
    // post-transition value -- both are correct answers to "current state".
    auto s = const_cast<ModemBridge*>(this)->current_state();
    if (s == Disconnected_St) return State::Disconnected;
    if (s == Connecting_St)   return State::Connecting;
    if (s == Subscribing_St)  return State::Subscribing;
    if (s == Operational_St)  return State::Operational;
    if (s == Backoff_St)      return State::Backoff;
    if (s == ShuttingDown_St) return State::ShuttingDown;
    // Connected_St is a composite parent (Subscribing/Operational live under
    // it); a well-formed HSM never rests on a composite -- INIT drills to a
    // leaf. If we ever observe it, the caller sees the parent's default:
    // Disconnected (matches the pre-start initial-target).
    // Before start_at()/init(), current_state() may return nullptr or
    // &Hsm::top -- treat both as Disconnected (the initial-target state).
    return State::Disconnected;
}

// =============================================================================
// Public API forwarding into AO event queue
// =============================================================================

void
ModemBridge::send_request(
  std::string_view topic,
  std::string_view rsp_schema_id,
  Envelope req,
  RpcCallback cb,
  std::chrono::milliseconds timeout
)
{
    LOG_DEBUG(
      "[ModemBridge] send_request topic=%.*s corrId=%s timeout=%ldms",
      static_cast<int>(topic.size()),
      topic.data(),
      req.corrId.c_str(),
      static_cast<long>(timeout.count())
    );
    auto pld = std::make_shared<SendReqPld>();
    pld->topic = std::string(topic);
    pld->rsp_schema_id = std::string(rsp_schema_id);
    pld->envelope = std::move(req);
    pld->cb = std::move(cb);
    pld->deadline = std::chrono::steady_clock::now() + timeout;
    post_fifo({ SendReq_Signal, pld });
}

void
ModemBridge::subscribe_event(std::string_view topic, std::string_view schema_id, EventCallback cb)
{
    auto pld = std::make_shared<SubscribeEventPld>();
    pld->topic = std::string(topic);
    pld->schema_id = std::string(schema_id);
    pld->cb = std::move(cb);
    post_fifo({ SubscribeEvent_Signal, pld });
}

void
ModemBridge::unsubscribe_event(std::string_view topic)
{
    auto pld = std::make_shared<UnsubscribeEventPld>();
    pld->topic = std::string(topic);
    post_fifo({ UnsubscribeEvent_Signal, pld });
}

ModemBridge::ConnectivityToken
ModemBridge::subscribe_connectivity(ConnectivityCallback cb)
{
    // The token has to be usable the instant this returns (the caller stores
    // it in a member and may be destroyed before the worker ever runs the
    // registration), so allocate it here on the caller's thread rather than
    // on the worker. fetch_add makes concurrent subscribers safe.
    auto token = next_conn_token_.fetch_add(1, std::memory_order_relaxed);
    auto pld = std::make_shared<SubscribeConnectivityPld>();
    pld->token = token;
    pld->cb = std::move(cb);
    post_fifo({ SubscribeConnectivity_Signal, pld });
    return token;
}

void
ModemBridge::unsubscribe_connectivity(ConnectivityToken token)
{
    if (token == 0)
        return;
    auto pld = std::make_shared<UnsubscribeConnectivityPld>();
    pld->token = token;
    post_fifo({ UnsubscribeConnectivity_Signal, pld });
}

void
ModemBridge::drain()
{
    // Never from the worker itself -- we'd be waiting on an event only we
    // could dequeue.
    CHART_REQUIRE(std::this_thread::get_id() != worker_id());
    if (!running())
        return;   // no worker to fence against; nothing can still be queued

    auto pld = std::make_shared<DrainPld>();
    auto fut = pld->done.get_future();
    post_fifo({ Drain_Signal, pld });
    // Drop our own reference so that, if post_fifo dropped the event (AO
    // stopped between the running() check and the post), the queue's copy is
    // the last one and its dtor fulfils the promise. Without this the local
    // pld would keep the DrainPld alive and wait() would hang.
    pld.reset();
    fut.wait();
}

// =============================================================================
// libmosquitto callbacks — strict callback-only-post discipline
//
// All run on the mosquitto network-loop thread (mosquitto_loop_start). Each
// does exactly one post_fifo into the AO worker; no business logic here.
// =============================================================================

void
ModemBridge::onConnect_(struct mosquitto* /*m*/, void* self_v, int rc)
{
    auto* self = static_cast<ModemBridge*>(self_v);
    LOG_INFO("[ModemBridge] mosq::on_connect rc=%d (%s)", rc, mosquitto_connack_string(rc));
    // rc==0 => accepted. libmosquitto fires on_connect for every (re)connect,
    // so this is also the reconnect-success edge that lifts us out of Backoff.
    auto pld = std::make_shared<LinkUpPld>();
    pld->rc = (rc == 0) ? 0 : -1;
    self->post_fifo({ LinkUp_Signal, pld });
}

void
ModemBridge::onDisconnect_(struct mosquitto* /*m*/, void* self_v, int rc)
{
    auto* self = static_cast<ModemBridge*>(self_v);
    LOG_WARN("[ModemBridge] mosq::on_disconnect rc=%d", rc);
    auto pld = std::make_shared<LinkDownPld>();
    pld->rc = -1;
    self->post_fifo({ LinkDown_Signal, pld });
}

void
ModemBridge::onMessage_(struct mosquitto* /*m*/, void* self_v, const struct mosquitto_message* msg)
{
    auto* self = static_cast<ModemBridge*>(self_v);
    LOG_DEBUG("[ModemBridge] on_message topic=%s len=%d", msg->topic, msg->payloadlen);
    auto pld = std::make_shared<MessagePld>();
    pld->topic = msg->topic;
    pld->body.assign(static_cast<const char*>(msg->payload),
                     static_cast<size_t>(msg->payloadlen));
    self->post_fifo({ EvtRecv_Signal, pld });
}

// =============================================================================
// Internal helpers (called only on the AO worker thread)
// =============================================================================

void
ModemBridge::issueAllSubscribes_()
{
    mosquitto_subscribe(client_.get(), nullptr, std::string(kRspWildcard).c_str(), /*qos*/ 1);
    for (auto& kv : subscriptions_)
    {
        mosquitto_subscribe(client_.get(), nullptr, kv.first.c_str(), /*qos*/ 1);
    }
}

void
ModemBridge::notifyConnectivity_(bool operational)
{
    // Copy first: a callback may unsubscribe (its own token or another's)
    // from inside the notification, and unsubscribe_connectivity's erase is
    // itself queued, so the map can't change under us here -- but copying
    // also keeps this safe if that ever becomes synchronous.
    auto snapshot = connectivity_observers_;
    for (auto& [token, cb] : snapshot)
    {
        (void)token;
        try
        {
            cb(operational);
        }
        catch (...)
        { /* swallow */
        }
    }
}

void
ModemBridge::failAllInFlight_()
{
    for (auto& [corrId, inf] : in_flight_)
    {
        if (inf.timer_token != 0)
        {
            chart::TimerService::instance().cancel(inf.timer_token);
        }
        if (inf.cb)
        {
            try
            {
                inf.cb(std::nullopt);
            }
            catch (...)
            { /* swallow */
            }
        }
    }
    in_flight_.clear();

    // Deferred SendReq_Signal events never made it to doSendRequest_(), so
    // their callbacks haven't been invoked yet either — drain and fail them
    // the same way, and just drop any deferred subscribe/unsubscribe (they
    // carry no callback to fail).
    chart::Event e;
    while (deferred_.pop(e))
    {
        if (e.sig == SendReq_Signal)
        {
            auto p = event_cast<SendReqPld>(e);
            try
            {
                p->cb(std::nullopt);
            }
            catch (...)
            { /* swallow */
            }
        }
    }
}

bool
ModemBridge::validate_(
  const std::string& topic,
  const std::string& schema_id,
  const nlohmann::json& payload
) const
{
    try
    {
        const auto& schema_json = topics::schemas::schema(schema_id);
        valijson::Schema schema;
        valijson::SchemaParser parser;
        valijson::adapters::NlohmannJsonAdapter schema_adapter(schema_json);
        parser.populateSchema(schema_adapter, schema);

        valijson::Validator validator;
        valijson::ValidationResults results;
        valijson::adapters::NlohmannJsonAdapter payload_adapter(payload);
        if (!validator.validate(schema, payload_adapter, &results))
        {
            valijson::ValidationResults::Error error;
            std::string detail = "unknown";
            if (results.popError(error))
            {
                detail = error.jsonPointer + ": " + error.description;
            }
            LOG_WARN(
              "[ModemBridge] payload schema invalid topic=%s schema_id=%s: %s",
              topic.c_str(),
              schema_id.c_str(),
              detail.c_str()
            );
            return false;
        }
        return true;
    }
    catch (const std::exception& exc)
    {
        LOG_WARN(
          "[ModemBridge] schema validation error topic=%s schema_id=%s: %s",
          topic.c_str(),
          schema_id.c_str(),
          exc.what()
        );
        return true;
    }
}

// =============================================================================
// State handlers
// =============================================================================

// Parent composite for {Disconnected, Connecting, Backoff}: deduplicates
// subscription-table bookkeeping that all three share verbatim.
chart::Status
NotOperational_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        case chart::Exit_Signal:
            return chart::Status::HANDLED;
        case SubscribeConnectivity_Signal:
        {
            auto p = event_cast<SubscribeConnectivityPld>(*e);
            self->connectivity_observers_[p->token] = std::move(p->cb);
            return chart::Status::HANDLED;
        }
        case UnsubscribeConnectivity_Signal:
        {
            auto p = event_cast<UnsubscribeConnectivityPld>(*e);
            self->connectivity_observers_.erase(p->token);
            return chart::Status::HANDLED;
        }
        case SubscribeEvent_Signal:
        {
            auto p = event_cast<SubscribeEventPld>(*e);
            self->subscriptions_[p->topic] = ModemBridge::Subscription{ p->schema_id, std::move(p->cb) };
            return chart::Status::HANDLED;
        }
        case UnsubscribeEvent_Signal:
        {
            auto p = event_cast<UnsubscribeEventPld>(*e);
            self->subscriptions_.erase(p->topic);
            return chart::Status::HANDLED;
        }
        case Drain_Signal:
        {
            // Reaching this event is the whole point -- see
            // IModemBridge::drain(). Everything posted before it (including
            // any Unsubscribe*/EvtRecv) has already been dispatched.
            event_cast<DrainPld>(*e)->fulfil();
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
Disconnected_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            return chart::Status::HANDLED;
        case Start_Signal:
            return self->to(Connecting_St);
        case SendReq_Signal:
        {
            auto p = event_cast<SendReqPld>(*e);
            try
            {
                p->cb(std::nullopt);
            }
            catch (...)
            { /* swallow */
            }
            return chart::Status::HANDLED;
        }
        case Stop_Signal:
            return self->to(ShuttingDown_St);
        default:
            return self->super(NotOperational_St);
    }
}

chart::Status
Connecting_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            // UDS => (socket_path, port=0); TCP => (host, port). port==0 is
            // libmosquitto's AF_UNIX trigger (verified: mosquitto_pub --unix
            // takes this path). mosquitto_connect_async + loop_start hand the
            // socket work -- and all future auto-reconnects -- to the library's
            // background thread; on_connect/on_disconnect drive the HSM.
            const char* host;
            int port;
            if (self->cfg_.use_uds)
            {
                static thread_local std::string sock;
                sock = self->cfg_.socket_path.empty()
                         ? std::string(SIMULA_MQTT_UDS_SOCKET)
                         : self->cfg_.socket_path;
                host = sock.c_str();
                port = 0;
            }
            else
            {
                host = self->cfg_.host.c_str();
                port = self->cfg_.port;
            }
            int rc = mosquitto_connect_async(self->client_.get(), host, port, self->cfg_.keepalive_s);
            if (rc != MOSQ_ERR_SUCCESS)
            {
                LOG_ERROR("[ModemBridge] mosquitto_connect_async failed: %s", mosquitto_strerror(rc));
                auto pld = std::make_shared<LinkDownPld>();
                pld->rc = -1;
                self->post_fifo({ LinkDown_Signal, pld });
            }
            else
            {
                // Idempotent: loop_start no-ops if the network thread is
                // already running (survives reconnect churn).
                rc = mosquitto_loop_start(self->client_.get());
                if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_INVAL)
                {
                    LOG_ERROR("[ModemBridge] mosquitto_loop_start failed: %s", mosquitto_strerror(rc));
                }
            }
            self->connect_timeout_.arm_one_shot(self->cfg_.connect_timeout);
            return chart::Status::HANDLED;
        }
        case chart::Exit_Signal:
            self->connect_timeout_.disarm();
            return chart::Status::HANDLED;
        case LinkUp_Signal:
        {
            auto p = event_cast<LinkUpPld>(*e);
            if (p->rc == 0)
            {
                self->consecutive_failures_ = 0;
                return self->to(Connected_St);
            }
            ++self->consecutive_failures_;
            return self->to(Backoff_St);
        }
        case ConnectTimeout_Signal:
        case LinkDown_Signal:
            ++self->consecutive_failures_;
            return self->to(Backoff_St);
        case Stop_Signal:
            return self->to(ShuttingDown_St);
        case SendReq_Signal:
            chart::defer(self->deferred_, *e);
            return chart::Status::HANDLED;
        default:
            return self->super(NotOperational_St);
    }
}

chart::Status
Connected_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            return chart::Status::HANDLED;
        case chart::Init_Signal:
            return self->to(Subscribing_St);
        case LinkDown_Signal:
            ++self->consecutive_failures_;
            return self->to(Backoff_St);
        case Stop_Signal:
            return self->to(ShuttingDown_St);
        // Withdrawing an observer and reaching the drain fence need no
        // state-specific behaviour, so both live on the composite parent and
        // serve Subscribing and Operational alike.
        case UnsubscribeConnectivity_Signal:
        {
            auto p = event_cast<UnsubscribeConnectivityPld>(*e);
            self->connectivity_observers_.erase(p->token);
            return chart::Status::HANDLED;
        }
        case Drain_Signal:
        {
            event_cast<DrainPld>(*e)->fulfil();
            return chart::Status::HANDLED;
        }
        // Subscribe/Unsubscribe live on the composite parent: once we're
        // inside Connected_St the broker link is up, and Subscribing_St's
        // Entry has already issued the initial batch synchronously before
        // returning. Any Subscribe/Unsubscribe dequeued after that -- whether
        // we're still nominally in Subscribing (before AllSubAcked) or in
        // Operational -- can safely mutate subscriptions_ AND drive
        // mosquitto_subscribe/_unsubscribe on the broker in one place. Single
        // handler => no way for the map and the wire to disagree.
        case SubscribeEvent_Signal:
        {
            auto p = event_cast<SubscribeEventPld>(*e);
            self->subscriptions_[p->topic] = ModemBridge::Subscription{ p->schema_id, std::move(p->cb) };
            mosquitto_subscribe(self->client_.get(), nullptr, p->topic.c_str(), /*qos*/ 1);
            return chart::Status::HANDLED;
        }
        case UnsubscribeEvent_Signal:
        {
            auto p = event_cast<UnsubscribeEventPld>(*e);
            self->subscriptions_.erase(p->topic);
            mosquitto_unsubscribe(self->client_.get(), nullptr, p->topic.c_str());
            return chart::Status::HANDLED;
        }
        default:
            return self->super(&chart::Hsm::top);
    }
}

chart::Status
Subscribing_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->issueAllSubscribes_();
            // No per-topic SUBACK confirmation is wired up (see the note on
            // issueAllSubscribes_() in ModemBridge.hpp) — "issued" already
            // implies "acked" for this bridge, so advance immediately.
            self->post_fifo({ AllSubAcked_Signal, nullptr });
            return chart::Status::HANDLED;
        case AllSubAcked_Signal:
            return self->to(Operational_St);
        // Subscribe/Unsubscribe are handled by Connected_St -- they can run
        // as soon as we're inside Connected (link is up and the initial
        // issueAllSubscribes_() batch already ran synchronously in Entry).
        // Only SendReq still defers here: in_flight/timer bookkeeping is
        // Operational's job.
        case SubscribeConnectivity_Signal:
        {
            auto p = event_cast<SubscribeConnectivityPld>(*e);
            self->connectivity_observers_[p->token] = std::move(p->cb);
            return chart::Status::HANDLED;
        }
        case SendReq_Signal:
            chart::defer(self->deferred_, *e);
            return chart::Status::HANDLED;
        default:
            return self->super(Connected_St);
    }
}

chart::Status
Operational_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->notifyConnectivity_(true);
            chart::recall_all(self->deferred_, *self);
            return chart::Status::HANDLED;

        case chart::Exit_Signal:
            self->notifyConnectivity_(false);
            self->failAllInFlight_();
            return chart::Status::HANDLED;

        case SendReq_Signal:
        {
            auto p = event_cast<SendReqPld>(*e);
            self->doSendRequest_(p->topic, p->rsp_schema_id, std::move(p->envelope), std::move(p->cb), p->deadline);
            return chart::Status::HANDLED;
        }

        case SubscribeConnectivity_Signal:
        {
            auto p = event_cast<SubscribeConnectivityPld>(*e);
            // Operational already — fire once synchronously so the new
            // subscriber never misses the "already up" case (mirrors
            // subscribe_event's retained-topic-like semantics).
            try
            {
                p->cb(true);
            }
            catch (...)
            { /* swallow */
            }
            self->connectivity_observers_[p->token] = std::move(p->cb);
            return chart::Status::HANDLED;
        }

        case EvtRecv_Signal:
        {
            auto m = event_cast<MessagePld>(*e);
            nlohmann::json j;
            try
            {
                j = nlohmann::json::parse(m->body);
            }
            catch (...)
            {
                return chart::Status::HANDLED;
            }
            std::string parse_err;
            auto env_opt = Envelope::fromJson(j, &parse_err);
            if (!env_opt)
            {
                return chart::Status::HANDLED;
            }
            // rsp topics carry `dest`; req/ind never do. Any envelope with
            // `dest` set is routed as an rsp: discard immediately if it
            // wasn't addressed to us (misdelivery on the shared rsp topic),
            // otherwise look up corrId. This dest-check happens strictly
            // before the corrId table lookup so a same-corrId collision
            // across processes can
            // never cross-wire two unrelated callbacks.
            if (env_opt->dest)
            {
                if (*env_opt->dest != self->pa_id_)
                {
                    return chart::Status::HANDLED;
                }
                auto it = self->in_flight_.find(env_opt->corrId);
                if (it != self->in_flight_.end())
                {
                    if (it->second.timer_token != 0)
                    {
                        chart::TimerService::instance().cancel(it->second.timer_token);
                    }
                    // Error envelopes have no schema in validators.h (only
                    // .rsp success shapes do) -- only validate `data`,
                    // matching Phase 1's Python-side _pub_rsp treatment.
                    if (env_opt->data &&
                        !self->validate_(m->topic, it->second.rsp_schema_id, *env_opt->data))
                    {
                        // Schema-invalid rsp is treated identically to a
                        // timed-out/unparseable one: fail fast with nullopt
                        // rather than handing a garbled envelope to the
                        // Manager's wireToXxx decode,
                        // which would otherwise silently default. Every
                        // RpcCallback already treats nullopt as
                        // OPERATION_TIMEOUT, so no caller-side change needed.
                        auto cb = std::move(it->second.cb);
                        self->in_flight_.erase(it);
                        try
                        {
                            cb(std::nullopt);
                        }
                        catch (...)
                        { /* swallow */
                        }
                        return chart::Status::HANDLED;
                    }
                    auto cb = std::move(it->second.cb);
                    self->in_flight_.erase(it);
                    try
                    {
                        cb(std::move(*env_opt));
                    }
                    catch (...)
                    { /* swallow */
                    }
                }
                return chart::Status::HANDLED;
            }
            auto sit = self->subscriptions_.find(m->topic);
            if (sit != self->subscriptions_.end())
            {
                if (env_opt->data &&
                    !self->validate_(m->topic, sit->second.schema_id, *env_opt->data))
                {
                    // Schema-invalid ind: drop, don't dispatch to the
                    // Manager's EventCallback (Phase 3, decision #4 -- no
                    // caller waiting on an ind, so nothing to fail, just
                    // don't hand bad data downstream).
                    return chart::Status::HANDLED;
                }
                try
                {
                    sit->second.cb(m->topic, *env_opt);
                }
                catch (...)
                { /* swallow */
                }
            }
            return chart::Status::HANDLED;
        }

        case RspTimeout_Signal:
        {
            auto p = event_cast<RspTimeoutPld>(*e);
            LOG_WARN("[ModemBridge] RPC timeout corrId=%s", p->corrId.c_str());
            auto it = self->in_flight_.find(p->corrId);
            if (it != self->in_flight_.end())
            {
                auto cb = std::move(it->second.cb);
                self->in_flight_.erase(it);
                try
                {
                    cb(std::nullopt);
                }
                catch (...)
                { /* swallow */
                }
            }
            return chart::Status::HANDLED;
        }

        default:
            return self->super(Connected_St);
    }
}

// Backoff is now PASSIVE. libmosquitto's background loop (mosquitto_loop_
// start + reconnect_delay_set) owns reconnection, so this state must NOT
// disconnect, arm a timer, or re-invoke connect -- doing any of those would
// fight the library's own reconnect and desync wire-vs-HSM. It simply tears
// down in-flight work and waits for the library's next on_connect (LinkUp),
// which drills back through Connected -> Subscribing to re-establish subs
// (clean_session loses them on every reconnect).
chart::Status
Backoff_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
        {
            self->notifyConnectivity_(false);
            self->failAllInFlight_();
            LOG_WARN("[ModemBridge] -> Backoff (passive, library auto-reconnecting) failures=%d",
                     self->consecutive_failures_);
            return chart::Status::HANDLED;
        }
        case LinkUp_Signal:
        {
            auto p = event_cast<LinkUpPld>(*e);
            if (p->rc == 0)
            {
                self->consecutive_failures_ = 0;
                return self->to(Connected_St);
            }
            // Still failing; stay passive and keep waiting.
            return chart::Status::HANDLED;
        }
        case Stop_Signal:
            return self->to(ShuttingDown_St);
        case SendReq_Signal:
            chart::defer(self->deferred_, *e);
            return chart::Status::HANDLED;
        case LinkDown_Signal:
            return chart::Status::HANDLED;
        default:
            return self->super(NotOperational_St);
    }
}

chart::Status
ShuttingDown_St(chart::Hsm* h, chart::Event const* e)
{
    auto* self = static_cast<ModemBridge*>(h);
    switch (e->sig)
    {
        case chart::Entry_Signal:
            self->notifyConnectivity_(false);
            self->failAllInFlight_();
            if (self->client_)
            {
                // Stop the background network thread first (force=true so a
                // pending reconnect-sleep is interrupted), then disconnect.
                mosquitto_disconnect(self->client_.get());
                mosquitto_loop_stop(self->client_.get(), /*force*/ true);
            }
            return chart::Status::HANDLED;
        case Drain_Signal:
            // Terminal but NOT for the fence: a Manager destructing during
            // shutdown still has to be able to wait out the worker, and this
            // state is reached while the AO is still running. Swallowing it
            // silently would hang that destructor.
            event_cast<DrainPld>(*e)->fulfil();
            return chart::Status::HANDLED;
        default:
            // Terminal: swallow everything else.
            return chart::Status::HANDLED;
    }
}

// =============================================================================
// Helpers used inside state handlers
// =============================================================================

void
ModemBridge::doSendRequest_(
  const std::string& topic,
  const std::string& rsp_schema_id,
  Envelope envelope,
  RpcCallback cb,
  std::chrono::steady_clock::time_point deadline
)
{
    auto now = std::chrono::steady_clock::now();
    if (deadline <= now)
    {
        try
        {
            cb(std::nullopt);
        }
        catch (...)
        { /* swallow */
        }
        return;
    }

    // PA's own req construction is a local bug if it mismatches the schema
    // (unlike inbound rsp/ind, where mismatch is external drift) -- reject
    // before publish rather than
    // sending known-bad data to MPSS (invariant (d): MPSS only ever sees
    // valid asks) or letting it sit until timeout.
    auto req_schema_id = resolveReqSchemaId_(topic);
    if (!req_schema_id.empty() && envelope.data &&
        !validate_(topic, req_schema_id, *envelope.data))
    {
        LOG_ERROR(
          "[ModemBridge] outbound payload schema invalid topic=%s schema_id=%s -- not publishing",
          topic.c_str(),
          req_schema_id.c_str()
        );
        try
        {
            cb(std::nullopt);
        }
        catch (...)
        { /* swallow */
        }
        return;
    }

    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);

    InFlight inf;
    inf.topic = topic;
    inf.rsp_schema_id = rsp_schema_id;
    inf.cb = std::move(cb);

    std::string corrId = envelope.corrId;
    auto* self_ptr = this;
    inf.timer_token = chart::TimerService::instance().schedule(
      [self_ptr, corrId]() {
          auto pld = std::make_shared<RspTimeoutPld>();
          pld->corrId = corrId;
          self_ptr->post_fifo({ RspTimeout_Signal, pld });
      },
      chart::TimerService::Clock::now() + remaining,
      std::chrono::milliseconds{ 0 }
    );

    in_flight_.emplace(corrId, std::move(inf));

    std::string body = envelope.toJson().dump();
    int rc = mosquitto_publish(client_.get(), nullptr, topic.c_str(),
                               static_cast<int>(body.size()), body.data(),
                               /*qos*/ 1, /*retain*/ false);
    if (rc != MOSQ_ERR_SUCCESS)
    {
        LOG_ERROR(
          "[ModemBridge] publish failed corrId=%s rc=%s topic=%s",
          corrId.c_str(),
          mosquitto_strerror(rc),
          topic.c_str()
        );
        auto it = in_flight_.find(corrId);
        if (it != in_flight_.end())
        {
            if (it->second.timer_token != 0)
            {
                chart::TimerService::instance().cancel(it->second.timer_token);
            }
            auto cb2 = std::move(it->second.cb);
            in_flight_.erase(it);
            try
            {
                cb2(std::nullopt);
            }
            catch (...)
            { /* swallow */
            }
        }
    }
}

CHART_NAMED_STATE(NotOperational_St, "ModemBridge::NotOperational");
CHART_NAMED_STATE(Disconnected_St, "ModemBridge::Disconnected");
CHART_NAMED_STATE(Connecting_St,   "ModemBridge::Connecting");
CHART_NAMED_STATE(Connected_St,    "ModemBridge::Connected");
CHART_NAMED_STATE(Subscribing_St,  "ModemBridge::Subscribing");
CHART_NAMED_STATE(Operational_St,  "ModemBridge::Operational");
CHART_NAMED_STATE(Backoff_St,      "ModemBridge::Backoff");
CHART_NAMED_STATE(ShuttingDown_St, "ModemBridge::ShuttingDown");

}  // namespace telux::common::simula
