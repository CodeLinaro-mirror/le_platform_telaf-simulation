# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side SIM subscription Active Object."""
from __future__ import annotations

import json
import logging
from typing import Callable

from miros import Factory, Event, return_status, signals, spy_on
from miros.hsm import HsmWithQueues

from sml.mpss import instrumentation as _instr
# Shared envelope handling validates inbound payloads before dispatch.
from sml.mpss.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import sim as topics_sim
from generated.python.validators import validate as validate_payload

TOPIC_IND_SUB_INFO_CHANGED = topics_sim.sub_info_changed.ind
TOPIC_IND_SUBSYS_READY_SUB = topics_sim.subsys_ready_sub.ind
TOPIC_REQ_GET_ICCID = topics_sim.get_iccid.req
TOPIC_REQ_GET_IMSI = topics_sim.get_imsi.req
TOPIC_RSP_GET_ICCID = topics_sim.get_iccid.rsp
TOPIC_RSP_GET_IMSI = topics_sim.get_imsi.rsp

_log = logging.getLogger("sml.mpss.sim.subscription")

SIG_START = "Start"
SIG_STARTING_DONE = "StartingDone"
SIG_STOP = "Stop"
SIG_MESSAGE_RECEIVED = "MessageReceived"


class SubscriptionAO(Factory):
    """Owns subscription identity and handles subscription RPCs while ready."""

    def __init__(self, slot: int, mpss_src: str, iccid: str = "", imsi: str = "") -> None:
        super().__init__("SubscriptionAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._publish_fn: Callable | None = None
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None
        self._pending_start_args: tuple | None = None

        self._owned_topics: set = set()
        self._handlers: dict = {}

        # Provisioned identity survives a card power cycle; visibility follows the
        # paired card AO. `_last_published` dedups by VALUE (not by an
        # availability flag) so the restore after power-on is never skipped.
        self.iccid = iccid
        self.imsi = imsi
        self._card_available = True
        self._last_published: tuple | None = None
        # Set while SimActionDispatcher sequences a composite hotswap.
        self._hotswap_active = False

        self.nest(smfn_off, parent=None)
        self.nest(smfn_starting, parent=None)
        self.nest(smfn_ready, parent=None)
        self.nest(smfn_stopping, parent=None)
        HsmWithQueues.start_at(self, smfn_off)
        # Apply instrumentation after the initial state transition.
        _instr.apply_mode(self, _instr.current_mode())


    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.dispatch(Event(signal=signals.SIG_START))
        self.dispatch(Event(signal=signals.SIG_STARTING_DONE))

    def stop(self) -> None:
        self.dispatch(Event(signal=signals.SIG_STOP))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Called by ``SimSubsystem`` after an MQTT reconnect; ignored outside
        ``smfn_ready`` (``smfn_starting`` subscribes anyway).
        """
        self.dispatch(Event(signal=signals.SIG_RESUBSCRIBE))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.dispatch(Event(signal=signals.SIG_MESSAGE_RECEIVED,
                            payload=(topic, payload)))

    # Action dispatcher entry point.

    @property
    def visible_iccid(self) -> str:
        """ICCID as the modem would report it: empty while the card is unreadable."""
        return self.iccid if self._card_available else ""

    @property
    def visible_imsi(self) -> str:
        """IMSI as the modem would report it: empty while the card is unreadable."""
        return self.imsi if self._card_available else ""

    def set_card_available(self, available: bool) -> None:
        """Hide identity while the card is powered down; restore it on power-on."""
        self._card_available = available
        # During a hotswap the dispatcher announces the new identity itself,
        # after the card is READY; intermediate card transitions must not emit
        # their own indications. See begin_hotswap().
        if self._publish_fn is not None and not self._hotswap_active:
            self._publish_sub_info_changed()

    def begin_hotswap(self) -> None:
        """Suppress availability-driven indications for a composite hotswap.

        `action_sim.yaml` and `action.sim.hotswap.req` both specify exactly
        3 card_state + 1 sub_info_changed per swap. The swap drives the card
        ABSENT -> PRESENT/UNKNOWN -> PRESENT/READY, and each of those would
        otherwise publish its own identity indication via set_card_available()
        -- an empty one while the old card is out, then the OLD identity when it
        comes back READY. The dispatcher announces the new identity itself once
        the card is READY; see SimActionDispatcher._hotswap.
        """
        self._hotswap_active = True

    def end_hotswap(self) -> None:
        """Re-enable availability-driven indications after a hotswap."""
        self._hotswap_active = False

    def hotswap(self, data: dict) -> None:
        """Record the swapped-in identity and announce it exactly once.

        Called AFTER the card is back to PRESENT/READY: the PA only copies
        identity into its cache for a live slot, so announcing mid-swap (while
        the card is ABSENT) is silently dropped and never re-read.
        """
        if self._publish_fn is None:
            return
        self.iccid = data["iccid"]
        self.imsi = data["imsi"]
        # The suppressed transitions above left _last_published stale (it still
        # holds whatever was sent before the swap). Clear it so a swap back to a
        # previously-published identity is never deduped away.
        self._last_published = None
        self._publish_identity(self.iccid, self.imsi)

    # State-machine helpers.

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            TOPIC_REQ_GET_ICCID: self._handle_get_iccid,
            TOPIC_REQ_GET_IMSI: self._handle_get_imsi,
        }
        self._handlers = mapping
        self._owned_topics = set(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("subscription AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        # sub_info_changed is non-retained; late clients use the identity RPCs.
        self._publish_sub_info_changed()
        self._publish_subsys_ready(ready=True)
        _log.info("subscription AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish sub ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish Ready state.

        ``_last_published`` is cleared first so ``_do_enter_ready``'s
        ``sub_info_changed`` is actually sent: the value-based dedup would
        otherwise swallow it as unchanged, and the PA -- which caches identity
        from that event -- may have lost the cache along with the connection.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._last_published = None
        self._do_enter_ready()
        _log.info("subscription AO resubscribed (slot=%d)", self._slot)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "subscription AO")

    # RPC handlers.

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        # Outbound payloads must satisfy their response schemas.
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict,
                 retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _handle_get_iccid(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data.get("slot")
        if slot != self._slot:
            _log.warning("subscription AO: get_iccid for slot %s rejected (only slot %d supported)",
                         slot, self._slot)
            env = build_error_envelope(
                self._mpss_src, msg["corrId"], msg["src"],
                "INVALID_PARAMETER",
                f"slot {slot!r} not supported; this simulation models slot {self._slot} only",
            )
            self._pub_rsp(TOPIC_RSP_GET_ICCID, "sim.get_iccid.rsp", env)
            return
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"],
            {"iccid": self.visible_iccid},
        )
        self._pub_rsp(TOPIC_RSP_GET_ICCID, "sim.get_iccid.rsp", env)

    def _handle_get_imsi(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data.get("slot")
        if slot != self._slot:
            _log.warning("subscription AO: get_imsi for slot %s rejected (only slot %d supported)",
                         slot, self._slot)
            env = build_error_envelope(
                self._mpss_src, msg["corrId"], msg["src"],
                "INVALID_PARAMETER",
                f"slot {slot!r} not supported; this simulation models slot {self._slot} only",
            )
            self._pub_rsp(TOPIC_RSP_GET_IMSI, "sim.get_imsi.rsp", env)
            return
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"],
            {"imsi": self.visible_imsi},
        )
        self._pub_rsp(TOPIC_RSP_GET_IMSI, "sim.get_imsi.rsp", env)

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(TOPIC_IND_SUBSYS_READY_SUB, "sim.subsys_ready_sub.ind",
                      {"ready": ready, "status": status}, retain=True)
        _log.debug("sub subsystem ready=%s published (slot=%d)", ready, self._slot)

    def _publish_sub_info_changed(self) -> None:
        self._publish_identity(self.visible_iccid, self.visible_imsi)

    def _publish_identity(self, iccid: str, imsi: str) -> None:
        """Publish sub_info_changed, deduped by the values actually sent.

        Dedup is by published VALUE rather than by an availability flag, so
        repeated card_state indications stay quiet while any genuine hide
        (power off) or restore (power on / hotswap) is always announced -- the
        PA caches identity from this event, so a skipped restore would leave it
        permanently empty.
        """
        current = (iccid, imsi)
        if self._last_published == current:
            return
        self._last_published = current
        self._pub_ind(TOPIC_IND_SUB_INFO_CHANGED, "sim.sub_info_changed.ind",
                      {"iccid": iccid, "imsi": imsi})


# State handlers.

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SIG_START:
        status = chart.trans(smfn_starting)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    """Subscribe before entering the ready state."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SIG_STARTING_DONE:
        status = chart.trans(smfn_ready)
    elif e.signal == signals.SIG_MESSAGE_RECEIVED:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_enter_ready()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._do_exit_ready()
        status = return_status.HANDLED
    elif e.signal == signals.SIG_MESSAGE_RECEIVED:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.SIG_STOP:
        status = chart.trans(smfn_stopping)
    elif e.signal == signals.SIG_RESUBSCRIBE:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_stopping(chart, e):
    """Unsubscribe and stop."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["SubscriptionAO"]
