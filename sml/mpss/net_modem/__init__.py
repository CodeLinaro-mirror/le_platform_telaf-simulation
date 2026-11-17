# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side modem-owned slice of ``telux::data::IDataSettingsManager``.

The rest of the net domain lives on APSS, because the real target PA acquires
every net manager with ``OperationType::DATA_LOCAL`` -- the operation settles
on the processor that invoked it. These three families are the exception, and
they stay here because the modem is the only component that can honour them:

* **WWAN connectivity** -- gates whether the modem may bring up a WWAN PDN.
  Disallowing tears down active calls, which only the modem can do.
* **DDS** -- selects which SIM slot carries default data; a multi-SIM modem
  arbitration coupled to the sim/radio domains.
* **Band interference** -- N79-5G vs WLAN-5GHz RF coexistence scheduling.

Keeping them on MPSS preserves World State invariant (d): authoritative modem
state lives in MPSS and the PA holds at most a derived cache. The PA's single
``SimulaDataSettingsManager`` fronts both halves and gates Ready on both
readiness indications.

This module is a top-level subsystem (registered directly with ``MqttClient``),
not a child AO, because it is the only net family left on MPSS -- a one-child
coordinator would be pure indirection.
"""
from __future__ import annotations

import json
import logging
import os
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr
from sml.common.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import net_modem as topics_modem
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.net_modem")

# The SDK's documented default: WWAN connectivity is allowed unless a client
# disallows it.
_DEFAULT_WWAN_ALLOWED = True


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


class ModemSettingsStore:
    """The modem-owned settings. Plain container, no state machine."""

    def __init__(self, slot_id: int = 1, dds_slot: Optional[int] = None,
                 dds_type: str = "PERMANENT") -> None:
        self.band_enabled = False
        self.band_config: Optional[dict] = None
        # Per-slot, because setWwanConnectivityConfig is slot-scoped.
        self.wwan_allowed: dict[int, bool] = {}
        self.dds_slot = dds_slot if dds_slot is not None else slot_id
        self.dds_type = dds_type

    def is_wwan_allowed(self, slot: int) -> bool:
        return self.wwan_allowed.get(slot, _DEFAULT_WWAN_ALLOWED)


class NetModemSubsystem(ActiveObject):
    """MPSS-side modem-owned data-settings slice -- own AO thread."""

    def __init__(self, slot_id: int = 1, role: Optional[str] = None) -> None:
        super().__init__("NetModemSubsystem")
        self._slot_id = slot_id
        self._role = role or _read_whoami()
        self._store = ModemSettingsStore(slot_id=slot_id)

        self._mpss_src = ""
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}
        self._action_dispatcher = None

        self.start_at(smfn_modem_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (called from MqttClient)
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable,
              direct_publish_fn: Optional[Callable] = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn,
                                    direct_publish_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        self.post_fifo(Event(signal=signals.Resubscribe))

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    def owns_topic(self, topic: str) -> bool:
        if topic in self._owned_topics:
            return True
        if self._action_dispatcher is not None:
            return self._action_dispatcher.owns_topic(topic)
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        if self._action_dispatcher is None:
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    # Action entry points (invoked by NetModemActionDispatcher).
    def force_wwan_connectivity(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceWwanConnectivity, payload=data))

    def force_dds(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceDds, payload=data))

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.mpss.net_modem.action_dispatcher import NetModemActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = \
            self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._direct_publish_fn = direct_publish_fn or publish_fn
        self._mpss_src = f"mpss-{self._role}-{os.getpid()}"

        mapping = {
            topics_modem.set_wwan_connectivity.req:     self._handle_set_wwan,
            topics_modem.request_wwan_connectivity.req: self._handle_req_wwan,
            topics_modem.set_band_interference.req:     self._handle_set_band,
            topics_modem.request_band_interference.req: self._handle_req_band,
            topics_modem.request_dds_switch.req:        self._handle_dds_switch,
            topics_modem.request_current_dds.req:       self._handle_current_dds,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)

        self._action_dispatcher = NetModemActionDispatcher(self)
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("NetModemSubsystem started (slot=%d, src=%s)",
                  self._slot_id, self._mpss_src)

    def _do_stop(self) -> None:
        # Switch to direct (synchronous) publish so the retained ready=false
        # from the Ready-exit hook reaches the broker before paho disconnects.
        if self._direct_publish_fn:
            self._publish_fn = self._direct_publish_fn
        if self._action_dispatcher is not None:
            try:
                self._action_dispatcher.stop()
            except Exception as exc:  # noqa: BLE001
                _log.error("net_modem action dispatcher stop failed: %s", exc)
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)
        _log.info("NetModemSubsystem stopped")

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        if self._action_dispatcher is not None and \
                self._action_dispatcher.owns_topic(topic):
            self._action_dispatcher.handle_message(topic, payload)
            return
        dispatch_inbound(topic, payload, self._handlers, _log, "net_modem AO")

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
        self._publish_subsys_ready(ready=True)
        _log.info("NetModemSubsystem resubscribed (slot=%d)", self._slot_id)

    # ------------------------------------------------------------------
    # RPC handlers
    # ------------------------------------------------------------------

    def _handle_set_wwan(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data["slot"]
        allow = data["allow"]
        changed = self._store.is_wwan_allowed(slot) != allow
        self._store.wwan_allowed[slot] = allow
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_modem.set_wwan_connectivity.rsp,
                      "net_modem.set_wwan_connectivity.rsp", env)
        # Only announce an actual change: re-asserting the current value is a
        # no-op, and a spurious callback would look like a real transition to a
        # listener.
        if changed:
            self._publish_wwan_changed(slot, allow)
        _log.info("WWAN connectivity slot=%d allow=%s (changed=%s)",
                  slot, allow, changed)

    def _handle_req_wwan(self, msg: dict) -> None:
        slot = (msg.get("data") or {})["slot"]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"slot": slot,
                                      "isAllowed": self._store.is_wwan_allowed(slot)})
        self._pub_rsp(topics_modem.request_wwan_connectivity.rsp,
                      "net_modem.request_wwan_connectivity.rsp", env)

    def _handle_set_band(self, msg: dict) -> None:
        data = msg.get("data") or {}
        enable = data["enable"]
        config = data.get("config")
        if enable and config is None:
            self._send_error(topics_modem.set_band_interference.rsp, msg,
                             "INVALID_ARGUMENTS",
                             "config is required when enabling interference management")
            return
        self._store.band_enabled = enable
        # Drop the config when disabling so request_band_interference can honour
        # the SDK's "nullptr if disabled" contract rather than reporting stale
        # priorities.
        self._store.band_config = dict(config) if (enable and config) else None
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_modem.set_band_interference.rsp,
                      "net_modem.set_band_interference.rsp", env)

    def _handle_req_band(self, msg: dict) -> None:
        data = {"isEnabled": self._store.band_enabled}
        if self._store.band_enabled and self._store.band_config:
            data["config"] = dict(self._store.band_config)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_modem.request_band_interference.rsp,
                      "net_modem.request_band_interference.rsp", env)

    def _handle_dds_switch(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data["slot"]
        dds_type = data["ddsType"]
        # The SDK documents OPERATION_NOT_ALLOWED for a permanent switch to the
        # slot that is already permanent DDS -- there is nothing to switch.
        if (dds_type == "PERMANENT" and self._store.dds_type == "PERMANENT"
                and self._store.dds_slot == slot):
            self._send_error(topics_modem.request_dds_switch.rsp, msg,
                             "OPERATION_NOT_ALLOWED",
                             f"slot {slot} is already permanent DDS")
            return
        self._store.dds_slot = slot
        self._store.dds_type = dds_type
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_modem.request_dds_switch.rsp,
                      "net_modem.request_dds_switch.rsp", env)
        self._publish_dds_changed(slot, dds_type)

    def _handle_current_dds(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"slot": self._store.dds_slot,
                                      "ddsType": self._store.dds_type})
        self._pub_rsp(topics_modem.request_current_dds.rsp,
                      "net_modem.request_current_dds.rsp", env)

    # ------------------------------------------------------------------
    # Action appliers
    # ------------------------------------------------------------------

    def _apply_force_wwan(self, data: dict) -> None:
        slot = data["slot"]
        allow = data["isConnectivityAllowed"]
        # Same change-detection as _handle_set_wwan: the indication is what
        # drives the data domain's call teardown, so re-asserting the current
        # value must not fire it again.
        changed = self._store.is_wwan_allowed(slot) != allow
        self._store.wwan_allowed[slot] = allow
        if changed:
            self._publish_wwan_changed(slot, allow)

    def _apply_force_dds(self, data: dict) -> None:
        self._store.dds_slot = data["slot"]
        self._store.dds_type = data["ddsType"]
        self._publish_dds_changed(data["slot"], data["ddsType"])

    # ------------------------------------------------------------------
    # Envelope publish helpers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _send_error(self, topic: str, msg: dict, code: str, detail: str = "") -> None:
        env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                   code, detail)
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict,
                 retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _publish_subsys_ready(self, ready: bool) -> None:
        self._pub_ind(topics_modem.subsys_ready_net_modem.ind,
                      "net_modem.subsys_ready_net_modem.ind",
                      {"ready": ready,
                       "status": "AVAILABLE" if ready else "UNAVAILABLE"},
                      retain=True)

    def _publish_wwan_changed(self, slot: int, allowed: bool) -> None:
        """Announce the permission change.

        The data domain (``DataConnectionAO``) subscribes to this indication to
        honour the SDK contract that disallowing WWAN tears down active calls
        and blocks new ones -- this AO owns the flag, the data domain owns the
        sessions, so the coupling travels over the wire rather than by reaching
        across AO boundaries. Fired only on an actual change.
        """
        self._pub_ind(topics_modem.wwan_connectivity_changed.ind,
                      "net_modem.wwan_connectivity_changed.ind",
                      {"slot": slot, "isConnectivityAllowed": allowed})

    def _publish_dds_changed(self, slot: int, dds_type: str) -> None:
        self._pub_ind(topics_modem.dds_changed.ind, "net_modem.dds_changed.ind",
                      {"slot": slot, "ddsType": dds_type})


# NOTE: deliberately no module-level tuple of Force* signals here.
# miros allocates a signal's integer id on *first attribute access*, so a
# module-level `(signals.ForceDds, ...)` would claim the low ids at import
# time and leave Start/Stop/MessageReceived/StartingDone with different
# numbers than sibling modules expect -- StartingDone then collides with a
# Force signal and the AO never leaves Starting. Comparing inline inside the
# handlers defers every lookup to call time.


@spy_on
def smfn_modem_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_modem_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_modem_operating(chart, e):
    """Composite parent: defers MessageReceived until Ready."""
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_modem_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_modem_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal in (signals.ForceWwanConnectivity, signals.ForceDds):
        _log.debug("net_modem AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("net_modem AO: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_modem_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_modem_ready)
    else:
        chart.temp.fun = smfn_modem_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_modem_ready(chart, e):
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
                _log.warning("failed to publish net_modem ready=false: %s", exc)
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
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_modem_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_modem_stopping(chart, e):
    """Terminal: unsubscribes and stops the AO on entry."""
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


__all__ = ["ModemSettingsStore", "NetModemSubsystem"]
