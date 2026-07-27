# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for SubscriptionAO — no broker required."""
from __future__ import annotations

import json

from sml.mpss.sim.subscription import SubscriptionAO
from generated.python.topics import sim as topics_sim

TOPIC_IND_SUB_INFO_CHANGED = topics_sim.sub_info_changed.ind
TOPIC_IND_SUBSYS_READY_SUB = topics_sim.subsys_ready_sub.ind
TOPIC_REQ_GET_ICCID = topics_sim.get_iccid.req
TOPIC_REQ_GET_IMSI = topics_sim.get_imsi.req
TOPIC_RSP_GET_ICCID = topics_sim.get_iccid.rsp
TOPIC_RSP_GET_IMSI = topics_sim.get_imsi.rsp


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})

    def reset(self):
        self.calls.clear()


def _make_started_ao(pub, iccid="8986011234567890123", imsi="460000123456789") -> SubscriptionAO:
    ao = SubscriptionAO(slot=1, mpss_src="mpss-dev-1", iccid=iccid, imsi=imsi)
    ao.start(pub, lambda t: None, lambda t: None)
    return ao


def _req(ao, pub, topic, rsp_topic, data):
    before = len(pub.calls)
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data
    }).encode()
    ao.handle_message(topic, msg)
    rsp_calls = [c for c in pub.calls[before:] if c["topic"] == rsp_topic]
    assert rsp_calls, f"no response published on {rsp_topic} for {topic}"
    return rsp_calls[0]["payload"]


def test_start_reaches_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.state_fn.__name__ == "smfn_ready"


def test_start_publishes_subsys_ready_sub():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ready_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUBSYS_READY_SUB]
    assert len(ready_calls) == 1
    assert ready_calls[0]["payload"]["data"]["ready"] is True
    assert ready_calls[0]["retain"] is True


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(TOPIC_REQ_GET_ICCID)
    assert ao.owns_topic(TOPIC_REQ_GET_IMSI)
    assert not ao.owns_topic("mp/req/sim/unrelated")


def test_stop_publishes_not_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()
    ao.stop()
    not_ready = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUBSYS_READY_SUB]
    assert not_ready
    assert not_ready[-1]["payload"]["data"]["ready"] is False


# ---------------------------------------------------------------------------
# Subscription RPCs
# ---------------------------------------------------------------------------

def test_get_iccid():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="8986011234567890123")
    rsp = _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 1})
    assert rsp["data"]["iccid"] == "8986011234567890123"


def test_get_imsi():
    pub = _MockPublish()
    ao = _make_started_ao(pub, imsi="460000123456789")
    rsp = _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 1})
    assert rsp["data"]["imsi"] == "460000123456789"


def test_card_unavailable_hides_identity_and_publishes_empty_change():
    """Powering down the paired card must invalidate PA-side identity caches."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()

    ao.set_card_available(False)

    ind = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]
    assert len(ind) == 1
    assert ind[0]["payload"]["data"] == {"iccid": "", "imsi": ""}

    iccid = _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 1})
    imsi = _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 1})
    assert iccid["data"]["iccid"] == ""
    assert imsi["data"]["imsi"] == ""

    ao.set_card_available(True)
    ind = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]
    assert ind[-1]["payload"]["data"] == {
        "iccid": "8986011234567890123",
        "imsi": "460000123456789",
    }


def test_power_cycle_restores_default_identity():
    """Power off then on must restore the original ICCID/IMSI, not stay empty."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)

    ao.set_card_available(False)
    pub.reset()
    ao.set_card_available(True)

    assert _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID,
                {"slot": 1})["data"]["iccid"] == "8986011234567890123"
    assert _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI,
                {"slot": 1})["data"]["imsi"] == "460000123456789"
    ind = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]
    assert ind, "power-on must re-announce the restored identity"
    assert ind[-1]["payload"]["data"] == {
        "iccid": "8986011234567890123",
        "imsi": "460000123456789",
    }


def test_hotswap_while_powered_off_shows_new_identity_after_power_on():
    """A hotswap during power-off wins: power-on exposes the swapped-in card."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)

    ao.set_card_available(False)
    ao.hotswap({"iccid": "8986011234567890999", "imsi": "460000999999999"})
    pub.reset()
    ao.set_card_available(True)

    assert _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID,
                {"slot": 1})["data"]["iccid"] == "8986011234567890999"
    assert _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI,
                {"slot": 1})["data"]["imsi"] == "460000999999999"


def test_repeated_card_state_updates_do_not_spam_indications():
    """Only real changes of visible identity are announced."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()

    ao.set_card_available(True)
    ao.set_card_available(True)
    assert not [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]

    ao.set_card_available(False)
    ao.set_card_available(False)
    assert len([c for c in pub.calls
                if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]) == 1


