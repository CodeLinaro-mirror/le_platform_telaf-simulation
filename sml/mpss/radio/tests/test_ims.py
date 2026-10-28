# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioImsAO — no broker required."""
from __future__ import annotations

import json

from sml.mpss.radio.ims import RadioImsAO
from sml.mpss.radio.tests._helpers import wait_for_state, wait_until
from generated.python.topics import radio as topics_radio


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})


def _make_started_ao(pub) -> RadioImsAO:
    ao = RadioImsAO(slot=1, mpss_src="mpss-dev-1")
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


def test_start_publishes_both_ims_manager_readiness_retained():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    for topic in (topics_radio.subsys_ready_ims.ind, topics_radio.subsys_ready_ims_settings.ind):
        ready_calls = [c for c in pub.calls if c["topic"] == topic]
        assert len(ready_calls) == 1
        assert ready_calls[0]["retain"] is True
        assert ready_calls[0]["payload"]["data"] == {"ready": True, "status": "AVAILABLE"}


def test_resubscribe_republishes_both_ims_manager_readiness():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    before = len(pub.calls)
    ao.resubscribe()
    topics = {topics_radio.subsys_ready_ims.ind, topics_radio.subsys_ready_ims_settings.ind}
    assert wait_until(lambda: topics <= {c["topic"] for c in pub.calls[before:]})
    for call in pub.calls[before:]:
        if call["topic"] in topics:
            assert call["retain"] is True
            assert call["payload"]["data"]["ready"] is True


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(topics_radio.request_ims_registration_info.req)
    assert not ao.owns_topic("mp/req/radio/unrelated")


def test_request_registration_info_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_ims_registration_info.req,
               topics_radio.request_ims_registration_info.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["imsRegStatus"] == "NOT_REGISTERED"
    assert data["rat"] == "RADIO_TECH_UNKNOWN"
    assert data["errorCode"] == 0
    assert data["errorString"] == ""


def test_request_service_info_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_ims_service_info.req,
               topics_radio.request_ims_service_info.rsp, {"slot": 1})
    assert rsp["data"] == {"sms": "NO_SERVICE", "voice": "NO_SERVICE"}


def test_request_pdp_status_defaults():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.request_ims_pdp_status.req,
               topics_radio.request_ims_pdp_status.rsp, {"slot": 1})
    data = rsp["data"]
    assert data["isPdpConnected"] is False
    assert data["apnName"] == ""


def test_set_and_request_service_config_vonr_roundtrip():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.set_ims_service_config.req,
               topics_radio.set_ims_service_config.rsp,
               {"slot": 1, "service": "VONR", "enable": True})
    assert "error" not in rsp

    rsp2 = _req(ao, pub, topics_radio.request_ims_service_config.req,
                topics_radio.request_ims_service_config.rsp, {"slot": 1, "service": "VONR"})
    assert rsp2["data"]["enable"] is True

    # Other service types stay independent (untouched -> still disabled).
    rsp3 = _req(ao, pub, topics_radio.request_ims_service_config.req,
                topics_radio.request_ims_service_config.rsp, {"slot": 1, "service": "SMS"})
    assert rsp3["data"]["enable"] is False


def test_set_and_request_sip_user_agent_roundtrip():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_radio.set_ims_sip_user_agent.req,
               topics_radio.set_ims_sip_user_agent.rsp,
               {"slot": 1, "userAgent": "telux-simula/1.0"})
    assert "error" not in rsp

    rsp2 = _req(ao, pub, topics_radio.request_ims_sip_user_agent.req,
                topics_radio.request_ims_sip_user_agent.rsp, {"slot": 1})
    assert rsp2["data"]["userAgent"] == "telux-simula/1.0"
