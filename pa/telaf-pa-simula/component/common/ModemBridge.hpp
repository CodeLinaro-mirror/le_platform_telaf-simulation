// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// ModemBridge.hpp - singleton MQTT request/response router shared by every
// simula-pa domain.
//
// Owns a libmosquitto client (struct mosquitto*) and a chart::ActiveObject
// worker thread. All libmosquitto callbacks marshal events back into the AO
// via post_fifo; no business logic runs on the mosquitto network-loop thread.
// Domain Managers/Sessions talk to MPSS only through this bridge.
//
// Transport: libmosquitto reaches the broker over either a UNIX domain socket
// (host = socket path, port = 0) or TCP (host, port). libmosquitto's port==0
// mechanism is exercised by mosquitto_pub/sub and verified working against
// this tree's broker.
//
// Reconnect is delegated to libmosquitto: mosquitto_loop_start() runs a
// background network thread that auto-reconnects with mosquitto_reconnect_
// delay_set()'s exponential backoff. The HSM keeps every state (invariant h)
// but Backoff is now a *passive* wait for the library's next on_connect, not a
// manually-driven disconnect/timer/reconnect cycle.
//
// State machine:
//   Disconnected -[Start]-> Connecting
//                              │ LinkUp
//                              ▼
//                            Connected (composite: Subscribing → Operational)
//                              │ LinkDown
//                              ▼
//                            Backoff -[BackoffElapsed]-> Connecting
//   {Disconnected,Connecting,Backoff,Connected} -[Stop]-> ShuttingDown (terminal)
//
// Requests issued while not Operational are held in a DeferQueue and
// recalled in order on entry to Operational; if the bridge stops first,
// every deferred and in-flight RPC is failed with std::nullopt.

#ifndef TELUX_COMMON_SIMULA_MODEM_BRIDGE_HPP
#define TELUX_COMMON_SIMULA_MODEM_BRIDGE_HPP

#include "IModemBridge.hpp"

#include <atomic>
#include <chart/active_object.hpp>
#include <chart/defer.hpp>
#include <chart/time_event.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mosquitto.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace telux::common::simula {

// Build-time transport default. Defined PRIVATE on telux_common; any other
// TU that includes this header (and never sees the define) falls back to UDS,
// matching the default build. Runtime BrokerConfig::use_uds can override.
#ifndef SIMULA_MQTT_DEFAULT_UDS
#define SIMULA_MQTT_DEFAULT_UDS 1
#endif

// Process-wide identifier embedded in envelope `src` and matched against
// rsp envelope `dest`. Returns "<binary>-<role>-<short>". Stable for the
// lifetime of the process.
std::string
buildPaId();

// Configurable broker endpoint. Defaults match `sml/mosquitto.conf`.
struct BrokerConfig
{
    // Transport: true = unix://socket_path, false = tcp://host:port. Defaults
    // to the build-time SIMULA_MQTT_UDS choice but is runtime-overridable --
    // libmosquitto handles both from one binary (port==0 => UDS).
    bool use_uds = (SIMULA_MQTT_DEFAULT_UDS != 0);
    std::string host = "localhost";
    int port = 1883;
    int keepalive_s = 60;
    std::chrono::milliseconds connect_timeout{ 5000 };
    std::chrono::milliseconds backoff_initial{ 500 };
    std::chrono::milliseconds backoff_max{ 30000 };
    double backoff_multiplier = 2.0;
    std::string socket_path;   // override UDS socket path (empty = SIMULA_MQTT_UDS_SOCKET)
};

