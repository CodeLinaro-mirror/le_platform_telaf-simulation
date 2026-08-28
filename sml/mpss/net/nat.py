# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side static-NAT manager for ``taf_net.api``.

Entries are keyed by backhaul because the same forwarding tuple can exist on
multiple backhauls.
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
from generated.python.topics import net_nat as topics_nat
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.net.nat")

_WWAN = "WWAN"


class NatEntry:
    """One static NAT entry; mirrors ``NatConfig`` plus its backhaul scope."""

    __slots__ = ("addr", "port", "global_port", "proto", "backhaul", "slot",
                 "profile_id", "bh_vlan_id")

    def __init__(self, addr: str, port: int, global_port: int, proto: int,
                 backhaul: str, slot: Optional[int] = None,
                 profile_id: Optional[int] = None,
                 bh_vlan_id: Optional[int] = None) -> None:
        self.addr = addr
        self.port = port
        self.global_port = global_port
        self.proto = proto
        self.backhaul = backhaul
        self.slot = slot
        self.profile_id = profile_id
        self.bh_vlan_id = bh_vlan_id

    @property
    def key(self) -> tuple:
        # Scoped by backhaul: the same forward may exist on two backhauls.
        # globalPort is part of the identity because two private endpoints can
        # share a private port while differing on the global one.
        return (self.backhaul, self.addr, self.port, self.global_port, self.proto)

    def to_wire(self) -> dict:
        # requestStaticNatEntries returns std::vector<NatConfig>, which carries
        # no backhaul field -- the caller already scoped the query -- so only
        # the NatConfig members go on the wire.
        return {"addr": self.addr, "port": self.port,
                "globalPort": self.global_port, "proto": self.proto}


class NatStore:
    def __init__(self, seed_entries: Optional[list] = None) -> None:
        self._entries: dict[tuple, NatEntry] = {}
        for e in seed_entries or []:
            entry = NatEntry(**e) if isinstance(e, dict) else e
            self._entries[entry.key] = entry

    def add(self, entry: NatEntry) -> bool:
        """Insert an entry and return False for duplicates."""
        if entry.key in self._entries:
            return False
        self._entries[entry.key] = entry
        return True

    def remove(self, entry: NatEntry) -> bool:
        return self._entries.pop(entry.key, None) is not None

    def for_backhaul(self, backhaul: str) -> list[NatEntry]:
        return [e for e in self._entries.values() if e.backhaul == backhaul]

    def all_entries(self) -> list[NatEntry]:
        return list(self._entries.values())


class NetNatAO(ActiveObject):
    """MPSS-side NAT manager -- own AO thread."""

    def __init__(self, mpss_src: str, seed_entries: Optional[list] = None) -> None:
        super().__init__("NetNatAO")
        self._mpss_src = mpss_src
        self._store = NatStore(seed_entries)

        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_nat_off)
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

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            topics_nat.add_nat_entry.req:      self._handle_add,
            topics_nat.remove_nat_entry.req:   self._handle_remove,
            topics_nat.request_nat_entries.req: self._handle_request,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("NAT AO subscribed")

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "NAT AO")

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._publish_subsys_ready(ready=True)

    def _entry_from(self, data: dict) -> NatEntry:
        backhaul = data["backhaul"]
        return NatEntry(
            addr=data["addr"], port=data["port"],
            global_port=data["globalPort"], proto=data["proto"],
            backhaul=backhaul,
            # slot/profileId are don't-care for non-WWAN backhauls; keeping
            # them would make two otherwise-identical ETH entries look distinct.
            slot=data.get("slot") if backhaul == _WWAN else None,
            profile_id=data.get("profileId") if backhaul == _WWAN else None,
            bh_vlan_id=data.get("bhVlanId"),
        )

    def _handle_add(self, msg: dict) -> None:
        entry = self._entry_from(msg.get("data") or {})
        if not self._store.add(entry):
            self._send_error(topics_nat.add_nat_entry.rsp, msg, "INVALID_OPERATION",
                             "identical NAT entry already exists")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_nat.add_nat_entry.rsp, "net_nat.add_nat_entry.rsp", env)
        _log.info("NAT entry added on %s: %s:%d -> :%d proto=%d",
                  entry.backhaul, entry.addr, entry.port, entry.global_port, entry.proto)

    def _handle_remove(self, msg: dict) -> None:
        entry = self._entry_from(msg.get("data") or {})
        if not self._store.remove(entry):
            self._send_error(topics_nat.remove_nat_entry.rsp, msg, "INVALID_ARGUMENTS",
                             "no matching NAT entry")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_nat.remove_nat_entry.rsp, "net_nat.remove_nat_entry.rsp", env)

    def _handle_request(self, msg: dict) -> None:
        data = msg.get("data") or {}
        entries = self._store.for_backhaul(data["backhaul"])
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"entries": [e.to_wire() for e in entries]})
        self._pub_rsp(topics_nat.request_nat_entries.rsp,
                      "net_nat.request_nat_entries.rsp", env)

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
        self._pub_ind(topics_nat.subsys_ready_net_nat.ind,
                      "net_nat.subsys_ready_net_nat.ind",
                      {"ready": ready,
                       "status": "AVAILABLE" if ready else "UNAVAILABLE"},
                      retain=True)


@spy_on
def smfn_nat_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_nat_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_nat_operating(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_nat_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_nat_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_nat_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_nat_ready)
    else:
        chart.temp.fun = smfn_nat_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_nat_ready(chart, e):
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
                _log.warning("failed to publish net_nat ready=false: %s", exc)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_nat_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_nat_stopping(chart, e):
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


__all__ = ["NatEntry", "NatStore", "NetNatAO"]
