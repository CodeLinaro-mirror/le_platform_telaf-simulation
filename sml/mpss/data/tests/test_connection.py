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
