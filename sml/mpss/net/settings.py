# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side settings and IP configuration manager.

The AO owns backhaul, WWAN, DDS, IPPT, and IP-config state. Retained sync
indications seed the PA-side mirrors used by synchronous SDK calls.
"""
from __future__ import annotations

import json
import logging
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr
from sml.common.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import net_settings as topics_set
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.net.settings")

# The SDK's documented default: WWAN connectivity is allowed unless a client
# disallows it.
_DEFAULT_WWAN_ALLOWED = True


class SettingsStore:
    """Every IDataSettingsManager-owned setting. Plain container."""

    def __init__(self, slot_id: int = 1, backhaul_pref: Optional[list] = None,
                 macsec_enabled: bool = False, dds_slot: Optional[int] = None,
                 dds_type: str = "PERMANENT") -> None:
        self.backhaul_pref: list[str] = list(backhaul_pref or [])
        self.band_enabled = False
        self.band_config: Optional[dict] = None
        self.macsec_enabled = macsec_enabled
        # Per-slot, because setWwanConnectivityConfig is slot-scoped.
        self.wwan_allowed: dict[int, bool] = {}
        self.dds_slot = dds_slot if dds_slot is not None else slot_id
        self.dds_type = dds_type
        # IPPT / IP-config mirrors (see module docstring).
        self.ippt_nat_enabled = True
        self.ippt_entries: dict[tuple, dict] = {}
        self.ip_config_entries: dict[tuple, dict] = {}

    def is_wwan_allowed(self, slot: int) -> bool:
        return self.wwan_allowed.get(slot, _DEFAULT_WWAN_ALLOWED)

    def ippt_wire(self) -> dict:
        return {"natEnabled": self.ippt_nat_enabled,
                "entries": list(self.ippt_entries.values())}

    def ip_config_wire(self) -> dict:
        return {"entries": list(self.ip_config_entries.values())}


class NetSettingsAO(ActiveObject):
    """MPSS-side data-settings manager -- own AO thread."""

    def __init__(self, slot_id: int, mpss_src: str,
                 backhaul_pref: Optional[list] = None) -> None:
        super().__init__("NetSettingsAO")
        self._slot_id = slot_id
        self._mpss_src = mpss_src
        self._store = SettingsStore(slot_id=slot_id, backhaul_pref=backhaul_pref)

        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_set_off)
        _instr.apply_mode(self, _instr.current_mode())

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Optional[Callable] = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

    def force_wwan_connectivity(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceWwanConnectivity, payload=data))

    def force_dds(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceDds, payload=data))

    def force_ippt_config(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceIpptConfig, payload=data))

    def force_ip_config(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceIpConfig, payload=data))

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            topics_set.restore_factory_settings.req:  self._handle_restore,
            topics_set.set_backhaul_pref.req:         self._handle_set_bh_pref,
            topics_set.request_backhaul_pref.req:     self._handle_req_bh_pref,
            topics_set.set_band_interference.req:     self._handle_set_band,
            topics_set.request_band_interference.req: self._handle_req_band,
            topics_set.set_wwan_connectivity.req:     self._handle_set_wwan,
            topics_set.request_wwan_connectivity.req: self._handle_req_wwan,
            topics_set.set_macsec_state.req:          self._handle_set_macsec,
            topics_set.request_macsec_state.req:      self._handle_req_macsec,
            topics_set.switch_backhaul.req:           self._handle_switch_bh,
            topics_set.request_dds_switch.req:        self._handle_dds_switch,
            topics_set.request_current_dds.req:       self._handle_current_dds,
            topics_set.set_ippt_config.req:           self._handle_set_ippt,
            topics_set.set_ippt_nat_config.req:       self._handle_set_ippt_nat,
            topics_set.set_ip_config.req:             self._handle_set_ip_config,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("settings AO subscribed (slot=%d)", self._slot_id)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "settings AO")

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._publish_subsys_ready(ready=True)
        # Re-assert the retained mirrors: a PA that reconnected needs them to
        # re-seed, and MQTT only replays retained messages on subscribe.
        self._publish_ippt_sync()
        self._publish_ip_config_sync()

    def _handle_restore(self, msg: dict) -> None:
        data = msg.get("data") or {}
        # Factory reset clears every setting this manager owns, which is the
        # point of the API -- keeping any would leave the device in a state the
        # factory never shipped.
        self._store = SettingsStore(slot_id=self._slot_id)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.restore_factory_settings.rsp,
                      "net_settings.restore_factory_settings.rsp", env)
        self._publish_ippt_sync()
        self._publish_ip_config_sync()
        _log.info("factory settings restored (reboot=%s)",
                  data.get("isRebootNeeded", True))

    def _handle_set_bh_pref(self, msg: dict) -> None:
        data = msg.get("data") or {}
        pref = data["backhaulPref"]
        if len(set(pref)) != len(pref):
            self._send_error(topics_set.set_backhaul_pref.rsp, msg, "INVALID_ARGUMENTS",
                             "duplicate backhaul in preference list")
            return
        self._store.backhaul_pref = list(pref)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_backhaul_pref.rsp,
                      "net_settings.set_backhaul_pref.rsp", env)
        _log.info("backhaul preference set: %s", pref)

    def _handle_req_bh_pref(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"backhaulPref": list(self._store.backhaul_pref)})
        self._pub_rsp(topics_set.request_backhaul_pref.rsp,
                      "net_settings.request_backhaul_pref.rsp", env)

    def _handle_set_band(self, msg: dict) -> None:
        data = msg.get("data") or {}
        enable = data["enable"]
        config = data.get("config")
        if enable and config is None:
            self._send_error(topics_set.set_band_interference.rsp, msg,
                             "INVALID_ARGUMENTS",
                             "config is required when enabling interference management")
            return
        self._store.band_enabled = enable
        # Drop the config when disabling so request_band_interference can honour
        # the SDK's "nullptr if disabled" contract rather than reporting stale
        # priorities.
        self._store.band_config = dict(config) if (enable and config) else None
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_band_interference.rsp,
                      "net_settings.set_band_interference.rsp", env)

    def _handle_req_band(self, msg: dict) -> None:
        data = {"isEnabled": self._store.band_enabled}
        if self._store.band_enabled and self._store.band_config:
            data["config"] = dict(self._store.band_config)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_set.request_band_interference.rsp,
                      "net_settings.request_band_interference.rsp", env)

    def _handle_set_wwan(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data["slot"]
        allow = data["allow"]
        changed = self._store.is_wwan_allowed(slot) != allow
        self._store.wwan_allowed[slot] = allow
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_wwan_connectivity.rsp,
                      "net_settings.set_wwan_connectivity.rsp", env)
        # Only announce an actual change: re-asserting the current value is a
        # no-op, and a spurious callback would look like a real transition to a
        # listener.
        if changed:
            self._publish_wwan_changed(slot, allow)
        _log.info("WWAN connectivity slot=%d allow=%s (changed=%s)", slot, allow, changed)

    def _handle_req_wwan(self, msg: dict) -> None:
        slot = (msg.get("data") or {})["slot"]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"slot": slot,
                                      "isAllowed": self._store.is_wwan_allowed(slot)})
        self._pub_rsp(topics_set.request_wwan_connectivity.rsp,
                      "net_settings.request_wwan_connectivity.rsp", env)

    def _handle_set_macsec(self, msg: dict) -> None:
        self._store.macsec_enabled = (msg.get("data") or {})["enable"]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_macsec_state.rsp,
                      "net_settings.set_macsec_state.rsp", env)

    def _handle_req_macsec(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"enabled": self._store.macsec_enabled})
        self._pub_rsp(topics_set.request_macsec_state.rsp,
                      "net_settings.request_macsec_state.rsp", env)

    def _handle_switch_bh(self, msg: dict) -> None:
        data = msg.get("data") or {}
        source = data["source"]
        dest = data["dest"]
        if source.get("backhaul") == dest.get("backhaul"):
            self._send_error(topics_set.switch_backhaul.rsp, msg, "INVALID_ARGUMENTS",
                             "source and destination backhaul are identical")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.switch_backhaul.rsp,
                      "net_settings.switch_backhaul.rsp", env)
        _log.info("backhaul switched %s -> %s (applyToAll=%s)",
                  source.get("backhaul"), dest.get("backhaul"),
                  data.get("applyToAll", False))

    def _handle_dds_switch(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data["slot"]
        dds_type = data["ddsType"]
        # The SDK documents OPERATION_NOT_ALLOWED for a permanent switch to the
        # slot that is already permanent DDS -- there is nothing to switch.
        if (dds_type == "PERMANENT" and self._store.dds_type == "PERMANENT"
                and self._store.dds_slot == slot):
            self._send_error(topics_set.request_dds_switch.rsp, msg,
                             "OPERATION_NOT_ALLOWED",
                             f"slot {slot} is already permanent DDS")
            return
        self._store.dds_slot = slot
        self._store.dds_type = dds_type
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.request_dds_switch.rsp,
                      "net_settings.request_dds_switch.rsp", env)
        self._publish_dds_changed(slot, dds_type)

    def _handle_current_dds(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"slot": self._store.dds_slot,
                                      "ddsType": self._store.dds_type})
        self._pub_rsp(topics_set.request_current_dds.rsp,
                      "net_settings.request_current_dds.rsp", env)

    # These mirror the force_* action paths, but arrive as RPCs from the PA
    # because a real client calling setIpConfig() expects its write to reach
    # ground truth. The PA does not await the response (its own signature is
    # synchronous); the authoritative feedback is the retained *_sync mirror
    # republished below.
    def _handle_set_ippt(self, msg: dict) -> None:
        data = msg.get("data") or {}
        key = (data["profileId"], data["vlanId"], data["slot"])
        # The SDK forbids adding or modifying a device config while disabling.
        if data["ipptOpr"] == "DISABLE" and ("nwInterface" in data or "macAddr" in data):
            self._send_error(topics_set.set_ippt_config.rsp, msg, "INVALID_ARGUMENTS",
                             "cannot modify device config while disabling IPPT")
            return
        self._store.ippt_entries[key] = data
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_ippt_config.rsp,
                      "net_settings.set_ippt_config.rsp", env)
        self._publish_ippt_sync()

    def _handle_set_ippt_nat(self, msg: dict) -> None:
        self._store.ippt_nat_enabled = (msg.get("data") or {})["enableNat"]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_ippt_nat_config.rsp,
                      "net_settings.set_ippt_nat_config.rsp", env)
        self._publish_ippt_sync()

    def _handle_set_ip_config(self, msg: dict) -> None:
        data = msg.get("data") or {}
        # The SDK explicitly does not support IPV4V6 here: the client must call
        # once per family. Rejecting is what the real modem does, so accepting
        # it would hide a client bug that fails on hardware.
        if data["ipFamily"] == "IPV4V6":
            self._send_error(topics_set.set_ip_config.rsp, msg, "NOT_SUPPORTED",
                             "setIpConfig does not support IPV4V6; call once per family")
            return
        key = (data["ifType"], data["ipFamily"], data.get("vlanId", -1))
        self._store.ip_config_entries[key] = data
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_set.set_ip_config.rsp,
                      "net_settings.set_ip_config.rsp", env)
        self._publish_ip_config_sync()

    def _apply_force_wwan(self, data: dict) -> None:
        slot = data["slot"]
        allow = data["isConnectivityAllowed"]
        self._store.wwan_allowed[slot] = allow
        self._publish_wwan_changed(slot, allow)

    def _apply_force_dds(self, data: dict) -> None:
        self._store.dds_slot = data["slot"]
        self._store.dds_type = data["ddsType"]
        self._publish_dds_changed(data["slot"], data["ddsType"])

    def _apply_force_ippt(self, data: dict) -> None:
        if "natEnabled" in data:
            self._store.ippt_nat_enabled = data["natEnabled"]
        for entry in data.get("entries", []):
            key = (entry["profileId"], entry["vlanId"], entry["slot"])
            self._store.ippt_entries[key] = entry
        self._publish_ippt_sync()

    def _apply_force_ip_config(self, data: dict) -> None:
        for entry in data.get("entries", []):
            key = (entry["ifType"], entry["ipFamily"], entry.get("vlanId", -1))
            self._store.ip_config_entries[key] = entry
        self._publish_ip_config_sync()

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _send_error(self, topic: str, msg: dict, code: str, detail: str = "") -> None:
        env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], code, detail)
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict,
                 retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _publish_subsys_ready(self, ready: bool) -> None:
        self._pub_ind(topics_set.subsys_ready_net_settings.ind,
                      "net_settings.subsys_ready_net_settings.ind",
                      {"ready": ready,
                       "status": "AVAILABLE" if ready else "UNAVAILABLE"},
                      retain=True)

    def _publish_wwan_changed(self, slot: int, allowed: bool) -> None:
        self._pub_ind(topics_set.wwan_connectivity_changed.ind,
                      "net_settings.wwan_connectivity_changed.ind",
                      {"slot": slot, "isConnectivityAllowed": allowed})

    def _publish_dds_changed(self, slot: int, dds_type: str) -> None:
        self._pub_ind(topics_set.dds_changed.ind, "net_settings.dds_changed.ind",
                      {"slot": slot, "ddsType": dds_type})

    def _publish_ippt_sync(self) -> None:
        if self._publish_fn is None:
            return
        self._pub_ind(topics_set.ippt_config_sync.ind,
                      "net_settings.ippt_config_sync.ind",
                      self._store.ippt_wire(), retain=True)

    def _publish_ip_config_sync(self) -> None:
        if self._publish_fn is None:
            return
        self._pub_ind(topics_set.ip_config_sync.ind,
                      "net_settings.ip_config_sync.ind",
                      self._store.ip_config_wire(), retain=True)


# NOTE: deliberately no module-level tuple of Force* signals here.
# miros allocates a signal's integer id on *first attribute access*, so a
# module-level `(signals.ForceDds, ...)` would claim the low ids at import
# time and leave Start/Stop/MessageReceived/StartingDone with different
# numbers than sibling modules expect -- StartingDone then collides with a
# Force signal and the AO never leaves Starting. Comparing inline inside the
# handlers defers every lookup to call time, which is what the data domain
# does (see mpss/data/serving_system.py).


@spy_on
def smfn_set_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_set_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_set_operating(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_set_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_set_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal in (signals.ForceWwanConnectivity, signals.ForceDds,
                      signals.ForceIpptConfig, signals.ForceIpConfig):
        _log.debug("settings AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_set_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_set_ready)
    else:
        chart.temp.fun = smfn_set_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_set_ready(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._publish_subsys_ready(ready=True)
        # Seed the PA's mirrors for the synchronous APIs on the way up, so a PA
        # that attaches later reads ground truth rather than local defaults.
        chart._publish_ippt_sync()
        chart._publish_ip_config_sync()
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        if chart._publish_fn is not None:
            try:
                chart._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish net_settings ready=false: %s", exc)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceWwanConnectivity:
        chart._apply_force_wwan(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceDds:
        chart._apply_force_dds(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceIpptConfig:
        chart._apply_force_ippt(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceIpConfig:
        chart._apply_force_ip_config(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_set_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_set_stopping(chart, e):
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


__all__ = ["NetSettingsAO", "SettingsStore"]
