# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side SIM card Active Object."""
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

TOPIC_IND_CARD_STATE = topics_sim.card_state.ind
TOPIC_IND_SUBSYS_READY_CARD = topics_sim.subsys_ready_card.ind
TOPIC_REQ_GET_STATE = topics_sim.get_state.req
TOPIC_REQ_SET_POWER = topics_sim.set_power.req
TOPIC_RSP_GET_STATE = topics_sim.get_state.rsp
TOPIC_RSP_SET_POWER = topics_sim.set_power.rsp

_log = logging.getLogger("sml.mpss.sim.card")

SIG_START = "Start"
SIG_STARTING_DONE = "StartingDone"
SIG_STOP = "Stop"
SIG_MESSAGE_RECEIVED = "MessageReceived"


class SimCardAO(Factory):
    """Owns SIM card state and handles card RPCs while ready."""

    def __init__(self, slot: int, mpss_src: str,
                 iccid: str = "", imsi: str = "",
                 initial_card_state: str = "PRESENT",
                 initial_app_state: str = "READY") -> None:
        super().__init__("SimCardAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._publish_fn: Callable | None = None
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None
        self._pending_start_args: tuple | None = None
        self._state_changed_cb: Callable[[bool], None] | None = None

        self._owned_topics: set = set()
        self._handlers: dict = {}

        # Physical presence is independent from the power rail.
        self._card_installed = bool(iccid or imsi)
        self.card_state = initial_card_state if self._card_installed else "ABSENT"
        self.app_state = initial_app_state if self._card_installed else "UNKNOWN"
        self.power_on = True

        self.nest(smfn_off, parent=None)
        self.nest(smfn_starting, parent=None)
        self.nest(smfn_ready, parent=None)
        self.nest(smfn_stopping, parent=None)
        HsmWithQueues.start_at(self, smfn_off)
        # Apply instrumentation after the initial state transition.
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (called from SimSubsystem)
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.dispatch(Event(signal=signals.SIG_START))
        self.dispatch(Event(signal=signals.SIG_STARTING_DONE))

    def stop(self) -> None:
        self.dispatch(Event(signal=signals.SIG_STOP))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Called by ``SimSubsystem`` after an MQTT reconnect. Outside
        ``smfn_ready`` the signal falls through to ``top`` and is ignored --
        ``smfn_starting`` subscribes anyway and Ready-entry publishes, so a
        reconnect racing the initial start needs nothing extra.
        """
        self.dispatch(Event(signal=signals.SIG_RESUBSCRIBE))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.dispatch(Event(signal=signals.SIG_MESSAGE_RECEIVED,
                            payload=(topic, payload)))

    @property
    def identity_available(self) -> bool:
        """Whether the card is available to read subscription identity."""
        return self.card_state == "PRESENT" and self.app_state == "READY"

    def set_state_changed_callback(self, callback: Callable[[bool], None]) -> None:
        """Register the subscription-identity availability observer."""
        self._state_changed_cb = callback

    # Action dispatcher entry points.

    def force_card_state(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "cardState" in data:
            self.card_state = data["cardState"]
        if "appState" in data:
            self.app_state = data["appState"]
        # Keep power_on consistent with the forced state, else a later
        # set_power(True) looks like a no-op against a card that already
        # reports READY while power_on is still False.
        if self.app_state == "READY" and self.card_state == "PRESENT":
            self.power_on = True
        self._publish_card_state()

    def force_power(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        self._apply_power_state(data["powerOn"])
        self._publish_card_state()

    # Composite-action primitives.

    def set_card_app_state(self, card_state: str, app_state: str,
                           power_on: bool | None = None) -> None:
        """Set visible card/app state and publish one indication."""
        if self._publish_fn is None:
            return
        self.card_state = card_state
        self.app_state = app_state
        if power_on is not None:
            self.power_on = power_on
        self._publish_card_state()

    def insert_card(self) -> None:
        """Mark a card physically present without publishing."""
        self._card_installed = True

    def remove_card(self) -> None:
        """Mark the slot physically empty without publishing."""
        self._card_installed = False

    # ------------------------------------------------------------------
    # Power model
    # ------------------------------------------------------------------

    def _apply_power_state(self, power_on: bool) -> None:
        """Apply a power change to card/app state.

        Presence and power are INDEPENDENT axes. A powered-down card is still
        physically in the slot, so it stays `PRESENT` with the USIM app not up
        (`appState=UNKNOWN`); only an empty slot is `ABSENT`.

        This is not cosmetic. `taf_pa_sim_SetPower` (telaf-pa/component/
        taf_pa_sim/tafSimPa.cpp) first looks up `pa.managers.cards[slot]` and
        returns FAULT when it is null, and that cache is nulled whenever we
        report ABSENT (via onSlotStatusChanged). Reporting ABSENT on power-off
        therefore made power-on unroutable: the request died in the PA, MPSS
        never saw it, so nothing could ever restore the card -- the slot was
        wedged in `absent` until restart.

        Identity stays hidden while powered down because `identity_available`
        additionally requires `appState == READY`.
        """
        self.power_on = power_on
        if not self._card_installed:
            # An empty slot is genuinely absent regardless of the power rail.
            self.card_state = "ABSENT"
            self.app_state = "UNKNOWN"
        elif power_on:
            self.card_state = "PRESENT"
            self.app_state = "READY"
        else:
            self.card_state = "PRESENT"
            self.app_state = "UNKNOWN"

    # State-machine helpers.

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            TOPIC_REQ_GET_STATE: self._handle_get_state,
            TOPIC_REQ_SET_POWER: self._handle_set_power,
        }
        self._handlers = mapping
        self._owned_topics = set(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("card AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        # card_state is a non-retained change event; late clients use get_state.
        self._publish_card_state()
        self._publish_subsys_ready(ready=True)
        _log.info("card AO ready (slot=%d, cardState=%s)", self._slot, self.card_state)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish card ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish Ready state.

        Re-running ``_do_enter_ready`` is deliberate: it is exactly the set of
        publishes a fresh subscriber needs, and the retained ``ready=true`` has
        to be re-sent in case the broker restarted rather than merely dropping
        our session.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("card AO resubscribed (slot=%d)", self._slot)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "card AO")

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

    def _handle_get_state(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data.get("slot")
        if slot != self._slot:
            _log.warning("card AO: get_state for slot %s rejected (only slot %d supported)",
                         slot, self._slot)
            env = build_error_envelope(
                self._mpss_src, msg["corrId"], msg["src"],
                "INVALID_PARAMETER",
                f"slot {slot!r} not supported; this simulation models slot {self._slot} only",
            )
            self._pub_rsp(TOPIC_RSP_GET_STATE, "sim.get_state.rsp", env)
            return
        rsp_data = {"cardState": self.card_state, "appState": self.app_state}
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], rsp_data)
        self._pub_rsp(TOPIC_RSP_GET_STATE, "sim.get_state.rsp", env)

    def _handle_set_power(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data.get("slot")
        if slot != self._slot:
            _log.warning("card AO: set_power for slot %s rejected (only slot %d supported)",
                         slot, self._slot)
            env = build_error_envelope(
                self._mpss_src, msg["corrId"], msg["src"],
                "INVALID_PARAMETER",
                f"slot {slot!r} not supported; this simulation models slot {self._slot} only",
            )
            self._pub_rsp(TOPIC_RSP_SET_POWER, "sim.set_power.rsp", env)
            return
        self._apply_power_state(data.get("powerOn", True))
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(TOPIC_RSP_SET_POWER, "sim.set_power.rsp", env)
        self._publish_card_state()

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(TOPIC_IND_SUBSYS_READY_CARD, "sim.subsys_ready_card.ind",
                      {"ready": ready, "status": status}, retain=True)
        _log.debug("card subsystem ready=%s published (slot=%d)", ready, self._slot)

    def _publish_card_state(self) -> None:
        data = {"cardState": self.card_state, "appState": self.app_state}
        self._pub_ind(TOPIC_IND_CARD_STATE, "sim.card_state.ind", data)
        if self._state_changed_cb:
            self._state_changed_cb(self.identity_available)


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


__all__ = ["SimCardAO"]
