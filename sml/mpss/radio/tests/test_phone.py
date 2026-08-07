# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioPhoneAO — no broker required."""
from __future__ import annotations

import json

from sml.config.models import RadioSeed
from sml.mpss.radio.phone import RadioPhoneAO
from sml.mpss.radio.tests._helpers import wait_for_state, wait_until
from generated.python.topics import radio as topics_radio


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})

    def reset(self):
        self.calls.clear()


def _make_started_ao(pub, radio_seed=None) -> RadioPhoneAO:
    ao = RadioPhoneAO(slot=1, mpss_src="mpss-dev-1", radio_seed=radio_seed)
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
    wait_until(lambda: any(c["topic"] == rsp_topic for c in pub.calls[before:]),
               timeout=1.0)
    rsp_calls = [c for c in pub.calls[before:] if c["topic"] == rsp_topic]
    assert rsp_calls, f"no response published on {rsp_topic} for {topic}"
    return rsp_calls[0]["payload"]


def test_start_reaches_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.state_fn.__name__ == "smfn_ready"


def test_start_publishes_subsys_ready_phone():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ready_calls = [c for c in pub.calls if c["topic"] == topics_radio.subsys_ready_phone.ind]
    assert len(ready_calls) == 1
    assert ready_calls[0]["payload"]["data"]["ready"] is True
    assert ready_calls[0]["payload"]["data"]["status"] == "AVAILABLE"
    assert ready_calls[0]["retain"] is True


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(topics_radio.set_operating_mode.req)
    assert ao.owns_topic(topics_radio.request_signal_strength.req)
    assert not ao.owns_topic("mp/req/radio/unrelated")


def test_request_operating_mode_default():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_operating_mode.req,
               topics_radio.request_operating_mode.rsp, {"slot": 1})
    assert rsp["data"]["mode"] == "ONLINE"


def test_set_operating_mode_updates_state_and_fires_indication():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    before = len(pub.calls)
    rsp = _req(ao, pub, topics_radio.set_operating_mode.req,
               topics_radio.set_operating_mode.rsp, {"mode": "AIRPLANE", "slot": 1})
    assert "error" not in rsp
    ind_calls = [c for c in pub.calls[before:] if c["topic"] == topics_radio.op_mode.ind]
    assert ind_calls, "op_mode indication not published"
    assert ind_calls[0]["payload"]["data"]["mode"] == "AIRPLANE"

    rsp2 = _req(ao, pub, topics_radio.request_operating_mode.req,
                topics_radio.request_operating_mode.rsp, {"slot": 1})
    assert rsp2["data"]["mode"] == "AIRPLANE"


def test_request_cellular_capability_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_cellular_capability.req,
               topics_radio.request_cellular_capability.rsp, {})
    data = rsp["data"]
    assert data["simCount"] == 2
    assert data["maxActiveSims"] == 2
    assert len(data["simRatCapabilities"]) == 2
    assert len(data["deviceRatCapability"]) == 2


def test_request_voice_service_state_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_voice_service_state.req,
               topics_radio.request_voice_service_state.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["voiceServiceState"] == "REG_HOME"
    assert data["radioTech"] == "RADIO_TECH_LTE"


def test_request_signal_strength_shape():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_signal_strength.req,
               topics_radio.request_signal_strength.rsp, {"slot": 1})
    data = rsp["data"]
    assert set(data.keys()) == {"gsm", "wcdma", "lte", "nr5g"}
    assert data["lte"]["lteRsrp"] == -83


def test_configure_signal_strength_acks():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.configure_signal_strength.req,
               topics_radio.configure_signal_strength.rsp,
               {"ratSigType": "LTE_RSRP", "thresholds": [-100, -90], "slot": 1})
    assert "error" not in rsp


def test_request_cell_info_returns_registered_serving_cell():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_cell_info.req,
               topics_radio.request_cell_info.rsp, {"slot": 1})
    cells = rsp["data"]["cells"]
    assert len(cells) == 1
    assert cells[0]["registered"] is True
    assert cells[0]["cellType"] == "LTE"
    assert cells[0]["lte"]["mcc"] == "310"


def test_request_operator_info_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_operator_info.req,
               topics_radio.request_operator_info.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["shortName"] == "Operator1"
    assert data["longName"] == "Operator1"
    assert data["isHome"] is True


def test_radio_seed_lte_applies_to_cell_info_and_signal_strength():
    # Mirrors a scenario's initial_state.radio block resolved via
    # resolve_radio_seed() -- see the bug this closes: sml/runtime/loader.py
    # used to validate serving_cell/signal_model ids and then discard them.
    seed = RadioSeed(mcc="460", mnc="00", rat="LTE", rsrp_dbm=-97, variance_db=3)
    pub = _MockPublish()
    ao = _make_started_ao(pub, radio_seed=seed)

    cell_rsp = _req(ao, pub, topics_radio.request_cell_info.req,
                     topics_radio.request_cell_info.rsp, {"slot": 1})
    cell = cell_rsp["data"]["cells"][0]
    assert cell["lte"]["mcc"] == "460"
    assert cell["lte"]["mnc"] == "00"
    assert cell["lte"]["rsrp"] == -97

    signal_rsp = _req(ao, pub, topics_radio.request_signal_strength.req,
                       topics_radio.request_signal_strength.rsp, {"slot": 1})
    assert signal_rsp["data"]["lte"]["lteRsrp"] == -97


def test_radio_seed_unrecognized_rat_leaves_defaults():
    # Only LTE has a matching cells[]/lte_signal shape to seed into today
    # (see RadioPhoneAO's radio_seed handling) -- an unsupported rat must
    # not crash the AO, just leave the built-in defaults in place.
    seed = RadioSeed(mcc="999", mnc="99", rat="NR5G", rsrp_dbm=-60, variance_db=0)
    pub = _MockPublish()
    ao = _make_started_ao(pub, radio_seed=seed)

    cell_rsp = _req(ao, pub, topics_radio.request_cell_info.req,
                     topics_radio.request_cell_info.rsp, {"slot": 1})
    cell = cell_rsp["data"]["cells"][0]
    assert cell["lte"]["mcc"] == "310"
    assert cell["lte"]["rsrp"] == -83


def test_force_signal_strength_updates_and_publishes_indication():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    before = len(pub.calls)
    ao.force_signal_strength({"lte": {"lteRsrp": -70}})
    wait_until(lambda: any(c["topic"] == topics_radio.signal_strength.ind
                          for c in pub.calls[before:]))
    ind = next(c for c in pub.calls[before:] if c["topic"] == topics_radio.signal_strength.ind)
    assert ind["payload"]["data"]["lte"]["lteRsrp"] == -70

    rsp = _req(ao, pub, topics_radio.request_signal_strength.req,
               topics_radio.request_signal_strength.rsp, {"slot": 1})
    assert rsp["data"]["lte"]["lteRsrp"] == -70
