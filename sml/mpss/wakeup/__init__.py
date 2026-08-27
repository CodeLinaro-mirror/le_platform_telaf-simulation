# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS wakeup sub-package.

Provides :class:`WakeupSubsystem`, the mpss-side producer of the
wakeup relay: consumes `ctrl/cmd/action/wakeup/force_wakeup` and publishes
`mp/ind/wakeup/event`, which apss's `PowerSubsystem` relays onward as
`ap/ind/power/wakeup`.

It also owns the modem wakeup-source **filter** bitset, configured by the PA
over `mp/req/wakeup/{set,get}_ws_filter` (see sml/shared/registry/wakeup.yaml
and pa/telaf-pa-simula/component/telaf-np/component/taf_prop_pms/). MPSS is
where that bitset's ground truth lives; the PA layer caches nothing and asks.
The filter is enforced, not merely recorded: a `force_wakeup` whose source bit
is cleared publishes no `mp/ind/wakeup/event` at all, because on real hardware
the modem is what withholds the wakeup. A `force_wakeup` whose
(serviceId, msgId) combo isn't a recognized wakeup source has no bit to
filter on, so it publishes unconditionally; this module doesn't validate that
combo, it only logs it.

Usage (from ``sml/mpss/__main__.py``)::

    wakeup_sub = WakeupSubsystem()
    dispatcher.register_domain("wakeup", wakeup_sub)
    client.register_subsystem(wakeup_sub)
