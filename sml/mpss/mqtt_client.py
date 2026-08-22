# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS MQTT client Active Object.

See miros: https://aleph2c.github.io/miros/

HARD RULE: paho-mqtt callbacks registered here MUST NOT contain business
logic. Their only permitted side effect is `self.post_fifo(Event(...))`.
This decouples paho's loop thread from the AO event-dispatch thread and
keeps the state machine the single source of truth.

State graph (`smfn_*` handlers below):
    Off → Connecting → Connected.Operational
                  ↘   ↓
               Disconnected.Reconnecting → Connecting (looping)
    Any state → Stopping (on SIG_STOP) → process exits
"""
from __future__ import annotations

import logging
import random
import socket
import threading
from dataclasses import dataclass
from typing import Optional

import paho.mqtt.client as mqtt
from miros import Event, ActiveObject, return_status, signals, spy_on

from sml.mpss import instrumentation as _instr
from sml.mpss.config import MpssConfig

_FIRST_CONNECT_INFO_LIMIT = 3


# ---------------------------------------------------------------------------
# Event payload dataclasses
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class MessageReceivedPayload:
    topic: str
    payload: bytes
    qos: int


@dataclass(frozen=True)
class PublishPayload:
    topic: str
    payload: bytes
    qos: int = 1
    retain: bool = False


@dataclass(frozen=True)
class LinkUpPayload:
    """Carried with SIG_LINK_UP; rc==0 means clean CONNACK."""
    rc: int


@dataclass(frozen=True)
class LinkDownPayload:
    """Carried with SIG_LINK_DOWN; rc==0 means clean disconnect requested."""
    rc: int


# ---------------------------------------------------------------------------
# Publish-topic validation
# ---------------------------------------------------------------------------

_WILDCARD_CHARS = frozenset("+#")


def validate_publish_topic(topic: str) -> None:
    """Raise ValueError if `topic` contains MQTT wildcard characters.

    MQTT wildcards (`+`, `#`) are valid for subscription filters but never
    for publish targets. paho accepts them silently; the broker then
    rejects, producing confusing runtime errors far from the call site.
    """
    if not isinstance(topic, str) or not topic:
        raise ValueError(f"publish topic must be a non-empty string, got {topic!r}")
    bad = _WILDCARD_CHARS & set(topic)
    if bad:
        raise ValueError(
            f"publish topic {topic!r} contains MQTT wildcard(s) {sorted(bad)}; "
            "wildcards are only valid for subscription filters"
        )


class _UnixMqttClient(mqtt.Client):
    """paho.Client subclass that connects via Unix Domain Socket.

    paho-mqtt 2.x only accepts transport='tcp' or 'websockets' and rejects
    port=0. This subclass stores the socket path separately and overrides
    _create_socket_connection() to use AF_UNIX regardless of host/port.
    Set _unix_socket_path before calling connect_async().
    """

    _unix_socket_path: str = ""

    def _create_socket_connection(self):  # type: ignore[override]
        if self._unix_socket_path:
            import socket as _socket
            sock = _socket.socket(_socket.AF_UNIX, _socket.SOCK_STREAM)
            if self._connect_timeout:
                sock.settimeout(self._connect_timeout)
            sock.connect(self._unix_socket_path)
            return sock
        return super()._create_socket_connection()


def _rc_to_int(reason_code) -> int:
    """Coerce paho 2.x ReasonCode (or legacy int) to a plain int.

    paho-mqtt 2.x with CallbackAPIVersion.VERSION2 passes a `ReasonCode`
    object that does NOT implement `__int__`. It exposes `.value` (int)
    and `.is_failure` (bool). Tests sometimes pass a raw int instead.
    """
    if reason_code is None:
        return 0
    if hasattr(reason_code, "value"):
        return int(reason_code.value)
    if isinstance(reason_code, int):
        return reason_code
    return int(reason_code)


class MqttClient(ActiveObject):
    """Connection AO for the MPSS MQTT client."""

    def __init__(self, config: MpssConfig, name: str = "MqttClient"):
        super().__init__(name)
        self._config = config
        self._log = logging.getLogger("sml.mpss.mqtt_client")

        self._paho: Optional[mqtt.Client] = None
        self._first_connect_seen = False
        self._failure_count = 0
        self._current_delay_ms = config.reconnect.initial_ms
        self._reconnect_event_uuid: Optional[str] = None
        self._connect_timeout_uuid: Optional[str] = None

        self._stop_complete_event = threading.Event()
        self._subsystems: list = []
        # Subsystem AOs live as long as the process, so "have they been
        # started" is process-wide state rather than a property of the
        # Operational state we happen to be in.
        self._sub_aos_started = False

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    def start(self) -> None:
        """Post Start event; dispatch thread is already running from ctor."""
        self.post_fifo(Event(signal=signals.Start))

    def request_stop(self) -> None:
        """Signal the AO to begin clean shutdown. Idempotent."""
        self.post_fifo(Event(signal=signals.Stop))

    def wait_until_stopped(self, timeout_s: float = 5.0) -> bool:
        return self._stop_complete_event.wait(timeout=timeout_s)

    # --- Sub-AO management -----------------------------------------------

    def register_subsystem(self, ds) -> None:
        """Register a subsystem whose lifetime spans the whole process.

        Subsystems are started once, on the FIRST entry into Operational, and
        stopped once, on the way to process exit -- never on a link flap. See
        `_start_sub_aos` / `_resubscribe_sub_aos`.
        """
        self._subsystems.append(ds)

    def _start_sub_aos(self) -> None:
        """Start every registered subsystem. Called at most once per process.

        A subsystem AO's dispatch thread is joined by its `stop()` and cannot
        be revived, so starting has to be a once-only step decoupled from
        Operational entry/exit -- otherwise the first reconnect would stop the
        subsystems for good and leave their RPC handlers gone (stale retained
        ready=true in the broker + every `*.req` unanswered).
        """
        if self._sub_aos_started:
            return
        self._sub_aos_started = True
        for ds in self._subsystems:
            try:
                # Bind the paho client by identity, not by attribute lookup:
                # `_ensure_paho_client` creates it once and reuses it across
                # reconnects, so these closures stay valid for the process
                # lifetime -- which is exactly as long as the subsystems now
                # hold on to them.
                def _subscribe(topic, _paho=self._paho):
                    if _paho:
                        _paho.subscribe(topic, qos=1)

                def _unsubscribe(topic, _paho=self._paho):
                    if _paho:
                        _paho.unsubscribe(topic)

                def _direct_pub(topic, payload, qos=1, retain=False,
                                _paho=self._paho):
                    if _paho:
                        try:
                            _paho.publish(topic, payload, qos=qos, retain=retain)
                        except Exception as exc:  # noqa: BLE001
                            self._log.warning("direct publish failed: %s", exc)

                ds.start(
                    publish_fn=self.publish,
                    subscribe_fn=_subscribe,
                    unsubscribe_fn=_unsubscribe,
                    direct_publish_fn=_direct_pub,
                )
            except Exception as exc:  # noqa: BLE001
                self._log.error("sub-AO start failed: %s", exc)

    def _resubscribe_sub_aos(self) -> None:
        """Re-establish broker subscriptions and retained state after a
        reconnect, leaving every subsystem AO running.

        This is the Python mirror of the C++ bridge's `Subscribing_St` ->
        `issueAllSubscribes_()`: a link event only rebuilds what the broker
        forgot (the session's subscriptions, and retained topics if the broker
        itself restarted), never the objects that own them. Both sides are
        symmetric as a result.
        """
        for ds in self._subsystems:
            fn = getattr(ds, "resubscribe", None)
            if fn is None:
                # A subsystem that predates the protocol keeps working; it
                # just won't get its subscriptions back after a flap.
                self._log.warning("subsystem %s has no resubscribe(); "
                                  "its subscriptions are not restored",
                                  type(ds).__name__)
                continue
            try:
                fn()
            except Exception as exc:  # noqa: BLE001
                self._log.error("sub-AO resubscribe failed: %s", exc)

    def _stop_sub_aos(self) -> None:
        """Stop every subsystem. Process-exit path only (see `smfn_stopping`)."""
        if not self._sub_aos_started:
            return
        self._sub_aos_started = False
        for ds in reversed(self._subsystems):
            try:
                ds.stop()
            except Exception as exc:  # noqa: BLE001
                self._log.error("sub-AO stop failed: %s", exc)

    # --- Public API ------------------------------------------------------

    def publish(self, topic: str, payload: bytes, qos: int = 1,
                retain: bool = False) -> None:
        """Validate `topic` and enqueue a Publish event for the AO."""
        validate_publish_topic(topic)
        self.post_fifo(
            Event(
                signal=signals.Publish,
                payload=PublishPayload(topic=topic, payload=payload,
                                       qos=qos, retain=retain),
            )
        )

    # --- Helpers invoked from state handlers -----------------------------

    def _ensure_paho_client(self) -> None:
        if self._paho is not None:
            return
        broker = self._config.broker
        if broker.transport == "uds":
            client = _UnixMqttClient(
                callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
                client_id=broker.client_id,
                reconnect_on_failure=False,
            )
            client._unix_socket_path = broker.socket_path
        else:
            client = mqtt.Client(
                callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
                client_id=broker.client_id,
                reconnect_on_failure=False,
            )
        client.on_connect = self._cb_on_connect
        client.on_disconnect = self._cb_on_disconnect
        client.on_message = self._cb_on_message
        client.on_subscribe = self._cb_on_subscribe
        self._paho = client

    def _initiate_connect(self) -> None:
        assert self._paho is not None
        broker = self._config.broker
        if broker.transport == "uds":
            self._log.debug("connect_async uds path=%s", broker.socket_path)
            host, port = broker.host, broker.port  # paho validates these; socket overridden
        else:
            self._log.debug("connect_async host=%s port=%d", broker.host, broker.port)
            host, port = broker.host, broker.port
        try:
            self._reap_dead_paho_loop_thread()
            self._paho.connect_async(host, port, broker.keepalive_s)
            self._paho.loop_start()
        except (OSError, socket.gaierror) as exc:
            self._log.warning("connect_async raised %s; will rely on backoff", exc)
            self.post_fifo(
                Event(signal=signals.LinkDown,
                      payload=LinkDownPayload(rc=-1))
            )

    def _reap_dead_paho_loop_thread(self) -> None:
        """Clear paho's stale `_thread` so the next `loop_start()` can spawn one.

        paho 2.x lets `loop_forever` return when the link drops (we run with
        `reconnect_on_failure=False`, so its own retry loop is off), but only
        `loop_stop()` sets `_thread` back to None. `loop_start()` answers
        MQTT_ERR_INVAL instead of spawning a replacement whenever it finds a
        non-None `_thread`, and its return value is not raised -- so a reconnect
        would look issued while no thread ever flushes the queued CONNECT, and
        every attempt would die on the connect timeout (rc=-2) forever.

        Only a thread that has already finished is reaped: `loop_stop()` joins,
        and this runs on the AO dispatch thread, so calling it on a live loop
        would stall every other event for as long as that loop keeps running.
        """
        paho = self._paho
        thread = getattr(paho, "_thread", None)
        if thread is None or thread.is_alive():
            return
        try:
            paho.loop_stop()
        except Exception as exc:  # noqa: BLE001 - best-effort bookkeeping
            self._log.warning("loop_stop while reaping dead loop thread: %s", exc)

    def _initiate_clean_disconnect(self) -> None:
        if self._paho is None:
            self._stop_complete_event.set()
            return
        try:
            self._paho.disconnect()
            self._paho.loop_stop()
        except Exception as exc:    # noqa: BLE001 - shutdown best-effort
            self._log.warning("disconnect during stop raised: %s", exc)
            self._stop_complete_event.set()

    def _do_publish(self, payload: PublishPayload) -> None:
        if self._paho is None:
            self._log.warning("publish requested but paho client absent; dropping")
            return
        self._paho.publish(payload.topic, payload.payload,
                           qos=payload.qos, retain=payload.retain)

    def _on_message(self, payload: MessageReceivedPayload) -> None:
        # `handle_message` on subsystems is fire-and-forget --
        # its old `bool` return is gone.  Route based on `owns_topic`,
        # which is synchronous and lock-free (each subsystem exposes a
        # frozen set fixed at start).
        for ds in self._subsystems:
            try:
                if ds.owns_topic(payload.topic):
                    ds.handle_message(payload.topic, payload.payload)
                    return
            except Exception as exc:  # noqa: BLE001
                self._log.error("sub-AO owns_topic raised on %s: %s",
                                payload.topic, exc)
        self._log.debug("inbound message on %s (%d bytes, qos=%d) not consumed",
                        payload.topic, len(payload.payload), payload.qos)

    # --- Backoff bookkeeping --------------------------------------------

    def _record_connect_failure(self, rc: int) -> None:
        self._failure_count += 1
        broker = self._config.broker
        endpoint = broker.socket_path if broker.transport == "uds" else f"{broker.host}:{broker.port}"
        if self._failure_count <= _FIRST_CONNECT_INFO_LIMIT and not self._first_connect_seen:
            self._log.info("connect attempt %d failed (rc=%d) — retrying",
                           self._failure_count, rc)
        else:
            self._log.error(
                "connect attempt %d failed (rc=%d) — broker=%s unreachable; "
                "check sml/mpss/config.yaml",
                self._failure_count, rc, endpoint,
            )

    def _reset_backoff(self) -> None:
        self._failure_count = 0
        self._current_delay_ms = self._config.reconnect.initial_ms
        self._first_connect_seen = True

    def _compute_next_delay_ms(self) -> int:
        """Pure function of failure_count and config; also updates _current_delay_ms.

        delay = min(initial * multiplier^(failures-1), max), then optional jitter.
        """
        cfg = self._config.reconnect
        exponent = max(0, self._failure_count - 1)
        base = cfg.initial_ms * (cfg.multiplier ** exponent)
        capped = min(int(base), cfg.max_ms)
        if cfg.jitter_pct > 0.0:
            lo = 1.0 - cfg.jitter_pct
            hi = 1.0 + cfg.jitter_pct
            capped = int(capped * random.uniform(lo, hi))
        self._current_delay_ms = capped
        return capped

    # --- Reconnect timer -------------------------------------------------

    def _schedule_reconnect_timer(self) -> None:
        delay_ms = self._compute_next_delay_ms()
        self._log.info("scheduling reconnect in %d ms (failure_count=%d)",
                       delay_ms, self._failure_count)
        ev = Event(signal=signals.ReconnectTimer)
        self._reconnect_event_uuid = self.post_fifo(
            ev, times=1, period=delay_ms / 1000.0, deferred=True
        )

    def _cancel_reconnect_timer(self) -> None:
        if self._reconnect_event_uuid is None:
            return
        try:
            self.cancel_event(self._reconnect_event_uuid)
        except Exception:   # noqa: BLE001 - miros may have no such uuid post-fire
            pass
        self._reconnect_event_uuid = None

    # --- Connect timeout (Connecting state guard) ------------------------

    def _schedule_connect_timeout(self) -> None:
        """Guard against paho's silent TCP retry loop hanging us in
        Connecting forever when the broker is fully down."""
        timeout_ms = self._config.reconnect.connect_timeout_ms
        if timeout_ms <= 0:
            return
        self._log.debug("scheduling connect timeout in %d ms", timeout_ms)
        self._connect_timeout_uuid = self.post_fifo(
            Event(signal=signals.ConnectTimeout),
            times=1, period=timeout_ms / 1000.0, deferred=True,
        )

    def _cancel_connect_timeout(self) -> None:
        if self._connect_timeout_uuid is None:
            return
        try:
            self.cancel_event(self._connect_timeout_uuid)
        except Exception:   # noqa: BLE001
            pass
        self._connect_timeout_uuid = None

    # --- paho callbacks (CALLBACK-ONLY-POST DISCIPLINE) ------------------

    def _cb_on_connect(self, client, userdata, flags, reason_code, properties):
        rc = _rc_to_int(reason_code)
        self.post_fifo(
            Event(signal=signals.LinkUp,
                  payload=LinkUpPayload(rc=rc))
        )

    def _cb_on_disconnect(self, client, userdata, disconnect_flags,
                          reason_code, properties):
        rc = _rc_to_int(reason_code)
        self.post_fifo(
            Event(signal=signals.LinkDown,
                  payload=LinkDownPayload(rc=rc))
        )
        if rc == 0:
            self.post_fifo(Event(signal=signals.DisconnectedAck))

    def _cb_on_message(self, client, userdata, msg):
        self.post_fifo(
            Event(
                signal=signals.MessageReceived,
                payload=MessageReceivedPayload(
                    topic=msg.topic, payload=bytes(msg.payload), qos=msg.qos
                ),
            )
        )

    def _cb_on_subscribe(self, client, userdata, mid, reason_code_list, properties):
        # No-op: we don't track subscribe mids yet. Future change may.
        pass


# ---------------------------------------------------------------------------
# HSM state handlers.
#
# Each `smfn_*` function is a miros state handler with signature
# `(chart, e) -> return_status`. `chart` is the `MqttClient` instance above.
#
# Hierarchy (declared via parent-handler references in each smfn_*):
#
#     top
#     ├── smfn_off
#     ├── smfn_connecting
#     ├── smfn_connected
#     │   └── smfn_operational
#     ├── smfn_disconnected
#     │   └── smfn_reconnecting
#     └── smfn_stopping
#
# Hard rules enforced here:
#     * No business logic in paho callbacks — handlers see the AO event,
#       never the raw paho callback arguments.
#     * `Stopping` MUST NOT transition into `Reconnecting`, even if paho
#       emits an unsolicited disconnect during shutdown.
# ---------------------------------------------------------------------------

def _log_transition(chart, src: str, dst: str) -> None:
    chart._log.info("state: %s -> %s", src, dst)


# --- smfn_off -------------------------------------------------------------


@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._log.debug("entered Off")
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        _log_transition(chart, "Off", "Connecting")
        status = chart.trans(smfn_connecting)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


# --- smfn_connecting ------------------------------------------------------


@spy_on
def smfn_connecting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._ensure_paho_client()
        chart._initiate_connect()
        chart._schedule_connect_timeout()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._cancel_connect_timeout()
        status = return_status.HANDLED
    elif e.signal == signals.LinkUp:
        payload: LinkUpPayload = e.payload
        if payload.rc == 0:
            chart._reset_backoff()
            _log_transition(chart, "Connecting", "Connected.Operational")
            status = chart.trans(smfn_operational)
        else:
            chart._record_connect_failure(payload.rc)
            _log_transition(chart, "Connecting", "Reconnecting")
            status = chart.trans(smfn_reconnecting)
    elif e.signal == signals.LinkDown:
        chart._record_connect_failure(rc=-1)
        _log_transition(chart, "Connecting", "Reconnecting")
        status = chart.trans(smfn_reconnecting)
    elif e.signal == signals.ConnectTimeout:
        chart._record_connect_failure(rc=-2)
        _log_transition(chart, "Connecting", "Reconnecting")
        status = chart.trans(smfn_reconnecting)
    elif e.signal == signals.Stop:
        _log_transition(chart, "Connecting", "Stopping")
        status = chart.trans(smfn_stopping)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


# --- smfn_connected (parent of smfn_operational) --------------------------


@spy_on
def smfn_connected(chart, e):
    """Parent of operational substates. Handles LinkDown/Stop transitions."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.LinkDown:
        payload = e.payload if e.payload is not None else LinkDownPayload(rc=-1)
        chart._record_connect_failure(rc=payload.rc)
        _log_transition(chart, "Connected", "Disconnected.Reconnecting")
        status = chart.trans(smfn_reconnecting)
    elif e.signal == signals.Stop:
        _log_transition(chart, "Connected", "Stopping")
        status = chart.trans(smfn_stopping)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


