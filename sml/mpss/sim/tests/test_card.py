# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for SimCardAO — no broker required."""
from __future__ import annotations

import json

from sml.mpss.sim.card import SimCardAO
from generated.python.topics import sim as topics_sim

TOPIC_IND_CARD_STATE = topics_sim.card_state.ind
TOPIC_IND_SUBSYS_READY_CARD = topics_sim.subsys_ready_card.ind
TOPIC_REQ_GET_STATE = topics_sim.get_state.req
TOPIC_REQ_SET_POWER = topics_sim.set_power.req
TOPIC_RSP_GET_STATE = topics_sim.get_state.rsp
TOPIC_RSP_SET_POWER = topics_sim.set_power.rsp


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})

    def reset(self):
        self.calls.clear()


def _make_started_ao(pub, **kwargs) -> SimCardAO:
    ao = SimCardAO(slot=1, mpss_src="mpss-dev-1", **kwargs)
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


def test_start_with_installed_sim_is_present_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="8986011234567890123", imsi="460000123456789")
    assert ao.card_state == "PRESENT"
    assert ao.app_state == "READY"


def test_start_without_installed_sim_is_absent():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.card_state == "ABSENT"
    assert ao.app_state == "UNKNOWN"


def test_start_publishes_subsys_ready_card():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    ready_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUBSYS_READY_CARD]
    assert len(ready_calls) == 1
    assert ready_calls[0]["payload"]["data"]["ready"] is True
    assert ready_calls[0]["payload"]["data"]["status"] == "AVAILABLE"
    assert ready_calls[0]["retain"] is True


def test_owns_only_rpc_topics():
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    assert ao.owns_topic(TOPIC_REQ_GET_STATE)
    assert ao.owns_topic(TOPIC_REQ_SET_POWER)
    assert not ao.owns_topic("mp/req/sim/unrelated")


def test_stop_publishes_not_ready():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()
    ao.stop()
    assert ao.state_fn.__name__ == "smfn_stopping"
    not_ready = [c for c in pub.calls if c["topic"] == TOPIC_IND_SUBSYS_READY_CARD]
    assert not_ready
    assert not_ready[-1]["payload"]["data"]["ready"] is False


# ---------------------------------------------------------------------------
# Card RPCs
# ---------------------------------------------------------------------------

def test_get_state():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    rsp = _req(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 1})
    assert rsp["data"]["cardState"] == "PRESENT"
    assert rsp["data"]["appState"] == "READY"


def test_set_power_off_then_on():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")

    rsp = _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER, {"slot": 1, "powerOn": False})
    assert "error" not in rsp
    # A powered-down card is still PHYSICALLY PRESENT; only the USIM app drops.
    # Reporting ABSENT here nulls the PA's cached ICard pointer, and
    # taf_pa_sim_SetPower returns FAULT on a null card -- so power-on could
    # never be delivered and the slot stayed wedged at absent.
    assert ao.card_state == "PRESENT"
    assert ao.app_state == "UNKNOWN"
    assert ao.power_on is False

    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert ind_calls[-1]["payload"]["data"]["cardState"] == "PRESENT"
    assert ind_calls[-1]["payload"]["data"]["appState"] == "UNKNOWN"

    rsp = _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER, {"slot": 1, "powerOn": True})
    assert "error" not in rsp
    assert ao.card_state == "PRESENT"
    assert ao.app_state == "READY"


def test_powered_down_card_stays_present_for_pa_cache():
    """Regression: the PA must never see ABSENT for a merely powered-down card.

    `taf_pa_sim_SetPower` looks up `pa.managers.cards[slot]` and returns FAULT
    when it is null; that cache is nulled by an ABSENT report (via
    onSlotStatusChanged). So an ABSENT-on-power-off made power-ON unroutable:
    the request died inside the PA, MPSS never saw it, and nothing could
    restore the card until a restart.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()

    _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER, {"slot": 1, "powerOn": False})

    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert ind_calls, "power-off must publish a card_state indication"
    assert all(c["payload"]["data"]["cardState"] != "ABSENT" for c in ind_calls), (
        "an installed-but-powered-down card must not be reported ABSENT"
    )


def test_power_cycle_is_idempotent_and_restores_ready():
    """Repeated OFF/ON cycles must always return to PRESENT/READY."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    for _ in range(3):
        _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
             {"slot": 1, "powerOn": False})
        assert (ao.card_state, ao.app_state) == ("PRESENT", "UNKNOWN")
        _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
             {"slot": 1, "powerOn": True})
        assert (ao.card_state, ao.app_state) == ("PRESENT", "READY")


# ---------------------------------------------------------------------------
# Power state vs physical card presence
#
# These are INDEPENDENT facts. card_state/app_state are derived from the pair,
# so presence must be tracked separately (`_card_installed`) rather than
# derived once at construction and discarded -- otherwise set_power(ON) has
# nothing to consult and drives PRESENT/READY on an empty slot, producing a
# "ready" card with empty ICCID/IMSI that no real modem can emit.
# ---------------------------------------------------------------------------

