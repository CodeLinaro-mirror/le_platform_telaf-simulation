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
from sml.common import instrumentation as _instr
from sml.common.envelope import (
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

        self.rf_band = "E_UTRA_OPERATING_BAND_1"
        self.rf_channel = 400
        self.rf_bandwidth = "LTE_BW_NRB_100"

        self.gsm_band_capability = ["GSM_900_PRIMARY", "GSM_1800"]
        self.wcdma_band_capability = ["WCDMA_2100", "WCDMA_850"]
        self.lte_band_capability = ["E_UTRA_BAND_1", "E_UTRA_BAND_3"]
        self.nr_band_capability = ["NR5G_BAND_1", "NR5G_BAND_78"]

        self.gsm_band_prefs = list(self.gsm_band_capability)
        self.wcdma_band_prefs = list(self.wcdma_band_capability)
        self.lte_band_prefs = list(self.lte_band_capability)
        self.nr_band_prefs = list(self.nr_band_capability)

        self.network_time = {
            "year": 0, "month": 0, "day": 0, "hour": 0, "minute": 0, "second": 0,
            "dayOfWeek": 0, "timeZone": 0, "dstAdj": 0, "nitzTime": "",
        }
        self.network_rejection = {
            "rat": "RADIO_TECH_UNKNOWN", "domain": "UNKNOWN", "rejectCause": 0,
            "mcc": "", "mnc": "",
        }

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
            topics_radio.set_service_domain_preference.req:     self._handle_set_service_domain_preference,
            topics_radio.request_service_domain_preference.req: self._handle_request_service_domain_preference,
            topics_radio.get_system_info.req:        self._handle_get_system_info,
            topics_radio.get_dc_status.req:          self._handle_get_dc_status,
            topics_radio.request_rf_band_info.req:   self._handle_request_rf_band_info,
            topics_radio.request_rf_band_capability.req:  self._handle_request_rf_band_capability,
            topics_radio.set_rf_band_preferences.req:      self._handle_set_rf_band_preferences,
            topics_radio.request_rf_band_preferences.req: self._handle_request_rf_band_preferences,
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
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("radio serving-system AO resubscribed (slot=%d)", self._slot)

    def _run_force(self, apply_fn: Callable, data: dict) -> None:
        # Runs on the AO thread, where an uncaught exception kills the thread for good.
        try:
            apply_fn(data)
        except Exception as exc:  # noqa: BLE001
            _log.error("%s failed: %s", apply_fn.__name__, exc)

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

    def _handle_set_service_domain_preference(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self.service_domain_pref = data.get("serviceDomain", self.service_domain_pref)
        _log.info("RadioServingSystemAO set_service_domain_preference applied: %s",
                  self.service_domain_pref)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_service_domain_preference.rsp,
                      "radio.set_service_domain_preference.rsp", env)
        self._pub_ind(topics_radio.rat_pref.ind, "radio.rat_pref.ind",
                      {"ratPrefs": list(self.rat_prefs), "serviceDomainPref": self.service_domain_pref})

    def _handle_request_service_domain_preference(self, msg: dict) -> None:
        _log.debug("RadioServingSystemAO request_service_domain_preference -> %s",
                  self.service_domain_pref)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"serviceDomain": self.service_domain_pref})
        self._pub_rsp(topics_radio.request_service_domain_preference.rsp,
                      "radio.request_service_domain_preference.rsp", env)

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

    def _handle_request_rf_band_capability(self, msg: dict) -> None:
        data = {
            "gsmBands": list(self.gsm_band_capability),
            "wcdmaBands": list(self.wcdma_band_capability),
            "lteBands": list(self.lte_band_capability),
            "nrBands": list(self.nr_band_capability),
        }
        _log.debug("RadioServingSystemAO request_rf_band_capability -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_rf_band_capability.rsp,
                      "radio.request_rf_band_capability.rsp", env)

    def _handle_set_rf_band_preferences(self, msg: dict) -> None:
        data = msg.get("data") or {}
        if "gsmBands" in data:
            self.gsm_band_prefs = list(data["gsmBands"])
        if "wcdmaBands" in data:
            self.wcdma_band_prefs = list(data["wcdmaBands"])
        if "lteBands" in data:
            self.lte_band_prefs = list(data["lteBands"])
        if "nrBands" in data:
            self.nr_band_prefs = list(data["nrBands"])
        _log.info("RadioServingSystemAO set_rf_band_preferences applied: gsm=%s wcdma=%s lte=%s nr=%s",
                  self.gsm_band_prefs, self.wcdma_band_prefs, self.lte_band_prefs, self.nr_band_prefs)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_rf_band_preferences.rsp,
                      "radio.set_rf_band_preferences.rsp", env)

    def _handle_request_rf_band_preferences(self, msg: dict) -> None:
        data = {
            "gsmBands": list(self.gsm_band_prefs),
            "wcdmaBands": list(self.wcdma_band_prefs),
            "lteBands": list(self.lte_band_prefs),
            "nrBands": list(self.nr_band_prefs),
        }
        _log.debug("RadioServingSystemAO request_rf_band_preferences -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_rf_band_preferences.rsp,
                      "radio.request_rf_band_preferences.rsp", env)


    def force_profile(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceProfile, payload=data))

    def _apply_force_profile(self, data: dict) -> None:
        profile = data.get("profile")
        profiles = {
            "4g": {"rat": "RADIO_TECH_LTE", "domain": "CS_PS", "state": "IN_SERVICE",
                   "endcAvailability": "UNAVAILABLE", "dcnrRestriction": "RESTRICTED"},
            "5g-nsa": {"rat": "RADIO_TECH_LTE", "domain": "CS_PS", "state": "IN_SERVICE",
                       "endcAvailability": "AVAILABLE", "dcnrRestriction": "UNRESTRICTED"},
            "5g-sa": {"rat": "RADIO_TECH_NR5G", "domain": "CS_PS", "state": "IN_SERVICE",
                      "endcAvailability": "UNAVAILABLE", "dcnrRestriction": "UNRESTRICTED"},
            "out-of-service": {"rat": "RADIO_TECH_UNKNOWN", "domain": "NO_SRV", "state": "NO_SERVICE",
                               "endcAvailability": "UNAVAILABLE", "dcnrRestriction": "RESTRICTED"},
            "radio-failure": {"rat": "RADIO_TECH_UNKNOWN", "domain": "NO_SRV", "state": "NO_SERVICE",
                              "endcAvailability": "UNAVAILABLE", "dcnrRestriction": "RESTRICTED"},
            "recovery": {"rat": "RADIO_TECH_LTE", "domain": "CS_PS", "state": "IN_SERVICE",
                         "endcAvailability": "UNAVAILABLE", "dcnrRestriction": "RESTRICTED"},
            "weak-signal": {},
        }
        update = profiles.get(profile, {})
        if not update:
            return
        self.rat = update["rat"]
        self.domain = update["domain"]
        self.reg_state = update["state"]
        self.endc_availability = update["endcAvailability"]
        self.dcnr_restriction = update["dcnrRestriction"]
        _log.info("RadioServingSystemAO force_profile applied: %s", profile)
        self._publish_sys_info()
        self._publish_dc_status()

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

    def force_network_time(self, data: dict) -> None:
        """Publish a NITZ network-time indication. Drives
        IServingSystemListener::onNetworkTimeChanged -- there is no AP "set
        network time" RPC, so this is the only way to exercise that
        handler in the simulator."""
        self.post_fifo(Event(signal=signals.ForceNetworkTime, payload=data))

    def _apply_force_network_time(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        fields = ("year", "month", "day", "hour", "minute", "second",
                  "dayOfWeek", "timeZone", "dstAdj", "nitzTime")
        for f in fields:
            if f in data:
                self.network_time[f] = data[f]
        _log.info("RadioServingSystemAO force_network_time applied: %s", data)
        self._pub_ind(topics_radio.network_time.ind, "radio.network_time.ind",
                      dict(self.network_time))

    def force_rf_band_info(self, data: dict) -> None:
        """Force the currently-active serving RF band/channel/bandwidth and
        re-publish mp/ind/radio/rf_band_info. Distinct from
        set_rf_band_preferences (device preference, an AP RPC): this is the
        network-side push of which band is actually active. Drives
        IServingSystemListener::onRFBandInfoChanged."""
        self.post_fifo(Event(signal=signals.ForceRfBandInfo, payload=data))

    def _apply_force_rf_band_info(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "band" in data:
            self.rf_band = data["band"]
        if "channel" in data:
            self.rf_channel = data["channel"]
        if "bandWidth" in data:
            self.rf_bandwidth = data["bandWidth"]
        _log.info("RadioServingSystemAO force_rf_band_info applied: %s", data)
        self._pub_ind(topics_radio.rf_band_info.ind, "radio.rf_band_info.ind",
                      {"band": self.rf_band, "channel": self.rf_channel,
                       "bandWidth": self.rf_bandwidth})

    def force_network_rejection(self, data: dict) -> None:
        """Publish a network-rejection indication. Drives
        IServingSystemListener::onNetworkRejection -- pure network-side
        push, no AP RPC to piggyback on."""
        self.post_fifo(Event(signal=signals.ForceNetworkRejection, payload=data))

    def _apply_force_network_rejection(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        for f in ("rat", "domain", "rejectCause", "mcc", "mnc"):
            if f in data:
                self.network_rejection[f] = data[f]
        _log.info("RadioServingSystemAO force_network_rejection applied: %s", data)
        self._pub_ind(topics_radio.network_rejection.ind, "radio.network_rejection.ind",
                      dict(self.network_rejection))


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
    elif e.signal in (signals.ForceProfile, signals.ForceSysInfo, signals.ForceDcStatus, signals.ForceLteCsCapability):
        _log.debug("radio serving-system AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal in (signals.ForceNetworkTime, signals.ForceRfBandInfo,
                       signals.ForceNetworkRejection):
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
    elif e.signal == signals.ForceProfile:
        chart._run_force(chart._apply_force_profile, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceSysInfo:
        chart._run_force(chart._apply_force_sys_info, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceDcStatus:
        chart._run_force(chart._apply_force_dc_status, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceLteCsCapability:
        chart._run_force(chart._apply_force_lte_cs_capability, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceNetworkTime:
        chart._run_force(chart._apply_force_network_time, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceRfBandInfo:
        chart._run_force(chart._apply_force_rf_band_info, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceNetworkRejection:
        chart._run_force(chart._apply_force_network_rejection, e.payload)
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
