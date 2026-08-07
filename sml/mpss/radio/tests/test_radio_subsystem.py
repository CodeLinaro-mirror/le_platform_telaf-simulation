# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for RadioSubsystem — no broker required.

Focuses on _do_stop's direct-publish swap (see its docstring for the
rationale): each of the three radio sub-AOs must publish its own
subsys_ready(ready=False) through direct_publish_fn, not the regular async
publish_fn, so the retained indication reaches the broker before the MQTT
client disconnects rather than being queued behind an AO that has already
left Operational.
"""
from __future__ import annotations

import json

from sml.mpss.radio import RadioSubsystem
from sml.mpss.radio.tests._helpers import wait_for_state, wait_until
from generated.python.topics import radio as topics_radio


class _MockPublish:
    """Tags every call with which channel (async vs direct) it came in on."""

    def __init__(self, channel: str):
        self.channel = channel
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain, "channel": self.channel})


def _make_started_subsystem(async_pub, direct_pub) -> RadioSubsystem:
    rs = RadioSubsystem(slot_id=1, role="dev")
    rs.start(async_pub, lambda t: None, lambda t: None, direct_publish_fn=direct_pub)
    wait_for_state(rs, "smfn_ready", timeout=2.0)
    return rs


def test_stop_publishes_all_three_subsys_ready_false_via_direct_channel():
    async_pub = _MockPublish("async")
    direct_pub = _MockPublish("direct")
    rs = _make_started_subsystem(async_pub, direct_pub)

    rs.stop()
    wait_for_state(rs, "smfn_stopping", timeout=2.0)
    wait_until(lambda: any(
        c["topic"] in (
            topics_radio.subsys_ready_phone.ind,
            topics_radio.subsys_ready_serv.ind,
            topics_radio.subsys_ready_netsel.ind,
        ) and c["payload"]["data"].get("ready") is False
        for c in direct_pub.calls
    ))

    ready_false_topics = {
        c["topic"] for c in direct_pub.calls
        if c["payload"]["data"].get("ready") is False
    }
    assert ready_false_topics == {
        topics_radio.subsys_ready_phone.ind,
        topics_radio.subsys_ready_serv.ind,
        topics_radio.subsys_ready_netsel.ind,
    }, f"expected all three subsys_ready=false on the direct channel, got {ready_false_topics}"

    # None of the ready=false indications should have gone out on the
    # regular async channel instead -- that's the whole point of the swap.
    async_ready_false = [
        c for c in async_pub.calls
        if c["topic"] in ready_false_topics and c["payload"]["data"].get("ready") is False
    ]
    assert async_ready_false == [], (
        f"subsys_ready=false leaked onto the async publish channel: {async_ready_false}"
    )


def test_stop_without_direct_publish_fn_falls_back_to_async_channel():
    # start() with no direct_publish_fn (matches serving_system.py's own
    # `self._direct_publish_fn = direct_publish_fn or publish_fn` fallback)
    # -- the swap must be harmless (swap-to-itself) rather than erroring.
    async_pub = _MockPublish("async")
    rs = RadioSubsystem(slot_id=1, role="dev")
    rs.start(async_pub, lambda t: None, lambda t: None)
    wait_for_state(rs, "smfn_ready", timeout=2.0)

    rs.stop()
    wait_for_state(rs, "smfn_stopping", timeout=2.0)
    wait_until(lambda: any(
        c["payload"]["data"].get("ready") is False for c in async_pub.calls
    ))

    ready_false_topics = {
        c["topic"] for c in async_pub.calls
        if c["payload"]["data"].get("ready") is False
    }
    assert ready_false_topics == {
        topics_radio.subsys_ready_phone.ind,
        topics_radio.subsys_ready_serv.ind,
        topics_radio.subsys_ready_netsel.ind,
    }