def test_set_power_on_empty_slot_does_not_fabricate_a_card():
    """THE regression test: powering an empty slot must leave it ABSENT.

    Without a separate presence flag this returned PRESENT/READY, so
    taf_sim_IsReady() reported true while taf_sim_GetICCID()/GetIMSI()
    returned empty strings.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub)  # no iccid/imsi -> empty slot
    assert ao.card_state == "ABSENT"

    rsp = _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
               {"slot": 1, "powerOn": True})

    # The RPC succeeds -- the rail was powered as asked. "No card" is
    # reported through card_state, not as an RPC error, so taf_sim_SetPower
    # does not return LE_FAULT for a well-formed request.
    assert "error" not in rsp
    assert ao.card_state == "ABSENT"
    assert ao.app_state == "UNKNOWN"


def test_set_power_on_empty_slot_never_publishes_present():
    """No card_state_ind may claim presence on an empty slot.

    The indication is what the PA caches and forwards to
    ICardListener::onCardInfoChanged, so a single fabricated PRESENT here
    propagates a non-existent card all the way to the app.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    pub.reset()

    _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
         {"slot": 1, "powerOn": True})

    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert all(c["payload"]["data"]["cardState"] == "ABSENT" for c in ind_calls)


def test_repeated_power_on_empty_slot_stays_absent():
    """Presence is not a latch that repeated power-ons can flip."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    for _ in range(3):
        _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
             {"slot": 1, "powerOn": True})
        assert ao.card_state == "ABSENT"
        assert ao.app_state == "UNKNOWN"


def test_get_state_on_powered_empty_slot_reports_absent():
    """The pull path must agree with the push path (invariant (d))."""
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
         {"slot": 1, "powerOn": True})

    rsp = _req(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 1})
    assert rsp["data"]["cardState"] == "ABSENT"
    assert rsp["data"]["appState"] == "UNKNOWN"


def test_insert_card_then_power_on_reports_present():
    """insert_card() is the only way an empty slot becomes occupied.

    Guards the fix from over-correcting into "an empty slot can never become
    present" -- which would break sim.hotswap into an empty slot.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub)
    ao.insert_card()

    _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
         {"slot": 1, "powerOn": True})
    assert ao.card_state == "PRESENT"
    assert ao.app_state == "READY"


def test_remove_card_then_power_on_reports_absent():
    """Removing a card must survive a power cycle."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    ao.remove_card()

    _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
         {"slot": 1, "powerOn": True})
    assert ao.card_state == "ABSENT"
    assert ao.app_state == "UNKNOWN"


def test_set_card_app_state_does_not_change_presence():
    """set_card_app_state() steps observable state only, never presence.

    sim.hotswap relies on this: it reports PRESENT/UNKNOWN *before* the card
    is usable, and presence is recorded by its own insert_card() call. If this
    primitive also set presence, a caller forcing PRESENT on an empty slot
    would make the next set_power(ON) keep a card that was never inserted.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub)  # empty slot
    ao.set_card_app_state("PRESENT", "READY")
    assert ao.card_state == "PRESENT"  # observable state forced

    # ...but presence was never recorded, so a power cycle corrects it.
    _req(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
         {"slot": 1, "powerOn": True})
    assert ao.card_state == "ABSENT"


# ---------------------------------------------------------------------------
# World State mutators (Action Dispatcher entry points)
# ---------------------------------------------------------------------------

def test_force_card_state_updates_world_state_and_republishes():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()

    ao.force_card_state({"cardState": "RESTRICTED", "appState": "ILLEGAL"})

    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert len(ind_calls) == 1
    assert ind_calls[0]["payload"]["data"]["cardState"] == "RESTRICTED"
    assert ind_calls[0]["payload"]["data"]["appState"] == "ILLEGAL"

    rsp = _req(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 1})
    assert rsp["data"]["cardState"] == "RESTRICTED"
    assert rsp["data"]["appState"] == "ILLEGAL"


def test_force_power_off_updates_world_state():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()

    ao.force_power({"powerOn": False})

    # Still physically present, USIM app down -- see _apply_power_state.
    assert ao.card_state == "PRESENT"
    assert ao.app_state == "UNKNOWN"
    assert ao.power_on is False
    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert ind_calls[-1]["payload"]["data"]["appState"] == "UNKNOWN"


