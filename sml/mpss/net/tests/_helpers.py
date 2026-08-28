# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Shared helpers for the net-domain AO tests.

Kept in one place so the five families' tests can't drift on envelope shape or
readiness-wait semantics.
"""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

from sml.mpss.data.tests._helpers import wait_for_state


class MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "retain": retain})

    def reset(self):
        self.calls.clear()


def boot(ao, pub, ready_state: str):
    """Start an AO with a mocked subscribe, wait for Ready, clear publishes."""
    ao.start(pub, MagicMock())
    wait_for_state(ao, ready_state)
    pub.reset()
    return ao


def send(ao, topic, data):
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "netsvc-master-1234", "data": data,
    }).encode()
    ao.handle_message(topic, msg)
    time.sleep(0.05)


def rsp(pub, topic):
    return [c for c in pub.calls if c["topic"] == topic]


def ok(pub, topic):
    """Assert exactly one successful rsp on `topic`; return its data block."""
    calls = rsp(pub, topic)
    assert len(calls) == 1, f"expected 1 rsp on {topic}, got {len(calls)}"
    env = calls[0]["payload"]
    assert "error" not in env, f"unexpected error: {env.get('error')}"
    return env["data"]


def err(pub, topic):
    """Assert exactly one error rsp on `topic`; return its error block."""
    calls = rsp(pub, topic)
    assert len(calls) == 1, f"expected 1 rsp on {topic}, got {len(calls)}"
    env = calls[0]["payload"]
    assert "error" in env, f"expected an error, got data={env.get('data')}"
    return env["error"]


def inds(pub, topic):
    return [c for c in pub.calls if c["topic"] == topic]
