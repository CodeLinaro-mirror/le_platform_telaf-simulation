# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for L2tpStore and NetL2tpAO. No broker required."""
from __future__ import annotations

import time

from sml.mpss.net.l2tp import (
    L2tpSession,
    L2tpSessionBinding,
    L2tpStore,
    L2tpTunnel,
    NetL2tpAO,
)
from sml.mpss.net.tests._helpers import MockPublish, boot, err, inds, ok, send
from generated.python.topics import net_l2tp as T


def _ao(pub, **kw) -> NetL2tpAO:
    return boot(NetL2tpAO(mpss_src="mpss-dev-1", **kw), pub, "smfn_l2tp_ready")


_T1 = {"loc_id": 1, "prot": "UDP", "peer_id": 11,
       "sessions": [{"loc_id": 100, "peer_id": 200}]}


class TestL2tpStore:
    def test_find_session_searches_every_tunnel(self):
        """bindSessionToBackhaul takes only a locId, so resolution must be
        global even though sessions are declared inside a tunnel."""
        store = L2tpStore(seed_tunnels=[
            {"loc_id": 1, "sessions": [{"loc_id": 100, "peer_id": 200}]},
            {"loc_id": 2, "sessions": [{"loc_id": 300, "peer_id": 400}]},
        ])
        assert store.find_session(300) is not None
        assert store.find_session(999) is None

    def test_remove_bindings_for_sessions(self):
        store = L2tpStore()
        store.add_binding(L2tpSessionBinding(100, "WWAN", slot=1, profile_id=3))
        store.add_binding(L2tpSessionBinding(100, "ETH"))
        store.add_binding(L2tpSessionBinding(300, "ETH"))
        dropped = store.remove_bindings_for_sessions({100})
        assert len(dropped) == 2
        assert len(store.all_bindings()) == 1

    def test_tunnel_to_wire_carries_every_accessor_field(self):
        """taf_net's tunnel accessors never make a second round-trip, so
        requestConfig must carry every field they read."""
        t = L2tpTunnel(loc_id=1, prot="IP", peer_id=2, local_udp_port=1701,
                       peer_udp_port=1702, peer_ipv4_addr="10.0.0.1",
                       loc_iface="eth0", ip_type="IPV4")
        wire = t.to_wire()
        for field in ("locId", "peerId", "prot", "localUdpPort", "peerUdpPort",
                      "peerIpv6Addr", "peerIpv6GwAddr", "peerIpv4Addr",
                      "peerIpv4GwAddr", "locIface", "ipType", "sessions"):
            assert field in wire, f"missing {field}"


