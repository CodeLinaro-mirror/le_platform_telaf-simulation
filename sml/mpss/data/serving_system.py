# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side serving system Active Object (DataServingSystemAO).

State topology::

    top
    |-- smfn_off
    |-- smfn_operating
    |     ENTRY: nothing
    |     INIT : -> smfn_starting
    |     SIG_STOP           : -> smfn_stopping
    |     SIG_MESSAGE_RECEIVED: defer (buffered until Ready)
    |     |-- smfn_starting
    |     |     ENTRY: _do_start(); post_fifo(StartingDone)
    |     |     SIG_STARTING_DONE : -> smfn_ready
    |     |-- smfn_ready
    |           ENTRY: publish ready=true; recall_all()
    |           EXIT : publish ready=false
    |           SIG_MESSAGE_RECEIVED: _dispatch_message() (overrides parent's defer)
    |-- smfn_stopping

The two-hop ``Operating -> Starting -> Ready`` chain within a single
``dispatch()`` is not usable here: miros' INIT loop
(``while(t(self, init_e) == return_status.TRAN)``, ``hsm.py:626``) only
walks descendants -- an INIT that trans() to a sibling loops forever
inside ``while(self.temp.fun != t)``.  So ``smfn_starting`` posts
``StartingDone`` into its own fifo on ENTRY, and the next RTC step
drives the Starting -> Ready ``trans()`` (a sibling trans on a regular
signal, which miros' ``trans_`` handles correctly via LCA).

This AO runs on its own miros ``ActiveObject`` thread
(``ActiveObject.start_at``).  Public entry points (``start``/``stop``/
``force_*``/``handle_message``) post events into the fifo and return
immediately; all state mutation happens on the AO thread.
"""
from __future__ import annotations

import json
import logging
from typing import Callable

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.mpss import instrumentation as _instr
from sml.mpss.envelope import (
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import data as topics_data
from generated.python.validators import validate as validate_payload


TOPIC_IND_SUBSYS_READY_TEL = "sml/mpss/data/evt/subsys/tel/1/ready"

_log = logging.getLogger("sml.mpss.data.serving_system")

class DataServingSystemAO(ActiveObject):
    """Publishes serving system events and answers serving-system RPCs
    while Ready.  See module docstring for the full state topology."""

    def __init__(self, slot: int, mpss_src: str,
                 on_rat_changed: Callable[[str], None] | None = None) -> None:
        super().__init__("DataServingSystemAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._on_rat_changed = on_rat_changed
        self._publish_fn: Callable | None = None
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None
        self._pending_start_args: tuple | None = None

        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        # World State
        self.service_state = "IN_SERVICE"
        self.network_rat = "LTE"
        self.drb_status = "DORMANT"
        self.is_roaming = False
        self.roaming_type = "DOMESTIC"
        self.nr_icon_type = "NONE"

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (called from DataSubsystem / ActionDispatcher /
    # MqttClient -- always from another thread; every method just posts
    # an event into the AO fifo and returns immediately).
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Posted by ``DataSubsystem.resubscribe`` after an MQTT reconnect. The
        AO keeps running across the flap -- only what the broker forgot (the
        session's subscriptions, and retained topics if the broker itself
        restarted) is rebuilt. Mirrors the C++ bridge's
        ``Subscribing_St`` -> ``issueAllSubscribes_()``.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    # World State mutators post events; the state machine decides
    # whether we're in Ready (do the work) or not (parent Operating
    # logs+drops).  Invariant (e): same Action Dispatcher path drives
    # these events whether they came from a manual injection or a
    # scenario-timeline action.

    def force_serv_state(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceServState, payload=data))

    def force_roaming(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceRoaming, payload=data))

    def force_nr_icon_type(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceNrIconType, payload=data))

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers (run on the AO thread)
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            topics_data.request_service_status.req: self._handle_request_service_status,
            topics_data.request_roaming_status.req: self._handle_request_roaming_status,
            topics_data.request_nr_icon_type.req:   self._handle_request_nr_icon_type,
            topics_data.make_dormant.req:           self._handle_make_dormant,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("serving-system AO subscribed (slot=%d)", self._slot)

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish state.

        Re-running ``_do_enter_ready`` is deliberate: it is exactly the set of
        publishes a fresh subscriber needs, and the retained ones have to be
        re-sent in case the broker restarted rather than merely dropping us.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("serving-system AO resubscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        self._publish_serv_state()
        self._publish_serv_roaming()
        self._publish_subsys_ready(ready=True)
        self._publish_tel_ready(ready=True)
        _log.info("serving-system AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish serv ready=false on stop: %s", exc)
            try:
                self._publish_tel_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish tel ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "serving-system AO")

    def _apply_force_serv_state(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        rat_changed = "networkRat" in data and data["networkRat"] != self.network_rat
        if "serviceState" in data:
            self.service_state = data["serviceState"]
        if "networkRat" in data:
            self.network_rat = data["networkRat"]
        if "drbStatus" in data:
            self.drb_status = data["drbStatus"]
        self._publish_serv_state()
        if rat_changed and self._on_rat_changed is not None:
            self._on_rat_changed(self.network_rat)

    def _apply_force_roaming(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "isRoaming" in data:
            self.is_roaming = data["isRoaming"]
        if "roamingType" in data:
            self.roaming_type = data["roamingType"]
        self._publish_serv_roaming()

    def _apply_force_nr_icon_type(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "iconType" in data:
            self.nr_icon_type = data["iconType"]

    # ------------------------------------------------------------------
    # RPC handlers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _handle_request_service_status(self, msg: dict) -> None:
        data = {"serviceState": self.service_state, "networkRat": self.network_rat}
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_data.request_service_status.rsp, "data.request_service_status.rsp", env)

    def _handle_request_roaming_status(self, msg: dict) -> None:
        data = {"isRoaming": self.is_roaming, "roamingType": self.roaming_type}
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_data.request_roaming_status.rsp, "data.request_roaming_status.rsp", env)

    def _handle_request_nr_icon_type(self, msg: dict) -> None:
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"], {"iconType": self.nr_icon_type},
        )
        self._pub_rsp(topics_data.request_nr_icon_type.rsp, "data.request_nr_icon_type.rsp", env)

    def _handle_make_dormant(self, msg: dict) -> None:
        self.drb_status = "DORMANT"
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_data.make_dormant.rsp, "data.make_dormant.rsp", env)
        self._publish_serv_state()

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(topics_data.subsys_ready_serv.ind, "data.subsys_ready_serv.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("serv subsystem ready=%s published (slot=%d)", ready, self._slot)

    def _publish_tel_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        env = build_event_envelope(self._mpss_src, {"ready": ready, "status": status})
        self._publish_fn(TOPIC_IND_SUBSYS_READY_TEL, json.dumps(env).encode(), 1, True)
        _log.debug("tel subsystem ready=%s published (slot=%d)", ready, self._slot)

    def _publish_serv_state(self) -> None:
        data = {
            "serviceState": self.service_state,
            "networkRat": self.network_rat,
            "drbStatus": self.drb_status,
        }
        self._pub_ind(topics_data.serv_state.ind, "data.serv_state.ind", data)

    def _publish_serv_roaming(self) -> None:
        data = {"isRoaming": self.is_roaming, "roamingType": self.roaming_type}
        self._pub_ind(topics_data.serv_roaming.ind, "data.serv_roaming.ind", data)


# ---------------------------------------------------------------------------
# HSM state handlers.
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
    """Composite parent -- catches Stop once for both Starting and Ready,
    and defers MessageReceived so RPCs that land while Starting are
    replayed once Ready.entry runs `recall_all` (see A7)."""
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
    elif e.signal in (signals.ForceServState, signals.ForceRoaming, signals.ForceNrIconType):
        _log.debug("serving-system AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway, and Ready-entry publishes; a reconnect
        # racing the initial start needs nothing extra.
        _log.debug("serving-system AO: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    """Runs ``_do_start`` on entry then posts StartingDone into its own
    fifo -- the next RTC drives the sibling trans() to Ready.  (INIT to a
    sibling would hang; see module docstring.)"""
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
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_enter_ready()
        chart.recall()  # replay any MessageReceived deferred by parent
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._do_exit_ready()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceServState:
        chart._apply_force_serv_state(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceRoaming:
        chart._apply_force_roaming(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceNrIconType:
        chart._apply_force_nr_icon_type(e.payload)
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
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["DataServingSystemAO"]
