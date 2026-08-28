# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side L2TP manager for ``taf_net.api``.

The AO owns tunnel, session, binding, and system configuration state. All
mutations run on its AO thread.
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
from generated.python.topics import net_l2tp as topics_l2tp
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.net.l2tp")

_WWAN = "WWAN"
# The SDK documents 1422 as the MTU applied when setConfig is called with an
# unset (0) size.
_DEFAULT_MTU = 1422

# Tunnel fields that are optional on the wire and default to "".
_STR_FIELDS = ("peerIpv6Addr", "peerIpv6GwAddr", "peerIpv4Addr", "peerIpv4GwAddr",
               "locIface")


class L2tpSession:
    """One session inside a tunnel; mirrors ``L2tpSessionConfig``."""

    __slots__ = ("loc_id", "peer_id")

    def __init__(self, loc_id: int, peer_id: int) -> None:
        self.loc_id = loc_id
        self.peer_id = peer_id

    def to_wire(self) -> dict:
        return {"locId": self.loc_id, "peerId": self.peer_id}


class L2tpTunnel:
    """One L2TP tunnel; mirrors ``L2tpTunnelConfig``."""

    __slots__ = ("loc_id", "peer_id", "prot", "local_udp_port", "peer_udp_port",
                 "peer_ipv6_addr", "peer_ipv6_gw_addr", "peer_ipv4_addr",
                 "peer_ipv4_gw_addr", "loc_iface", "ip_type", "sessions")

    def __init__(self, loc_id: int, prot: str = "NONE", peer_id: int = 0,
                 local_udp_port: int = 0, peer_udp_port: int = 0,
                 peer_ipv6_addr: str = "", peer_ipv6_gw_addr: str = "",
                 peer_ipv4_addr: str = "", peer_ipv4_gw_addr: str = "",
                 loc_iface: str = "", ip_type: str = "UNKNOWN",
                 sessions: Optional[list] = None) -> None:
        self.loc_id = loc_id
        self.prot = prot
        self.peer_id = peer_id
        self.local_udp_port = local_udp_port
        self.peer_udp_port = peer_udp_port
        self.peer_ipv6_addr = peer_ipv6_addr
        self.peer_ipv6_gw_addr = peer_ipv6_gw_addr
        self.peer_ipv4_addr = peer_ipv4_addr
        self.peer_ipv4_gw_addr = peer_ipv4_gw_addr
        self.loc_iface = loc_iface
        self.ip_type = ip_type
        self.sessions: list[L2tpSession] = []
        for s in sessions or []:
            self.sessions.append(
                L2tpSession(**s) if isinstance(s, dict) else s
            )

    def get_session(self, loc_id: int) -> Optional[L2tpSession]:
        return next((s for s in self.sessions if s.loc_id == loc_id), None)

    def to_wire(self) -> dict:
        return {
            "locId": self.loc_id,
            "peerId": self.peer_id,
            "prot": self.prot,
            "localUdpPort": self.local_udp_port,
            "peerUdpPort": self.peer_udp_port,
            "peerIpv6Addr": self.peer_ipv6_addr,
            "peerIpv6GwAddr": self.peer_ipv6_gw_addr,
            "peerIpv4Addr": self.peer_ipv4_addr,
            "peerIpv4GwAddr": self.peer_ipv4_gw_addr,
            "locIface": self.loc_iface,
            "ipType": self.ip_type,
            "sessions": [s.to_wire() for s in self.sessions],
        }


