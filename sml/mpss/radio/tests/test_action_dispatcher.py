# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioActionDispatcher — no broker required."""
from __future__ import annotations

import json
from unittest.mock import MagicMock

import pytest

from generated.python.ctrl_topics import action as action_topics
from sml.mpss.radio.action_dispatcher import RadioActionDispatcher


@pytest.fixture
def serving_system_ao():
    return MagicMock()


@pytest.fixture
def phone_ao():
    return MagicMock()


@pytest.fixture
def dispatcher(serving_system_ao, phone_ao):
    return RadioActionDispatcher(serving_system_ao=serving_system_ao, phone_ao=phone_ao)


def test_owns_registered_topics(dispatcher):
    assert dispatcher.owns_topic(action_topics.radio.force_lte_cs_capability.req)
    assert dispatcher.owns_topic(action_topics.radio.force_sys_info.req)
    assert dispatcher.owns_topic(action_topics.radio.force_dc_status.req)
    assert dispatcher.owns_topic(action_topics.radio.force_signal_strength.req)
    assert not dispatcher.owns_topic("test/cmd/action/radio/unrelated")


def test_start_subscribes_all_owned_topics(dispatcher):
    sub_mock = MagicMock()
    dispatcher.start(sub_mock, MagicMock())
    subscribed = {c.args[0] for c in sub_mock.call_args_list}
    assert subscribed == dispatcher._owned_topics


def test_force_lte_cs_capability_dispatches_to_serv_ao(dispatcher, serving_system_ao):
    payload = json.dumps({"lteCsCapability": "BARRED"}).encode()
    dispatcher.handle_message(action_topics.radio.force_lte_cs_capability.req, payload)
    serving_system_ao.force_lte_cs_capability.assert_called_once_with({"lteCsCapability": "BARRED"})


def test_force_sys_info_dispatches_to_serv_ao(dispatcher, serving_system_ao):
    payload = json.dumps({"rat": "RADIO_TECH_NR5G"}).encode()
    dispatcher.handle_message(action_topics.radio.force_sys_info.req, payload)
    serving_system_ao.force_sys_info.assert_called_once_with({"rat": "RADIO_TECH_NR5G"})


def test_force_dc_status_dispatches_to_serv_ao(dispatcher, serving_system_ao):
    payload = json.dumps({"endcAvailability": "AVAILABLE"}).encode()
    dispatcher.handle_message(action_topics.radio.force_dc_status.req, payload)
    serving_system_ao.force_dc_status.assert_called_once_with({"endcAvailability": "AVAILABLE"})


def test_force_signal_strength_dispatches_to_phone_ao(dispatcher, phone_ao):
    # Routed to RadioPhoneAO, not the serving-system AO -- signal strength is
    # IPhone state, not tel::IServingSystemManager state.
    payload = json.dumps({"lte": {"lteRsrp": -105}}).encode()
    dispatcher.handle_message(action_topics.radio.force_signal_strength.req, payload)
    phone_ao.force_signal_strength.assert_called_once_with({"lte": {"lteRsrp": -105}})


def test_force_signal_strength_accepts_partial_rat_block(dispatcher, phone_ao):
    # Each RAT block is a partial patch: unlike radio.signal_strength.ind, the
    # action schema must NOT require every field of a block, because
    # _apply_force_signal_strength does gsm_signal.update(...) etc.
    payload = json.dumps({"gsm": {"gsmSignalStrength": 12}}).encode()
    dispatcher.handle_message(action_topics.radio.force_signal_strength.req, payload)
    phone_ao.force_signal_strength.assert_called_once_with({"gsm": {"gsmSignalStrength": 12}})


def test_invalid_payload_dropped_not_dispatched(dispatcher, serving_system_ao):
    # force_lte_cs_capability's schema requires lteCsCapability.
    payload = json.dumps({}).encode()
    dispatcher.handle_message(action_topics.radio.force_lte_cs_capability.req, payload)
    serving_system_ao.force_lte_cs_capability.assert_not_called()


def test_unknown_enum_value_dropped_not_dispatched(dispatcher, serving_system_ao):
    payload = json.dumps({"lteCsCapability": "NOT_A_REAL_VALUE"}).encode()
    dispatcher.handle_message(action_topics.radio.force_lte_cs_capability.req, payload)
    serving_system_ao.force_lte_cs_capability.assert_not_called()


def test_unknown_rat_value_dropped_not_dispatched(dispatcher, serving_system_ao):
    payload = json.dumps({"rat": "RADIO_TECH_6G"}).encode()
    dispatcher.handle_message(action_topics.radio.force_sys_info.req, payload)
    serving_system_ao.force_sys_info.assert_not_called()


def test_unknown_signal_field_dropped_not_dispatched(dispatcher, phone_ao):
    # additionalProperties:false inside each RAT block guards against typos
    # silently no-op'ing (dict.update would happily add a junk key).
    payload = json.dumps({"lte": {"notARealMetric": 1}}).encode()
    dispatcher.handle_message(action_topics.radio.force_signal_strength.req, payload)
    phone_ao.force_signal_strength.assert_not_called()


def test_bad_json_dropped(dispatcher, serving_system_ao):
    dispatcher.handle_message(action_topics.radio.force_lte_cs_capability.req, b"not json")
    serving_system_ao.force_lte_cs_capability.assert_not_called()


def test_unowned_topic_ignored(dispatcher, serving_system_ao, phone_ao):
    dispatcher.handle_message("test/cmd/action/radio/unrelated", b"{}")
    serving_system_ao.force_lte_cs_capability.assert_not_called()
    phone_ao.force_signal_strength.assert_not_called()


def test_stop_unsubscribes_all_owned_topics(dispatcher):
    unsub_mock = MagicMock()
    dispatcher.start(MagicMock(), unsub_mock)
    dispatcher.stop()
    unsubscribed = {c.args[0] for c in unsub_mock.call_args_list}
    assert unsubscribed == dispatcher._owned_topics


def test_handler_exception_does_not_propagate(dispatcher, serving_system_ao):
    serving_system_ao.force_lte_cs_capability.side_effect = RuntimeError("boom")
    payload = json.dumps({"lteCsCapability": "BARRED"}).encode()
    dispatcher.handle_message(action_topics.radio.force_lte_cs_capability.req, payload)  # must not raise


def test_dispatch_action_unknown_name_returns_false(dispatcher):
    assert dispatcher.dispatch_action("radio.unknown_action", {}) is False
