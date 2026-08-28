# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for NatStore/NetNatAO and NetSocksAO. No broker required."""
from __future__ import annotations

import time

from sml.mpss.net.nat import NatEntry, NatStore, NetNatAO
from sml.mpss.net.socks import NetSocksAO
from sml.mpss.net.tests._helpers import MockPublish, boot, err, inds, ok, send
from generated.python.topics import net_nat as TN
from generated.python.topics import net_socks as TS

# TCP; IpProtocol is a raw uint8_t on the wire.
_TCP = 6
_UDP = 17
_ENTRY = {"backhaul": "ETH", "addr": "192.168.1.10", "port": 8080,
          "globalPort": 80, "proto": _TCP}


def _nat(pub, **kw) -> NetNatAO:
    return boot(NetNatAO(mpss_src="mpss-dev-1", **kw), pub, "smfn_nat_ready")


def _socks(pub, **kw) -> NetSocksAO:
    return boot(NetSocksAO(mpss_src="mpss-dev-1", **kw), pub, "smfn_socks_ready")


class TestNatStore:
    def test_same_forward_on_two_backhauls_coexists(self):
        """Every NAT operation is BackhaulInfo-scoped, so an identical forward
        on ETH and WWAN is two distinct entries."""
        store = NatStore()
        assert store.add(NatEntry("10.0.0.1", 80, 8080, _TCP, "ETH"))
        assert store.add(NatEntry("10.0.0.1", 80, 8080, _TCP, "WWAN"))
        assert len(store.all_entries()) == 2

    def test_identical_entry_rejected(self):
        store = NatStore()
        assert store.add(NatEntry("10.0.0.1", 80, 8080, _TCP, "ETH"))
        assert store.add(NatEntry("10.0.0.1", 80, 8080, _TCP, "ETH")) is False

    def test_protocol_is_part_of_identity(self):
        """Same ports on TCP and UDP are different forwards."""
        store = NatStore()
        assert store.add(NatEntry("10.0.0.1", 80, 8080, _TCP, "ETH"))
        assert store.add(NatEntry("10.0.0.1", 80, 8080, _UDP, "ETH"))
        assert len(store.all_entries()) == 2

    def test_for_backhaul_filters(self):
        store = NatStore(seed_entries=[
            {"addr": "10.0.0.1", "port": 80, "global_port": 8080,
             "proto": _TCP, "backhaul": "ETH"},
            {"addr": "10.0.0.2", "port": 81, "global_port": 8081,
             "proto": _TCP, "backhaul": "WWAN"},
        ])
        assert len(store.for_backhaul("ETH")) == 1


class TestNatRpcs:
    def test_add_then_request(self):
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req, dict(_ENTRY))
        ok(pub, TN.add_nat_entry.rsp)
        pub.reset()
        send(ao, TN.request_nat_entries.req, {"backhaul": "ETH"})
        entries = ok(pub, TN.request_nat_entries.rsp)["entries"]
        assert entries == [{"addr": "192.168.1.10", "port": 8080,
                            "globalPort": 80, "proto": _TCP}]

    def test_wire_omits_backhaul(self):
        """requestStaticNatEntries returns std::vector<NatConfig>, which has no
        backhaul member -- the caller already scoped the query."""
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req, dict(_ENTRY))
        pub.reset()
        send(ao, TN.request_nat_entries.req, {"backhaul": "ETH"})
        entry = ok(pub, TN.request_nat_entries.rsp)["entries"][0]
        assert "backhaul" not in entry

    def test_duplicate_add_rejected(self):
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req, dict(_ENTRY))
        pub.reset()
        send(ao, TN.add_nat_entry.req, dict(_ENTRY))
        assert err(pub, TN.add_nat_entry.rsp)["code"] == "INVALID_OPERATION"

    def test_remove_existing(self):
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req, dict(_ENTRY))
        pub.reset()
        send(ao, TN.remove_nat_entry.req, dict(_ENTRY))
        ok(pub, TN.remove_nat_entry.rsp)
        assert ao._store.all_entries() == []

    def test_remove_unknown_errors(self):
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.remove_nat_entry.req, dict(_ENTRY))
        assert err(pub, TN.remove_nat_entry.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_request_other_backhaul_is_empty(self):
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req, dict(_ENTRY))
        pub.reset()
        send(ao, TN.request_nat_entries.req, {"backhaul": "WWAN"})
        assert ok(pub, TN.request_nat_entries.rsp)["entries"] == []

    def test_non_wwan_dont_care_fields_discarded(self):
        """slot/profileId are don't-care for non-WWAN; retaining them would make
        two otherwise-identical ETH entries look distinct."""
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req, dict(_ENTRY, slot=1, profileId=3))
        ok(pub, TN.add_nat_entry.rsp)
        entry = ao._store.all_entries()[0]
        assert entry.slot is None and entry.profile_id is None

    def test_wwan_entry_keeps_slot_and_profile(self):
        pub = MockPublish()
        ao = _nat(pub)
        send(ao, TN.add_nat_entry.req,
             dict(_ENTRY, backhaul="WWAN", slot=1, profileId=3))
        ok(pub, TN.add_nat_entry.rsp)
        entry = ao._store.all_entries()[0]
        assert (entry.slot, entry.profile_id) == (1, 3)


class TestSocks:
    def test_enable(self):
        pub = MockPublish()
        ao = _socks(pub)
        assert ao.enabled is False
        send(ao, TS.enable_socks.req, {"enable": True})
        ok(pub, TS.enable_socks.rsp)
        assert ao.enabled is True

    def test_disable(self):
        pub = MockPublish()
        ao = _socks(pub, enabled=True)
        send(ao, TS.enable_socks.req, {"enable": False})
        ok(pub, TS.enable_socks.rsp)
        assert ao.enabled is False

    def test_enable_is_idempotent(self):
        """The SDK does not document re-enabling as an error, so it succeeds."""
        pub = MockPublish()
        ao = _socks(pub, enabled=True)
        send(ao, TS.enable_socks.req, {"enable": True})
        ok(pub, TS.enable_socks.rsp)
        assert ao.enabled is True

    def test_retained_ready(self):
        pub = MockPublish()
        ao = _socks(pub)
        ao.resubscribe()
        time.sleep(0.1)
        got = inds(pub, TS.subsys_ready_net_socks.ind)
        assert got and got[0]["retain"] is True
        ao.stop()
