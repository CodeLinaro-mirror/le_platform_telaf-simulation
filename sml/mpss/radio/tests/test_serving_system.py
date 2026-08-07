# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioServingSystemAO — no broker required."""
from __future__ import annotations

import json

from sml.config.models import RadioSeed
from sml.mpss.radio.serving_system import RadioServingSystemAO
from sml.mpss.radio.tests._helpers import wait_for_state, wait_until
from generated.python.topics import radio as topics_radio


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})


def _make_started_ao(pub, radio_seed=None) -> RadioServingSystemAO:
    ao = RadioServingSystemAO(slot=1, mpss_src="mpss-dev-1", radio_seed=radio_seed)
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


def test_start_publishes_subsys_ready_serv():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ready_calls = [c for c in pub.calls if c["topic"] == topics_radio.subsys_ready_serv.ind]
    assert len(ready_calls) == 1
    assert ready_calls[0]["retain"] is True


def test_start_publishes_sys_info_and_dc_status():
    # tel::IServingSystemManager's getSystemInfo()/getDcStatus() are
    # synchronous, no-callback accessors -- the PA answers them from a
    # locally-cached copy, so these indications must be published before
    # Ready so the cache is warm for the very first call.
    # Both must be retained so the PA cache is warm regardless of startup
    # order (PA may subscribe after MPSS already published the boot value).
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    sys_info_calls = [c for c in pub.calls if c["topic"] == topics_radio.sys_info.ind]
    dc_status_calls = [c for c in pub.calls if c["topic"] == topics_radio.dc_status.ind]
    assert len(sys_info_calls) == 1
    assert len(dc_status_calls) == 1
    assert sys_info_calls[0]["payload"]["data"]["rat"] == "RADIO_TECH_LTE"
    assert dc_status_calls[0]["payload"]["data"]["endcAvailability"] == "UNKNOWN"
    assert sys_info_calls[0]["retain"] is True,  "sys_info must be retained so PA cache is warm on late subscribe"
    assert dc_status_calls[0]["retain"] is True, "dc_status must be retained so PA cache is warm on late subscribe"


def test_start_publishes_lte_cs_capability():
    # tel::IServingSystemManager's getLteCsCapability() is likewise a
    # synchronous, no-callback accessor answered purely from cache -- must be
    # published (retained) before Ready so the PA cache is warm on the very
    # first call, defaulting to FULL_SERVICE (a registered LTE sim's realistic
    # pairing with the default rat/domain above).
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    cap_calls = [c for c in pub.calls if c["topic"] == topics_radio.lte_cs_capability.ind]
    assert len(cap_calls) == 1
    assert cap_calls[0]["payload"]["data"]["lteCsCapability"] == "FULL_SERVICE"
    assert cap_calls[0]["retain"] is True, "lte_cs_capability must be retained so PA cache is warm on late subscribe"


def test_request_rat_preference_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_rat_preference.req,
               topics_radio.request_rat_preference.rsp, {"slot": 1})
    assert rsp["data"]["ratPrefs"] == ["CDMA_EVDO", "GSM", "WCDMA"]


def test_set_rat_preference_updates_state_and_fires_indication():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    before = len(pub.calls)
    rsp = _req(ao, pub, topics_radio.set_rat_preference.req,
               topics_radio.set_rat_preference.rsp, {"ratPrefs": ["LTE", "NR5G"], "slot": 1})
    assert "error" not in rsp
    ind_calls = [c for c in pub.calls[before:] if c["topic"] == topics_radio.rat_pref.ind]
    assert ind_calls
    assert ind_calls[0]["payload"]["data"]["ratPrefs"] == ["LTE", "NR5G"]

    rsp2 = _req(ao, pub, topics_radio.request_rat_preference.req,
                topics_radio.request_rat_preference.rsp, {"slot": 1})
    assert rsp2["data"]["ratPrefs"] == ["LTE", "NR5G"]


def test_get_system_info_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.get_system_info.req,
               topics_radio.get_system_info.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["rat"] == "RADIO_TECH_LTE"
    assert data["domain"] == "CS_PS"
    assert data["state"] == "IN_SERVICE"


def test_radio_seed_lte_applies_to_system_info():
    # Mirrors a scenario's initial_state.radio block resolved via
    # resolve_radio_seed() -- see the bug this closes: sml/runtime/loader.py
    # used to validate serving_cell/signal_model ids and then discard them.
    seed = RadioSeed(mcc="460", mnc="00", rat="LTE", rsrp_dbm=-97, variance_db=3)
    pub = _MockPublish()
    ao = _make_started_ao(pub, radio_seed=seed)
    rsp = _req(ao, pub, topics_radio.get_system_info.req,
               topics_radio.get_system_info.rsp, {"slot": 1})
    assert rsp["data"]["rat"] == "RADIO_TECH_LTE"


def test_radio_seed_unrecognized_rat_leaves_rat_default():
    # Only LTE has a wire mapping today (see RadioServingSystemAO's
    # radio_seed handling) -- an unsupported rat must not crash the AO,
    # just leave the built-in default in place.
    seed = RadioSeed(mcc="999", mnc="99", rat="NR5G", rsrp_dbm=-60, variance_db=0)
    pub = _MockPublish()
    ao = _make_started_ao(pub, radio_seed=seed)
    rsp = _req(ao, pub, topics_radio.get_system_info.req,
               topics_radio.get_system_info.rsp, {"slot": 1})
    assert rsp["data"]["rat"] == "RADIO_TECH_LTE"


def test_get_dc_status_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.get_dc_status.req,
               topics_radio.get_dc_status.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["endcAvailability"] == "UNKNOWN"
    assert data["dcnrRestriction"] == "RESTRICTED"


def test_request_rf_band_info_defaults():
    # E-UTRA Band 1 / channel 400 matches RadioPhoneAO.cells' seeded LTE
    # cell (earfcn=400) -- see serving_system.py's rf_band world-state comment.
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_rf_band_info.req,
               topics_radio.request_rf_band_info.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["band"] == "E_UTRA_OPERATING_BAND_1"
    assert data["channel"] == 400
    assert data["bandWidth"] == "LTE_BW_NRB_100"


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(topics_radio.set_rat_preference.req)
    assert not ao.owns_topic("mp/req/radio/unrelated")


def test_force_dc_status_updates_and_publishes_indication():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    before = len(pub.calls)
    ao.force_dc_status({"endcAvailability": "AVAILABLE"})
    wait_until(lambda: any(c["topic"] == topics_radio.dc_status.ind for c in pub.calls[before:]))
    ind = next(c for c in pub.calls[before:] if c["topic"] == topics_radio.dc_status.ind)
    assert ind["payload"]["data"]["endcAvailability"] == "AVAILABLE"

    rsp = _req(ao, pub, topics_radio.get_dc_status.req,
               topics_radio.get_dc_status.rsp, {"slot": 1})
    assert rsp["data"]["endcAvailability"] == "AVAILABLE"


def test_force_lte_cs_capability_updates_and_publishes_indication():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    before = len(pub.calls)
    ao.force_lte_cs_capability({"lteCsCapability": "BARRED"})
    wait_until(lambda: any(c["topic"] == topics_radio.lte_cs_capability.ind for c in pub.calls[before:]))
    ind = next(c for c in pub.calls[before:] if c["topic"] == topics_radio.lte_cs_capability.ind)
    assert ind["payload"]["data"]["lteCsCapability"] == "BARRED"
    assert ao.lte_cs_capability == "BARRED"

