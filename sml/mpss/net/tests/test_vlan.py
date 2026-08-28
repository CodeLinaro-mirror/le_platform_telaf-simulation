# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for VlanStore and NetVlanAO.

No broker required -- publish_fn is mocked.
"""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

from sml.mpss.data.tests._helpers import wait_for_state
from sml.mpss.net.vlan import NetVlanAO, VlanBinding, VlanEntry, VlanStore
from generated.python.topics import net_vlan as topics_vlan


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "retain": retain})

    def reset(self):
        self.calls.clear()


def _make_ao(pub, seed_vlans=None, seed_bindings=None) -> NetVlanAO:
    ao = NetVlanAO(slot=1, mpss_src="mpss-dev-1",
                   seed_vlans=seed_vlans, seed_bindings=seed_bindings)
    ao.start(pub, MagicMock())
    wait_for_state(ao, "smfn_ready")
    pub.reset()
    return ao


def _send(ao, topic, data):
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "netsvc-master-1234", "data": data,
    }).encode()
    ao.handle_message(topic, msg)
    time.sleep(0.05)


def _rsp(pub, topic):
    return [c for c in pub.calls if c["topic"] == topic]


def _ok(pub, topic):
    """Single successful rsp payload's data block."""
    calls = _rsp(pub, topic)
    assert len(calls) == 1, f"expected 1 rsp on {topic}, got {len(calls)}"
    env = calls[0]["payload"]
    assert "error" not in env, f"unexpected error: {env.get('error')}"
    return env["data"]


def _err(pub, topic):
    calls = _rsp(pub, topic)
    assert len(calls) == 1, f"expected 1 rsp on {topic}, got {len(calls)}"
    env = calls[0]["payload"]
    assert "error" in env, f"expected an error, got data={env.get('data')}"
    return env["error"]


_ETH_VLAN = {"vlanId": 100, "ifaceType": "ETH"}


# ---------------------------------------------------------------------------
# VlanStore -- plain container, no AO
# ---------------------------------------------------------------------------

class TestVlanStore:
    def test_vlan_id_is_unique_per_interface_not_globally(self):
        """removeVlan takes (vlanId, ifaceType) precisely because the same id
        can exist on two PHYs at once."""
        store = VlanStore()
        store.add(VlanEntry(100, "ETH"))
        store.add(VlanEntry(100, "WLAN"))
        assert len(store.all_vlans()) == 2
        assert store.get(100, "ETH") is not None
        assert store.get(100, "WLAN") is not None

    def test_remove_returns_none_for_unknown(self):
        store = VlanStore()
        assert store.remove(999, "ETH") is None

    def test_remove_bindings_for_vlan_spans_backhauls(self):
        store = VlanStore()
        store.add(VlanEntry(100, "ETH"))
        store.add_binding(VlanBinding(100, "WWAN", slot=1, profile_id=3))
        store.add_binding(VlanBinding(100, "ETH"))
        store.add_binding(VlanBinding(200, "WWAN", slot=1, profile_id=4))
        dropped = store.remove_bindings_for_vlan(100)
        assert len(dropped) == 2
        assert len(store.all_bindings()) == 1

    def test_seed_accepts_dicts(self):
        store = VlanStore(seed_vlans=[{"vlan_id": 7, "iface_type": "ETH"}])
        assert store.get(7, "ETH") is not None


# ---------------------------------------------------------------------------
# create_vlan
# ---------------------------------------------------------------------------

class TestCreateVlan:
    def test_creates_and_reports_accel_status(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.create_vlan.req,
              {"vlanId": 100, "ifaceType": "ETH", "isAccelerated": True})
        data = _ok(pub, topics_vlan.create_vlan.rsp)
        assert data["isAccelerated"] is True
        assert ao._store.get(100, "ETH") is not None

    def test_defaults_applied_when_fields_omitted(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.create_vlan.req, dict(_ETH_VLAN))
        _ok(pub, topics_vlan.create_vlan.rsp)
        entry = ao._store.get(100, "ETH")
        assert entry.priority == 0
        assert entry.nw_type == "LAN"
        assert entry.create_bridge is True
        assert entry.is_accelerated is False

    def test_duplicate_on_same_iface_rejected(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}])
        _send(ao, topics_vlan.create_vlan.req, dict(_ETH_VLAN))
        assert _err(pub, topics_vlan.create_vlan.rsp)["code"] == "INVALID_OPERATION"

    def test_same_id_different_iface_allowed(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}])
        _send(ao, topics_vlan.create_vlan.req, {"vlanId": 100, "ifaceType": "WLAN"})
        _ok(pub, topics_vlan.create_vlan.rsp)
        assert len(ao._store.all_vlans()) == 2

    def test_bridged_wan_vlan_rejected(self):
        """The SDK forbids creating a bridged VLAN on NetworkType::WAN."""
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.create_vlan.req,
              {"vlanId": 100, "ifaceType": "ETH", "nwType": "WAN",
               "createBridge": True})
        assert _err(pub, topics_vlan.create_vlan.rsp)["code"] == "INVALID_ARGUMENTS"
        assert ao._store.get(100, "ETH") is None

    def test_unbridged_wan_vlan_allowed(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.create_vlan.req,
              {"vlanId": 100, "ifaceType": "ETH", "nwType": "WAN",
               "createBridge": False})
        _ok(pub, topics_vlan.create_vlan.rsp)
        assert ao._store.get(100, "ETH").nw_type == "WAN"