def test_force_power_on_restores_installed_card():
    """THE regression test for finding 2: force_power(true) after force_power(false)
    must restore PRESENT/READY when a card is physically installed.

    Before the fix, force_power(true) set power_on=True but left card_state
    at ABSENT/UNKNOWN because the restore branch was missing entirely.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")

    ao.force_power({"powerOn": False})
    assert ao.app_state == "UNKNOWN"  # precondition

    pub.reset()
    ao.force_power({"powerOn": True})

    assert ao.card_state == "PRESENT"
    assert ao.app_state == "READY"
    assert ao.power_on is True
    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert len(ind_calls) == 1
    assert ind_calls[0]["payload"]["data"]["cardState"] == "PRESENT"
    assert ind_calls[0]["payload"]["data"]["appState"] == "READY"


def test_force_power_on_empty_slot_stays_absent():
    """force_power(true) on an empty slot must not fabricate a card.

    Guards the restore fix from over-correcting: presence is tracked by
    _card_installed, not by power_on, so powering an empty slot must leave
    it ABSENT/UNKNOWN -- identical to the _handle_set_power contract.
    """
    pub = _MockPublish()  # no iccid/imsi -> empty slot
    ao = _make_started_ao(pub)
    pub.reset()

    ao.force_power({"powerOn": True})

    assert ao.card_state == "ABSENT"
    assert ao.app_state == "UNKNOWN"
    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert ind_calls[-1]["payload"]["data"]["cardState"] == "ABSENT"


def test_force_power_on_publishes_exactly_one_indication():
    """Restore must publish exactly one card_state_ind, not zero or two."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    ao.force_power({"powerOn": False})
    pub.reset()

    ao.force_power({"powerOn": True})

    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert len(ind_calls) == 1


def test_force_power_restore_agrees_with_get_state():
    """Pushed (card_state_ind) and pulled (get_state RPC) state must agree after restore."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    ao.force_power({"powerOn": False})
    ao.force_power({"powerOn": True})

    rsp = _req(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 1})
    assert rsp["data"]["cardState"] == "PRESENT"
    assert rsp["data"]["appState"] == "READY"


# ---------------------------------------------------------------------------
# Contract relied on by SimulaCardManager::resyncCardState_()
#
# The PA pulls ground truth via the get_state RPC on entry to its Ready state
# (invariant (d)) because card_state is a non-retained change event. These
# tests pin the two properties that pull depends on.
# ---------------------------------------------------------------------------

def test_card_state_ind_is_not_retained():
    """card_state is a change event, not state storage.

    If this ever flips to retain=True, the registry's deliberate split
    (readiness retained, changes not) has been violated -- and the PA-side
    resync would have been "fixed" the wrong way.
    """
    pub = _MockPublish()
    _make_started_ao(pub, iccid="123", imsi="456")
    ind_calls = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE]
    assert ind_calls, "boot-time card_state should still be published"
    assert all(c["retain"] is False for c in ind_calls)


def test_get_state_answers_after_missed_boot_indication():
    """A late-attaching PA can still recover full state via get_state alone.

    Simulates the real failure: MPSS boots and publishes card_state before the
    PA subscribes (so that indication is lost), then the PA attaches and pulls.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="8986011234567890123", imsi="460000123456789")
    pub.reset()  # discard the boot publish the PA never saw

    rsp = _req(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 1})
    assert rsp["data"]["cardState"] == "PRESENT"
    assert rsp["data"]["appState"] == "READY"


def test_get_state_rsp_shape_matches_card_state_ind():
    """Pulled and pushed payloads must be interchangeable.

    SimulaCardManager feeds the get_state response through the SAME
    CardStateEvt_Signal handler as card_state_ind, so the two payload shapes
    must stay identical or the resync silently decodes nothing.
    """
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    ind = [c for c in pub.calls if c["topic"] == TOPIC_IND_CARD_STATE][-1]
    rsp = _req(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 1})
    assert set(rsp["data"].keys()) == set(ind["payload"]["data"].keys())


# ---------------------------------------------------------------------------
# Slot validation -- schema (Layer 1) and runtime (Layer 2)
#
# The schemas now carry "const": 1, so any slot != 1 is rejected by
# dispatch_inbound before reaching the handler. The runtime check inside
# each handler is a belt-and-suspenders guard for callers that bypass the
# wire (e.g. direct unit-test calls). Both layers are exercised here.
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


def test_get_state_wrong_slot_rejected_by_schema():
    """slot: 2 fails the schema const:1 constraint; dispatch_inbound drops it."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 2})
    assert rsp is None, "wrong-slot get_state must be dropped, not answered"
    assert ao.card_state == "PRESENT"  # world state untouched


def test_set_power_wrong_slot_rejected_by_schema():
    """slot: 2 fails the schema const:1 constraint; world state of slot 1 is unchanged."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
                      {"slot": 2, "powerOn": False})
    assert rsp is None, "wrong-slot set_power must be dropped, not answered"
    assert ao.card_state == "PRESENT"  # slot 1 not powered down


def test_get_state_slot_zero_rejected_by_schema():
    """slot: 0 also fails const:1 -- minimum:0 alone was the old (broken) constraint."""
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_GET_STATE, TOPIC_RSP_GET_STATE, {"slot": 0})
    assert rsp is None


def test_set_power_slot_zero_rejected_by_schema():
    pub = _MockPublish()
    ao = _make_started_ao(pub, iccid="123", imsi="456")
    pub.reset()
    rsp = _req_no_rsp(ao, pub, TOPIC_REQ_SET_POWER, TOPIC_RSP_SET_POWER,
                      {"slot": 0, "powerOn": False})
    assert rsp is None
    assert ao.card_state == "PRESENT"
