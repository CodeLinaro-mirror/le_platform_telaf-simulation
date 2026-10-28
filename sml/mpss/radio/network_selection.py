# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side radio network-selection Active Object (RadioNetworkSelectionAO).

Mirrors ``telux::tel::INetworkSelectionManager`` 
"""
from __future__ import annotations

import json
import logging
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr
from sml.config.models import RadioSeed
from sml.common.envelope import (
    build_success_envelope,
    build_event_envelope,
    dispatch_inbound,
)
from generated.python.topics import radio as topics_radio
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.radio.network_selection")


class RadioNetworkSelectionAO(ActiveObject):
    """Answers INetworkSelectionManager RPCs while Ready."""

    def __init__(self, slot: int, mpss_src: str,
                 radio_seed: Optional[RadioSeed] = None) -> None:
        super().__init__("RadioNetworkSelectionAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None

        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        # World State -- same home network as RadioPhoneAO's serving cell
        # and operator info (310/01), so every API reports one network.
        self.mode = "AUTOMATIC"
        self.mcc = "310"
        self.mnc = "01"

        # World State -- INetworkSelectionManager::setPreferredNetworks/
        # requestPreferredNetworks. Starts empty, matching the SDK's own
        # "no preferred networks configured" default.
        self.preferred_networks: list = []

        self.scan_results = [
            {"mcc": "310", "mnc": "01", "name": "Operator1", "rat": "RADIO_TECH_LTE",
             "isAvailable": True, "isHome": True, "isForbidden": False, "isInUse": True},
            {"mcc": "310", "mnc": "410", "name": "SimulaRoam", "rat": "RADIO_TECH_UMTS",
             "isAvailable": True, "isHome": False, "isForbidden": False, "isInUse": False},
            {"mcc": "001", "mnc": "01", "name": "SimulaForbidden", "rat": "RADIO_TECH_GSM",
             "isAvailable": True, "isHome": False, "isForbidden": True, "isInUse": False},
        ]
        # Same LTE-only seeding rule as RadioPhoneAO.
        if radio_seed is not None and radio_seed.rat.upper() == "LTE":
            self.mcc, self.mnc = radio_seed.mcc, radio_seed.mnc
            self.scan_results[0].update(mcc=radio_seed.mcc, mnc=radio_seed.mnc)

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Posted by ``RadioSubsystem.resubscribe`` after an MQTT reconnect; the AO
        keeps running across the flap. Mirrors the C++ bridge's
        ``Subscribing_St`` -> ``issueAllSubscribes_()``.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            topics_radio.set_network_selection_mode.req:     self._handle_set_mode,
            topics_radio.request_network_selection_mode.req: self._handle_request_mode,
            topics_radio.perform_network_scan.req:           self._handle_perform_network_scan,
            topics_radio.set_preferred_networks.req:         self._handle_set_preferred_networks,
            topics_radio.request_preferred_networks.req:     self._handle_request_preferred_networks,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("radio network-selection AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        self._publish_subsys_ready(ready=True)
        _log.info("radio network-selection AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish netsel ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "radio network-selection AO")

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("radio network-selection AO resubscribed (slot=%d)", self._slot)

    # ------------------------------------------------------------------
    # Publish helpers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(topics_radio.subsys_ready_netsel.ind, "radio.subsys_ready_netsel.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("netsel subsystem ready=%s published (slot=%d)", ready, self._slot)

    # ------------------------------------------------------------------
    # RPC handlers -- each mirrors one NetworkSelectionManagerServerImpl::
    # method
    # ------------------------------------------------------------------

    def _handle_set_mode(self, msg: dict) -> None:
        # Mirrors NetworkSelectionManagerServerImpl::SetNetworkSelectionMode.
        # taf_radio_SetAutomaticRegisterMode sends mode=AUTOMATIC, mcc="", mnc="".
        data = msg.get("data") or {}
        self.mode = data.get("mode", self.mode)
        self.mcc = data.get("mcc", "")
        self.mnc = data.get("mnc", "")
        _log.info("RadioNetworkSelectionAO set_mode applied: mode=%s mcc=%s mnc=%s",
                  self.mode, self.mcc, self.mnc)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_network_selection_mode.rsp,
                      "radio.set_network_selection_mode.rsp", env)
        self._pub_ind(topics_radio.selection_mode.ind, "radio.selection_mode.ind",
                      {"mode": self.mode, "mcc": self.mcc, "mnc": self.mnc})

    def _handle_request_mode(self, msg: dict) -> None:
        data = {"mode": self.mode, "mcc": self.mcc, "mnc": self.mnc}
        _log.debug("RadioNetworkSelectionAO request_mode requested -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_network_selection_mode.rsp,
                      "radio.request_network_selection_mode.rsp", env)

    def _handle_perform_network_scan(self, msg: dict) -> None:
        data = {"networks": [dict(n) for n in self.scan_results]}
        _log.info("RadioNetworkSelectionAO perform_network_scan -> %d networks",
                  len(self.scan_results))
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.perform_network_scan.rsp,
                      "radio.perform_network_scan.rsp", env)

    def _handle_set_preferred_networks(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self.preferred_networks = [
            dict(n) for n in data.get("networks", [])
        ]
        _log.info("RadioNetworkSelectionAO set_preferred_networks applied: %s",
                  self.preferred_networks)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_preferred_networks.rsp,
                      "radio.set_preferred_networks.rsp", env)

    def _handle_request_preferred_networks(self, msg: dict) -> None:
        data = {"networks": [dict(n) for n in self.preferred_networks]}
        _log.debug("RadioNetworkSelectionAO request_preferred_networks -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_preferred_networks.rsp,
                      "radio.request_preferred_networks.rsp", env)


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
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway and Ready-entry publishes; a reconnect
        # racing the initial start needs nothing extra.
        _log.debug("radio network-selection AO: Resubscribe dropped -- not Ready")
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
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_enter_ready()
        while chart.recall() is not None:  # recall() replays only one deferred event per call
            pass
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._do_exit_ready()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
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


__all__ = ["RadioNetworkSelectionAO"]
