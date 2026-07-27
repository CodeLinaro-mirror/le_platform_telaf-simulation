# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for SimActionDispatcher — no broker required."""
from __future__ import annotations

import json
from unittest.mock import MagicMock

import pytest

from generated.python.ctrl_topics import action as action_topics
from sml.mpss.sim.action_dispatcher import SimActionDispatcher


@pytest.fixture
def card_ao():
    return MagicMock()


@pytest.fixture
def subscription_ao():
    return MagicMock()


@pytest.fixture
def dispatcher(card_ao, subscription_ao):
    return SimActionDispatcher(card_ao=card_ao, subscription_ao=subscription_ao)


def test_owns_registered_topics(dispatcher):
    assert dispatcher.owns_topic(action_topics.sim.force_card_state.req)
    assert dispatcher.owns_topic(action_topics.sim.force_power.req)
    assert dispatcher.owns_topic(action_topics.sim.hotswap.req)
    assert not dispatcher.owns_topic("test/cmd/action/sim/unrelated")


def test_start_subscribes_all_owned_topics(dispatcher):
    sub_mock = MagicMock()
    dispatcher.start(sub_mock, MagicMock())
    subscribed = {c.args[0] for c in sub_mock.call_args_list}
    assert subscribed == dispatcher._owned_topics


def test_force_card_state_dispatches_to_card_ao(dispatcher, card_ao):
    payload = json.dumps({"cardState": "RESTRICTED"}).encode()
    dispatcher.handle_message(action_topics.sim.force_card_state.req, payload)
    card_ao.force_card_state.assert_called_once_with({"cardState": "RESTRICTED"})


def test_force_power_dispatches_to_card_ao(dispatcher, card_ao):
    payload = json.dumps({"powerOn": False}).encode()
    dispatcher.handle_message(action_topics.sim.force_power.req, payload)
    card_ao.force_power.assert_called_once_with({"powerOn": False})


def test_force_power_on_dispatches_to_card_ao(dispatcher, card_ao):
    """Restore path (powerOn=true) must reach the AO -- previously the
    missing restore branch meant the AO was called but did nothing useful.
    This pins that the dispatcher forwards the payload unchanged.
    """
    payload = json.dumps({"powerOn": True}).encode()
    dispatcher.handle_message(action_topics.sim.force_power.req, payload)
    card_ao.force_power.assert_called_once_with({"powerOn": True})


def test_hotswap_dispatches_to_subscription_ao(dispatcher, subscription_ao):
    payload = json.dumps({"iccid": "8986011234567890999", "imsi": "460000999999999"}).encode()
    dispatcher.handle_message(action_topics.sim.hotswap.req, payload)
    subscription_ao.hotswap.assert_called_once_with(
        {"iccid": "8986011234567890999", "imsi": "460000999999999"}
    )


# ---------------------------------------------------------------------------
# hotswap is a COMPOSITE action: a real swap is several observable
# transitions, so the dispatcher sequences SimCardAO + SubscriptionAO. These
# pin the sequence, because "one publish per swap" is the easy regression and
# it silently hides re-read-on-card-change bugs in the PA.
# ---------------------------------------------------------------------------

_HOTSWAP = {"iccid": "8986011234567890999", "imsi": "460000999999999"}


def test_hotswap_drives_remove_insert_appready_sequence(dispatcher, card_ao):
    dispatcher.handle_message(action_topics.sim.hotswap.req, json.dumps(_HOTSWAP).encode())
    states = [c.args[:2] for c in card_ao.set_card_app_state.call_args_list]
    assert states == [
        ("ABSENT", "UNKNOWN"),    # old card out
        ("PRESENT", "UNKNOWN"),   # new card detected, USIM not up
        ("PRESENT", "READY"),     # USIM ready -> TAF_SIM_READY again
    ]


def test_hotswap_never_reports_present_before_removal(dispatcher, card_ao):
    """The card MUST go ABSENT first.

    Publishing the new identity while still PRESENT/READY is a state no real
    modem produces, and a client would observe the new ICCID without ever
    seeing a card-change transition.
    """
    dispatcher.handle_message(action_topics.sim.hotswap.req, json.dumps(_HOTSWAP).encode())
    first_card, first_app = card_ao.set_card_app_state.call_args_list[0].args[:2]
    assert (first_card, first_app) == ("ABSENT", "UNKNOWN")


def test_hotswap_announces_identity_only_after_card_is_ready(
    dispatcher, card_ao, subscription_ao
):
    """iccid/imsi must land AFTER the card is back to PRESENT/READY.

    This ordering is a PA requirement, not a cosmetic choice. The PA copies
    identity into its cache only for a live slot
    (tafPaSubscriptionListener::onSubscriptionInfoChanged -> InitializeSimInfo),
    while the ABSENT card_state ahead of it clears that cache via
    UpdateLocalSimState(simPtr, nullptr). The PRESENT path afterwards only
    calls CheckAndSendRefreshEvent() and never re-reads identity -- so an
    announcement made mid-swap is dropped and never recovered, and
    taf_sim_GetICCID()/GetIMSI() keep serving the PRE-SWAP card even though
    the new one was published on the wire.

    Ordering is asserted via a shared call log, since the two AOs are separate
    mocks and per-mock call lists cannot be interleaved.
    """
    order: list = []
    card_ao.set_card_app_state.side_effect = lambda c, a, **kw: order.append(("card", c, a))
    subscription_ao.hotswap.side_effect = lambda d: order.append(("sub", d["iccid"]))

    dispatcher.handle_message(action_topics.sim.hotswap.req, json.dumps(_HOTSWAP).encode())

    assert order == [
        ("card", "ABSENT", "UNKNOWN"),
        ("card", "PRESENT", "UNKNOWN"),
        ("card", "PRESENT", "READY"),
        ("sub", "8986011234567890999"),
    ]