class TestSetAndRequestConfig:
    def test_zero_mtu_resolves_to_platform_default(self):
        """The SDK documents 1422 as the MTU applied for an unset size; the
        client should see the MTU in force, not the 0 sentinel."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_l2tp_config.req,
             {"enable": True, "enableMss": True, "enableMtu": True, "mtuSize": 0})
        ok(pub, T.set_l2tp_config.rsp)
        assert ao._store.mtu_size == 1422

    def test_explicit_mtu_preserved(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_l2tp_config.req,
             {"enable": True, "enableMss": False, "enableMtu": True, "mtuSize": 1400})
        ok(pub, T.set_l2tp_config.rsp)
        assert ao._store.mtu_size == 1400

    def test_request_config_reports_flags_and_tunnels(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.request_l2tp_config.req, {})
        data = ok(pub, T.request_l2tp_config.rsp)
        assert data["tunnels"][0]["locId"] == 1
        assert data["tunnels"][0]["sessions"] == [{"locId": 100, "peerId": 200}]
        assert set(data) == {"enableMtu", "enableTcpMss", "mtuSize", "tunnels"}


class TestTunnels:
    def test_add_tunnel_with_sessions(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.add_tunnel.req,
             {"locId": 1, "prot": "UDP", "peerId": 11, "locIface": "eth0",
              "sessions": [{"locId": 100, "peerId": 200}]})
        ok(pub, T.add_tunnel.rsp)
        tunnel = ao._store.get_tunnel(1)
        assert tunnel.loc_iface == "eth0"
        assert len(tunnel.sessions) == 1

    def test_duplicate_tunnel_rejected(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.add_tunnel.req, {"locId": 1, "prot": "UDP"})
        assert err(pub, T.add_tunnel.rsp)["code"] == "INVALID_OPERATION"

    def test_remove_unknown_tunnel_errors(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.remove_tunnel.req, {"tunnelId": 99})
        assert err(pub, T.remove_tunnel.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_remove_tunnel_cascades_to_session_bindings(self):
        """A removed tunnel's sessions are gone, so querySessionToBackhaul
        Bindings must not still report them."""
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)],
                 seed_bindings=[{"loc_id": 100, "backhaul": "ETH"}])
        send(ao, T.remove_tunnel.req, {"tunnelId": 1})
        ok(pub, T.remove_tunnel.rsp)
        assert ao._store.all_bindings() == []

    def test_remove_tunnel_leaves_other_tunnels_bindings(self):
        pub = MockPublish()
        ao = _ao(pub,
                 seed_tunnels=[dict(_T1),
                               {"loc_id": 2, "sessions": [{"loc_id": 300, "peer_id": 400}]}],
                 seed_bindings=[{"loc_id": 100, "backhaul": "ETH"},
                                {"loc_id": 300, "backhaul": "ETH"}])
        send(ao, T.remove_tunnel.req, {"tunnelId": 1})
        ok(pub, T.remove_tunnel.rsp)
        assert [b.loc_id for b in ao._store.all_bindings()] == [300]


class TestSessions:
    def test_add_session_to_existing_tunnel(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[{"loc_id": 1}])
        send(ao, T.add_session.req, {"tunnelId": 1, "locId": 100, "peerId": 200})
        ok(pub, T.add_session.rsp)
        assert ao._store.get_tunnel(1).get_session(100) is not None

    def test_add_session_unknown_tunnel_errors(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.add_session.req, {"tunnelId": 9, "locId": 100, "peerId": 200})
        assert err(pub, T.add_session.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_duplicate_session_rejected(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.add_session.req, {"tunnelId": 1, "locId": 100, "peerId": 200})
        assert err(pub, T.add_session.rsp)["code"] == "INVALID_OPERATION"

    def test_add_session_preserves_existing_sessions(self):
        """addSession is documented not to disturb the tunnel's other
        sessions."""
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.add_session.req, {"tunnelId": 1, "locId": 101, "peerId": 201})
        ok(pub, T.add_session.rsp)
        assert sorted(s.loc_id for s in ao._store.get_tunnel(1).sessions) == [100, 101]

    def test_remove_session_cascades_its_binding(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)],
                 seed_bindings=[{"loc_id": 100, "backhaul": "ETH"}])
        send(ao, T.remove_session.req, {"tunnelId": 1, "sessionId": 100})
        ok(pub, T.remove_session.rsp)
        assert ao._store.get_tunnel(1).sessions == []
        assert ao._store.all_bindings() == []

    def test_remove_unknown_session_errors(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[{"loc_id": 1}])
        send(ao, T.remove_session.req, {"tunnelId": 1, "sessionId": 999})
        assert err(pub, T.remove_session.rsp)["code"] == "INVALID_ARGUMENTS"


class TestSessionBindings:
    def test_bind_wwan_keeps_slot_and_profile(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.bind_session.req,
             {"locId": 100, "backhaul": "WWAN", "slot": 1, "profileId": 3})
        ok(pub, T.bind_session.rsp)
        b = ao._store.get_binding(100, "WWAN")
        assert (b.slot, b.profile_id) == (1, 3)

    def test_bind_non_wwan_discards_dont_care_fields(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.bind_session.req,
             {"locId": 100, "backhaul": "ETH", "slot": 1, "profileId": 3})
        ok(pub, T.bind_session.rsp)
        b = ao._store.get_binding(100, "ETH")
        assert b.slot is None and b.profile_id is None
        assert "slot" not in b.to_wire()

    def test_bind_unknown_session_errors(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.bind_session.req, {"locId": 999, "backhaul": "ETH"})
        assert err(pub, T.bind_session.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_duplicate_bind_rejected(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)],
                 seed_bindings=[{"loc_id": 100, "backhaul": "ETH"}])
        send(ao, T.bind_session.req, {"locId": 100, "backhaul": "ETH"})
        assert err(pub, T.bind_session.rsp)["code"] == "INVALID_OPERATION"

    def test_unbind_not_bound_errors(self):
        pub = MockPublish()
        ao = _ao(pub, seed_tunnels=[dict(_T1)])
        send(ao, T.unbind_session.req, {"locId": 100, "backhaul": "ETH"})
        assert err(pub, T.unbind_session.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_query_filters_by_backhaul(self):
        pub = MockPublish()
        ao = _ao(pub, seed_bindings=[{"loc_id": 100, "backhaul": "ETH"},
                                     {"loc_id": 101, "backhaul": "WWAN"}])
        send(ao, T.query_session_bindings.req, {"backhaul": "WWAN"})
        got = ok(pub, T.query_session_bindings.rsp)["bindings"]
        assert [b["locId"] for b in got] == [101]

    def test_query_unfiltered_returns_all(self):
        pub = MockPublish()
        ao = _ao(pub, seed_bindings=[{"loc_id": 100, "backhaul": "ETH"},
                                     {"loc_id": 101, "backhaul": "WWAN"}])
        send(ao, T.query_session_bindings.req, {})
        assert len(ok(pub, T.query_session_bindings.rsp)["bindings"]) == 2


class TestReadiness:
    def test_retained_ready_on_entry(self):
        pub = MockPublish()
        ao = NetL2tpAO(mpss_src="mpss-dev-1")
        boot(ao, pub, "smfn_l2tp_ready")
        # boot() clears, so re-check by asserting the AO republishes on
        # resubscribe -- the same retained path a reconnecting PA relies on.
        ao.resubscribe()
        time.sleep(0.1)
        got = inds(pub, T.subsys_ready_net_l2tp.ind)
        assert len(got) == 1
        assert got[0]["retain"] is True
        assert got[0]["payload"]["data"] == {"ready": True, "status": "AVAILABLE"}
        ao.stop()
