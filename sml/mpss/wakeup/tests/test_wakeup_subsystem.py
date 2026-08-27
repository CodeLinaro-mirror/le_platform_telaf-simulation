# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for WakeupSubsystem (mpss-side D11/O1-b wakeup source).

WakeupSubsystem is now an ActiveObject (its own dispatch thread), so
state transitions and message handling are asynchronous -- see
sml/mpss/data/tests/_helpers.py's module docstring for why tests poll.
"""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

from sml.mpss.data.tests._helpers import wait_for_state, wait_until
from sml.mpss.wakeup import WakeupSubsystem
from generated.python.topics import wakeup as topics_wakeup
from generated.python.action_registry import ACTIONS


def _published(publish_fn, topic):
    for call in publish_fn.call_args_list:
        if call.args[0] == topic:
            yield json.loads(call.args[1])


def _ao(**kwargs):
    subsystem = WakeupSubsystem(role="dev", **kwargs)
    publish_fn = MagicMock()
    subscribe_fn = MagicMock()
    unsubscribe_fn = MagicMock()
    subsystem.start(publish_fn, subscribe_fn, unsubscribe_fn)
    wait_for_state(subsystem, "smfn_ready")
    return subsystem, publish_fn, subscribe_fn


def _env(data: dict, corr="abcd", src="test", dest="mpss-dev-1") -> bytes:
    return json.dumps({
        "v": 1, "corrId": corr, "ts": 1, "src": src, "dest": dest, "data": data,
    }).encode()


def test_action_dispatcher_subscribes_force_wakeup_topic():
    subsystem, _, subscribe_fn = _ao()
    subscribed_topics = [call.args[0] for call in subscribe_fn.call_args_list]
    assert ACTIONS["wakeup.force_wakeup"]["topic"] in subscribed_topics


def test_owns_topic():
    subsystem, _, _ = _ao()
    assert subsystem.owns_topic(ACTIONS["wakeup.force_wakeup"]["topic"])
    assert not subsystem.owns_topic("ap/req/power/set_state")


def test_default_ws_filter_is_zero():
    subsystem, _, _ = _ao()
    assert subsystem._ws_filter == 0x0000


def test_force_wakeup_suppressed_by_default_filter():
    """With the default all-zero filter, even a recognized wakeup source is
    withheld -- matches real hardware until the PA calls EnableAllWs."""
    subsystem, publish_fn, _ = _ao()
    ok = subsystem.dispatch_action("wakeup.force_wakeup", {})
    assert ok is True
    assert not list(_published(publish_fn, topics_wakeup.event.ind))


def test_force_wakeup_published_once_filter_allows_it():
    subsystem, publish_fn, _ = _ao()
    subsystem._ws_filter = 0xFFFF
    subsystem.dispatch_action("wakeup.force_wakeup", {})
    assert wait_until(lambda: list(_published(publish_fn, topics_wakeup.event.ind)))
    events = list(_published(publish_fn, topics_wakeup.event.ind))
    assert events[-1]["data"] == {
        "serviceId": 5, "sourceNodeId": 0, "destinationNodeId": 0,
        "isMsgIdValid": True, "msgId": 1, "isPIDValid": False, "pid": 0,
        "isProcessNameValid": False, "processName": "",
    }


def test_force_wakeup_overrides_apply():
    subsystem, publish_fn, _ = _ao()
    subsystem._ws_filter = 0xFFFF
    subsystem.dispatch_action("wakeup.force_wakeup", {"serviceId": 0x09, "msgId": 0x002E})
    assert wait_until(lambda: list(_published(publish_fn, topics_wakeup.event.ind)))
    events = list(_published(publish_fn, topics_wakeup.event.ind))
    assert events[-1]["data"]["serviceId"] == 0x09
    assert events[-1]["data"]["msgId"] == 0x002E


def test_force_wakeup_unknown_combo_publishes_regardless_of_filter():
    """A (serviceId, msgId) combo absent from the combo table has no
    MODEM_WS_* bit to check the filter against, so it's not this module's
    job to gate it -- it publishes unconditionally, even with the
    all-zero default filter."""
    subsystem, publish_fn, _ = _ao()
    subsystem.dispatch_action(
        "wakeup.force_wakeup", {"serviceId": 0x7E, "msgId": 0x1234})
    assert wait_until(lambda: list(_published(publish_fn, topics_wakeup.event.ind)))
    events = list(_published(publish_fn, topics_wakeup.event.ind))
    assert events[-1]["data"]["serviceId"] == 0x7E
    assert events[-1]["data"]["msgId"] == 0x1234


def test_dispatch_action_rejects_unknown_name():
    subsystem, _, _ = _ao()
    assert subsystem.dispatch_action("wakeup.unknown", {}) is False


def test_dispatch_action_invalid_payload_dropped_not_raised():
    subsystem, publish_fn, _ = _ao()
    subsystem._ws_filter = 0xFFFF
    ok = subsystem.dispatch_action("wakeup.force_wakeup", {"serviceId": "not-an-int"})
    assert ok is True
    time.sleep(0.05)
    assert not list(_published(publish_fn, topics_wakeup.event.ind))


def test_set_ws_filter_updates_and_responds():
    subsystem, publish_fn, _ = _ao()
    subsystem.handle_message(
        topics_wakeup.set_ws_filter.req,
        _env({"bitset": 0x0003}),
    )
    assert wait_until(lambda: list(_published(publish_fn, topics_wakeup.set_ws_filter.rsp)))
    assert subsystem._ws_filter == 0x0003


def test_get_ws_filter_reports_current_value():
    subsystem, publish_fn, _ = _ao()
    subsystem._ws_filter = 0x0005
    subsystem.handle_message(
        topics_wakeup.get_ws_filter.req,
        _env({}),
    )
    assert wait_until(lambda: list(_published(publish_fn, topics_wakeup.get_ws_filter.rsp)))
    rsp = list(_published(publish_fn, topics_wakeup.get_ws_filter.rsp))[-1]
    assert rsp["data"]["bitset"] == 0x0005


def test_ws_filter_persists_across_restart(tmp_path):
    persist_path = tmp_path / "ws_filter.json"

    subsystem, publish_fn, _ = _ao(persist_path=persist_path)
    subsystem.handle_message(
        topics_wakeup.set_ws_filter.req,
        _env({"bitset": 0x000A}),
    )
    assert wait_until(lambda: list(_published(publish_fn, topics_wakeup.set_ws_filter.rsp)))
    subsystem.stop()

    restarted, _, _ = _ao(persist_path=persist_path)
    assert restarted._ws_filter == 0x000A


def test_ws_filter_defaults_when_no_persist_file(tmp_path):
    persist_path = tmp_path / "ws_filter.json"
    subsystem, _, _ = _ao(persist_path=persist_path)
    assert subsystem._ws_filter == 0x0000
