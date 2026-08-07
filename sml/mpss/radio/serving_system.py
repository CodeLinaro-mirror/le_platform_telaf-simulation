# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side radio serving-system Active Object (RadioServingSystemAO).

Mirrors ``telux::tel::IServingSystemManager`` -- see ServingManagerServerImpl
(sdk/simulation/services/sdk-simulation-server/tel/ServingManagerServerImpl.cpp).
Distinct from :class:`sml.mpss.data.serving_system.DataServingSystemAO`, which
is the *data* serving system (packet-switched state/roaming/NR icon); this one
is the *telephony* serving system -- RAT preference, serving-system info
(rat/domain/registration state), and 5G NR dual-connectivity status.

Same Off -> Operating.{Starting,Ready} -> Stopping shell as every other MPSS
business AO; see sml.mpss.data.serving_system's module docstring for the
shared two-hop-INIT rationale. World-state lives in flat instance attributes,
not a nested `self.state` object -- miros' own `Hsm` already owns
`self.state` for HSM bookkeeping (`self.state.fun`), so this module follows
the same "World State" convention `DataServingSystemAO` uses.
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

_log = logging.getLogger("sml.mpss.radio.serving_system")

# telux::tel::RatPrefType enumerators (ServingSystemManager.hpp) with their
# PREF_ prefix stripped -- see registry/radio.yaml's set_rat_preference.req
# schema comment.
_RAT_PREF_TYPES = (
    "CDMA_1X", "CDMA_EVDO", "GSM", "WCDMA", "LTE", "TDSCDMA",
    "NR5G", "NR5G_NSA", "NR5G_SA", "NB1_NTN",
)


