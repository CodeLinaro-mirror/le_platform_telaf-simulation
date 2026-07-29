# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side data connection Active Object.

- ``IfnamePool`` -- pool of simulated interface names (no state machine).
- ``CallSession`` -- per-call sub-chart (Idle -> Connecting -> Connected ->
  Disconnecting -> Terminal).  Driven synchronously by the owner
  ``DataConnectionAO``'s thread; constructs an ``HsmWithQueues`` chart
  with no thread of its own.
- ``ThroughputReporter`` -- Off/On sub-chart, same threading discipline as
  ``CallSession``.  Its periodic tick is scheduled through the owner
  AO's fifo via ``post_fifo(period=..., deferred=True)``, NOT a
  ``threading.Timer``.
- ``DataConnectionAO`` -- Off -> Operating(Starting -> Ready) -> Stopping,
  its own miros ``ActiveObject`` thread.

All state mutation for ``DataConnectionAO`` happens on the AO thread.
Public entry points (``start`` / ``stop`` / ``force_*`` /
``handle_message`` / ``on_network_rat_changed``) post events into the
fifo; no lock around ``_sessions`` (only the AO thread reads or writes).
"""
from __future__ import annotations

import json
import logging
import subprocess
from typing import Callable, Dict, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on
from miros.hsm import HsmWithQueues

from sml.mpss import instrumentation as _instr
from sml.mpss.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from sml.config.models import CallTimingPresetSeed, InterfacePresetSeed, IpConfigSeed
from generated.python.topics import data as topics_data
from generated.python.validators import validate as validate_payload


_log = logging.getLogger("sml.mpss.data.connection")

RAT_TO_BEARER_TECH = {
    "GSM": "GSM",
    "WCDMA": "WCDMA",
    "LTE": "LTE",
    "NR5G": "BEARER_TECH_5G",
    "CDMA_1X": "CDMA_1X",
    "CDMA_EVDO": "EVDO_REV0",
    "UNKNOWN": "UNKNOWN",
}

END_REASON_CLEAN_STOP = {"type": 0x06, "code": 36}
END_REASON_NETWORK_DROP = {"type": 0x02, "code": 206}


# ---------------------------------------------------------------------------
# IfnamePool
# ---------------------------------------------------------------------------

class IfnamePool:
    """Pool of simulated network interface names."""

    def __init__(self, prefix: str, pool_size: int) -> None:
        self._prefix = prefix
        self._pool_size = pool_size
        self._in_use: set[str] = set()

    def allocate(self, requested: str = "") -> Optional[str]:
        if requested:
            if requested in self._in_use:
                return None
            self._in_use.add(requested)
            return requested
        for i in range(self._pool_size):
            name = f"{self._prefix}{i}"
            if name not in self._in_use:
                self._in_use.add(name)
                return name
        return None

    def release(self, ifname: str) -> None:
        self._in_use.discard(ifname)

    def is_in_use(self, ifname: str) -> bool:
        return ifname in self._in_use


# ---------------------------------------------------------------------------
# CallSession -- per-call sub-chart, driven synchronously by owner AO thread.
# ---------------------------------------------------------------------------

_STATUS_BY_STATE_NAME = {
    "smfn_session_idle":          "IDLE",
    "smfn_session_connecting":    "CONNECTING",
    "smfn_session_connected":     "CONNECTED",
    "smfn_session_disconnecting": "DISCONNECTING",
    "smfn_session_terminal":      "NO_NET",
}


class CallSession(HsmWithQueues):
    """Idle -> Connecting -> Connected -> Disconnecting -> Terminal.

    Threadless sub-chart: constructed and driven from ``DataConnectionAO``'s
    own thread via ``HsmWithQueues.start_at`` + ``dispatch()``.
    """

    def __init__(self, profile_id: int, slot: int, ifname: str,
                ip_family: str, op_type: str) -> None:
        super().__init__(f"CallSession-{profile_id}")
        self.profile_id = profile_id
        self.slot = slot
        self.ifname = ifname
        self.ip_family = ip_family
        self.op_type = op_type
        # uuid returned by the owner AO's post_fifo(..., deferred=True);
        # kept so DataConnectionAO can cancel_event() when the call is
        # torn down before the connect/disconnect timer fires.
        self.timer_uuid: Optional[str] = None
        self.ipv4: Optional[dict] = None
        self.ipv6: Optional[dict] = None
        self.bearer_tech: Optional[str] = None
        self.end_reason: Optional[dict] = None

        HsmWithQueues.start_at(self, smfn_session_idle)

    @property
    def status(self) -> str:
        return _STATUS_BY_STATE_NAME[self.state_fn.__name__]

    def to_data_call_state(self) -> dict:
        out = {
            "profileId": self.profile_id,
            "ifname":    self.ifname,
            "ipFamily":  self.ip_family,
            "status":    self.status,
            "slot":      self.slot,
        }
        if self.bearer_tech is not None:
            out["bearer_tech"] = self.bearer_tech
        if self.end_reason is not None:
            out["end_reason"] = self.end_reason
        if self.ipv4 is not None:
            out["ipv4"] = self.ipv4
        if self.ipv6 is not None:
            out["ipv6"] = self.ipv6
        return out

    def to_list_item(self) -> dict:
        out = {
            "profileId": self.profile_id,
            "ifname":    self.ifname,
            "ipFamily":  self.ip_family,
            "status":    self.status,
            "slot":      self.slot,
        }
        if self.ipv4 is not None:
            out["ipv4"] = {k: v for k, v in self.ipv4.items() if k != "subnet_mask"}
        if self.ipv6 is not None:
            out["ipv6"] = {k: v for k, v in self.ipv6.items() if k != "prefix_len"}
        return out

    def begin_connecting(self) -> None:
        self.dispatch(Event(signal=signals.SessionConnect))

    def connected(self, ipv4: Optional[dict], ipv6: Optional[dict], bearer_tech: str) -> None:
        self.ipv4 = ipv4
        self.ipv6 = ipv6
        self.bearer_tech = bearer_tech
        self.dispatch(Event(signal=signals.SessionConnected))

    def update_bearer_tech(self, bearer_tech: str) -> None:
        self.bearer_tech = bearer_tech

    def begin_disconnecting(self) -> None:
        self.end_reason = END_REASON_CLEAN_STOP
        self.dispatch(Event(signal=signals.SessionDisconnect))

    def disconnected(self) -> None:
        self.bearer_tech = None
        self.dispatch(Event(signal=signals.SessionTerminate))

    def force_drop(self) -> None:
        self.end_reason = END_REASON_NETWORK_DROP
        self.bearer_tech = None
        self.dispatch(Event(signal=signals.SessionForceDrop))


@spy_on
def smfn_session_idle(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SessionConnect:
        status = chart.trans(smfn_session_connecting)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_session_connecting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SessionConnected:
        status = chart.trans(smfn_session_connected)
    elif e.signal == signals.SessionForceDrop:
        status = chart.trans(smfn_session_terminal)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_session_connected(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SessionDisconnect:
        status = chart.trans(smfn_session_disconnecting)
    elif e.signal == signals.SessionForceDrop:
        status = chart.trans(smfn_session_terminal)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_session_disconnecting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SessionTerminate:
        status = chart.trans(smfn_session_terminal)
    elif e.signal == signals.SessionForceDrop:
        status = chart.trans(smfn_session_terminal)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_session_terminal(chart, e):
    """No outgoing transitions."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