# ---------------------------------------------------------------------------
# World State mutators (Action Dispatcher entry points)
# ---------------------------------------------------------------------------

def test_hotswap_updates_world_state_and_republishes():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="111", imsi="222")
    pub.reset()

    ao.hotswap({"iccid": "8986011234567890999", "imsi": "460000999999999"})

    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]
    assert len(ind_calls) == 1
    assert ind_calls[0]["payload"]["data"]["iccid"] == "8986011234567890999"
    assert ind_calls[0]["payload"]["data"]["imsi"] == "460000999999999"

    rsp = _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 1})
    assert rsp["data"]["iccid"] == "8986011234567890999"
    rsp = _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 1})
    assert rsp["data"]["imsi"] == "460000999999999"


# ---------------------------------------------------------------------------
# Contract relied on by SimulaSubscriptionManager::resyncSubInfo_()
#
# The PA joins get_iccid + get_imsi into one sub_info_changed-shaped payload on
# entry to Ready (invariant (d)), because sub_info_changed is a non-retained
# change event.
# ---------------------------------------------------------------------------

def test_sub_info_changed_ind_is_not_retained():
    pub = _MockPublish()
    _make_started_ao(pub)
    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED]
    assert ind_calls, "boot-time sub_info_changed should still be published"
    assert all(c["retain"] is False for c in ind_calls)


def test_iccid_and_imsi_recoverable_after_missed_boot_indication():
    """A late-attaching PA can rebuild the full pair from the two RPCs alone."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="8986011234567890123", imsi="460000123456789")
    pub.reset()  # discard the boot publish the PA never saw

    iccid = _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 1})
    imsi = _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 1})

    # The PA joins these two into one {iccid, imsi} payload and posts it
    # through the same handler sub_info_changed uses -- so together they must
    # reconstruct exactly the indication's key set.
    joined = {"iccid": iccid["data"]["iccid"], "imsi": imsi["data"]["imsi"]}
    assert joined == {"iccid": "8986011234567890123", "imsi": "460000123456789"}


def test_joined_rpc_shape_matches_sub_info_changed_ind():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ind = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUB_INFO_CHANGED][-1]
    iccid = _req(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 1})
    imsi = _req(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 1})
    joined_keys = set(iccid["data"].keys()) | set(imsi["data"].keys())
    assert joined_keys == set(ind["payload"]["data"].keys())


# ---------------------------------------------------------------------------
# Slot validation -- schema (Layer 1) and runtime (Layer 2)
# ---------------------------------------------------------------------------

def _req_no_rsp(ao, pub, topic, rsp_topic, data):
    """Like _req but returns None instead of asserting when no response arrives."""
    before = len(pub.calls)
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data
    }).encode()
    ao.handle_message(topic, msg)
    rsp_calls = [c for c in pub.calls[before:] if c["topic"] == rsp_topic]
    return rsp_calls[0]["payload"] if rsp_calls else None


def test_get_iccid_wrong_slot_rejected_by_schema():
    """slot: 2 fails the schema const:1 constraint; dispatch_inbound drops it."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="8986011234567890123")
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 2})
    assert rsp is None, "wrong-slot get_iccid must be dropped, not answered"
    assert ao.iccid == "8986011234567890123"  # world state untouched


def test_get_imsi_wrong_slot_rejected_by_schema():
    """slot: 2 fails the schema const:1 constraint; dispatch_inbound drops it."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, imsi="460000123456789")
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 2})
    assert rsp is None, "wrong-slot get_imsi must be dropped, not answered"
    assert ao.imsi == "460000123456789"  # world state untouched


def test_get_iccid_slot_zero_rejected_by_schema():
    """slot: 0 also fails const:1 -- minimum:0 alone was the old (broken) constraint."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_GET_ICCID, TOPIC_RSP_GET_ICCID, {"slot": 0})
    assert rsp is None


def test_get_imsi_slot_zero_rejected_by_schema():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_GET_IMSI, TOPIC_RSP_GET_IMSI, {"slot": 0})
    assert rsp is None
