# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for DataServingSystemAO — no broker required."""
from __future__ import annotations


import json
import time

from sml.mpss.data.serving_system import DataServingSystemAO
from sml.mpss.data.tests._helpers import wait_for_state, wait_until
from generated.python.topics import data as topics_data

TOPIC_IND_SUBSYS_READY_TEL = "sml/mpss/data/evt/subsys/tel/1/ready"


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})

    def reset(self):
        self.calls.clear()


def _make_started_ao(pub) -> DataServingSystemAO:
    ao = DataServingSystemAO(slot=1, mpss_src="mpss-dev-1")
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


def test_start_publishes_state_and_roaming():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    topics = [c["topic"] for c in pub.calls]
    assert topics_data.serv_state.ind in topics
    assert topics_data.serv_roaming.ind in topics


def test_state_event_payload():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    state_call = next(c for c in pub.calls if c["topic"] == topics_data.serv_state.ind)
    data = state_call["payload"]["data"]
    assert data["serviceState"] == "IN_SERVICE"
    assert data["networkRat"] == "LTE"
    assert data["drbStatus"] == "DORMANT"


def test_roaming_event_payload():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    roam_call = next(c for c in pub.calls if c["topic"] == topics_data.serv_roaming.ind)
    data = roam_call["payload"]["data"]
    assert data["isRoaming"] is False
    assert data["roamingType"] == "DOMESTIC"


def test_state_published_before_roaming():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    data_topics = [c["topic"] for c in pub.calls
                   if c["topic"] in (topics_data.serv_state.ind, topics_data.serv_roaming.ind)]
    assert data_topics[0] == topics_data.serv_state.ind
    assert data_topics[1] == topics_data.serv_roaming.ind


def test_start_publishes_tel_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ready_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUBSYS_READY_TEL]
    assert len(ready_calls) == 1
    assert ready_calls[0]["payload"]["data"]["ready"] is True


def test_start_publishes_subsys_ready_serv():
    # PA's ServingSystemManager blocks on this retained indication; it must
    # be published (retain=True) with status AVAILABLE when the AO reaches
    # Ready. Regression guard for the dropped SubsysReadyOrchestrator path.
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ready_calls = [c for c in pub.calls
                   if c["topic"] == topics_data.subsys_ready_serv.ind]
    assert len(ready_calls) == 1
    assert ready_calls[0]["payload"]["data"]["ready"] is True
    assert ready_calls[0]["payload"]["data"]["status"] == "AVAILABLE"
    assert ready_calls[0]["retain"] is True


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(topics_data.request_service_status.req)
    assert not ao.owns_topic("mp/req/data/unrelated")


def test_envelope_is_v1():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    for c in pub.calls[:2]:
        assert c["payload"]["v"] == 1
        assert isinstance(c["payload"]["corrId"], str)


# ---------------------------------------------------------------------------
# Serving-system RPCs
# ---------------------------------------------------------------------------

def test_request_service_status():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_data.request_service_status.req, topics_data.request_service_status.rsp, {"slot": 1})
    assert rsp["data"]["serviceState"] == "IN_SERVICE"
    assert rsp["data"]["networkRat"] == "LTE"


def test_request_roaming_status():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_data.request_roaming_status.req, topics_data.request_roaming_status.rsp, {"slot": 1})
    assert rsp["data"]["isRoaming"] is False
    assert rsp["data"]["roamingType"] == "DOMESTIC"


def test_request_nr_icon_type():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_data.request_nr_icon_type.req, topics_data.request_nr_icon_type.rsp, {"slot": 1})
    assert rsp["data"]["iconType"] == "NONE"


def test_make_dormant():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    rsp = _req(ao, pub, topics_data.make_dormant.req, topics_data.make_dormant.rsp, {"slot": 1})
    assert "error" not in rsp


def test_stop_publishes_not_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()
    ao.stop()
    wait_for_state(ao, "smfn_stopping")
    assert ao.state_fn.__name__ == "smfn_stopping"
    # Exit hook publishes ready=false for both the per-subsystem serv
    # indication (PA's ServingSystemManager) and the tel placeholder.
    not_ready = {c["topic"]: c["payload"]["data"]["ready"] for c in pub.calls}
    assert not_ready.get(topics_data.subsys_ready_serv.ind) is False
    assert not_ready.get(TOPIC_IND_SUBSYS_READY_TEL) is False


# ---------------------------------------------------------------------------
# World State mutators (Action Dispatcher entry points)
# ---------------------------------------------------------------------------


def test_force_serv_state_updates_world_state_and_republishes():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()

    ao.force_serv_state({"serviceState": "OUT_OF_SERVICE", "networkRat": "GSM"})
    wait_until(lambda: any(c["topic"] == topics_data.serv_state.ind for c in pub.calls),
               timeout=1.0)

    state_calls = [c for c in pub.calls if c["topic"] == topics_data.serv_state.ind]
    assert len(state_calls) == 1
    data = state_calls[0]["payload"]["data"]
    assert data["serviceState"] == "OUT_OF_SERVICE"
    assert data["networkRat"] == "GSM"
    assert data["drbStatus"] == "DORMANT"  # untouched field keeps its value

    rsp = _req(ao, pub, topics_data.request_service_status.req, topics_data.request_service_status.rsp, {"slot": 1})
    assert rsp["data"]["serviceState"] == "OUT_OF_SERVICE"
    assert rsp["data"]["networkRat"] == "GSM"


def test_force_roaming_updates_world_state_and_republishes():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()

    ao.force_roaming({"isRoaming": True, "roamingType": "INTERNATIONAL"})
    wait_until(lambda: any(c["topic"] == topics_data.serv_roaming.ind for c in pub.calls),
               timeout=1.0)

    roam_calls = [c for c in pub.calls if c["topic"] == topics_data.serv_roaming.ind]
    assert len(roam_calls) == 1
    data = roam_calls[0]["payload"]["data"]
    assert data["isRoaming"] is True
    assert data["roamingType"] == "INTERNATIONAL"

    rsp = _req(ao, pub, topics_data.request_roaming_status.req, topics_data.request_roaming_status.rsp, {"slot": 1})
    assert rsp["data"]["isRoaming"] is True
    assert rsp["data"]["roamingType"] == "INTERNATIONAL"


def test_make_dormant_updates_world_state():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ao.force_serv_state({"drbStatus": "ACTIVE"})
    wait_until(lambda: any(c["topic"] == topics_data.serv_state.ind and
                           c["payload"]["data"]["drbStatus"] == "ACTIVE"
                           for c in pub.calls), timeout=1.0)
    pub.reset()

    _req(ao, pub, topics_data.make_dormant.req, topics_data.make_dormant.rsp, {"slot": 1})
    wait_until(lambda: any(c["topic"] == topics_data.serv_state.ind for c in pub.calls),
               timeout=1.0)

    state_calls = [c for c in pub.calls if c["topic"] == topics_data.serv_state.ind]
    assert state_calls[-1]["payload"]["data"]["drbStatus"] == "DORMANT"


def test_force_nr_icon_type_updates_world_state():
    pub = _MockPublish()
    ao = _make_started_ao(pub)

    ao.force_nr_icon_type({"iconType": "UWB"})
    wait_until(lambda: ao.nr_icon_type == "UWB", timeout=1.0)

    rsp = _req(ao, pub, topics_data.request_nr_icon_type.req,
               topics_data.request_nr_icon_type.rsp, {"slot": 1})
    assert rsp["data"]["iconType"] == "UWB"