# ---------------------------------------------------------------------------
# ThroughputReporter -- threadless sub-chart.  Periodic tick is scheduled
# by the owner ``DataConnectionAO`` via its own ``post_fifo(period=...,
# deferred=True)`` which delivers ``SIG_THROUGHPUT_TICK`` to the AO
# fifo; the AO handler forwards ``publish_fn()`` calls back here.
# ---------------------------------------------------------------------------

class ThroughputReporter(HsmWithQueues):
    """Off -> On, driven by set_throughput_interval (intervalMs=0 disables).

    Threadless sub-chart owned by ``DataConnectionAO``.  The owner AO
    schedules ticks through its own fifo (miros' periodic ``post_fifo``);
    ``interval_ms`` and ``timer_uuid`` are read/written only on the AO
    thread (no lock).
    """

    def __init__(self, publish_fn: Callable[[], None],
                 schedule_fn: Callable[[int], Optional[str]],
                 cancel_fn: Callable[[Optional[str]], None]) -> None:
        super().__init__("ThroughputReporter")
        self._publish_fn = publish_fn
        self._schedule_fn = schedule_fn  # (interval_ms) -> uuid or None
        self._cancel_fn = cancel_fn      # (uuid) -> None
        self.interval_ms = 0
        self.timer_uuid: Optional[str] = None

        HsmWithQueues.start_at(self, smfn_throughput_off)

    @property
    def is_on(self) -> bool:
        return self.state_fn.__name__ == "smfn_throughput_on"

    def enable(self, interval_ms: int) -> None:
        self.interval_ms = interval_ms
        self.dispatch(Event(signal=signals.ThroughputEnable))

    def disable(self) -> None:
        self.dispatch(Event(signal=signals.ThroughputDisable))

    def _start_timer(self) -> None:
        self._cancel_timer()
        self.timer_uuid = self._schedule_fn(self.interval_ms)

    def _cancel_timer(self) -> None:
        if self.timer_uuid is not None:
            self._cancel_fn(self.timer_uuid)
            self.timer_uuid = None