"""
from __future__ import annotations

import json
import logging
import os
from pathlib import Path
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr
from sml.common.envelope import (
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from sml.runtime.persist import atomic_write_json, read_json
from generated.python.topics import wakeup as topics_wakeup
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.wakeup")

# taf_prop_pa_pms_ModemWakeupSource_t (telaf-np's prop-prefix ABI). The
# ns-prefix ABI only had the first three bits.
MODEM_WS_INCOMING_SMS = 0x0001
MODEM_WS_INCOMING_VCALL = 0x0002
MODEM_WS_SIM_PROFILE_SWAP = 0x0004
MODEM_WS_NAS_SYS_INFO = 0x0008

# Default: no source allowed through until the PA calls EnableAllWs (or
# sets a specific bitset) at init -- matches the modem's own power-up state.
_DEFAULT_WS_FILTER = 0x0000

# (serviceId, msgId) -> MODEM_WS_* bit. Mirrors the PA's own combo table in
# telaf-pa/component/taf_pa_pms/tafPmsPa.cpp
# (WakeupReasonListener::onWakeup): the modem decides which wakeup source a
# QMI service/message pair represents, and the PA translates the same pairs
# back into the same bits.
_WS_COMBO_TO_BIT = {
    (0x05, 0x0001): MODEM_WS_INCOMING_SMS,        # WMS  / SMS arriving
    (0x09, 0x002E): MODEM_WS_INCOMING_VCALL,      # VOICE/ incoming call
    (0x0B, 0x0033): MODEM_WS_SIM_PROFILE_SWAP,    # SIM  / remote profile swap
    (0x03, 0x004E): MODEM_WS_NAS_SYS_INFO,        # NAS  / system info
}

_WS_BIT_NAMES = {
    MODEM_WS_INCOMING_SMS: "INCOMING_SMS",
    MODEM_WS_INCOMING_VCALL: "INCOMING_VCALL",
    MODEM_WS_SIM_PROFILE_SWAP: "SIM_PROFILE_SWAP",
    MODEM_WS_NAS_SYS_INFO: "NAS_SYS_INFO",
}

_FORCE_WAKEUP_DEFAULTS = {
    "serviceId": 5,
    "sourceNodeId": 0,
    "destinationNodeId": 0,
    "isMsgIdValid": True,
    "msgId": 1,
    "isPIDValid": False,
    "pid": 0,
    "isProcessNameValid": False,
    "processName": "",
}


def _read_whoami() -> str:
    home = os.environ.get("HOME", "")
    if not home:
        return "dev"
    try:
        with open(os.path.join(home, ".whoami"), encoding="utf-8") as fh:
            role = fh.read().strip()
            return role if role else "dev"
    except OSError:
        return "dev"


class WakeupSubsystem(ActiveObject):
    """Off -> Operating(Starting -> Ready) -> Stopping. Owns
    `ctrl/cmd/action/wakeup/force_wakeup` (via `WakeupActionDispatcher`) plus
    the PA-facing wakeup-source filter RPCs. `mp/ind/wakeup/event` is
    outbound-only, so there are no ind subscriptions.
    """

    def __init__(self, role: Optional[str] = None,
                 persist_path: Optional[Path] = None,
                 default_filter: int = _DEFAULT_WS_FILTER) -> None:
        super().__init__("WakeupSubsystem")
        self._role = role or _read_whoami()
        self._persist_path = persist_path
        self._default_filter = default_filter
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._mpss_src = ""

        # Modem ground truth for the wakeup-source filter (invariant d).
        self._ws_filter = default_filter

        self._action_dispatcher = None
        self._handlers: dict = {}
        self._owned_topics: frozenset = frozenset()

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (called from MqttClient / ActionDispatcher)
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Optional[Callable] = None,
              direct_publish_fn: Optional[Callable] = None) -> None:
        # `direct_publish_fn` is passed to every sub-AO by the shared
        # MqttClient._start_sub_aos(); wakeup has no shutdown-time publish
        # path so it is stored only for signature parity.
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn,
                                    direct_publish_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        if topic in self._owned_topics:
            return True
        if self._action_dispatcher is not None and self._action_dispatcher.owns_topic(topic):
            return True
        return False

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        """Same entry point for a manual `ctrl/cmd/action/**` injection and a
        scenario-timeline step (invariant e).
        """
        if self._action_dispatcher is None:
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    def force_wakeup(self, data: dict) -> None:
        """Invoked by `WakeupActionDispatcher` on the caller's thread; posts
        into this AO's own fifo so the mutation runs on the AO thread."""
        self.post_fifo(Event(signal=signals.WakeupForceWakeup, payload=data))

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.mpss.wakeup.action_dispatcher import WakeupActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, _direct_publish_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._mpss_src = f"mpss-{self._role}-{os.getpid()}"

        if self._persist_path is not None:
            snapshot = read_json(self._persist_path)
            if snapshot is not None:
                self._ws_filter = int(snapshot.get("bitset", self._default_filter))

        mapping = {
            topics_wakeup.set_ws_filter.req: self._handle_set_ws_filter,
            topics_wakeup.get_ws_filter.req: self._handle_get_ws_filter,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)

        self._action_dispatcher = WakeupActionDispatcher(self)
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("wakeup subsystem started (src=%s, wsFilter=0x%04x)",
                  self._mpss_src, self._ws_filter)

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        if self._action_dispatcher is not None:
            self._action_dispatcher.resubscribe()
        _log.info("wakeup subsystem resubscribed")

    def _do_stop(self) -> None:
        if self._action_dispatcher is not None:
            try:
                self._action_dispatcher.stop()
            except Exception as exc:  # noqa: BLE001
                _log.error("wakeup action dispatcher stop failed: %s", exc)
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)
        _log.info("wakeup subsystem stopped")

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        if topic in self._owned_topics:
            dispatch_inbound(topic, payload, self._handlers, _log, "wakeup subsystem")
        elif self._action_dispatcher is not None and self._action_dispatcher.owns_topic(topic):
            self._action_dispatcher.handle_message(topic, payload)

    def _apply_force_wakeup(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        payload = dict(_FORCE_WAKEUP_DEFAULTS)
        payload.update(data)

        # The modem is what drops a filtered-out wakeup source: the AP never
        # sees the event at all (as opposed to seeing it and ignoring it), so
        # the suppression has to happen here, before publishing.
        bit = self._ws_bit_for(payload)
        if bit is None:
            _log.info(
                "wakeup event source unknown: combo [svc_id:0x%04x, msg_id:0x%04x] "
                "is not a known wakeup source -- publishing anyway, wsFilter not checked",
                payload["serviceId"], payload["msgId"])
        elif not (self._ws_filter & bit):
            _log.info(
                "wakeup event suppressed: source %s (0x%04x) is filtered out "
                "by wsFilter=0x%04x",
                _WS_BIT_NAMES.get(bit, "?"), bit, self._ws_filter)
            return

        validate_payload("wakeup.event.ind", payload)
        env = build_event_envelope(self._mpss_src, payload)
        self._publish_fn(topics_wakeup.event.ind, json.dumps(env).encode(), 1, False)
        _log.info("wakeup event published (source=%s): %s",
                  _WS_BIT_NAMES.get(bit, "unknown"), payload)

    @staticmethod
    def _ws_bit_for(payload: dict) -> Optional[int]:
        """Which MODEM_WS_* bit this wakeup event represents, or None if the
        (serviceId, msgId) pair is not a recognized wakeup source.

        msgId only carries meaning when isMsgIdValid is set -- the PA reads it
        under exactly that guard -- so an event without it cannot be attributed
        to a source and is therefore not one.
        """
        if not payload.get("isMsgIdValid"):
            return None
        return _WS_COMBO_TO_BIT.get(
            (payload.get("serviceId"), payload.get("msgId")))

    # ------------------------------------------------------------------
    # PA-facing wakeup-source filter RPCs
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _handle_set_ws_filter(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self._ws_filter = int(data["bitset"])
        self._maybe_persist()
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_wakeup.set_ws_filter.rsp, "wakeup.set_ws_filter.rsp", env)
        _log.info("wakeup source filter set to 0x%04x", self._ws_filter)

    def _handle_get_ws_filter(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"bitset": self._ws_filter})
        self._pub_rsp(topics_wakeup.get_ws_filter.rsp, "wakeup.get_ws_filter.rsp", env)
        _log.debug("wakeup source filter reported as 0x%04x", self._ws_filter)

    def _maybe_persist(self) -> None:
        if self._persist_path is None:
            return
        try:
            atomic_write_json(self._persist_path, {"bitset": self._ws_filter})
        except OSError as exc:
            _log.warning("failed to persist ws_filter to %s: %s", self._persist_path, exc)


# ---------------------------------------------------------------------------
# HSM state handlers -- Off -> Operating(Starting -> Ready) -> Stopping,
# same topology as PowerSubsystem (see sml/apss/power/__init__.py).
# ---------------------------------------------------------------------------

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.Start:
        status = chart.trans(smfn_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_operating(chart, e):
    """Composite parent: defers MessageReceived/WakeupForceWakeup until Ready."""
    status = return_status.UNHANDLED
    if e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_stopping)
    elif e.signal in (signals.MessageReceived, signals.WakeupForceWakeup):
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("WakeupSubsystem: Resubscribe dropped -- not Ready")
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
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_ready)
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.WakeupForceWakeup:
        chart._apply_force_wakeup(e.payload)
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
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        chart.stop()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["WakeupSubsystem"]