class ModemBridge final
    : public IModemBridge
    , private chart::ActiveObject
{
public:
    static ModemBridge& instance();

    // IModemBridge — see IModemBridge.hpp.
    void start() override;
    void stop() override;
    std::string currentPaId() const override;
    void send_request(
      std::string_view topic,
      std::string_view rsp_schema_id,
      Envelope req,
      RpcCallback cb,
      std::chrono::milliseconds timeout
    ) override;
    void subscribe_event(std::string_view topic, std::string_view schema_id, EventCallback cb) override;
    void unsubscribe_event(std::string_view topic) override;
    ConnectivityToken subscribe_connectivity(ConnectivityCallback cb) override;
    void unsubscribe_connectivity(ConnectivityToken token) override;
    void drain() override;

    // Test seam: replace the broker config (must be called before start()).
    void setBrokerConfig(BrokerConfig cfg);

    // Test seam: ask the running AO what state it currently sits in.
    enum class State
    {
        Disconnected,
        Connecting,
        Subscribing,   // Connected sub-state
        Operational,   // Connected sub-state
        Backoff,
        ShuttingDown
    };
    State currentState() const;

private:
    ModemBridge();
    ~ModemBridge() override;

    ModemBridge(const ModemBridge&) = delete;
    ModemBridge& operator=(const ModemBridge&) = delete;

    // libmosquitto C callbacks. Registered via mosquitto_*_callback_set with
    // `this` as userdata; each trampolines to the matching member and does
    // exactly one post_fifo. They run on the mosquitto network-loop thread.
    static void onConnect_(struct mosquitto* m, void* self, int rc);
    static void onDisconnect_(struct mosquitto* m, void* self, int rc);
    static void onMessage_(struct mosquitto* m, void* self, const struct mosquitto_message* msg);

    // State handlers (chart-style free functions). Declared friends so they
    // can poke private members.
    friend chart::Status NotOperational_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Disconnected_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Connecting_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Connected_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Subscribing_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Operational_St(chart::Hsm*, chart::Event const*);
    friend chart::Status Backoff_St(chart::Hsm*, chart::Event const*);
    friend chart::Status ShuttingDown_St(chart::Hsm*, chart::Event const*);

    // Internal helpers (all called only on the AO worker thread).
    //
    // issueAllSubscribes_() uses mosquitto_subscribe(mosq, NULL, topic, qos).
    // There is no per-topic SUBACK confirmation hooked up, so "all subscribes
    // issued" is treated as sufficient and AllSubAcked_Signal is self-posted
    // synchronously right after issuing them, rather than waiting for a real
    // ack (on_subscribe).
    void issueAllSubscribes_();
    void notifyConnectivity_(bool operational);
    void failAllInFlight_();
    void doSendRequest_(
      const std::string& topic,
      const std::string& rsp_schema_id,
      Envelope envelope,
      RpcCallback cb,
      std::chrono::steady_clock::time_point deadline
    );
    // Validates `payload` against schema_id (sml/generated/cpp/validators.h).
    // Returns true if valid, OR if schema_id is unresolvable/the validator
    // itself errors (fail open on infra problems, fail closed only on an
    // actual payload/schema mismatch) -- always LOG_WARN on any false/error
    // path. Shared by both EvtRecv_Signal branches (rsp/ind, inbound) and
    // doSendRequest_ (req, outbound): one implementation, not duplicated per
    // direction (invariant (e)). Enforced, not log-only.
    bool validate_(
      const std::string& topic,
      const std::string& schema_id,
      const nlohmann::json& payload
    ) const;
    std::string computeBrokerUri_() const;

    BrokerConfig cfg_{};
    std::string pa_id_;
    // No shadow `cur_state_atomic_` -- currentState() derives the enum
    // directly from the chart's current-state pointer (aligned-pointer read
    // on x86-64 is safe cross-thread; between two dispatch()es the AO
    // worker is the sole writer, and a reader on another thread sees
    // pre- or post-transition, both of which are valid answers).

    std::unique_ptr<mosquitto, void (*)(mosquitto*)> client_{ nullptr, &mosquitto_destroy };

    // RPC bookkeeping.
    struct InFlight
    {
        std::string topic;  // request topic
        std::string rsp_schema_id;
        RpcCallback cb;
        chart::TimerService::Token timer_token{ 0 };
    };
    // Keyed by corrId. Only mutated from the AO worker thread. corrId
    // uniqueness is only required within this process — cross-process
    // misdelivery on the shared rsp topic is filtered by `dest` before a
    // lookup here is even attempted (see Operational_St EvtRecv handling).
    std::unordered_map<std::string, InFlight> in_flight_;

    // Subscription bookkeeping. `subscriptions_` holds the live event-topic
    // callback map (rebuilt on every reconnect via issueAllSubscribes_()),
    // paired with the schema id its payload is validated against on receipt.
    struct Subscription
    {
        std::string schema_id;
        EventCallback cb;
    };
    std::map<std::string, Subscription> subscriptions_;

    // Connectivity observers, notified on Operational entry/exit. New
    // subscribers get one synchronous callback immediately if already
    // Operational (mirrors subscribe_event's "don't miss the already-up
    // case" semantics).
    //
    // Keyed by token rather than a plain vector so a Manager can withdraw
    // its own observer in its destructor: the callbacks capture `this`, and
    // an observer left behind is a use-after-free the next time the link
    // flaps. Only mutated from the AO worker thread; `next_conn_token_` is
    // atomic because subscribe_connectivity allocates the token on the
    // *caller's* thread (it has to return it synchronously) while the
    // worker consumes it.
    std::map<ConnectivityToken, ConnectivityCallback> connectivity_observers_;
    std::atomic<ConnectivityToken> next_conn_token_{ 1 };

    // Reconnect backoff state. The delay itself is owned by libmosquitto
    // (mosquitto_reconnect_delay_set); this counter is kept only for logging
    // how many consecutive connect attempts have failed.
    int consecutive_failures_{ 0 };

    // Connect-attempt watchdog. Reconnect delay is delegated to libmosquitto,
    // so no separate backoff timer is needed anymore.
    chart::TimeEvent connect_timeout_;

    // Deferred RPC/subscribe/unsubscribe events held while not Operational.
    chart::DeferQueue deferred_;
};

}  // namespace telux::common::simula

#endif  // TELUX_COMMON_SIMULA_MODEM_BRIDGE_HPP
