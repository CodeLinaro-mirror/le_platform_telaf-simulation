# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for DataSubsystem lifecycle — no broker required."""
from __future__ import annotations


import json
from unittest.mock import MagicMock, call

import pytest

from sml.mpss.data import DataSubsystem
from sml.mpss.data.tests._helpers import wait_for_state, wait_until
from generated.python.ctrl_topics import action as action_topics
from generated.python.topics import data as topics_data


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append(topic)


@pytest.fixture
def pub():
    return _MockPublish()


@pytest.fixture
def ds():
    return DataSubsystem(role="dev")


def test_start_creates_sub_aos(ds, pub):
    sub_mock = MagicMock()
    unsub_mock = MagicMock()
    ds.start(pub, sub_mock, unsub_mock)
    wait_for_state(ds, "smfn_ready")
    assert ds._serving_system_ao is not None
    assert ds._profile_ao is not None
    assert ds._connection_ao is not None
    assert ds._action_dispatcher is not None
    ds.stop()


def test_double_start_is_noop(ds, pub):
    sub_mock = MagicMock()
    ds.start(pub, sub_mock, MagicMock())
    wait_for_state(ds, "smfn_ready")
    serving_id = id(ds._serving_system_ao)
    ds.start(pub, sub_mock, MagicMock())  # second start; state topology
    # ignores SIG_START while already in smfn_ready.
    import time; time.sleep(0.05)
    assert id(ds._serving_system_ao) == serving_id  # same object, not recreated
    ds.stop()


def test_stop_leaves_stopping_state(ds, pub):
    ds.start(pub, MagicMock(), MagicMock())
    wait_for_state(ds, "smfn_ready")
    ds.stop()
    wait_for_state(ds, "smfn_stopping")
    assert ds.state_fn.__name__ == "smfn_stopping"


def test_double_stop_is_noop(ds, pub):
    ds.start(pub, MagicMock(), MagicMock())
    wait_for_state(ds, "smfn_ready")
    ds.stop()
    wait_for_state(ds, "smfn_stopping")
    ds.stop()  # must not raise
    import time; time.sleep(0.05)
    assert ds.state_fn.__name__ == "smfn_stopping"


def test_handle_message_before_start_does_not_route(ds):
    # Post-T-09: handle_message is fire-and-forget (no bool return).
    # Before start, owns_topic() is False for every AO so nothing is
    # routed; the call is a no-op.
    ds.handle_message(topics_data.query_profile.req,
                      b'{"v":1,"corrId":"00ab","ts":1,"src":"x","data":{}}')
    # No sub-AO exists yet; nothing to assert beyond "does not raise".
    assert ds._profile_ao is None


def test_handle_message_after_start_routes_profile(ds, pub):
    sub_mock = MagicMock()
    ds.start(pub, sub_mock, MagicMock())
    wait_for_state(ds, "smfn_ready")
    # Wait until the profile AO reaches Ready so it actually owns the topic.
    wait_for_state(ds._profile_ao, "smfn_ready")
    assert ds.owns_topic(topics_data.query_profile.req)
    ds.handle_message(
        topics_data.query_profile.req,
        json.dumps({"v": 1, "corrId": "00ab", "ts": 1718000000,
                    "src": "dcs-master-1", "data": {}}).encode()
    )
    ds.stop()


def test_handle_message_routes_data_action(ds, pub):
    sub_mock = MagicMock()
    ds.start(pub, sub_mock, MagicMock())
    wait_for_state(ds, "smfn_ready")
    wait_for_state(ds._serving_system_ao, "smfn_ready")
    assert ds.owns_topic(action_topics.data.force_roaming.req)
    ds.handle_message(
        action_topics.data.force_roaming.req,
        json.dumps({"isRoaming": True, "roamingType": "INTERNATIONAL"}).encode()
    )
    wait_until(lambda: topics_data.serv_roaming.ind in pub.calls, timeout=1.0)
    assert topics_data.serv_roaming.ind in pub.calls
    ds.stop()