# --- smfn_operational (substate of smfn_connected) ------------------------


@spy_on
def smfn_operational(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._log.debug("entered Operational")
        # First entry starts the subsystems; every later one (i.e. after a
        # reconnect) only rebuilds subscriptions and retained state. Stopping a
        # subsystem joins its dispatch thread, and a joined AO cannot be
        # revived -- so tearing them down on a link flap used to kill them
        # permanently (fake retained ready=true, RPCs unanswered forever).
        # Mirrors the C++ bridge, where Managers outlive every reconnect and
        # Subscribing_St just re-runs issueAllSubscribes_().
        if chart._sub_aos_started:
            chart._resubscribe_sub_aos()
        else:
            chart._start_sub_aos()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        # Deliberately does NOT stop the subsystems: leaving Operational means
        # the link went down, not that the process is going away. Teardown
        # belongs to smfn_stopping.
        status = return_status.HANDLED
    elif e.signal == signals.Publish:
        payload: PublishPayload = e.payload
        chart._do_publish(payload)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        payload: MessageReceivedPayload = e.payload
        chart._on_message(payload)
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_connected
        status = return_status.SUPER
    return status


# --- smfn_disconnected (parent of smfn_reconnecting) ----------------------


@spy_on
def smfn_disconnected(chart, e):
    """Neutral 'offline' container. Behavioural reconnect logic lives in
    `smfn_reconnecting` substate; this state is what `Stopping` leaves
    us in transiently during clean shutdown."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Stop:
        _log_transition(chart, "Disconnected", "Stopping")
        status = chart.trans(smfn_stopping)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


# --- smfn_reconnecting (substate of smfn_disconnected) --------------------


@spy_on
def smfn_reconnecting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._schedule_reconnect_timer()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._cancel_reconnect_timer()
        status = return_status.HANDLED
    elif e.signal == signals.ReconnectTimer:
        _log_transition(chart, "Reconnecting", "Connecting")
        status = chart.trans(smfn_connecting)
    else:
        chart.temp.fun = smfn_disconnected
        status = return_status.SUPER
    return status


# --- smfn_stopping --------------------------------------------------------


@spy_on
def smfn_stopping(chart, e):
    """Clean shutdown. Reconnect attempts are forbidden from here even
    if paho emits an unsolicited LinkDown during disconnect."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        # The only place subsystems are stopped: this is the process-exit
        # path, so joining their dispatch threads is what we want. Ordered
        # before the disconnect so each subsystem's retained ready=false --
        # published synchronously through direct_publish_fn -- still reaches
        # the broker over a live connection.
        chart._stop_sub_aos()
        chart._initiate_clean_disconnect()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.DisconnectedAck:
        chart._log.info("clean disconnect acknowledged, stopping AO")
        chart._stop_complete_event.set()
        chart.stop()
        status = return_status.HANDLED
    elif e.signal == signals.LinkDown:
        chart._stop_complete_event.set()
        chart.stop()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = [
    "LinkDownPayload",
    "LinkUpPayload",
    "MessageReceivedPayload",
    "MqttClient",
    "PublishPayload",
    "validate_publish_topic",
]