# ---------------------------------------------------------------------------
# remove_vlan
# ---------------------------------------------------------------------------

class TestRemoveVlan:
    def test_removes_existing(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}])
        _send(ao, topics_vlan.remove_vlan.req, dict(_ETH_VLAN))
        _ok(pub, topics_vlan.remove_vlan.rsp)
        assert ao._store.get(100, "ETH") is None

    def test_unknown_vlan_errors(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.remove_vlan.req, {"vlanId": 999, "ifaceType": "ETH"})
        assert _err(pub, topics_vlan.remove_vlan.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_cascades_to_bindings(self):
        """A removed VLAN must not stay visible via query_vlan_bindings."""
        pub = _MockPublish()
        ao = _make_ao(pub,
                      seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}],
                      seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN",
                                      "slot": 1, "profile_id": 3}])
        _send(ao, topics_vlan.remove_vlan.req, dict(_ETH_VLAN))
        _ok(pub, topics_vlan.remove_vlan.rsp)
        assert ao._store.all_bindings() == []

    def test_only_named_iface_removed(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"},
                                       {"vlan_id": 100, "iface_type": "WLAN"}])
        _send(ao, topics_vlan.remove_vlan.req, dict(_ETH_VLAN))
        _ok(pub, topics_vlan.remove_vlan.rsp)
        assert ao._store.get(100, "ETH") is None
        assert ao._store.get(100, "WLAN") is not None


# ---------------------------------------------------------------------------
# query_vlan_info -- feeds the whole taf_net accessor family
# ---------------------------------------------------------------------------

class TestQueryVlanInfo:
    def test_empty_list_when_none_configured(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.query_vlan_info.req, {})
        assert _ok(pub, topics_vlan.query_vlan_info.rsp)["vlans"] == []

    def test_returns_full_config_for_each_vlan(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[
            {"vlan_id": 100, "iface_type": "ETH", "is_accelerated": True,
             "priority": 5, "nw_type": "LAN", "create_bridge": False},
        ])
        _send(ao, topics_vlan.query_vlan_info.req, {})
        vlans = _ok(pub, topics_vlan.query_vlan_info.rsp)["vlans"]
        assert len(vlans) == 1
        # Every field the taf_net accessors (GetVlanId, IsVlanAccelerated,
        # GetVlanPriority, GetVlanNetworkType, GetVlanInterfaceType) read must
        # be present, since they never make a second round-trip.
        assert vlans[0] == {"vlanId": 100, "ifaceType": "ETH",
                            "isAccelerated": True, "priority": 5,
                            "nwType": "LAN", "createBridge": False}

    def test_reflects_created_vlan(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.create_vlan.req, {"vlanId": 7, "ifaceType": "RNDIS"})
        pub.reset()
        _send(ao, topics_vlan.query_vlan_info.req, {})
        vlans = _ok(pub, topics_vlan.query_vlan_info.rsp)["vlans"]
        assert [v["vlanId"] for v in vlans] == [7]


# ---------------------------------------------------------------------------
# bind / unbind
# ---------------------------------------------------------------------------

class TestBindVlan:
    def test_binds_wwan_with_slot_and_profile(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}])
        _send(ao, topics_vlan.bind_vlan.req,
              {"vlanId": 100, "backhaul": "WWAN", "slot": 1, "profileId": 3})
        _ok(pub, topics_vlan.bind_vlan.rsp)
        binding = ao._store.get_binding(100, "WWAN")
        assert (binding.slot, binding.profile_id) == (1, 3)

    def test_non_wwan_backhaul_discards_dont_care_fields(self):
        """slot/profileId are documented don't-care for non-WWAN backhauls;
        echoing them back would misrepresent them as meaningful."""
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}])
        _send(ao, topics_vlan.bind_vlan.req,
              {"vlanId": 100, "backhaul": "ETH", "slot": 1, "profileId": 3})
        _ok(pub, topics_vlan.bind_vlan.rsp)
        binding = ao._store.get_binding(100, "ETH")
        assert binding.slot is None
        assert binding.profile_id is None
        assert "slot" not in binding.to_wire()
        assert "profileId" not in binding.to_wire()

    def test_bind_unknown_vlan_errors(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_vlan.bind_vlan.req, {"vlanId": 999, "backhaul": "WWAN"})
        assert _err(pub, topics_vlan.bind_vlan.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_bind_matches_vlan_on_any_iface(self):
        """Binding targets a VLAN id, not a (id, iface) pair."""
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "WLAN"}])
        _send(ao, topics_vlan.bind_vlan.req, {"vlanId": 100, "backhaul": "ETH"})
        _ok(pub, topics_vlan.bind_vlan.rsp)

    def test_duplicate_bind_same_backhaul_rejected(self):
        pub = _MockPublish()
        ao = _make_ao(pub,
                      seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}],
                      seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN"}])
        _send(ao, topics_vlan.bind_vlan.req, {"vlanId": 100, "backhaul": "WWAN"})
        assert _err(pub, topics_vlan.bind_vlan.rsp)["code"] == "INVALID_OPERATION"

    def test_same_vlan_two_backhauls_allowed(self):
        pub = _MockPublish()
        ao = _make_ao(pub,
                      seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}],
                      seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN"}])
        _send(ao, topics_vlan.bind_vlan.req, {"vlanId": 100, "backhaul": "ETH"})
        _ok(pub, topics_vlan.bind_vlan.rsp)
        assert len(ao._store.all_bindings()) == 2


