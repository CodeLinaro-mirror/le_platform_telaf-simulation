# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for EcallManagerAO -- no broker required, publish_fn is mocked.

Same pattern as sml/mpss/data/tests/test_connection.py. This replaces a
previous test_call_manager.py that was an accidental verbatim copy of
call_manager.py itself (zero real test functions, and stale -- predated
the redial feature entirely).
"""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

import pytest

from sml.mpss.ecall.call_manager import EcallManagerAO
from sml.mpss.ecall.tests._helpers import wait_for_state, wait_until
from generated.python.topics import ecall as topics_ecall


# ---------------------------------------------------------------------------
# Helpers -- mirrors data/tests/test_connection.py's _MockPublish/_send/etc.
# ---------------------------------------------------------------------------

class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload)})

    def reset(self):
        self.calls.clear()


def _make_ao(pub) -> EcallManagerAO:
    ao = EcallManagerAO(slot=1, mpss_src="mpss-dev-1")
    sub_mock = MagicMock()
    ao.start(pub, sub_mock, MagicMock())
    wait_for_state(ao, "smfn_ready")
    pub.reset()
    return ao


def _send(ao, topic, data):
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data,
    }).encode()
    ao.handle_message(topic, msg)
    time.sleep(0.05)


def _rsp_calls(pub, rsp_topic, before=0):
    return [c for c in pub.calls[before:] if c["topic"] == rsp_topic]


def _ind_calls(pub, ind_topic, before=0):
    return [c for c in pub.calls[before:] if c["topic"] == ind_topic]


def _start_ecall(ao, pub, phone_id=1, ecall_type="TEST", is_msd_transmitted=False):
    before = len(pub.calls)
    _send(ao, topics_ecall.start_ecall.req,
          {"phoneId": phone_id, "type": ecall_type, "isMsdTransmitted": is_msd_transmitted})
    rsps = _rsp_calls(pub, topics_ecall.start_ecall.rsp, before)
    assert len(rsps) == 1, f"expected exactly one start_ecall rsp, got {rsps}"
    return rsps[0]["payload"]


# ---------------------------------------------------------------------------
# Fixtures (function-scoped -- each test gets a fresh AO, so force_* sticky
# state from one test never leaks into another).
# ---------------------------------------------------------------------------

@pytest.fixture
def pub():
    return _MockPublish()


@pytest.fixture
def ao(pub):
    a = _make_ao(pub)
    yield a
    a.stop()


# ---------------------------------------------------------------------------
# Lifecycle
# ---------------------------------------------------------------------------

def test_start_publishes_retained_subsys_ready(pub):
    ao = EcallManagerAO(slot=1, mpss_src="mpss-dev-1")
    ao.start(pub, MagicMock(), MagicMock())
    wait_for_state(ao, "smfn_ready")
    readies = _ind_calls(pub, topics_ecall.subsys_ready_call.ind)
    assert readies, "expected a subsys_ready_call indication on becoming Ready"
    assert readies[-1]["payload"]["data"]["status"] == "AVAILABLE"
    ao.stop()


def test_stop_cancels_all_scheduled_timers(ao, pub):
    # Configure a short redial table and trigger it so a redial timer is
    # outstanding, then stop -- must not raise, and must not leave any
    # scheduled timer that could fire after stop.
    _send(ao, topics_ecall.configure_ecall_redial.req, {"config": "CALLORIG", "timeGap": [50, 50]})
    ao.force_ecall_redial_mode("CALLORIG")
    _start_ecall(ao, pub)
    time.sleep(0.02)  # mid first CALLORIG failure / about to enter Redialing
    ao.stop()
    assert ao._hlap_timer_uuids == {}
    assert ao._redial_timer_uuids == {}


# ---------------------------------------------------------------------------
# start_ecall / DEVICE_IN_USE reuse logic
# ---------------------------------------------------------------------------

def test_start_ecall_second_call_same_phone_is_device_in_use(ao, pub):
    _start_ecall(ao, pub, phone_id=1)
    before = len(pub.calls)
    _send(ao, topics_ecall.start_ecall.req,
          {"phoneId": 1, "type": "TEST", "isMsdTransmitted": False})
    rsps = _rsp_calls(pub, topics_ecall.start_ecall.rsp, before)
    assert len(rsps) == 1
    assert rsps[0]["payload"].get("error", {}).get("code") == "DEVICE_IN_USE"


def test_start_ecall_different_phone_ids_do_not_conflict(ao, pub):
    r1 = _start_ecall(ao, pub, phone_id=1)
    r2 = _start_ecall(ao, pub, phone_id=2)
    assert "error" not in r1
    assert "error" not in r2


# ---------------------------------------------------------------------------
# set_config / get_config round trip
# ---------------------------------------------------------------------------

def test_set_then_get_config_round_trips(ao, pub):
    _send(ao, topics_ecall.set_config.req,
          {"isOverriddenNumValid": True, "overriddenNum": "911",
           "isNumTypeValid": True, "numType": "OVERRIDDEN"})
    assert _rsp_calls(pub, topics_ecall.set_config.rsp)

    before = len(pub.calls)
    _send(ao, topics_ecall.get_config.req, {})
    rsps = _rsp_calls(pub, topics_ecall.get_config.rsp, before)
    assert len(rsps) == 1
    data = rsps[0]["payload"]["data"]
    assert data["overriddenNum"] == "911"
    assert data["numType"] == "OVERRIDDEN"


# ---------------------------------------------------------------------------
# configure_ecall_redial / get_ecall_redial_config -- bounds + round trip
# (directly covers the schema bound bug that was fixed: CALLDROP must cap
# at 2 entries, not the CALLORIG 10-entry bound).
# ---------------------------------------------------------------------------

def test_configure_ecall_redial_calldrop_rejects_three_entries(ao, pub):
    _send(ao, topics_ecall.configure_ecall_redial.req,
          {"config": "CALLDROP", "timeGap": [10, 20, 30]})
    # Invalid against the schema (CALLDROP max 2) -- dispatch_inbound drops
    # it before the handler runs, so there is no rsp at all.
    assert not _rsp_calls(pub, topics_ecall.configure_ecall_redial.rsp)


def test_configure_ecall_redial_calldrop_accepts_two_entries(ao, pub):
    _send(ao, topics_ecall.configure_ecall_redial.req,
          {"config": "CALLDROP", "timeGap": [10, 20]})
    assert len(_rsp_calls(pub, topics_ecall.configure_ecall_redial.rsp)) == 1


def test_configure_ecall_redial_callorig_accepts_ten_entries(ao, pub):
    _send(ao, topics_ecall.configure_ecall_redial.req,
          {"config": "CALLORIG", "timeGap": list(range(10, 110, 10))})
    assert len(_rsp_calls(pub, topics_ecall.configure_ecall_redial.rsp)) == 1


def test_get_ecall_redial_config_reflects_last_configured_tables(ao, pub):
    _send(ao, topics_ecall.configure_ecall_redial.req, {"config": "CALLORIG", "timeGap": [111, 222]})
    _send(ao, topics_ecall.configure_ecall_redial.req, {"config": "CALLDROP", "timeGap": [333]})
    before = len(pub.calls)
    _send(ao, topics_ecall.get_ecall_redial_config.req, {})
    rsps = _rsp_calls(pub, topics_ecall.get_ecall_redial_config.rsp, before)
    assert len(rsps) == 1
    data = rsps[0]["payload"]["data"]
    assert data["callOrigTimeGap"] == [111, 222]
    assert data["callDropTimeGap"] == [333]


# ---------------------------------------------------------------------------
# force_ecall_redial_mode -- full CALLORIG loops (Scenario A/B) and the
# hangup-while-Redialing session leak that was fixed.
# ---------------------------------------------------------------------------

def _redial_reasons(pub):
    return [c["payload"]["data"]["reason"] for c in _ind_calls(pub, topics_ecall.redial.ind)]


def test_redial_callorig_exhausts_to_max_redial_attempted(ao, pub):
    _send(ao, topics_ecall.configure_ecall_redial.req, {"config": "CALLORIG", "timeGap": [20, 20]})
    ao.force_ecall_redial_mode("CALLORIG")
    _start_ecall(ao, pub)
    ok = wait_until(lambda: "MAX_REDIAL_ATTEMPTED" in _redial_reasons(pub), timeout=2.0)
    assert ok, f"never reached MAX_REDIAL_ATTEMPTED, saw: {_redial_reasons(pub)}"
    assert _redial_reasons(pub) == ["CALL_ORIG_FAILURE", "MAX_REDIAL_ATTEMPTED"]
    ao.force_ecall_redial_mode("SUCCESS")  # restore -- sticky across calls


def test_redial_callorig_succeeds_at_attempt_stops_early(ao, pub):
    _send(ao, topics_ecall.configure_ecall_redial.req, {"config": "CALLORIG", "timeGap": [20, 20, 20]})
    ao.force_ecall_redial_mode("CALLORIG", succeed_at_attempt=2)
    _start_ecall(ao, pub)
    ok = wait_until(lambda: "CALL_CONNECTED" in _redial_reasons(pub), timeout=2.0)
    assert ok, f"never connected, saw: {_redial_reasons(pub)}"
    # Attempt 1 fails, attempt 2 connects -- no 3rd attempt, no MAX_REDIAL_ATTEMPTED.
    assert _redial_reasons(pub) == ["CALL_ORIG_FAILURE", "CALL_CONNECTED"]
    ao.force_ecall_redial_mode("SUCCESS")  # restore


def test_end_ecall_while_redialing_releases_the_session(ao, pub):
    """Regression test for the fixed bug: hangup() had no branch for
    state=="Redialing", so ending a call mid-redial-loop left the session
    stuck forever and the phoneId permanently DEVICE_IN_USE."""
    _send(ao, topics_ecall.configure_ecall_redial.req, {"config": "CALLORIG", "timeGap": [5000, 5000]})
    ao.force_ecall_redial_mode("CALLORIG")
    start_rsp = _start_ecall(ao, pub, phone_id=1)
    call_index = start_rsp["data"]["call"]["callIndex"]
    time.sleep(0.02)  # let the first CALLORIG failure land the session in Redialing
    assert ao._sessions[1].state == "Redialing"

    _send(ao, topics_ecall.end_ecall.req, {"phoneId": 1, "callIndex": call_index})

    assert 1 not in ao._sessions, "session must be released, not stuck in Redialing"
    ao.force_ecall_redial_mode("SUCCESS")  # restore

    # And the phoneId must be immediately reusable -- not DEVICE_IN_USE.
    r2 = _start_ecall(ao, pub, phone_id=1)
    assert "error" not in r2