class RadioServingSystemAO(ActiveObject):
    """Answers tel::IServingSystemManager RPCs while Ready."""

    def __init__(self, slot: int, mpss_src: str,
                 radio_seed: Optional[RadioSeed] = None) -> None:
        super().__init__("RadioServingSystemAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None

        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        # World State -- seeded with the same defaults
        # json/system-info/tel/IServingSystemManagerStateSlot1.json shipped
        # ("1 2 3" -> CDMA_EVDO/GSM/WCDMA per RatPrefType's 0-based ordinal).
        self.rat_prefs = ["CDMA_EVDO", "GSM", "WCDMA"]
        self.service_domain_pref = "PS_ONLY"

        self.rat = "RADIO_TECH_LTE"
        self.domain = "CS_PS"
        self.reg_state = "IN_SERVICE"

        # Overrides the default rat above with the active scenario's
        # initial_state.radio block -- drives get_system_info and the
        # retained sys_info.ind published at Ready-entry. See
        # RadioPhoneAO's matching radio_seed handling for why only LTE is
        # seedable today.
        if radio_seed is not None:
            if radio_seed.rat.upper() == "LTE":
                self.rat = "RADIO_TECH_LTE"
            else:
                _log.warning(
                    "RadioServingSystemAO: scenario radio_seed.rat=%r has no "
                    "wire mapping yet (only LTE supported); rat left at "
                    "default %r", radio_seed.rat, self.rat,
                )

        self.endc_availability = "UNKNOWN"
        self.dcnr_restriction = "RESTRICTED"

        self.lte_cs_capability = "FULL_SERVICE"

        # RF band info: single active-RAT value (band/channel/bandwidth),
        # not a per-RAT list -- see request_rf_band_info.rsp's description.
        # E-UTRA Band 1 / channel 400 matches RadioPhoneAO.cells' seeded LTE
        # cell (earfcn=400), keeping the simulated cell internally consistent.
        self.rf_band = "E_UTRA_OPERATING_BAND_1"
        self.rf_channel = 400
        self.rf_bandwidth = "LTE_BW_NRB_100"

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
            topics_radio.set_rat_preference.req:     self._handle_set_rat_preference,
            topics_radio.request_rat_preference.req: self._handle_request_rat_preference,
            topics_radio.get_system_info.req:        self._handle_get_system_info,
            topics_radio.get_dc_status.req:          self._handle_get_dc_status,
            topics_radio.request_rf_band_info.req:   self._handle_request_rf_band_info,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("radio serving-system AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        self._publish_subsys_ready(ready=True)
        self._publish_sys_info()
        self._publish_dc_status()
        self._publish_lte_cs_capability()
        _log.info("radio serving-system AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish serving-system ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "radio serving-system AO")

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
        _log.info("radio serving-system AO resubscribed (slot=%d)", self._slot)

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
        self._pub_ind(topics_radio.subsys_ready_serv.ind, "radio.subsys_ready_serv.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("radio serv subsystem ready=%s published (slot=%d)", ready, self._slot)

    def _publish_sys_info(self) -> None:
        # retain=True so the PA's SimulaTelServingSystemManager receives the
        # current rat/domain/state immediately on subscribe, regardless of
        # whether it connected before or after MPSS published this at
        # Ready-entry. Without retain the PA misses the boot-time publish
        # and last_sys_info_ stays as default {} -> RAT: Unknown / No Service.
        self._pub_ind(topics_radio.sys_info.ind, "radio.sys_info.ind",
                      {"rat": self.rat, "domain": self.domain, "state": self.reg_state},
                      retain=True)

    def _publish_dc_status(self) -> None:
        # retain=True for the same reason as _publish_sys_info: getDcStatus()
        # reads last_dc_status_ which is only populated from this indication.
        self._pub_ind(topics_radio.dc_status.ind, "radio.dc_status.ind",
                      {"endcAvailability": self.endc_availability,
                       "dcnrRestriction": self.dcnr_restriction},
                      retain=True)

    def _publish_lte_cs_capability(self) -> None:
        # retain=True for the same reason as _publish_dc_status:
        # getLteCsCapability() reads last_lte_cs_capability_, which is only
        # populated from this indication.
        self._pub_ind(topics_radio.lte_cs_capability.ind, "radio.lte_cs_capability.ind",
                      {"lteCsCapability": self.lte_cs_capability}, retain=True)

    # ------------------------------------------------------------------
    # RPC handlers -- each mirrors one ServingManagerServerImpl:: method
    # ------------------------------------------------------------------

    def _handle_set_rat_preference(self, msg: dict) -> None:
        # Mirrors ServingManagerServerImpl::SetRATPreference: persist the new
        # list, then fire triggerSystemSelectionPreferenceEvent().
        data = msg.get("data") or {}
        self.rat_prefs = [r for r in data.get("ratPrefs", []) if r in _RAT_PREF_TYPES]
        _log.info("RadioServingSystemAO set_rat_preference applied: %s", self.rat_prefs)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_rat_preference.rsp, "radio.set_rat_preference.rsp", env)
        self._pub_ind(topics_radio.rat_pref.ind, "radio.rat_pref.ind",
                      {"ratPrefs": list(self.rat_prefs), "serviceDomainPref": self.service_domain_pref})

    def _handle_request_rat_preference(self, msg: dict) -> None:
        _log.debug("RadioServingSystemAO request_rat_preference requested -> %s", self.rat_prefs)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"ratPrefs": list(self.rat_prefs)})
        self._pub_rsp(topics_radio.request_rat_preference.rsp,
                      "radio.request_rat_preference.rsp", env)

    def _handle_get_system_info(self, msg: dict) -> None:
        data = {"rat": self.rat, "domain": self.domain, "state": self.reg_state}
        _log.debug("RadioServingSystemAO get_system_info requested -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.get_system_info.rsp, "radio.get_system_info.rsp", env)

    def _handle_get_dc_status(self, msg: dict) -> None:
        data = {"endcAvailability": self.endc_availability, "dcnrRestriction": self.dcnr_restriction}
        _log.debug("RadioServingSystemAO get_dc_status requested -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.get_dc_status.rsp, "radio.get_dc_status.rsp", env)

    def _handle_request_rf_band_info(self, msg: dict) -> None:
        data = {"band": self.rf_band, "channel": self.rf_channel, "bandWidth": self.rf_bandwidth}
        _log.debug("RadioServingSystemAO request_rf_band_info requested -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_rf_band_info.rsp, "radio.request_rf_band_info.rsp", env)

    # ------------------------------------------------------------------
    # Force-* actions (test/control-plane hook points). All three are wired
    # to ctrl/cmd/action/radio/** via action_radio.yaml and reachable from
    # the CLI (sml mqttcli pub) -- see RadioActionDispatcher. force_sys_info
    # in particular is what drives taf_radio_AddRatChangeHandler tests.
    # ------------------------------------------------------------------

    def force_sys_info(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceSysInfo, payload=data))

    def _apply_force_sys_info(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "rat" in data:
            self.rat = data["rat"]
        if "domain" in data:
            self.domain = data["domain"]
        if "state" in data:
            self.reg_state = data["state"]
        _log.info("RadioServingSystemAO force_sys_info applied: %s", data)
        self._publish_sys_info()

    def force_dc_status(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceDcStatus, payload=data))

    def _apply_force_dc_status(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "endcAvailability" in data:
            self.endc_availability = data["endcAvailability"]
        if "dcnrRestriction" in data:
            self.dcnr_restriction = data["dcnrRestriction"]
        _log.info("RadioServingSystemAO force_dc_status applied: %s", data)
        self._publish_dc_status()

    def force_lte_cs_capability(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceLteCsCapability, payload=data))

    def _apply_force_lte_cs_capability(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "lteCsCapability" in data:
            self.lte_cs_capability = data["lteCsCapability"]
        _log.info("RadioServingSystemAO force_lte_cs_capability applied: %s", data)
        self._publish_lte_cs_capability()


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
    elif e.signal in (signals.ForceSysInfo, signals.ForceDcStatus, signals.ForceLteCsCapability):
        _log.debug("radio serving-system AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway and Ready-entry publishes; a reconnect
        # racing the initial start needs nothing extra.
        _log.debug("radio serving-system AO: Resubscribe dropped -- not Ready")
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
    elif e.signal == signals.ForceSysInfo:
        chart._apply_force_sys_info(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceDcStatus:
        chart._apply_force_dc_status(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceLteCsCapability:
        chart._apply_force_lte_cs_capability(e.payload)
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


__all__ = ["RadioServingSystemAO"]
