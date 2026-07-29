# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for DataActionDispatcher — no broker required."""
from __future__ import annotations

import json
from unittest.mock import MagicMock

import pytest

from generated.python.ctrl_topics import action as action_topics
from sml.mpss.data.action_dispatcher import DataActionDispatcher


@pytest.fixture
def serving_system_ao():
    return MagicMock()


@pytest.fixture
def connection_ao():
    return MagicMock()


@pytest.fixture
def dispatcher(serving_system_ao, connection_ao):
    return DataActionDispatcher(serving_system_ao=serving_system_ao, connection_ao=connection_ao)


def test_owns_registered_topics(dispatcher):
    assert dispatcher.owns_topic(action_topics.data.force_serv_state.req)
    assert dispatcher.owns_topic(action_topics.data.force_roaming.req)
    assert dispatcher.owns_topic(action_topics.data.force_nr_icon_type.req)
    assert dispatcher.owns_topic(action_topics.data.force_call_drop.req)
    assert not dispatcher.owns_topic("test/cmd/action/data/unrelated")


def test_start_subscribes_all_owned_topics(dispatcher):
    sub_mock = MagicMock()
    dispatcher.start(sub_mock, MagicMock())
    subscribed = {c.args[0] for c in sub_mock.call_args_list}
    assert subscribed == dispatcher._owned_topics


def test_force_serv_state_dispatches_to_serv_ao(dispatcher, serving_system_ao):
    payload = json.dumps({"serviceState": "OUT_OF_SERVICE"}).encode()
    dispatcher.handle_message(action_topics.data.force_serv_state.req, payload)
    serving_system_ao.force_serv_state.assert_called_once_with({"serviceState": "OUT_OF_SERVICE"})


def test_force_roaming_dispatches_to_serv_ao(dispatcher, serving_system_ao):
    payload = json.dumps({"isRoaming": True}).encode()
    dispatcher.handle_message(action_topics.data.force_roaming.req, payload)
    serving_system_ao.force_roaming.assert_called_once_with({"isRoaming": True})


def test_force_nr_icon_type_dispatches_to_serv_ao(dispatcher, serving_system_ao):
    payload = json.dumps({"iconType": "BASIC"}).encode()
    dispatcher.handle_message(action_topics.data.force_nr_icon_type.req, payload)
    serving_system_ao.force_nr_icon_type.assert_called_once_with({"iconType": "BASIC"})


def test_force_call_drop_dispatches_to_connection_ao(dispatcher, connection_ao):
    payload = json.dumps({"profileId": 1}).encode()
    dispatcher.handle_message(action_topics.data.force_call_drop.req, payload)
    connection_ao.force_call_drop.assert_called_once_with(1)


def test_invalid_payload_dropped_not_dispatched(dispatcher, serving_system_ao):
    # force_roaming's schema requires isRoaming.
    payload = json.dumps({}).encode()
    dispatcher.handle_message(action_topics.data.force_roaming.req, payload)
    serving_system_ao.force_roaming.assert_not_called()


def test_unknown_enum_value_dropped_not_dispatched(dispatcher, serving_system_ao):
    payload = json.dumps({"serviceState": "NOT_A_REAL_STATE"}).encode()
    dispatcher.handle_message(action_topics.data.force_serv_state.req, payload)
    serving_system_ao.force_serv_state.assert_not_called()


def test_bad_json_dropped(dispatcher, serving_system_ao):
    dispatcher.handle_message(action_topics.data.force_roaming.req, b"not json")
    serving_system_ao.force_roaming.assert_not_called()


def test_unowned_topic_ignored(dispatcher, serving_system_ao, connection_ao):
    dispatcher.handle_message("test/cmd/action/data/unrelated", b"{}")
    serving_system_ao.force_serv_state.assert_not_called()
    serving_system_ao.force_roaming.assert_not_called()
    connection_ao.force_call_drop.assert_not_called()


def test_stop_unsubscribes_all_owned_topics(dispatcher):
    unsub_mock = MagicMock()
    dispatcher.start(MagicMock(), unsub_mock)
    dispatcher.stop()
    unsubscribed = {c.args[0] for c in unsub_mock.call_args_list}
    assert unsubscribed == dispatcher._owned_topics


def test_handler_exception_does_not_propagate(dispatcher, serving_system_ao):
    serving_system_ao.force_roaming.side_effect = RuntimeError("boom")
    payload = json.dumps({"isRoaming": True}).encode()
    dispatcher.handle_message(action_topics.data.force_roaming.req, payload)  # must not raise