class L2tpSessionBinding:
    """One session-to-backhaul binding; mirrors ``L2tpSessionBindConfig``."""

    __slots__ = ("loc_id", "backhaul", "slot", "profile_id", "bh_vlan_id")

    def __init__(self, loc_id: int, backhaul: str, slot: Optional[int] = None,
                 profile_id: Optional[int] = None,
                 bh_vlan_id: Optional[int] = None) -> None:
        self.loc_id = loc_id
        self.backhaul = backhaul
        self.slot = slot
        self.profile_id = profile_id
        self.bh_vlan_id = bh_vlan_id

    @property
    def key(self) -> tuple:
        return (self.loc_id, self.backhaul)

    def to_wire(self) -> dict:
        out = {"locId": self.loc_id, "backhaul": self.backhaul}
        # Emit WWAN-only fields only when set -- sending profileId=null for an
        # ETH binding would present don't-care as "explicitly none".
        if self.slot is not None:
            out["slot"] = self.slot
        if self.profile_id is not None:
            out["profileId"] = self.profile_id
        if self.bh_vlan_id is not None:
            out["bhVlanId"] = self.bh_vlan_id
        return out


class L2tpStore:
    """Tunnel table + session bindings + system flags."""

    def __init__(self, seed_tunnels: Optional[list] = None,
                 seed_bindings: Optional[list] = None,
                 enabled: bool = False, enable_mss: bool = False,
                 enable_mtu: bool = False, mtu_size: int = 0) -> None:
        self.enabled = enabled
        self.enable_mss = enable_mss
        self.enable_mtu = enable_mtu
        self.mtu_size = mtu_size
        self._tunnels: dict[int, L2tpTunnel] = {}
        self._bindings: dict[tuple, L2tpSessionBinding] = {}
        for t in seed_tunnels or []:
            tunnel = L2tpTunnel(**t) if isinstance(t, dict) else t
            self._tunnels[tunnel.loc_id] = tunnel
        for b in seed_bindings or []:
            binding = L2tpSessionBinding(**b) if isinstance(b, dict) else b
            self._bindings[binding.key] = binding

    def get_tunnel(self, loc_id: int) -> Optional[L2tpTunnel]:
        return self._tunnels.get(loc_id)

    def add_tunnel(self, tunnel: L2tpTunnel) -> None:
        self._tunnels[tunnel.loc_id] = tunnel

    def remove_tunnel(self, loc_id: int) -> Optional[L2tpTunnel]:
        return self._tunnels.pop(loc_id, None)

    def all_tunnels(self) -> list[L2tpTunnel]:
        return list(self._tunnels.values())

    def find_session(self, loc_id: int) -> Optional[L2tpSession]:
        """Find a session by local ID across all tunnels."""
        for t in self._tunnels.values():
            s = t.get_session(loc_id)
            if s is not None:
                return s
        return None

    def get_binding(self, loc_id: int, backhaul: str) -> Optional[L2tpSessionBinding]:
        return self._bindings.get((loc_id, backhaul))

    def add_binding(self, binding: L2tpSessionBinding) -> None:
        self._bindings[binding.key] = binding

    def remove_binding(self, loc_id: int, backhaul: str) -> Optional[L2tpSessionBinding]:
        return self._bindings.pop((loc_id, backhaul), None)

    def all_bindings(self) -> list[L2tpSessionBinding]:
        return list(self._bindings.values())

    def remove_bindings_for_sessions(self, loc_ids: set) -> list:
        """Remove bindings for the supplied session IDs."""
        doomed = [k for k in self._bindings if k[0] in loc_ids]
        return [self._bindings.pop(k) for k in doomed]