class TestUnbindVlan:
    def test_unbinds_existing(self):
        pub = _MockPublish()
        ao = _make_ao(pub,
                      seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}],
                      seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN"}])
        _send(ao, topics_vlan.unbind_vlan.req, {"vlanId": 100, "backhaul": "WWAN"})
        _ok(pub, topics_vlan.unbind_vlan.rsp)
        assert ao._store.get_binding(100, "WWAN") is None

    def test_unbind_not_bound_errors(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}])
        _send(ao, topics_vlan.unbind_vlan.req, {"vlanId": 100, "backhaul": "WWAN"})
        assert _err(pub, topics_vlan.unbind_vlan.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_unbind_leaves_other_backhaul_intact(self):
        pub = _MockPublish()
        ao = _make_ao(pub,
                      seed_vlans=[{"vlan_id": 100, "iface_type": "ETH"}],
                      seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN"},
                                     {"vlan_id": 100, "backhaul": "ETH"}])
        _send(ao, topics_vlan.unbind_vlan.req, {"vlanId": 100, "backhaul": "WWAN"})
        _ok(pub, topics_vlan.unbind_vlan.rsp)
        assert ao._store.get_binding(100, "ETH") is not None


# ---------------------------------------------------------------------------
# query_vlan_bindings
# ---------------------------------------------------------------------------

class TestQueryVlanBindings:
    def test_returns_all_when_unfiltered(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN"},
                                          {"vlan_id": 200, "backhaul": "ETH"}])
        _send(ao, topics_vlan.query_vlan_bindings.req, {})
        bindings = _ok(pub, topics_vlan.query_vlan_bindings.rsp)["bindings"]
        assert len(bindings) == 2

    def test_backhaul_filter(self):
        pub = _MockPublish()
        ao = _make_ao(pub, seed_bindings=[{"vlan_id": 100, "backhaul": "WWAN"},
                                          {"vlan_id": 200, "backhaul": "ETH"}])
        _send(ao, topics_vlan.query_vlan_bindings.req, {"backhaul": "ETH"})
        bindings = _ok(pub, topics_vlan.query_vlan_bindings.rsp)["bindings"]
        assert [b["vlanId"] for b in bindings] == [200]

    def test_slot_filter_keeps_slotless_non_wwan_bindings(self):
        """Only WWAN bindings carry a slot, so a slot filter must not wipe out
        every non-WWAN binding."""
        pub = _MockPublish()
        ao = _make_ao(pub, seed_bindings=[
            {"vlan_id": 100, "backhaul": "WWAN", "slot": 1},
            {"vlan_id": 200, "backhaul": "WWAN", "slot": 2},
            {"vlan_id": 300, "backhaul": "ETH"},
        ])
        _send(ao, topics_vlan.query_vlan_bindings.req, {"slot": 1})
        ids = sorted(b["vlanId"]
                     for b in _ok(pub, topics_vlan.query_vlan_bindings.rsp)["bindings"])
        assert ids == [100, 300]


# ---------------------------------------------------------------------------
# Readiness + hw-accel action
# ---------------------------------------------------------------------------

class TestReadinessAndHwAccel:
    def test_retained_ready_published_on_entry(self):
        pub = _MockPublish()
        ao = NetVlanAO(slot=1, mpss_src="mpss-dev-1")
        ao.start(pub, MagicMock())
        wait_for_state(ao, "smfn_ready")
        time.sleep(0.05)
        readys = [c for c in pub.calls
                  if c["topic"] == topics_vlan.subsys_ready_net_vlan.ind]
        assert len(readys) == 1
        assert readys[0]["payload"]["data"] == {"ready": True, "status": "AVAILABLE"}
        # Retained: a PA attaching later must still learn the subsystem is up.
        assert readys[0]["retain"] is True
        ao.stop()

    def test_force_hw_accel_publishes_indication(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        ao.force_vlan_hw_accel({"state": "INACTIVE"})
        time.sleep(0.1)
        inds = [c for c in pub.calls if c["topic"] == topics_vlan.hw_accel_state.ind]
        assert len(inds) == 1
        assert inds[0]["payload"]["data"] == {"state": "INACTIVE"}
        # Not retained -- it's an event, not latched state.
        assert inds[0]["retain"] is False