@spy_on
def smfn_throughput_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.ThroughputEnable:
        status = chart.trans(smfn_throughput_on)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_throughput_on(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._start_timer()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._cancel_timer()
        status = return_status.HANDLED
    elif e.signal == signals.ThroughputDisable:
        status = chart.trans(smfn_throughput_off)
    elif e.signal == signals.ThroughputEnable:
        chart._start_timer()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


# ---------------------------------------------------------------------------
# DataConnectionAO
# ---------------------------------------------------------------------------

class DataConnectionAO(ActiveObject):
    """MPSS-side data connection manager — own AO thread.

    All ``_sessions`` mutation happens on the AO thread.  Timers are
    scheduled via ``post_fifo(period=..., deferred=True)`` which delivers
    a signal to the AO fifo when it fires (no ``threading.Timer``, no
    ``_lock``).  Public entry points post events and return; state
    changes happen asynchronously on the AO thread.
    """

    def __init__(
        self,
        slot: int,
        interface_preset: InterfacePresetSeed,
        call_timing_preset: CallTimingPresetSeed,
        ip_config: IpConfigSeed,
        mpss_src: str,
        bitrate_by_rat: Optional[Dict[str, tuple]] = None,
        throughput_presets: Optional[Dict[int, dict]] = None,
        qos_presets: Optional[Dict[int, dict]] = None,
        throttle_presets: Optional[Dict[int, dict]] = None,
        get_network_rat_fn: Optional[Callable[[], str]] = None,
    ) -> None:
        super().__init__("DataConnectionAO")
        self._slot = slot
        self._interface_preset = interface_preset
        self._call_timing_preset = call_timing_preset
        self._ip_config = ip_config
        self._mpss_src = mpss_src
        self._bitrate_by_rat = bitrate_by_rat or {}
        self._throughput_by_profile: Dict[int, dict] = dict(throughput_presets or {})
        self._qos_by_profile: Dict[int, dict] = dict(qos_presets or {})
        self._throttle_by_profile: Dict[int, dict] = dict(throttle_presets or {})
        self._get_network_rat_fn = get_network_rat_fn or (lambda: "UNKNOWN")

        # AO-thread-only; no lock (only the AO thread mutates).
        self._sessions: Dict[int, CallSession] = {}
        self._pool: Optional[IfnamePool] = None
        # ThroughputReporter schedules its ticks through this AO's own
        # periodic post_fifo -- the tick payload holds the interval so
        # the AO can re-schedule after each fire.
        self._throughput = ThroughputReporter(
            publish_fn=self._publish_throughput_info,
            schedule_fn=self._schedule_throughput_tick,
            cancel_fn=self._cancel_scheduled,
        )

        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (posts events; returns immediately)
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Optional[Callable] = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Posted by ``DataSubsystem.resubscribe`` after an MQTT reconnect. Live
        call sessions and their timers are untouched -- a broker flap is not a
        data-call teardown. Mirrors the C++ bridge's ``Subscribing_St`` ->
        ``issueAllSubscribes_()``.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    def force_call_drop(self, profile_id: int) -> None:
        self.post_fifo(Event(signal=signals.ForceCallDrop, payload=profile_id))

    def force_throughput(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceThroughput, payload=data))

    def force_qos(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceQos, payload=data))

    def force_hw_accel(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceHwAccel, payload=data))

    def force_throttle(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceThrottle, payload=data))

    def on_network_rat_changed(self, network_rat: str) -> None:
        self.post_fifo(Event(signal=signals.NetworkRatChanged, payload=network_rat))

    # ------------------------------------------------------------------
    # Timer helpers -- all rooted at post_fifo(period=..., deferred=True).
    # ------------------------------------------------------------------

    def _schedule_call_connect(self, profile_id: int) -> Optional[str]:
        delay_s = self._call_timing_preset.call_connect_delay_ms / 1000.0
        if delay_s <= 0:
            # Post synchronously into fifo, non-deferred; caller's
            # dispatch() returns first, then the AO drains this event.
            self.post_fifo(Event(signal=signals.CallConnectTimeout, payload=profile_id))
            return None
        return self.post_fifo(
            Event(signal=signals.CallConnectTimeout, payload=profile_id),
            times=1, period=delay_s, deferred=True,
        )

    def _schedule_call_disconnect(self, profile_id: int) -> Optional[str]:
        delay_s = self._call_timing_preset.call_disconnect_delay_ms / 1000.0
        if delay_s <= 0:
            self.post_fifo(Event(signal=signals.CallDisconnectTimeout, payload=profile_id))
            return None
        return self.post_fifo(
            Event(signal=signals.CallDisconnectTimeout, payload=profile_id),
            times=1, period=delay_s, deferred=True,
        )

    def _schedule_throughput_tick(self, interval_ms: int) -> Optional[str]:
        # Periodic (times=0 -> infinite) so the AO fifo receives one
        # SIG_THROUGHPUT_TICK every ``interval_ms``.
        period_s = interval_ms / 1000.0
        if period_s <= 0:
            return None
        return self.post_fifo(
            Event(signal=signals.ThroughputTick),
            period=period_s, deferred=True,
        )

    def _cancel_scheduled(self, uuid: Optional[str]) -> None:
        if uuid is not None:
            try:
                self.cancel_event(uuid=uuid)
            except Exception as exc:  # noqa: BLE001
                _log.debug("cancel_event(%s) raised: %s", uuid, exc)

    # ------------------------------------------------------------------
    # State-handler helpers (run on the AO thread)
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._pool = IfnamePool(self._interface_preset.ifname_prefix,
                                self._interface_preset.ifname_pool_size)

        mapping = {
            topics_data.start_data_call.req: self._handle_start,
            topics_data.stop_data_call.req:  self._handle_stop,
            topics_data.list_data_call.req:  self._handle_list,
            topics_data.request_data_call_bitrate.req: self._handle_request_bitrate,
            topics_data.set_throughput_interval.req:   self._handle_set_throughput_interval,
            topics_data.request_throughput_info.req:   self._handle_request_throughput_info,
            topics_data.request_throttle_status.req:   self._handle_request_throttle_status,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("connection AO subscribed (slot=%d)", self._slot)

    def _do_stop(self) -> None:
        # Cancel throughput reporter (moves it Off; cancels its scheduled tick).
        self._throughput.disable()
        sessions_to_clean = list(self._sessions.values())
        for session in sessions_to_clean:
            self._cancel_scheduled(session.timer_uuid)
            session.timer_uuid = None
        self._sessions.clear()
        for session in sessions_to_clean:
            if session.status in ("CONNECTED", "DISCONNECTING"):
                self._teardown_iface(session.ifname)
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "connection AO")

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish retained ready.

        Deliberately does NOT touch ``_sessions``, their timers, or the
        throughput reporter: a broker flap is not a data-call teardown. The
        retained ``ready=true`` is re-sent in case the broker itself restarted
        (a restarted broker forgot the retained set, not just our session).
        Live call state is carried on non-retained ``call_state.ind``; a PA that
        reconnects re-syncs via ``list_data_call.req``.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._publish_subsys_ready(ready=True)
        _log.info("connection AO resubscribed (slot=%d)", self._slot)

    # ------------------------------------------------------------------
    # RPC handlers (AO thread only; direct _sessions mutation is safe)
    # ------------------------------------------------------------------

    def _handle_start(self, msg: dict) -> None:
        data = msg.get("data") or {}
        profile_id = data.get("profileId")
        ip_family = data.get("ipFamily", "IPV4V6")
        requested_ifname = data.get("ifname", "")
        op_type = data.get("opType", "DATA_LOCAL")

        existing = self._sessions.get(profile_id)
        if existing is not None:
            s = existing.status
            if s in ("CONNECTING", "DISCONNECTING"):
                self._send_error(topics_data.start_data_call.rsp, msg, "OP_IN_PROGRESS")
                return
            if s == "CONNECTED":
                if requested_ifname and requested_ifname != existing.ifname:
                    self._send_error(topics_data.start_data_call.rsp, msg, "DEVICE_IN_USE")
                else:
                    env = build_success_envelope(
                        self._mpss_src, msg["corrId"], msg["src"],
                        {"ifname": existing.ifname, "status": "CONNECTING"}
                    )
                    self._pub_rsp(topics_data.start_data_call.rsp, "data.start_data_call.rsp", env)
                return

        if requested_ifname and self._pool.is_in_use(requested_ifname):
            self._send_error(topics_data.start_data_call.rsp, msg, "INVALID_OPERATION",
                             f"ifname {requested_ifname} in use by another call")
            return

        assigned = self._pool.allocate(requested_ifname)
        if assigned is None:
            self._send_error(topics_data.start_data_call.rsp, msg, "NO_RESOURCES", "ifname pool exhausted")
            return

        session = CallSession(
            profile_id=profile_id,
            slot=self._slot,
            ifname=assigned,
            ip_family=ip_family,
            op_type=op_type,
        )
        session.begin_connecting()
        self._sessions[profile_id] = session

        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"],
            {"ifname": assigned, "status": "CONNECTING"}
        )
        self._pub_rsp(topics_data.start_data_call.rsp, "data.start_data_call.rsp", env)
        self._publish_call_state(session)

        session.timer_uuid = self._schedule_call_connect(profile_id)

    def _handle_stop(self, msg: dict) -> None:
        data = msg.get("data") or {}
        profile_id = data.get("profileId")
        session = self._sessions.get(profile_id)
        if session is None or session.status == "NO_NET":
            self._send_error(topics_data.stop_data_call.rsp, msg, "INVALID_ARGUMENTS",
                             f"no active session for profileId={profile_id}")
            return
        self._cancel_scheduled(session.timer_uuid)
        session.timer_uuid = None
        session.begin_disconnecting()

        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"], {"status": "DISCONNECTING"}
        )
        self._pub_rsp(topics_data.stop_data_call.rsp, "data.stop_data_call.rsp", env)
        self._publish_call_state(session)

        session.timer_uuid = self._schedule_call_disconnect(profile_id)

    def _handle_list(self, msg: dict) -> None:
        calls = [
            s.to_list_item()
            for s in self._sessions.values()
            if s.status != "NO_NET"
        ]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {"calls": calls})
        self._pub_rsp(topics_data.list_data_call.rsp, "data.list_data_call.rsp", env)

    def _handle_request_bitrate(self, msg: dict) -> None:
        data = msg.get("data") or {}
        profile_id = data.get("profileId")
        session = self._sessions.get(profile_id)
        active = session is not None and session.status == "CONNECTED"
        if not active:
            self._send_error(topics_data.request_data_call_bitrate.rsp, msg,
                             "INVALID_STATE", f"no active call for profileId={profile_id}")
            return
        max_tx, max_rx = self._bitrate_by_rat.get(self._get_network_rat_fn(), (0, 0))
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"],
            {"maxTxRate": max_tx, "maxRxRate": max_rx}
        )
        self._pub_rsp(topics_data.request_data_call_bitrate.rsp, "data.request_data_call_bitrate.rsp", env)

    def _handle_set_throughput_interval(self, msg: dict) -> None:
        data = msg.get("data") or {}
        interval_ms = data.get("intervalMs", 0)
        if interval_ms > 0:
            self._throughput.enable(interval_ms)
        else:
            self._throughput.disable()
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_data.set_throughput_interval.rsp, "data.set_throughput_interval.rsp", env)

    def _handle_request_throughput_info(self, msg: dict) -> None:
        infos = self._collect_throughput_infos()
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {"infos": infos})
        self._pub_rsp(topics_data.request_throughput_info.rsp, "data.request_throughput_info.rsp", env)

    def _handle_request_throttle_status(self, msg: dict) -> None:
        data = msg.get("data") or {}
        profile_id = data.get("profileId")
        preset = self._throttle_by_profile.get(profile_id)
        if preset is None:
            self._send_error(topics_data.request_throttle_status.rsp, msg,
                             "INVALID_ARGUMENTS", f"no throttle info for profileId={profile_id}")
            return
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"],
            {"apn": preset["apn"], "profileIds": [profile_id], "isBlocked": preset.get("isBlocked", False),
             "ipv4Time": preset["ipv4Time"], "ipv6Time": preset["ipv6Time"],
             "mcc": preset["mcc"], "mnc": preset["mnc"]}
        )
        self._pub_rsp(topics_data.request_throttle_status.rsp, "data.request_throttle_status.rsp", env)

    # ------------------------------------------------------------------
    # World State mutators -- run on AO thread via SIG_FORCE_* events.
    # ------------------------------------------------------------------

    def _apply_force_call_drop(self, profile_id: int) -> None:
        session = self._sessions.pop(profile_id, None)
        if session is None or session.status == "NO_NET":
            _log.warning("force_call_drop: no active session for profileId=%d", profile_id)
            return
        self._cancel_scheduled(session.timer_uuid)
        session.timer_uuid = None
        session.force_drop()
        self._pool.release(session.ifname)
        self._teardown_iface(session.ifname)
        self._publish_call_state(session)
        _log.info("call profileId=%d force-dropped (ifname=%s)", profile_id, session.ifname)

    def _apply_force_throughput(self, data: dict) -> None:
        profile_id = data["profileId"]
        current = self._throughput_by_profile.get(profile_id, {
            "ulThroughput": 0, "ulMaxThroughput": 0, "ulQueueSize": 0, "dlThroughput": 0,
        })
        updated = dict(current)
        for wire_key, data_key in (
            ("ulThroughput", "ulThroughput"), ("ulMaxThroughput", "ulMaxThroughput"),
            ("ulQueueSize", "ulQueueSize"), ("dlThroughput", "dlThroughput"),
        ):
            if data_key in data:
                updated[wire_key] = data[data_key]
        self._throughput_by_profile[profile_id] = updated
        if self._throughput.is_on and self._publish_fn is not None:
            self._publish_throughput_info()

    def _apply_force_qos(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        payload = {
            "profileId": data["profileId"],
            "qosId": data["qosId"],
            "stateChange": data["stateChange"],
            "mask": data.get("mask", 0),
        }
        preset = self._qos_by_profile.get(data["profileId"])
        if preset is not None and data["stateChange"] in ("ACTIVATED", "MODIFIED"):
            payload["txGranted"] = {"maxRate": preset["txMaxRate"], "minRate": preset["txMinRate"]}
            payload["rxGranted"] = {"maxRate": preset["rxMaxRate"], "minRate": preset["rxMinRate"]}
        self._pub_ind(topics_data.qos_status.ind, "data.qos_status.ind", payload)

    def _apply_force_hw_accel(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        self._pub_ind(topics_data.hw_accel_state.ind, "data.hw_accel_state.ind", {"state": data["state"]})

    def _apply_force_throttle(self, data: dict) -> None:
        profile_id = data["profileId"]
        preset = self._throttle_by_profile.get(profile_id, {
            "apn": "", "ipv4Time": 0, "ipv6Time": 0, "mcc": "", "mnc": "",
        })
        updated = dict(preset)
        updated["isBlocked"] = data["isBlocked"]
        if "ipv4Time" in data:
            updated["ipv4Time"] = data["ipv4Time"]
        if "ipv6Time" in data:
            updated["ipv6Time"] = data["ipv6Time"]
        self._throttle_by_profile[profile_id] = updated
        if self._publish_fn is None:
            return
        self._pub_ind(topics_data.throttle_status.ind, "data.throttle_status.ind", {
            "apn": updated["apn"], "profileIds": [profile_id], "isBlocked": updated["isBlocked"],
            "ipv4Time": updated["ipv4Time"], "ipv6Time": updated["ipv6Time"],
            "mcc": updated["mcc"], "mnc": updated["mnc"],
        })

    def _apply_network_rat_changed(self, network_rat: str) -> None:
        bearer_tech = RAT_TO_BEARER_TECH.get(network_rat, "UNKNOWN")
        connected_sessions = [s for s in self._sessions.values() if s.status == "CONNECTED"]
        for session in connected_sessions:
            session.update_bearer_tech(bearer_tech)
            self._publish_call_state(session)

    def _collect_throughput_infos(self) -> list[dict]:
        active_profile_ids = {s.profile_id for s in self._sessions.values() if s.status == "CONNECTED"}
        infos = []
        for profile_id in active_profile_ids:
            preset = self._throughput_by_profile.get(profile_id)
            if preset is None:
                continue
            infos.append({
                "slot": self._slot,
                "profileId": profile_id,
                "ul": {
                    "throughput": preset["ulThroughput"],
                    "maxThroughput": preset["ulMaxThroughput"],
                    "queueSize": preset["ulQueueSize"],
                },
                "dl": {"throughput": preset["dlThroughput"]},
            })
        return infos

    def _publish_throughput_info(self) -> None:
        if self._publish_fn is None:
            return
        infos = self._collect_throughput_infos()
        self._pub_ind(topics_data.throughput_info.ind, "data.throughput_info.ind", {"infos": infos})

    # ------------------------------------------------------------------
    # Timer-event handlers (run on AO thread when the fifo delivers them)
    # ------------------------------------------------------------------

    def _on_connect_timer(self, profile_id: int) -> None:
        session = self._sessions.get(profile_id)
        if session is None or session.status != "CONNECTING":
            return
        # cancel_event() is the only path that pops the miros
        # posted_events_queue deque (QUEUE_SIZE=500). A fired times=1 timer
        # thread exits on its own but leaves its PostedEvent record in the
        # deque -- clearing timer_uuid without cancel_event leaks that slot,
        # eventually raising ActiveObjectOutOfPostedEventResources.
        self._cancel_scheduled(session.timer_uuid)
        session.timer_uuid = None
        ip_cfg = self._ip_config
        ipv4 = None
        ipv6 = None
        if session.ip_family in ("IPV4", "IPV4V6"):
            ipv4 = {
                "if_address":           ip_cfg.ipv4_addr,
                "gw_address":           ip_cfg.ipv4_gateway,
                "primary_dns_address":   ip_cfg.ipv4_dns_primary,
                "secondary_dns_address": ip_cfg.ipv4_dns_secondary,
                "subnet_mask":           ip_cfg.ipv4_subnet_mask,
            }
        if session.ip_family in ("IPV6", "IPV4V6"):
            ipv6 = {
                "if_address":           ip_cfg.ipv6_addr,
                "gw_address":           ip_cfg.ipv6_gateway,
                "primary_dns_address":   ip_cfg.ipv6_dns_primary,
                "secondary_dns_address": ip_cfg.ipv6_dns_secondary,
                "prefix_len":            ip_cfg.ipv6_prefix_len,
            }
        bearer_tech = RAT_TO_BEARER_TECH.get(self._get_network_rat_fn(), "UNKNOWN")
        session.connected(ipv4, ipv6, bearer_tech)
        self._setup_iface(session.ifname, ip_cfg.ipv4_mtu)
        self._publish_call_state(session)
        _log.debug("call profileId=%d connected (ifname=%s)", profile_id, session.ifname)

    def _on_disconnect_timer(self, profile_id: int) -> None:
        session = self._sessions.pop(profile_id, None)
        if session is None:
            return
        # See _on_connect_timer: fired times=1 timers must still be
        # cancel_event()'d to release the posted_events_queue slot.
        self._cancel_scheduled(session.timer_uuid)
        session.timer_uuid = None
        session.disconnected()
        self._pool.release(session.ifname)
        self._teardown_iface(session.ifname)
        self._publish_call_state(session)
        _log.debug("call profileId=%d disconnected (ifname=%s)", profile_id, session.ifname)

    # ------------------------------------------------------------------
    # Linux dummy interface management (simulation-only)
    # ------------------------------------------------------------------

    @staticmethod
    def _setup_iface(ifname: str, mtu: int) -> None:
        try:
            subprocess.run(["ip", "link", "add", ifname, "type", "dummy"],
                           check=False, capture_output=True)
            subprocess.run(["ip", "link", "set", ifname, "mtu", str(mtu)],
                           check=False, capture_output=True)
            subprocess.run(["ip", "link", "set", ifname, "up"],
                           check=False, capture_output=True)
            _log.debug("dummy iface %s created mtu=%d", ifname, mtu)
        except Exception as exc:  # noqa: BLE001
            _log.warning("could not create dummy iface %s: %s", ifname, exc)

    @staticmethod
    def _teardown_iface(ifname: str) -> None:
        try:
            subprocess.run(["ip", "link", "del", ifname],
                           check=False, capture_output=True)
            _log.debug("dummy iface %s removed", ifname)
        except Exception as exc:  # noqa: BLE001
            _log.warning("could not remove dummy iface %s: %s", ifname, exc)

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _send_error(self, topic: str, msg: dict, code: str, detail: str = "") -> None:
        env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], code, detail)
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _publish_call_state(self, session: CallSession) -> None:
        self._pub_ind(topics_data.call_state.ind, "data.call_state.ind", session.to_data_call_state())

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(topics_data.subsys_ready_data.ind, "data.subsys_ready_data.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("data subsystem ready=%s published (slot=%d)", ready, self._slot)


# ---------------------------------------------------------------------------
# DataConnectionAO HSM state handlers
# ---------------------------------------------------------------------------

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_operating(chart, e):
    """Composite parent: one Stop handler, defers MessageReceived,
    guard-drops force_* when preconditions aren't met (not Ready)."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal in (signals.ForceCallDrop, signals.ForceThroughput,
                       signals.ForceQos, signals.ForceHwAccel,
                       signals.ForceThrottle, signals.NetworkRatChanged,
                       signals.ThroughputTick,
                       signals.CallConnectTimeout, signals.CallDisconnectTimeout):
        _log.debug("connection AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway and Ready-entry publishes retained ready;
        # a reconnect racing the initial start needs nothing extra.
        _log.debug("connection AO: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_ready)
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    """Business RPCs, force_* actions, and timer callbacks all
    handled here (parent Operating swallows them in every other
    state)."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._publish_subsys_ready(ready=True)
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        if chart._publish_fn is not None:
            try:
                chart._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish data ready=false on stop: %s", exc)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.CallConnectTimeout:
        chart._on_connect_timer(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.CallDisconnectTimeout:
        chart._on_disconnect_timer(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ThroughputTick:
        chart._publish_throughput_info()
        status = return_status.HANDLED
    elif e.signal == signals.ForceCallDrop:
        chart._apply_force_call_drop(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceThroughput:
        chart._apply_force_throughput(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceQos:
        chart._apply_force_qos(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceHwAccel:
        chart._apply_force_hw_accel(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceThrottle:
        chart._apply_force_throttle(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.NetworkRatChanged:
        chart._apply_network_rat_changed(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_stopping(chart, e):
    """Terminal: unsubscribes and tears down live sessions on entry."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        chart.stop()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["CallSession", "DataConnectionAO", "IfnamePool"]
