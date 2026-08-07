# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioNetworkSelectionAO — no broker required."""
from __future__ import annotations

import json

from sml.mpss.radio.network_selection import RadioNetworkSelectionAO
from sml.mpss.radio.tests._helpers import wait_for_state, wait_until
from generated.python.topics import radio as topics_radio


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})


def _make_started_ao(pub) -> RadioNetworkSelectionAO:
    ao = RadioNetworkSelectionAO(slot=1, mpss_src="mpss-dev-1")
    ao.start(pub, lambda t: None, lambda t: None)
    wait_for_state(ao, "smfn_ready")
    return ao


def _req(ao, pub, topic, rsp_topic, data):
    before = len(pub.calls)
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data
    }).encode()
    ao.handle_message(topic, msg)
    wait_until(lambda: any(c["topic"] == rsp_topic for c in pub.calls[before:]), timeout=1.0)
    rsp_calls = [c for c in pub.calls[before:] if c["topic"] == rsp_topic]
    assert rsp_calls, f"no response published on {rsp_topic} for {topic}"
    return rsp_calls[0]["payload"]


def test_start_reaches_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.state_fn.__name__ == "smfn_ready"


def test_start_publishes_subsys_ready_netsel():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ready_calls = [c for c in pub.calls if c["topic"] == topics_radio.subsys_ready_netsel.ind]
    assert len(ready_calls) == 1
    assert ready_calls[0]["retain"] is True


def test_request_mode_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_network_selection_mode.req,
               topics_radio.request_network_selection_mode.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["mode"] == "AUTOMATIC"
    assert data["mcc"] == "810"
    assert data["mnc"] == "99"


def test_set_automatic_mode_clears_mcc_mnc():
    """Mirrors taf_radio_SetAutomaticRegisterMode: mode=AUTOMATIC, mcc/mnc empty."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.set_network_selection_mode.req,
               topics_radio.set_network_selection_mode.rsp,
               {"mode": "AUTOMATIC", "mcc": "", "mnc": "", "slot": 1})
    assert "error" not in rsp

    rsp2 = _req(ao, pub, topics_radio.request_network_selection_mode.req,
                topics_radio.request_network_selection_mode.rsp, {"slot": 1})
    assert rsp2["data"]["mode"] == "AUTOMATIC"
    assert rsp2["data"]["mcc"] == ""
    assert rsp2["data"]["mnc"] == ""


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(topics_radio.set_network_selection_mode.req)
    assert not ao.owns_topic("mp/req/radio/unrelated")
