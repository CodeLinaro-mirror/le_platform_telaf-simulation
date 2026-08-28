# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side VLAN manager for ``taf_net.api``.

The AO owns VLAN and binding tables. The PA's VLAN accessors are backed by the
query responses and do not require separate wire operations.
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
from generated.python.topics import net_vlan as topics_vlan
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.net.vlan")

# BackhaulInfo::slotId/profileId are documented as don't-care for every
# backhaul except WWAN, so bindings are keyed on (vlanId, backhaul) and the
# WWAN-only fields ride along as payload.
_WWAN = "WWAN"


class VlanEntry:
    """One configured VLAN; mirrors ``telux::data::VlanConfig``."""

    __slots__ = ("vlan_id", "iface_type", "is_accelerated", "priority",
                 "nw_type", "create_bridge")

    def __init__(self, vlan_id: int, iface_type: str, is_accelerated: bool = False,
                 priority: int = 0, nw_type: str = "LAN",
                 create_bridge: bool = True) -> None:
        self.vlan_id = vlan_id
        self.iface_type = iface_type
        self.is_accelerated = is_accelerated
        self.priority = priority
        self.nw_type = nw_type
        self.create_bridge = create_bridge

    @property
    def key(self) -> tuple:
        # A VLAN id is only unique per PHY interface -- removeVlan takes
        # (vlanId, ifaceType) precisely because the same id may exist on ETH
        # and on WLAN simultaneously.
        return (self.vlan_id, self.iface_type)

    def to_wire(self) -> dict:
        return {
            "vlanId": self.vlan_id,
            "ifaceType": self.iface_type,
            "isAccelerated": self.is_accelerated,
            "priority": self.priority,
            "nwType": self.nw_type,
            "createBridge": self.create_bridge,
        }


class VlanBinding:
    """One VLAN-to-backhaul binding; mirrors ``VlanBindConfig``."""

    __slots__ = ("vlan_id", "backhaul", "slot", "profile_id", "bh_vlan_id")

    def __init__(self, vlan_id: int, backhaul: str, slot: Optional[int] = None,
                 profile_id: Optional[int] = None,
                 bh_vlan_id: Optional[int] = None) -> None:
        self.vlan_id = vlan_id
        self.backhaul = backhaul
        self.slot = slot
        self.profile_id = profile_id
        self.bh_vlan_id = bh_vlan_id

    @property
    def key(self) -> tuple:
        return (self.vlan_id, self.backhaul)

    def to_wire(self) -> dict:
        out = {"vlanId": self.vlan_id, "backhaul": self.backhaul}
        # Emit the WWAN-only fields only when set: the schema forbids
        # additionalProperties but leaves these optional, and sending
        # profileId=null for an ETH binding would misrepresent don't-care as
        # "explicitly none".
        if self.slot is not None:
            out["slot"] = self.slot
        if self.profile_id is not None:
            out["profileId"] = self.profile_id
        if self.bh_vlan_id is not None:
            out["bhVlanId"] = self.bh_vlan_id
        return out


class VlanStore:
    """VLAN table + binding table. Plain container, no state machine."""

    def __init__(self, seed_vlans: Optional[list] = None,
                 seed_bindings: Optional[list] = None) -> None:
        self._vlans: dict[tuple, VlanEntry] = {}
        self._bindings: dict[tuple, VlanBinding] = {}
        for v in seed_vlans or []:
            entry = VlanEntry(**v) if isinstance(v, dict) else v
            self._vlans[entry.key] = entry
        for b in seed_bindings or []:
            binding = VlanBinding(**b) if isinstance(b, dict) else b
            self._bindings[binding.key] = binding

    def get(self, vlan_id: int, iface_type: str) -> Optional[VlanEntry]:
        return self._vlans.get((vlan_id, iface_type))

    def add(self, entry: VlanEntry) -> None:
        self._vlans[entry.key] = entry

    def remove(self, vlan_id: int, iface_type: str) -> Optional[VlanEntry]:
        return self._vlans.pop((vlan_id, iface_type), None)

    def all_vlans(self) -> list[VlanEntry]:
        return list(self._vlans.values())

    def has_vlan_id(self, vlan_id: int) -> bool:
        return any(k[0] == vlan_id for k in self._vlans)

    def get_binding(self, vlan_id: int, backhaul: str) -> Optional[VlanBinding]:
        return self._bindings.get((vlan_id, backhaul))

    def add_binding(self, binding: VlanBinding) -> None:
        self._bindings[binding.key] = binding

    def remove_binding(self, vlan_id: int, backhaul: str) -> Optional[VlanBinding]:
        return self._bindings.pop((vlan_id, backhaul), None)

    def all_bindings(self) -> list[VlanBinding]:
        return list(self._bindings.values())

    def remove_bindings_for_vlan(self, vlan_id: int) -> list[VlanBinding]:
        """Remove all bindings for a VLAN and return the removed entries."""
        doomed = [k for k in self._bindings if k[0] == vlan_id]
        return [self._bindings.pop(k) for k in doomed]


