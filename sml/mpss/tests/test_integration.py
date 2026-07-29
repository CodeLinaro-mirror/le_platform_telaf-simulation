# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Integration tests for the MPSS MQTT client.

Requires a live mosquitto broker on `localhost:1883`. If the broker is
unreachable, the suite SHALL fail-fast (NOT skip) with an actionable
error message. See `openspec/changes/add-mpss-mqtt-client/specs/...`.

Run only this suite with:
    pytest sml/mpss/tests/test_integration.py
"""
from __future__ import annotations

import logging
import socket
import threading
import time

import pytest
import paho.mqtt.client as mqtt
from miros import Event, signals

from sml.mpss.config import (
    BrokerConfig,
    DebugConfig,
    MpssConfig,
    ReconnectConfig,
)
from sml.mpss.mqtt_client import MqttClient


BROKER_HOST = "localhost"
BROKER_PORT = 1883
_PROBE_TIMEOUT_S = 1.0


def _broker_reachable() -> bool:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(_PROBE_TIMEOUT_S)
    try:
        sock.connect((BROKER_HOST, BROKER_PORT))
        return True
    except OSError:
        return False
    finally:
        sock.close()


@pytest.fixture(scope="session", autouse=True)
def _require_broker():
    """Fail-fast (NOT skip) if broker missing — per spec Requirement
    'Test coverage' / Scenario 'Integration test aborts with guidance'."""
    if not _broker_reachable():
        pytest.exit(
            "integration tests require mosquitto on localhost:1883 — "
            "start it via 'supervisorctl start mosquitto' or run only "
            "unit tests with: pytest sml/mpss/tests/test_states_unit.py",
            returncode=2,
        )


@pytest.fixture
def cfg() -> MpssConfig:
    return MpssConfig(
        broker=BrokerConfig(host=BROKER_HOST, port=BROKER_PORT,
                            client_id="mpss-it-${HOSTNAME}", keepalive_s=10),
        reconnect=ReconnectConfig(initial_ms=200, max_ms=2_000,
                                  multiplier=2.0, jitter_pct=0.0),
        debug=DebugConfig(live_spy=False, log_level="WARNING"),
    )


def _wait_for_state(client: MqttClient, target_name: str, timeout_s: float = 3.0):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if client.state_fn.__name__ == target_name:
            return
        time.sleep(0.05)
    raise AssertionError(
        f"AO never reached {target_name}; last state was "
        f"{client.state_fn.__name__}"
    )


# --- 9.2 Off -> Connecting -> Operational against live broker -------------


def test_full_connect_path(cfg):
    c = MqttClient(cfg)
    try:
        c.start()
        _wait_for_state(c, "smfn_operational", timeout_s=3.0)
    finally:
        c.request_stop()
        c.wait_until_stopped(timeout_s=3.0)


# --- 9.3 round-trip publish/subscribe -------------------------------------


def test_publish_subscribe_roundtrip(cfg):
    """The AO publishes; a peer subscriber on a fresh paho client receives.

    We exercise the publish() API rather than subscribe wiring (which the
    Connection AO does not yet expose to user code — that arrives in the
    business-AO change). Sufficient to prove publish reaches the broker.
    """
    received: list[bytes] = []
    received_evt = threading.Event()

    peer = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="mpss-it-peer",
    )

    def on_message(client, userdata, msg):
        received.append(msg.payload)
        received_evt.set()

    peer.on_message = on_message
    peer.connect(BROKER_HOST, BROKER_PORT, 10)
    peer.subscribe("sml/it/echo", qos=1)
    peer.loop_start()

    c = MqttClient(cfg)
    try:
        c.start()
        _wait_for_state(c, "smfn_operational")
        c.publish("sml/it/echo", b"hello")
        assert received_evt.wait(timeout=3.0), "peer never received the message"
        assert received == [b"hello"]
    finally:
        c.request_stop()
        c.wait_until_stopped(timeout_s=3.0)
        peer.loop_stop()
        peer.disconnect()


# --- 9.4 graceful shutdown -----------------------------------------------


def test_graceful_shutdown(cfg):
    c = MqttClient(cfg)
    c.start()
    _wait_for_state(c, "smfn_operational")

    c.request_stop()
    assert c.wait_until_stopped(timeout_s=3.0), "AO did not stop within 3s"
    assert c.state_fn.__name__ == "smfn_stopping"


# --- 9.5 first-connect tolerance ------------------------------------------
# Note: this test exercises the LOG-LEVEL behavior on first failures. We
# cannot reliably block port 1883 without root (iptables), so we use a
# config that points at a CLOSED port for the first run, then re-point at
# the live broker. The first 3 failures should log at INFO; subsequent at
# ERROR. We assert via captured log records.


def test_first_connect_tolerance(caplog, cfg):
    bad_cfg = MpssConfig(
        broker=BrokerConfig(host="127.0.0.1", port=1,  # port 1 — closed
                            client_id="mpss-it-tolerance", keepalive_s=10,
                            transport="tcp"),
        reconnect=ReconnectConfig(initial_ms=80, max_ms=200,
                                  multiplier=2.0, jitter_pct=0.0,
                                  connect_timeout_ms=200),
        debug=cfg.debug,
    )

    c = MqttClient(bad_cfg)
    caplog.set_level(logging.DEBUG, logger="sml.mpss.mqtt_client")
    try:
        c.start()
        # Allow at least 5 attempts to occur: each attempt is
        # connect_timeout_ms (200) + backoff (starts 80, grows). 3 seconds
        # gives plenty of room.
        time.sleep(3.0)
    finally:
        c.request_stop()
        c.wait_until_stopped(timeout_s=2.0)

    info_failures = [r for r in caplog.records
                     if r.levelno == logging.INFO and "connect attempt" in r.message]
    error_failures = [r for r in caplog.records
                      if r.levelno == logging.ERROR and "connect attempt" in r.message]
    assert len(info_failures) >= 1, (
        f"expected at least one INFO-level early failure; "
        f"records: {[(r.levelname, r.message) for r in caplog.records]}"
    )
    assert len(error_failures) >= 1, "expected ERROR-level failure after tolerance window"
    # The first attempt logged at INFO must precede any ERROR.
    assert info_failures[0].created <= error_failures[0].created
