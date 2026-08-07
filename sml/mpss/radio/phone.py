# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side radio phone Active Object (RadioPhoneAO).

Mirrors ``telux::tel::IPhoneManager``/``IPhone`` -- see PhoneManagerServerImpl
(sdk/simulation/services/sdk-simulation-server/tel/PhoneManagerServerImpl.cpp),
whose gRPC handlers this AO's RPC handlers reproduce field-for-field, reading
what used to be json/system-state/tel/IPhoneManagerStateSlot*.json out of
plain instance attributes instead (same "World State" convention
``sml.mpss.data.serving_system.DataServingSystemAO`` uses -- flat attributes,
not a nested state object, since miros' own ``Hsm`` already owns
``self.state`` for HSM bookkeeping).

State topology is the same Off -> Operating.{Starting,Ready} -> Stopping
shell as ``sml.mpss.data.serving_system.DataServingSystemAO`` -- see that
module's docstring for the two-hop-INIT rationale, which applies verbatim
here.
"""
from __future__ import annotations

import json
import logging
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.config.models import RadioSeed
from sml.mpss import instrumentation as _instr
from sml.mpss.envelope import (
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import radio as topics_radio
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.radio.phone")


class RadioPhoneAO(ActiveObject):
    """Publishes op-mode/signal-strength/cell-info events and answers
    IPhoneManager/IPhone RPCs while Ready. See module docstring for the
    full state topology."""

    def __init__(self, slot: int, mpss_src: str,
                 radio_seed: Optional[RadioSeed] = None) -> None:
        super().__init__("RadioPhoneAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None

        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        # World State -- seeded with the same defaults
        # json/system-info/tel/IPhoneManagerStateSlot1.json shipped.
        self.operating_mode = "ONLINE"
        self.voice_service_state = "REG_HOME"
        self.voice_service_denial_cause = -1
        self.voice_radio_tech = "RADIO_TECH_LTE"

        self.voice_service_techs = ["VOICE_TECH_VOLTE"]
        self.sim_count = 2
        self.max_active_sims = 2
        self.sim_rat_capabilities = [
            {"slotId": 1, "capabilities": ["GSM", "WCDMA", "LTE", "NR5G", "NR5GSA"]},
            {"slotId": 2, "capabilities": ["GSM", "WCDMA", "LTE", "NR5G", "NR5GSA"]},
        ]
        self.device_rat_capability = [
            {"slotId": 1, "capabilities": ["GSM", "WCDMA", "LTE", "NR5G", "NR5GSA"]},
            {"slotId": 2, "capabilities": ["GSM", "WCDMA", "LTE", "NR5G", "NR5GSA"]},
        ]

        self.gsm_signal = {"gsmSignalStrength": 2147483647, "gsmBitErrorRate": 2147483647}
        self.wcdma_signal = {
            "signalStrength": 2147483647, "bitErrorRate": 2147483647,
            "ecio": 2147483647, "rscp": 2147483647,
        }
        self.lte_signal = {
            "lteSignalStrength": 31, "lteRsrp": -83, "lteRsrq": -10, "lteRssnr": 29,
            "lteCqi": 2147483647, "timingAdvance": 1,
        }
        self.nr5g_signal = {"rsrp": 2147483647, "rsrq": 2147483647, "rssnr": 2147483647}

        self.cells = [{
            "cellType": "LTE",
            "registered": True,
            "lte": {
                "mcc": "310", "mnc": "01", "ci": 78849, "pci": 411, "tac": 702, "earfcn": 400,
                "signalStrength": 31, "rsrp": -83, "rsrq": -10, "rssnr": 29, "cqi": 2147483647,
                "timingAdvance": 1,
            },
        }]

        # Overrides the LTE cell/signal defaults above with the active
        # scenario's initial_state.radio block (mcc/mnc split from the
        # serving cell's PLMN, rsrp from its default_rsrp_dbm) -- drives
        # request_cell_info and request_signal_strength. Only LTE has a
        # matching cells[]/lte_signal shape to seed into today; any other
        # radio_seed.rat is logged and left at the defaults above rather
        # than guessed at. radio_seed.variance_db is intentionally not
        # applied -- no MPSS signal-fluctuation model exists yet (see
        # sml/config/environments/default.yaml's header comment), so every
        # scenario seeds only this static baseline value.
        if radio_seed is not None:
            if radio_seed.rat.upper() == "LTE":
                self.cells[0]["lte"]["mcc"] = radio_seed.mcc
                self.cells[0]["lte"]["mnc"] = radio_seed.mnc
                self.cells[0]["lte"]["rsrp"] = radio_seed.rsrp_dbm
                self.lte_signal["lteRsrp"] = radio_seed.rsrp_dbm
            else:
                _log.warning(
                    "RadioPhoneAO: scenario radio_seed.rat=%r has no seedable "
                    "cell/signal shape yet (only LTE supported); cell/signal "
                    "state left at built-in defaults", radio_seed.rat,
                )

        self.long_name = "Operator1"
        self.short_name = "Operator1"
        self.plmn = "plmn1"
        self.is_home = True

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
            topics_radio.set_operating_mode.req:           self._handle_set_operating_mode,
            topics_radio.request_operating_mode.req:       self._handle_request_operating_mode,
            topics_radio.request_cellular_capability.req:  self._handle_request_cellular_capability,
            topics_radio.request_voice_service_state.req:  self._handle_request_voice_service_state,
            topics_radio.request_signal_strength.req:      self._handle_request_signal_strength,
            topics_radio.configure_signal_strength.req:    self._handle_configure_signal_strength,
            topics_radio.request_cell_info.req:             self._handle_request_cell_info,
            topics_radio.request_operator_info.req:         self._handle_request_operator_info,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("radio phone AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        self._publish_subsys_ready(ready=True)
        _log.info("radio phone AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish phone ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "radio phone AO")

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish retained ready.

        Re-running ``_do_enter_ready`` is deliberate: the retained
        ``ready=true`` has to be re-sent in case the broker restarted rather
        than merely dropping our session.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("radio phone AO resubscribed (slot=%d)", self._slot)

    def _signal_strength_payload(self) -> dict:
        return {
            "gsm": dict(self.gsm_signal),
            "wcdma": dict(self.wcdma_signal),
            "lte": dict(self.lte_signal),
            "nr5g": dict(self.nr5g_signal),
        }

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
        self._pub_ind(topics_radio.subsys_ready_phone.ind, "radio.subsys_ready_phone.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("phone subsystem ready=%s published (slot=%d)", ready, self._slot)

    # ------------------------------------------------------------------
    # RPC handlers -- each mirrors one PhoneManagerServerImpl:: method
    # ------------------------------------------------------------------

    def _handle_set_operating_mode(self, msg: dict) -> None:
        # Mirrors PhoneManagerServerImpl::SetOperatingMode's INVALID_ARGUMENTS
        # range check (mode outside ONLINE..PERSISTENT_LOW_POWER) -- schema
        # validation upstream (dispatch_inbound) already rejects wire values
        # outside the enum before this handler ever runs, so there is
        # nothing left to range-check here.
        data = msg.get("data") or {}
        mode = data["mode"]
        self.operating_mode = mode
        _log.info("RadioPhoneAO set_operating_mode applied: mode=%s", mode)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_operating_mode.rsp, "radio.set_operating_mode.rsp", env)
        # Mirrors notifyAndUpdateOperatingMode()'s operatingModeUpdate event.
        self._pub_ind(topics_radio.op_mode.ind, "radio.op_mode.ind", {"mode": mode})

    def _handle_request_operating_mode(self, msg: dict) -> None:
        _log.debug("RadioPhoneAO request_operating_mode requested -> %s", self.operating_mode)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"mode": self.operating_mode})
        self._pub_rsp(topics_radio.request_operating_mode.rsp, "radio.request_operating_mode.rsp", env)

    def _handle_request_cellular_capability(self, msg: dict) -> None:
        data = {
            "voiceServiceTechs": list(self.voice_service_techs),
            "simCount": self.sim_count,
            "maxActiveSims": self.max_active_sims,
            "simRatCapabilities": [dict(c) for c in self.sim_rat_capabilities],
            "deviceRatCapability": [dict(c) for c in self.device_rat_capability],
        }
        _log.debug("RadioPhoneAO request_cellular_capability requested -> simCount=%s maxActiveSims=%s",
                   self.sim_count, self.max_active_sims)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_cellular_capability.rsp,
                      "radio.request_cellular_capability.rsp", env)

    def _handle_request_voice_service_state(self, msg: dict) -> None:
        data = {
            "voiceServiceState": self.voice_service_state,
            "denialCause": self.voice_service_denial_cause,
            "radioTech": self.voice_radio_tech,
        }
        _log.debug("RadioPhoneAO request_voice_service_state requested -> %s", self.voice_service_state)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_voice_service_state.rsp,
                      "radio.request_voice_service_state.rsp", env)

    def _handle_request_signal_strength(self, msg: dict) -> None:
        payload = self._signal_strength_payload()
        _log.debug("RadioPhoneAO request_signal_strength requested -> %s", payload)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], payload)
        self._pub_rsp(topics_radio.request_signal_strength.rsp,
                      "radio.request_signal_strength.rsp", env)

    def _handle_configure_signal_strength(self, msg: dict) -> None:
        # Mirrors PhoneManagerServerImpl::ConfigureSignalStrength: acks the
        # config without changing the reported signal snapshot -- the old
        # gRPC handler only ever stored the config's success/error/delay
        # from json, it never mutated signalStrengthInfo itself either.
        _log.debug("RadioPhoneAO configure_signal_strength requested -> %s", msg.get("data") or {})
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.configure_signal_strength.rsp,
                      "radio.configure_signal_strength.rsp", env)

    def _handle_request_cell_info(self, msg: dict) -> None:
        _log.debug("RadioPhoneAO request_cell_info requested -> %d cell(s)", len(self.cells))
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"cells": [dict(c) for c in self.cells]})
        self._pub_rsp(topics_radio.request_cell_info.rsp, "radio.request_cell_info.rsp", env)

    def _handle_request_operator_info(self, msg: dict) -> None:
        data = {
            "longName": self.long_name, "shortName": self.short_name,
            "plmn": self.plmn, "isHome": self.is_home,
        }
        _log.debug("RadioPhoneAO request_operator_info requested -> plmn=%s longName=%s",
                   self.plmn, self.long_name)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_operator_info.rsp, "radio.request_operator_info.rsp", env)

    # ------------------------------------------------------------------
    # Force-* actions (test/control-plane hook points; wired to
    # ctrl/cmd/action/radio/force_signal_strength via action_radio.yaml --
    # see RadioActionDispatcher).
    # ------------------------------------------------------------------

    def force_signal_strength(self, data: dict) -> None:
        """Directly mutate the signal-strength snapshot and re-publish it as
        an indication -- mirrors DataServingSystemAO.force_serv_state's
        shape. Each RAT block in `data` is a partial patch (see the
        action.radio.force_signal_strength.req schema); drives
        taf_radio_AddSignalStrengthChangeHandler."""
        self.post_fifo(Event(signal=signals.ForceSignalStrength, payload=data))

    def _apply_force_signal_strength(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "gsm" in data:
            self.gsm_signal.update(data["gsm"])
        if "wcdma" in data:
            self.wcdma_signal.update(data["wcdma"])
        if "lte" in data:
            self.lte_signal.update(data["lte"])
        if "nr5g" in data:
            self.nr5g_signal.update(data["nr5g"])
        rats = [rat for rat in ("gsm", "wcdma", "lte", "nr5g") if rat in data]
        _log.info("RadioPhoneAO force_signal_strength applied: rats=%s", rats)
        self._pub_ind(topics_radio.signal_strength.ind, "radio.signal_strength.ind",
                      self._signal_strength_payload())


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
    elif e.signal == signals.ForceSignalStrength:
        _log.debug("radio phone AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway and Ready-entry publishes retained ready;
        # a reconnect racing the initial start needs nothing extra.
        _log.debug("radio phone AO: Resubscribe dropped -- not Ready")
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
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._do_exit_ready()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceSignalStrength:
        chart._apply_force_signal_strength(e.payload)
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


__all__ = ["RadioPhoneAO"]
