# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioNetworkSelectionAO — no broker required."""
from __future__ import annotations

import json

from sml.config.models import RadioSeed
from sml.mpss.radio.network_selection import RadioNetworkSelectionAO
from sml.mpss.radio.tests._helpers import wait_for_state, wait_until
from generated.python.topics import radio as topics_radio


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})


def _make_started_ao(pub, radio_seed=None) -> RadioNetworkSelectionAO:
    ao = RadioNetworkSelectionAO(slot=1, mpss_src="mpss-dev-1", radio_seed=radio_seed)
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
    assert data["mcc"] == "310"
    assert data["mnc"] == "01"


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


def test_perform_network_scan_returns_fixture_networks():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.perform_network_scan.req,
               topics_radio.perform_network_scan.rsp, {"slot": 1, "scanType": "SYNC"})
    networks = rsp["data"]["networks"]
    assert len(networks) == 3
    home = next(n for n in networks if n["isHome"])
    assert home["mcc"] == "310" and home["mnc"] == "01"
    forbidden = next(n for n in networks if n["isForbidden"])
    assert forbidden["isAvailable"] is True


def test_request_preferred_networks_starts_empty():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_preferred_networks.req,
               topics_radio.request_preferred_networks.rsp, {"slot": 1})
    assert rsp["data"]["networks"] == []


def test_add_preferred_operator_flow():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.set_preferred_networks.req,
               topics_radio.set_preferred_networks.rsp,
               {"slot": 1, "networks": [{"mcc": "310", "mnc": "410"}]})
    assert "error" not in rsp

    rsp2 = _req(ao, pub, topics_radio.request_preferred_networks.req,
                topics_radio.request_preferred_networks.rsp, {"slot": 1})
    assert rsp2["data"]["networks"] == [{"mcc": "310", "mnc": "410"}]


def test_remove_preferred_operator_flow():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    _req(ao, pub, topics_radio.set_preferred_networks.req,
         topics_radio.set_preferred_networks.rsp,
         {"slot": 1, "networks": [{"mcc": "310", "mnc": "410"}, {"mcc": "001", "mnc": "01"}]})

    rsp = _req(ao, pub, topics_radio.set_preferred_networks.req,
               topics_radio.set_preferred_networks.rsp,
               {"slot": 1, "networks": [{"mcc": "001", "mnc": "01"}]})
    assert "error" not in rsp

    rsp2 = _req(ao, pub, topics_radio.request_preferred_networks.req,
                topics_radio.request_preferred_networks.rsp, {"slot": 1})
    assert rsp2["data"]["networks"] == [{"mcc": "001", "mnc": "01"}]


def test_preferred_network_rat_masks_survive_replacement():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    networks = [{"mcc": "310", "mnc": "410", "ratMask": 65535},
                {"mcc": "001", "mnc": "01", "ratMask": 0}]
    _req(ao, pub, topics_radio.set_preferred_networks.req,
         topics_radio.set_preferred_networks.rsp, {"slot": 1, "networks": networks})
    rsp = _req(ao, pub, topics_radio.request_preferred_networks.req,
               topics_radio.request_preferred_networks.rsp, {"slot": 1})
    assert rsp["data"]["networks"] == networks
    _req(ao, pub, topics_radio.set_preferred_networks.req,
         topics_radio.set_preferred_networks.rsp, {"slot": 1, "networks": networks[:1]})
    rsp = _req(ao, pub, topics_radio.request_preferred_networks.req,
               topics_radio.request_preferred_networks.rsp, {"slot": 1})
    assert rsp["data"]["networks"] == networks[:1]


def test_radio_seed_lte_applies_to_mode_and_home_scan_result():
    seed = RadioSeed(mcc="460", mnc="00", rat="LTE", rsrp_dbm=-97, variance_db=3)
    pub = _MockPublish()
    ao = _make_started_ao(pub, radio_seed=seed)
    rsp = _req(ao, pub, topics_radio.perform_network_scan.req,
               topics_radio.perform_network_scan.rsp, {"slot": 1, "scanType": "SYNC"})
    home = next(n for n in rsp["data"]["networks"] if n["isHome"])
    assert (home["mcc"], home["mnc"], ao.mcc, ao.mnc) == ("460", "00", "460", "00")