class NetL2tpAO(ActiveObject):
    """MPSS-side L2TP manager -- own AO thread."""

    def __init__(self, mpss_src: str, seed_tunnels: Optional[list] = None,
                 seed_bindings: Optional[list] = None) -> None:
        super().__init__("NetL2tpAO")
        self._mpss_src = mpss_src
        self._store = L2tpStore(seed_tunnels, seed_bindings)

        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_l2tp_off)
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
            topics_l2tp.set_l2tp_config.req:        self._handle_set_config,
            topics_l2tp.request_l2tp_config.req:    self._handle_request_config,
            topics_l2tp.add_tunnel.req:             self._handle_add_tunnel,
            topics_l2tp.remove_tunnel.req:          self._handle_remove_tunnel,
            topics_l2tp.add_session.req:            self._handle_add_session,
            topics_l2tp.remove_session.req:         self._handle_remove_session,
            topics_l2tp.bind_session.req:           self._handle_bind_session,
            topics_l2tp.unbind_session.req:         self._handle_unbind_session,
            topics_l2tp.query_session_bindings.req: self._handle_query_bindings,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("L2TP AO subscribed")

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "L2TP AO")

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._publish_subsys_ready(ready=True)

    def _handle_set_config(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self._store.enabled = data["enable"]
        self._store.enable_mss = data["enableMss"]
        self._store.enable_mtu = data["enableMtu"]
        mtu = data.get("mtuSize", 0)
        # 0 means "platform default" per the SDK; resolve it here so
        # request_l2tp_config reports the MTU actually in force rather than a
        # sentinel the client would have to interpret.
        self._store.mtu_size = mtu if mtu else _DEFAULT_MTU
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.set_l2tp_config.rsp, "net_l2tp.set_l2tp_config.rsp", env)
        _log.info("L2TP config: enable=%s mss=%s mtu=%s size=%d",
                  self._store.enabled, self._store.enable_mss,
                  self._store.enable_mtu, self._store.mtu_size)

    def _handle_request_config(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {
            "enableMtu": self._store.enable_mtu,
            "enableTcpMss": self._store.enable_mss,
            "mtuSize": self._store.mtu_size,
            "tunnels": [t.to_wire() for t in self._store.all_tunnels()],
        })
        self._pub_rsp(topics_l2tp.request_l2tp_config.rsp,
                      "net_l2tp.request_l2tp_config.rsp", env)

    def _handle_add_tunnel(self, msg: dict) -> None:
        data = msg.get("data") or {}
        loc_id = data["locId"]
        if self._store.get_tunnel(loc_id) is not None:
            self._send_error(topics_l2tp.add_tunnel.rsp, msg, "INVALID_OPERATION",
                             f"tunnel {loc_id} already exists")
            return
        tunnel = L2tpTunnel(
            loc_id=loc_id,
            prot=data["prot"],
            peer_id=data.get("peerId", 0),
            local_udp_port=data.get("localUdpPort", 0),
            peer_udp_port=data.get("peerUdpPort", 0),
            ip_type=data.get("ipType", "UNKNOWN"),
            sessions=[L2tpSession(s["locId"], s["peerId"])
                      for s in data.get("sessions", [])],
            **{_snake(f): data.get(f, "") for f in _STR_FIELDS},
        )
        self._store.add_tunnel(tunnel)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.add_tunnel.rsp, "net_l2tp.add_tunnel.rsp", env)
        _log.info("L2TP tunnel %d added (%d session(s))", loc_id, len(tunnel.sessions))

    def _handle_remove_tunnel(self, msg: dict) -> None:
        data = msg.get("data") or {}
        tunnel_id = data["tunnelId"]
        removed = self._store.remove_tunnel(tunnel_id)
        if removed is None:
            self._send_error(topics_l2tp.remove_tunnel.rsp, msg, "INVALID_ARGUMENTS",
                             f"no tunnel {tunnel_id}")
            return
        # Cascade: the tunnel's sessions are gone, so their bindings must go too.
        dropped = self._store.remove_bindings_for_sessions(
            {s.loc_id for s in removed.sessions}
        )
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.remove_tunnel.rsp, "net_l2tp.remove_tunnel.rsp", env)
        _log.info("L2TP tunnel %d removed (%d binding(s) dropped)",
                  tunnel_id, len(dropped))

    def _handle_add_session(self, msg: dict) -> None:
        data = msg.get("data") or {}
        tunnel = self._store.get_tunnel(data["tunnelId"])
        if tunnel is None:
            self._send_error(topics_l2tp.add_session.rsp, msg, "INVALID_ARGUMENTS",
                             f"no tunnel {data['tunnelId']}")
            return
        if tunnel.get_session(data["locId"]) is not None:
            self._send_error(topics_l2tp.add_session.rsp, msg, "INVALID_OPERATION",
                             f"session {data['locId']} already in tunnel")
            return
        tunnel.sessions.append(L2tpSession(data["locId"], data["peerId"]))
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.add_session.rsp, "net_l2tp.add_session.rsp", env)

    def _handle_remove_session(self, msg: dict) -> None:
        data = msg.get("data") or {}
        tunnel = self._store.get_tunnel(data["tunnelId"])
        if tunnel is None:
            self._send_error(topics_l2tp.remove_session.rsp, msg, "INVALID_ARGUMENTS",
                             f"no tunnel {data['tunnelId']}")
            return
        session = tunnel.get_session(data["sessionId"])
        if session is None:
            self._send_error(topics_l2tp.remove_session.rsp, msg, "INVALID_ARGUMENTS",
                             f"no session {data['sessionId']} in tunnel")
            return
        tunnel.sessions.remove(session)
        # Same cascade reason as remove_tunnel, scoped to the one session.
        self._store.remove_bindings_for_sessions({session.loc_id})
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.remove_session.rsp, "net_l2tp.remove_session.rsp", env)

    def _handle_bind_session(self, msg: dict) -> None:
        data = msg.get("data") or {}
        loc_id = data["locId"]
        backhaul = data["backhaul"]
        if self._store.find_session(loc_id) is None:
            self._send_error(topics_l2tp.bind_session.rsp, msg, "INVALID_ARGUMENTS",
                             f"no session with locId {loc_id}")
            return
        if self._store.get_binding(loc_id, backhaul) is not None:
            self._send_error(topics_l2tp.bind_session.rsp, msg, "INVALID_OPERATION",
                             f"session {loc_id} already bound to {backhaul}")
            return
        self._store.add_binding(L2tpSessionBinding(
            loc_id=loc_id,
            backhaul=backhaul,
            slot=data.get("slot") if backhaul == _WWAN else None,
            profile_id=data.get("profileId") if backhaul == _WWAN else None,
            bh_vlan_id=data.get("bhVlanId"),
        ))
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.bind_session.rsp, "net_l2tp.bind_session.rsp", env)

    def _handle_unbind_session(self, msg: dict) -> None:
        data = msg.get("data") or {}
        if self._store.remove_binding(data["locId"], data["backhaul"]) is None:
            self._send_error(topics_l2tp.unbind_session.rsp, msg, "INVALID_ARGUMENTS",
                             f"session {data['locId']} not bound to {data['backhaul']}")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_l2tp.unbind_session.rsp, "net_l2tp.unbind_session.rsp", env)

    def _handle_query_bindings(self, msg: dict) -> None:
        data = msg.get("data") or {}
        backhaul = data.get("backhaul")
        bindings = self._store.all_bindings()
        if backhaul is not None:
            bindings = [b for b in bindings if b.backhaul == backhaul]
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"bindings": [b.to_wire() for b in bindings]})
        self._pub_rsp(topics_l2tp.query_session_bindings.rsp,
                      "net_l2tp.query_session_bindings.rsp", env)

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
        self._pub_ind(topics_l2tp.subsys_ready_net_l2tp.ind,
                      "net_l2tp.subsys_ready_net_l2tp.ind",
                      {"ready": ready,
                       "status": "AVAILABLE" if ready else "UNAVAILABLE"},
                      retain=True)


def _snake(camel: str) -> str:
    out = []
    for ch in camel:
        if ch.isupper():
            out.append("_")
            out.append(ch.lower())
        else:
            out.append(ch)
    return "".join(out)


@spy_on
def smfn_l2tp_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_l2tp_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_l2tp_operating(chart, e):
    status = return_status.UNHANDLED
    if e.signal in (signals.ENTRY_SIGNAL, signals.EXIT_SIGNAL):
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_l2tp_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_l2tp_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("L2TP AO: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_l2tp_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_l2tp_ready)
    else:
        chart.temp.fun = smfn_l2tp_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_l2tp_ready(chart, e):
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
                _log.warning("failed to publish net_l2tp ready=false: %s", exc)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_l2tp_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_l2tp_stopping(chart, e):
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


__all__ = ["L2tpSession", "L2tpSessionBinding", "L2tpStore", "L2tpTunnel", "NetL2tpAO"]