def test_hotswap_brackets_card_transitions_to_suppress_extra_identity_events(
    dispatcher, card_ao, subscription_ao
):
    """The card transitions must be bracketed by begin/end_hotswap().

    SimCardAO drives SubscriptionAO.set_card_available() on every card_state
    publish, so without the bracket this swap would emit three identity
    indications -- empty (card out), then the OLD identity (card back READY),
    then the new one -- breaking the documented 1-per-swap contract and
    leaving the PA briefly caching the old card again.
    """
    order: list = []
    subscription_ao.begin_hotswap.side_effect = lambda: order.append("begin")
    card_ao.set_card_app_state.side_effect = lambda c, a, **kw: order.append(f"card:{c}/{a}")
    subscription_ao.end_hotswap.side_effect = lambda: order.append("end")
    subscription_ao.hotswap.side_effect = lambda d: order.append("sub")

    dispatcher.handle_message(action_topics.sim.hotswap.req, json.dumps(_HOTSWAP).encode())

    assert order.index("begin") < order.index("card:ABSENT/UNKNOWN")
    assert order.index("card:PRESENT/READY") < order.index("end")
    # The single identity announcement happens after the bracket closes.
    assert order.index("end") < order.index("sub")
    assert subscription_ao.begin_hotswap.call_count == 1
    assert subscription_ao.end_hotswap.call_count == 1


def test_hotswap_repowers_the_card_on_reinsertion(dispatcher, card_ao):
    """Re-insertion must clear a prior force_power(false).

    Without power_on=True a hotswap after a power-off would report PRESENT
    while power_on stayed False -- inconsistent World State.
    """
    dispatcher.handle_message(action_topics.sim.hotswap.req, json.dumps(_HOTSWAP).encode())
    powered = [c for c in card_ao.set_card_app_state.call_args_list
               if c.kwargs.get("power_on") is True]
    assert len(powered) == 1
    assert powered[0].args[:2] == ("PRESENT", "UNKNOWN")


def test_hotswap_is_synchronous(dispatcher, card_ao, subscription_ao):
    """All publishes land before handle_message() returns.

    The Action Dispatcher runs on MPSS's MQTT thread and must not block or
    defer; wall-clock spacing belongs in a scenario timeline instead.
    """
    dispatcher.handle_message(action_topics.sim.hotswap.req, json.dumps(_HOTSWAP).encode())
    assert card_ao.set_card_app_state.call_count == 3
    assert subscription_ao.hotswap.call_count == 1


def test_hotswap_missing_imsi_is_rejected_before_any_mutation(dispatcher, card_ao, subscription_ao):
    """Schema validation must run before step 1.

    Otherwise a bad payload would leave the card ABSENT with no way back --
    worse than rejecting it outright.
    """
    dispatcher.handle_message(action_topics.sim.hotswap.req,
                              json.dumps({"iccid": "8986011234567890999"}).encode())
    card_ao.set_card_app_state.assert_not_called()
    subscription_ao.hotswap.assert_not_called()


def test_invalid_payload_is_dropped_not_raised(dispatcher, card_ao):
    # force_power.req requires powerOn -- missing it should fail schema
    # validation and never reach the AO.
    payload = json.dumps({}).encode()
    dispatcher.handle_message(action_topics.sim.force_power.req, payload)
    card_ao.force_power.assert_not_called()


def test_unknown_canonical_name_returns_false(dispatcher):
    assert dispatcher.dispatch_action("sim.unknown_action", {}) is False


# ---------------------------------------------------------------------------
# Slot validation (runtime Layer 2)
#
# The action schemas carry "const": 1, which rejects wrong-slot wire messages
# at schema validation (Layer 1). dispatch_action() is also called directly
# by the Scenario Runner, bypassing the wire, so a runtime slot guard in
# dispatch_action() is the second layer. These tests exercise that guard.
# ---------------------------------------------------------------------------

def test_force_card_state_wrong_slot_rejected(dispatcher, card_ao):
    """slot: 2 is rejected by the runtime guard; card AO is never called."""
    result = dispatcher.dispatch_action("sim.force_card_state",
                                        {"cardState": "ABSENT", "slot": 2})
    assert result is True  # owned, but rejected
    card_ao.force_card_state.assert_not_called()


def test_force_power_wrong_slot_rejected(dispatcher, card_ao):
    result = dispatcher.dispatch_action("sim.force_power",
                                        {"powerOn": False, "slot": 2})
    assert result is True
    card_ao.force_power.assert_not_called()


def test_hotswap_wrong_slot_rejected(dispatcher, card_ao, subscription_ao):
    result = dispatcher.dispatch_action(
        "sim.hotswap",
        {"iccid": "8986011234567890999", "imsi": "460000999999999", "slot": 2},
    )
    assert result is True
    card_ao.set_card_app_state.assert_not_called()
    subscription_ao.hotswap.assert_not_called()


def test_force_card_state_slot_one_still_dispatches(dispatcher, card_ao):
    """Explicit slot: 1 must still reach the AO (guard must not over-reject)."""
    dispatcher.dispatch_action("sim.force_card_state",
                               {"cardState": "RESTRICTED", "slot": 1})
    card_ao.force_card_state.assert_called_once()


def test_force_card_state_no_slot_still_dispatches(dispatcher, card_ao):
    """Omitting slot (default 1) must still reach the AO."""
    dispatcher.dispatch_action("sim.force_card_state", {"cardState": "RESTRICTED"})
    card_ao.force_card_state.assert_called_once()
