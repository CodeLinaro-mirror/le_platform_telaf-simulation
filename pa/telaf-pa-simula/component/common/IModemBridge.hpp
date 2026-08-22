// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// IModemBridge.hpp - abstract interface for the MQTT request/response bridge
// shared by every simula-pa domain (data, sim, network, phone, sms, ...).
//
// The concrete implementation (ModemBridge) wraps libmosquitto and a
// chart::ActiveObject; a test mock can implement this interface without
// doing any actual MQTT I/O. Domain Managers talk to MPSS only through
// this bridge — never construct topic strings or touch MQTT directly.

#ifndef TELUX_COMMON_SIMULA_I_MODEM_BRIDGE_HPP
#define TELUX_COMMON_SIMULA_I_MODEM_BRIDGE_HPP

#include "Envelope.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace telux::common::simula {

class IModemBridge
{
public:
    // Invoked when an RPC response arrives (Envelope present) or the request
    // times out (Envelope absent). Always called on a worker thread that is
    // neither the original caller's thread nor any manager AO's own thread.
    using RpcCallback = std::function<void(std::optional<Envelope> rsp)>;

    // Invoked for every event received on a subscribed topic. Topic is the
    // exact MQTT topic the message arrived on. Always called on the
    // bridge's own worker thread — same thread RpcCallback is invoked on,
    // never the subscriber's own AO thread. A subscriber whose reaction
    // only forwards to ListenerDispatchAO (no local state mutation) can
    // wire this callback directly; a subscriber whose state changes on
    // receipt must post an event into its own AO from inside this callback
    // rather than mutating state here.
    using EventCallback = std::function<void(std::string_view topic, const Envelope& env)>;

    // Invoked once synchronously on subscribe if the bridge is already
    // Operational (semantics mirror subscribe_event — a subscriber never
    // misses the "already up" case), then again on every later transition
    // into/out of Operational.
    using ConnectivityCallback = std::function<void(bool operational)>;

    // Opaque handle returned by subscribe_connectivity and consumed by
    // unsubscribe_connectivity. Unlike event subscriptions (keyed by their
    // topic string) connectivity observers have no natural key, so the
    // bridge hands one out. 0 is never a valid token, so a
    // default-initialised member is safe to pass to
    // unsubscribe_connectivity (it no-ops).
    using ConnectivityToken = std::uint64_t;

    virtual ~IModemBridge() = default;

    // Idempotent lifecycle. start() begins broker connect + subscription
    // bookkeeping; stop() fails every in-flight and deferred RPC and joins
    // the worker.
    virtual void start() = 0;
    virtual void stop() = 0;

    // Sender identity embedded into envelope `src`, and matched against
    // incoming rsp envelope `dest` to discard responses meant for other PA
    // processes on a shared rsp topic.
    virtual std::string currentPaId() const = 0;

    // Issue an RPC: publish `req` to `topic` (already carrying a corrId
    // allocated by the caller via nextCorrId()), wait for a matching rsp
    // (dest == currentPaId() && corrId match) on the shared rsp wildcard,
    // then invoke `cb`. If no rsp arrives within `timeout`, invoke `cb`
    // with std::nullopt. Calls made while the bridge is not yet Operational
    // are queued and replayed in order once it becomes Operational; if the
    // bridge stops (or the deadline elapses first) before that happens,
    // `cb` is invoked with std::nullopt.
    //
    // `rsp_schema_id` is the generated schema id (e.g.
    // "data.start_data_call.rsp", see sml/generated/cpp/validators.h) this
    // RPC's response is validated against on receipt -- log-only for now,
    // never rejects. Every call site is statically bound to one topic/schema,
    // so the id is a caller-known literal rather than derived from `topic`
    // (there's no fixed topic->schema-id string rule for mp/rsp/**, unlike
    // inbound ap/req/**).
    //
    // Thread-safe; may be called from any thread.
    virtual void send_request(
      std::string_view topic,
      std::string_view rsp_schema_id,
      Envelope req,
      RpcCallback cb,
      std::chrono::milliseconds timeout
    ) = 0;

    // Subscribe to an event topic. The callback fires on every retained or
    // live message received on `topic` once the bridge is Operational.
    // Idempotent: a second subscribe replaces the prior callback for the
    // same topic.
    //
    // `schema_id` is the generated schema id (e.g. "data.call_state.ind")
    // this topic's payload is validated against on receipt -- log-only for
    // now, never rejects.
    virtual void subscribe_event(std::string_view topic, std::string_view schema_id, EventCallback cb) = 0;

    // Cancel a previous subscription. No-op if topic was never subscribed.
    //
    // Asynchronous: the callback may still be invoked after this returns
    // (the erase happens on the bridge's own worker thread). A subscriber
    // whose callback captures `this` must follow the unsubscribe with
    // drain() before destroying itself -- see drain()'s contract.
    virtual void unsubscribe_event(std::string_view topic) = 0;

    // Observe bridge connectivity. Lets a domain Manager degrade to
    // NotReady on a bridge disconnect even without an explicit
    // readiness-topic UNAVAILABLE message.
    //
    // Returns a token for unsubscribe_connectivity(). The token is
    // allocated synchronously (so it is usable the instant this returns)
    // even though the observer itself is registered on the worker thread.
    virtual ConnectivityToken subscribe_connectivity(ConnectivityCallback cb) = 0;

    // Cancel a previous connectivity subscription. No-op for token 0 or a
    // token that was already unsubscribed. Asynchronous, with the same
    // drain() caveat as unsubscribe_event.
    virtual void unsubscribe_connectivity(ConnectivityToken token) = 0;

    // Synchronisation fence against the bridge's worker thread: blocks
    // until every event queued before this call has been fully processed.
    //
    // Required by any subscriber that registered a callback capturing
    // `this` and is about to be destroyed. unsubscribe_event and
    // unsubscribe_connectivity only *queue* the removal, so on their own
    // they leave a window in which an event received earlier is still
    // ahead of the removal in the worker's FIFO and would invoke the (by
    // then dangling) callback. Because the worker processes its queue
    // strictly in order, a fence posted after the unsubscribes cannot be
    // reached until those unsubscribes -- and every callback-dispatching
    // event ahead of them -- have run. So the canonical destructor shape is:
    //
    //     bridge_.unsubscribe_event(topic_a);
    //     ...
    //     bridge_.unsubscribe_connectivity(conn_token_);
    //     bridge_.drain();   // after this, no callback can reach `this`
    //
    // Safe to call when the bridge was never started or has already
    // stopped (returns immediately -- no worker to wait for). Must not be
    // called from the bridge's own worker thread (i.e. never from inside
    // an EventCallback/RpcCallback/ConnectivityCallback), which would
    // deadlock; the implementation asserts this.
    virtual void drain() = 0;
};

}  // namespace telux::common::simula

#endif  // TELUX_COMMON_SIMULA_I_MODEM_BRIDGE_HPP