class NetVlanAO(ActiveObject):
    """MPSS-side VLAN manager -- own AO thread."""

    def __init__(
        self,
        slot: int,
        mpss_src: str,
        seed_vlans: Optional[list] = None,
        seed_bindings: Optional[list] = None,
    ) -> None:
        super().__init__("NetVlanAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._store = VlanStore(seed_vlans, seed_bindings)

        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_off)
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
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    def force_vlan_hw_accel(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.ForceVlanHwAccel, payload=data))

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            topics_vlan.create_vlan.req:         self._handle_create_vlan,
            topics_vlan.remove_vlan.req:         self._handle_remove_vlan,
            topics_vlan.query_vlan_info.req:     self._handle_query_vlan_info,
            topics_vlan.bind_vlan.req:           self._handle_bind_vlan,
            topics_vlan.unbind_vlan.req:         self._handle_unbind_vlan,
            topics_vlan.query_vlan_bindings.req: self._handle_query_vlan_bindings,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("VLAN AO subscribed (slot=%d)", self._slot)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "VLAN AO")

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._publish_subsys_ready(ready=True)
        _log.info("VLAN AO resubscribed (slot=%d)", self._slot)

    def _handle_create_vlan(self, msg: dict) -> None:
        data = msg.get("data") or {}
        vlan_id = data["vlanId"]
        iface_type = data["ifaceType"]

        if self._store.get(vlan_id, iface_type) is not None:
            self._send_error(topics_vlan.create_vlan.rsp, msg, "INVALID_OPERATION",
                             f"VLAN {vlan_id} already exists on {iface_type}")
            return

        nw_type = data.get("nwType", "LAN")
        create_bridge = data.get("createBridge", True)
        # The SDK forbids bridged VLANs on a WAN network type; rejecting here
        # keeps World State from holding a combination the real modem would
        # never accept.
        if nw_type == "WAN" and create_bridge:
            self._send_error(topics_vlan.create_vlan.rsp, msg, "INVALID_ARGUMENTS",
                             "createBridge is not allowed for NetworkType.WAN")
            return

        entry = VlanEntry(
            vlan_id=vlan_id,
            iface_type=iface_type,
            is_accelerated=data.get("isAccelerated", False),
            priority=data.get("priority", 0),
            nw_type=nw_type,
            create_bridge=create_bridge,
        )
        self._store.add(entry)

        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"],
            {"isAccelerated": entry.is_accelerated},
        )
        self._pub_rsp(topics_vlan.create_vlan.rsp, "net_vlan.create_vlan.rsp", env)
        _log.info("VLAN %d created on %s (accel=%s)", vlan_id, iface_type,
                  entry.is_accelerated)

    def _handle_remove_vlan(self, msg: dict) -> None:
        data = msg.get("data") or {}
        vlan_id = data["vlanId"]
        iface_type = data["ifaceType"]

        removed = self._store.remove(vlan_id, iface_type)
        if removed is None:
            self._send_error(topics_vlan.remove_vlan.rsp, msg, "INVALID_ARGUMENTS",
                             f"no VLAN {vlan_id} on {iface_type}")
            return
        # Cascade: removing the VLAN must not leave bindings pointing at it
        # (see VlanStore.remove_bindings_for_vlan).
        dropped = self._store.remove_bindings_for_vlan(vlan_id)

        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_vlan.remove_vlan.rsp, "net_vlan.remove_vlan.rsp", env)
        _log.info("VLAN %d removed from %s (%d binding(s) dropped)",
                  vlan_id, iface_type, len(dropped))

    def _handle_query_vlan_info(self, msg: dict) -> None:
        vlans = [v.to_wire() for v in self._store.all_vlans()]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"vlans": vlans})
        self._pub_rsp(topics_vlan.query_vlan_info.rsp, "net_vlan.query_vlan_info.rsp", env)

    def _handle_bind_vlan(self, msg: dict) -> None:
        data = msg.get("data") or {}
        vlan_id = data["vlanId"]
        backhaul = data["backhaul"]

        # Bind targets a VLAN id regardless of which PHY it sits on, so check
        # id-existence rather than (id, iface).
        if not self._store.has_vlan_id(vlan_id):
            self._send_error(topics_vlan.bind_vlan.rsp, msg, "INVALID_ARGUMENTS",
                             f"no VLAN with id {vlan_id}")
            return
        if self._store.get_binding(vlan_id, backhaul) is not None:
            self._send_error(topics_vlan.bind_vlan.rsp, msg, "INVALID_OPERATION",
                             f"VLAN {vlan_id} already bound to {backhaul}")
            return

        binding = VlanBinding(
            vlan_id=vlan_id,
            backhaul=backhaul,
            # slot/profileId are only meaningful for WWAN; storing them for
            # other backhauls would echo don't-care values back out of
            # query_vlan_bindings as if they were real.
            slot=data.get("slot") if backhaul == _WWAN else None,
            profile_id=data.get("profileId") if backhaul == _WWAN else None,
            bh_vlan_id=data.get("bhVlanId"),
        )
        self._store.add_binding(binding)

        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_vlan.bind_vlan.rsp, "net_vlan.bind_vlan.rsp", env)
        _log.info("VLAN %d bound to %s", vlan_id, backhaul)

    def _handle_unbind_vlan(self, msg: dict) -> None:
        data = msg.get("data") or {}
        vlan_id = data["vlanId"]
        backhaul = data["backhaul"]

        if self._store.remove_binding(vlan_id, backhaul) is None:
            self._send_error(topics_vlan.unbind_vlan.rsp, msg, "INVALID_ARGUMENTS",
                             f"VLAN {vlan_id} is not bound to {backhaul}")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_vlan.unbind_vlan.rsp, "net_vlan.unbind_vlan.rsp", env)
        _log.info("VLAN %d unbound from %s", vlan_id, backhaul)

    def _handle_query_vlan_bindings(self, msg: dict) -> None:
        data = msg.get("data") or {}
        backhaul = data.get("backhaul")
        slot = data.get("slot")

        bindings = self._store.all_bindings()
        if backhaul is not None:
            bindings = [b for b in bindings if b.backhaul == backhaul]
        if slot is not None:
            # Only WWAN bindings carry a slot; a slot filter must not silently
            # drop every non-WWAN binding, so it applies only where slot is set.
            bindings = [b for b in bindings if b.slot is None or b.slot == slot]

        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"bindings": [b.to_wire() for b in bindings]})
        self._pub_rsp(topics_vlan.query_vlan_bindings.rsp,
                      "net_vlan.query_vlan_bindings.rsp", env)

    def _apply_force_vlan_hw_accel(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        self._pub_ind(topics_vlan.hw_accel_state.ind, "net_vlan.hw_accel_state.ind",
                      {"state": data["state"]})
        _log.info("VLAN hw-accel state forced to %s", data["state"])

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
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(topics_vlan.subsys_ready_net_vlan.ind,
                      "net_vlan.subsys_ready_net_vlan.ind",
                      {"ready": ready, "status": status}, retain=True)
        _log.debug("net_vlan subsystem ready=%s published (slot=%d)", ready, self._slot)


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
    """Composite parent; defers messages until Ready."""
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
    elif e.signal == signals.ForceVlanHwAccel:
        _log.debug("VLAN AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("VLAN AO: Resubscribe dropped -- not Ready")
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
        chart._publish_subsys_ready(ready=True)
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        if chart._publish_fn is not None:
            try:
                chart._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish net_vlan ready=false on stop: %s", exc)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceVlanHwAccel:
        chart._apply_force_vlan_hw_accel(e.payload)
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
    """Stop the AO and unsubscribe."""
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


__all__ = ["NetVlanAO", "VlanBinding", "VlanEntry", "VlanStore"]
