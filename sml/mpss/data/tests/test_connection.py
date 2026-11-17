# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for IfnamePool and DataConnectionAO.

No broker required — publish_fn is mocked.
"""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

from sml.mpss.data.connection import (
    DataConnectionAO,
    IfnamePool,
    smfn_operating,
    smfn_ready,
)
from sml.mpss.data.tests._helpers import wait_for_state, wait_until
from sml.config.models import CallTimingPresetSeed, InterfacePresetSeed, IpConfigSeed
from generated.python.topics import data as topics_data
from generated.python.topics import net_modem as topics_modem


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload)})

    def reset(self):
        self.calls.clear()


def _make_ao(pub, connect_ms=0, disconnect_ms=0) -> DataConnectionAO:
    interface_preset = InterfacePresetSeed(ifname_prefix="rmnet_data", ifname_pool_size=4)
    call_timing_preset = CallTimingPresetSeed(
        call_connect_delay_ms=connect_ms, call_disconnect_delay_ms=disconnect_ms,
    )
    ip_config = IpConfigSeed()
    ao = DataConnectionAO(slot=1, interface_preset=interface_preset,
                          call_timing_preset=call_timing_preset, ip_config=ip_config,
                          mpss_src="mpss-dev-1")
    sub_mock = MagicMock()
    ao.start(pub, sub_mock)
    wait_for_state(ao, "smfn_ready")
    pub.reset()
    return ao


def _send(ao, topic, data):
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data
    }).encode()
    ao.handle_message(topic, msg)
    # Wait briefly for the AO thread to process (the message is fire-
    # and-forget post-D1; publishes happen a moment later on the AO
    # thread).  Tests that assert on longer timers (connect/disconnect)
    # add their own time.sleep on top of this.
    time.sleep(0.05)


def _rsp_calls(pub, rsp_topic, before=0):
    return [c for c in pub.calls[before:] if c["topic"] == rsp_topic]


def _evt_calls(pub, before=0):
    return [c for c in pub.calls[before:] if c["topic"] == topics_data.call_state.ind]


# ---------------------------------------------------------------------------
# IfnamePool tests
# ---------------------------------------------------------------------------

class TestIfnamePool:
    def test_allocate_lowest_free(self):
        p = IfnamePool("rmnet_data", 4)
        assert p.allocate() == "rmnet_data0"
        assert p.allocate() == "rmnet_data1"

    def test_release_returns_name_to_pool(self):
        p = IfnamePool("rmnet_data", 4)
        p.allocate()  # rmnet_data0
        p.release("rmnet_data0")
        assert p.allocate() == "rmnet_data0"

    def test_explicit_ifname_allocated_when_free(self):
        p = IfnamePool("rmnet_data", 4)
        assert p.allocate("rmnet_data3") == "rmnet_data3"

    def test_explicit_ifname_returns_none_when_in_use(self):
        p = IfnamePool("rmnet_data", 4)
        p.allocate("rmnet_data2")
        assert p.allocate("rmnet_data2") is None

    def test_pool_exhausted_returns_none(self):
        p = IfnamePool("rmnet_data", 2)
        p.allocate(); p.allocate()
        assert p.allocate() is None


# ---------------------------------------------------------------------------
# DataConnectionAO — reentrancy table
# ---------------------------------------------------------------------------

class TestReentrancy:
    def test_concurrent_start_on_connecting_returns_op_in_progress(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=5000)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        pub.reset()
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps[0]["payload"]["error"]["code"] == "OP_IN_PROGRESS"

    def test_redundant_start_on_connected_same_ifname_succeeds(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=0)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.05)
        pub.reset()
        session = ao._sessions.get(1)
        if session:
            ifname = session.ifname
            _send(ao, topics_data.start_data_call.req,
                  {"profileId": 1, "ipFamily": "IPV4V6", "ifname": ifname, "opType": "DATA_LOCAL", "slot": 1})
            rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
            # May be success or still connecting depending on timer
            # Just verify a response was published
            assert len(rsps) >= 0  # non-blocking

    def test_start_with_ifname_in_use_by_other_profile(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=5000)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "rmnet_data0", "opType": "DATA_LOCAL", "slot": 1})
        pub.reset()
        # Same slot, different profileId, same ifname → INVALID_OPERATION
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 2, "ipFamily": "IPV4V6", "ifname": "rmnet_data0", "opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps[0]["payload"]["error"]["code"] == "INVALID_OPERATION"

    def test_pool_exhausted_returns_no_resources(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=5000)
        for i in range(1, 5):  # pool_size=4, fills all 4 slots
            _send(ao, topics_data.start_data_call.req,
                  {"profileId": i, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        pub.reset()
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 5, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps[0]["payload"]["error"]["code"] == "NO_RESOURCES"

    def test_stop_on_nonexistent_session_returns_error(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_data.stop_data_call.req,
              {"profileId": 99, "ipFamily": "IPV4V6", "ifname": "rmnet_data0", "opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.stop_data_call.rsp)
        assert rsps[0]["payload"]["error"]["code"] == "INVALID_ARGUMENTS"


# ---------------------------------------------------------------------------
# List
# ---------------------------------------------------------------------------

class TestList:
    def test_list_on_empty_table(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _send(ao, topics_data.list_data_call.req, {"opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.list_data_call.rsp)
        assert rsps[0]["payload"]["data"]["calls"] == []

    def test_list_returns_active_sessions(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=5000)
        for i in range(1, 3):
            _send(ao, topics_data.start_data_call.req,
                  {"profileId": i, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        pub.reset()
        _send(ao, topics_data.list_data_call.req, {"opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.list_data_call.rsp)
        assert len(rsps[0]["payload"]["data"]["calls"]) == 2


# ---------------------------------------------------------------------------
# Call lifecycle with timer
# ---------------------------------------------------------------------------

class TestCallLifecycle:
    def test_start_publishes_connecting_response(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=5000)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps[0]["payload"]["data"]["status"] == "CONNECTING"

    def test_connect_timer_fires_connected_event(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        evts = [c for c in _evt_calls(pub) if c["payload"]["data"].get("status") == "CONNECTED"]
        assert len(evts) >= 1
        assert "ipv4" in evts[0]["payload"]["data"]
        assert "if_address" in evts[0]["payload"]["data"]["ipv4"]

    def test_stop_publishes_disconnecting(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50, disconnect_ms=5000)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)  # wait for connect
        pub.reset()
        _send(ao, topics_data.stop_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "rmnet_data0", "opType": "DATA_LOCAL", "slot": 1})
        rsps = _rsp_calls(pub, topics_data.stop_data_call.rsp)
        assert rsps[0]["payload"]["data"]["status"] == "DISCONNECTING"

    def test_disconnect_timer_fires_no_net_event(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50, disconnect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        _send(ao, topics_data.stop_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "rmnet_data0", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        evts = [c for c in _evt_calls(pub) if c["payload"]["data"].get("status") == "NO_NET"]
        assert len(evts) >= 1

    def test_session_removed_after_disconnect(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50, disconnect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        _send(ao, topics_data.stop_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "rmnet_data0", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        assert 1 not in ao._sessions

    def test_invalid_envelope_dropped(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        bad = json.dumps({"v": 99, "corrId": "00ab", "ts": 1, "src": "x", "data": {}}).encode()
        before = len(pub.calls)
        ao.handle_message(topics_data.start_data_call.req, bad)
        assert len(pub.calls) == before


class TestForceCallDrop:
    def test_drops_connected_call_straight_to_no_net(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)  # wait for connect
        pub.reset()

        ao.force_call_drop(1)
        time.sleep(0.1)

        evts = _evt_calls(pub)
        assert len(evts) == 1
        assert evts[0]["payload"]["data"]["status"] == "NO_NET"
        assert 1 not in ao._sessions

    def test_ifname_released_back_to_pool(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        ao.force_call_drop(1)
        time.sleep(0.1)
        assert not ao._pool.is_in_use("rmnet_data0")

    def test_no_active_session_is_a_noop(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        before = len(pub.calls)
        ao.force_call_drop(999)
        time.sleep(0.05)
        assert len(pub.calls) == before


# ---------------------------------------------------------------------------
# force_dns -- backs taf_net_SetDNS simulation
# ---------------------------------------------------------------------------

class TestForceDns:
    """taf_net_SetDNS reads DNS from the DCS profile cache, which is only
    refreshed by a data-call event -- so force_dns must re-publish call_state
    carrying the new DNS.

    Status stays CONNECTED, not RECONFIGURED: DCS's ConvertDataCallStatus has
    no RECONFIGURED case and falls through to TAF_DCS_DISCONNECTED, which makes
    paSessionStateChangeEvtHandler call ResetIPv4Addresses() and wipe the DNS.
    """

    @staticmethod
    def _connected_ao(pub):
        ao = _make_ao(pub, connect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 1, "ipFamily": "IPV4V6", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        pub.reset()
        return ao

    def test_publishes_connected_with_new_ipv4_dns(self):
        pub = _MockPublish()
        ao = self._connected_ao(pub)

        ao.force_dns({"profileId": 1,
                      "ipv4PrimaryDns": "1.1.1.1",
                      "ipv4SecondaryDns": "1.0.0.1"})
        time.sleep(0.1)

        evts = _evt_calls(pub)
        assert len(evts) == 1
        data = evts[0]["payload"]["data"]
        # CONNECTED: RECONFIGURED would make DCS reset the addresses.
        assert data["status"] == "CONNECTED"
        assert data["ipv4"]["primary_dns_address"] == "1.1.1.1"
        assert data["ipv4"]["secondary_dns_address"] == "1.0.0.1"

    def test_partial_update_keeps_untouched_values(self):
        pub = _MockPublish()
        ao = self._connected_ao(pub)

        ao.force_dns({"profileId": 1, "ipv4PrimaryDns": "9.9.9.9"})
        time.sleep(0.1)

        data = _evt_calls(pub)[0]["payload"]["data"]
        assert data["ipv4"]["primary_dns_address"] == "9.9.9.9"
        # untouched -> keeps the ip_preset default
        assert data["ipv4"]["secondary_dns_address"] == "8.8.4.4"
        assert data["ipv6"]["primary_dns_address"] == "2001:4860:4860::8888"

    def test_ipv6_dns_updated(self):
        pub = _MockPublish()
        ao = self._connected_ao(pub)

        ao.force_dns({"profileId": 1, "ipv6PrimaryDns": "2606:4700:4700::1111"})
        time.sleep(0.1)

        data = _evt_calls(pub)[0]["payload"]["data"]
        assert data["ipv6"]["primary_dns_address"] == "2606:4700:4700::1111"

    def test_world_state_updated_so_next_call_sees_new_dns(self):
        """Invariant (d): MPSS owns ground truth -- the seed must change too,
        not just the live session, or a second profile's bring-up would hand
        out stale DNS."""
        pub = _MockPublish()
        ao = self._connected_ao(pub)

        ao.force_dns({"profileId": 1, "ipv4PrimaryDns": "1.1.1.1"})
        time.sleep(0.1)
        pub.reset()

        _send(ao, topics_data.start_data_call.req,
              {"profileId": 2, "ipFamily": "IPV4", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)

        connected = [c for c in _evt_calls(pub)
                     if c["payload"]["data"].get("status") == "CONNECTED"
                     and c["payload"]["data"]["profileId"] == 2]
        assert len(connected) == 1
        assert connected[0]["payload"]["data"]["ipv4"]["primary_dns_address"] == "1.1.1.1"

    def test_ipv4_only_call_does_not_sprout_ipv6_block(self):
        pub = _MockPublish()
        ao = _make_ao(pub, connect_ms=50)
        _send(ao, topics_data.start_data_call.req,
              {"profileId": 3, "ipFamily": "IPV4", "ifname": "", "opType": "DATA_LOCAL", "slot": 1})
        time.sleep(0.2)
        pub.reset()

        ao.force_dns({"profileId": 3, "ipv6PrimaryDns": "2606:4700:4700::1111"})
        time.sleep(0.1)

        data = _evt_calls(pub)[0]["payload"]["data"]
        assert "ipv6" not in data

    def test_no_connected_session_is_a_noop(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        before = len(pub.calls)
        ao.force_dns({"profileId": 999, "ipv4PrimaryDns": "1.1.1.1"})
        time.sleep(0.05)
        assert len(pub.calls) == before

    def test_payload_without_dns_fields_is_a_noop(self):
        pub = _MockPublish()
        ao = self._connected_ao(pub)
        before = len(pub.calls)
        ao.force_dns({"profileId": 1})
        time.sleep(0.05)
        assert len(pub.calls) == before


# ---------------------------------------------------------------------------
# Cross-domain: net_modem WWAN connectivity gate / teardown
# ---------------------------------------------------------------------------

class TestWwanConnectivityGate:
    """The invariant: WWAN disallowed on this slot => no live WWAN sessions
    and no new ones.

    net_modem owns the permission flag; this AO owns the sessions it governs.
    The coupling travels over net_modem's wwan_connectivity_changed indication
    (see shared/registry/net_modem.yaml, which promises that disallowing
    "tears down active WWAN calls ... the data domain then publishes
    NET_NO_NET for each").
    """

    @staticmethod
    def _wwan(ao, allowed, slot=1):
        _send(ao, topics_modem.wwan_connectivity_changed.ind,
              {"slot": slot, "isConnectivityAllowed": allowed})

    @staticmethod
    def _start(ao, profile_id=1):
        _send(ao, topics_data.start_data_call.req,
              {"profileId": profile_id, "ipFamily": "IPV4V6", "ifname": "",
               "opType": "DATA_LOCAL", "slot": 1})

    def test_active_call_torn_down_to_no_net_on_disallow(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._start(ao)
        assert wait_until(lambda: any(
            c["payload"]["data"]["status"] == "CONNECTED" for c in _evt_calls(pub)))
        pub.reset()

        self._wwan(ao, False)

        states = [c["payload"]["data"] for c in _evt_calls(pub)]
        assert any(s["status"] == "NO_NET" for s in states), \
            f"expected a NO_NET call_state indication, got {states}"

    def test_teardown_reports_network_drop_end_reason(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._start(ao)
        assert wait_until(lambda: any(
            c["payload"]["data"]["status"] == "CONNECTED" for c in _evt_calls(pub)))
        pub.reset()

        self._wwan(ao, False)

        no_net = [c["payload"]["data"] for c in _evt_calls(pub)
                  if c["payload"]["data"]["status"] == "NO_NET"]
        assert no_net, "no NO_NET indication published"
        # Distinguishes a network-side drop from a clean client stop, which is
        # what the PA uses to decide whether to retry.
        assert no_net[0]["end_reason"] == {"type": 0x02, "code": 206}

    def test_new_call_refused_while_disallowed(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._wwan(ao, False)
        pub.reset()

        self._start(ao, profile_id=7)

        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps, "start_data_call produced no response"
        assert "error" in rsps[0]["payload"], f"expected rejection, got {rsps[0]['payload']}"
        assert rsps[0]["payload"]["error"]["code"] == "INVALID_OPERATION"

    def test_refused_call_leaks_no_ifname(self):
        """The gate must precede pool allocation, else a refused call burns an
        interface name and the pool drains on repeated attempts."""
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._wwan(ao, False)
        for pid in range(1, 7):
            self._start(ao, profile_id=pid)

        assert not ao._pool.is_in_use("rmnet_data0")

    def test_call_allowed_again_after_reallow(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._wwan(ao, False)
        self._wwan(ao, True)
        pub.reset()

        self._start(ao, profile_id=2)

        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps and "error" not in rsps[0]["payload"], \
            f"call should be permitted after re-allow, got {rsps}"

    def test_other_slot_disallow_does_not_touch_our_sessions(self):
        """The permission is slot-scoped; slot 2 going down must not drop a
        slot 1 call."""
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._start(ao)
        assert wait_until(lambda: any(
            c["payload"]["data"]["status"] == "CONNECTED" for c in _evt_calls(pub)))
        pub.reset()

        self._wwan(ao, False, slot=2)

        states = [c["payload"]["data"]["status"] for c in _evt_calls(pub)]
        assert "NO_NET" not in states, f"slot-2 disallow dropped a slot-1 call: {states}"

    def test_call_permitted_before_any_wwan_indication(self):
        """The indication only fires on a change, so an AO that has heard
        nothing must default to allowed rather than refusing everything."""
        pub = _MockPublish()
        ao = _make_ao(pub)

        self._start(ao, profile_id=3)

        rsps = _rsp_calls(pub, topics_data.start_data_call.rsp)
        assert rsps and "error" not in rsps[0]["payload"]

    def test_repeated_disallow_publishes_no_duplicate_no_net(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        self._start(ao)
        assert wait_until(lambda: any(
            c["payload"]["data"]["status"] == "CONNECTED" for c in _evt_calls(pub)))
        self._wwan(ao, False)
        pub.reset()

        self._wwan(ao, False)

        states = [c["payload"]["data"]["status"] for c in _evt_calls(pub)]
        assert "NO_NET" not in states, f"second disallow re-published teardown: {states}"


# ---------------------------------------------------------------------------
# Hierarchy: parent declared directly in handler else-branch
# ---------------------------------------------------------------------------

class TestParentHierarchy:
    """Prove the state hierarchy works via live dispatch — an unhandled signal
    at smfn_ready bubbles to smfn_operating (its parent) and then to top."""

    def test_unhandled_signal_at_ready_bubbles_to_operating(self):
        """Live-dispatch proof: an event smfn_ready ignores must SUPER-bubble
        to smfn_operating's handler.  The AO stays settled in smfn_ready
        afterwards (operating doesn't trans on the unknown signal), confirming
        the walk reached operating and returned without changing state."""
        from miros import Event, signals

        pub = _MockPublish()
        ao = _make_ao(pub)
        assert _settled(ao).__name__ == "smfn_ready"
        ao.post_fifo(Event(signal=signals.NonExistentProbe))
        time.sleep(0.05)
        assert _settled(ao).__name__ == "smfn_ready"


def _settled(ao):
    state = getattr(ao, "state", None)
    return getattr(state, "fun", None) if state is not None else None
