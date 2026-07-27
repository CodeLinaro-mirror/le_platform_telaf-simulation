# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for SimSubsystem lifecycle — no broker required."""
from __future__ import annotations

import json
from unittest.mock import MagicMock

import pytest

from sml.mpss.sim import SimSubsystem
from sml.config.models import SimCard
from generated.python.ctrl_topics import action as action_topics
from generated.python.topics import sim as topics_sim

TOPIC_REQ_GET_STATE = topics_sim.get_state.req


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append(topic)


@pytest.fixture
def pub():
    return _MockPublish()


@pytest.fixture
def installed_sim():
    return SimCard(id="sim_card_001", iccid="8986011234567890123",
                   imsi="460000123456789", home_plmn="46000")


@pytest.fixture
def ss(installed_sim):
    return SimSubsystem(installed_sim=installed_sim, role="dev")


def test_start_creates_sub_aos(ss, pub):
    sub_mock = MagicMock()
    unsub_mock = MagicMock()
    ss.start(pub, sub_mock, unsub_mock)
    assert ss._card_ao is not None
    assert ss._subscription_ao is not None
    assert ss._action_dispatcher is not None
    ss.stop()


def test_seeds_from_installed_sim(ss, pub):
    ss.start(pub, MagicMock(), MagicMock())
    assert ss._card_ao.card_state == "PRESENT"
    assert ss._subscription_ao.iccid == "8986011234567890123"
    assert ss._subscription_ao.imsi == "460000123456789"
    ss.stop()


def test_no_installed_sim_is_absent(pub):
    ss = SimSubsystem(installed_sim=None, role="dev")
    ss.start(pub, MagicMock(), MagicMock())
    assert ss._card_ao.card_state == "ABSENT"
    ss.stop()


def test_double_start_is_noop(ss, pub):
    sub_mock = MagicMock()
    ss.start(pub, sub_mock, MagicMock())
    card_id = id(ss._card_ao)
    ss.start(pub, sub_mock, MagicMock())
    assert id(ss._card_ao) == card_id
    ss.stop()


def test_stop_leaves_stopping_state(ss, pub):
    ss.start(pub, MagicMock(), MagicMock())
    ss.stop()
    assert ss.state_fn.__name__ == "smfn_stopping"


def test_double_stop_is_noop(ss, pub):
    ss.start(pub, MagicMock(), MagicMock())
    ss.stop()
    ss.stop()  # must not raise
    assert ss.state_fn.__name__ == "smfn_stopping"


def test_handle_message_before_start_returns_false(ss):
    result = ss.handle_message(TOPIC_REQ_GET_STATE,
                               b'{"v":1,"corrId":"00ab","ts":1,"src":"x","data":{"slot":1}}')
    assert result is False


def test_handle_message_after_start_routes_card(ss, pub):
    sub_mock = MagicMock()
    ss.start(pub, sub_mock, MagicMock())
    result = ss.handle_message(
        TOPIC_REQ_GET_STATE,
        json.dumps({"v": 1, "corrId": "00ab", "ts": 1718000000,
                    "src": "dcs-master-1", "data": {"slot": 1}}).encode()
    )
    assert result is True
    ss.stop()


def test_handle_message_routes_sim_action(ss, pub):
    sub_mock = MagicMock()
    ss.start(pub, sub_mock, MagicMock())
    result = ss.handle_message(
        action_topics.sim.force_power.req,
        json.dumps({"powerOn": False}).encode()
    )
    assert result is True
    assert topics_sim.card_state.ind in pub.calls
    ss.stop()
