# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Integration tests for the MPSS sim sub-package.

Requires a live mosquitto broker on localhost:1883.
If the broker is unreachable the suite FAILS FAST (not skip) -- mirrors
sml/mpss/data/tests/test_integration.py.

Run with:
    pytest sml/mpss/sim/tests/test_integration.py -v

Start mosquitto first (inside container):
    supervisorctl -c /root/sml/supervisord.sml.conf start mosquitto
Or on host directly:
    mosquitto -c sml/mosquitto.conf -d
"""
from __future__ import annotations

import json
import socket
import threading
import time
from typing import List

import paho.mqtt.client as mqtt
import pytest

from sml.mpss.mqtt_client import MqttClient
from sml.mpss.config import (
    BrokerConfig,
    DebugConfig,
    MpssConfig,
    ReconnectConfig,
)
from sml.mpss.sim import SimSubsystem
from sml.config.models import SimCard
from generated.python.topics import sim as topics_sim

TOPIC_IND_CARD_STATE = topics_sim.card_state.ind
TOPIC_IND_SUB_INFO_CHANGED = topics_sim.sub_info_changed.ind
TOPIC_IND_SUBSYS_READY_CARD = topics_sim.subsys_ready_card.ind
TOPIC_IND_SUBSYS_READY_SUB = topics_sim.subsys_ready_sub.ind
TOPIC_REQ_GET_ICCID = topics_sim.get_iccid.req
TOPIC_REQ_GET_IMSI = topics_sim.get_imsi.req
TOPIC_REQ_GET_STATE = topics_sim.get_state.req
TOPIC_REQ_SET_POWER = topics_sim.set_power.req
TOPIC_RSP_GET_ICCID = topics_sim.get_iccid.rsp
TOPIC_RSP_GET_IMSI = topics_sim.get_imsi.rsp
TOPIC_RSP_GET_STATE = topics_sim.get_state.rsp
TOPIC_RSP_SET_POWER = topics_sim.set_power.rsp

BROKER_HOST = "localhost"
BROKER_PORT = 1883
_SRC = "test-dev-0001"

_ICCID = "8986011234567890123"
_IMSI = "460000123456789"

# ---------------------------------------------------------------------------
# Broker guard
# ---------------------------------------------------------------------------

def _broker_reachable() -> bool:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(1.0)
    try:
        s.connect((BROKER_HOST, BROKER_PORT))
        return True
    except OSError:
        return False
    finally:
        s.close()


@pytest.fixture(scope="session", autouse=True)
def _require_broker():
    if not _broker_reachable():
        pytest.exit(
            "ERROR: integration tests require mosquitto on localhost:1883.\n"
            "  Inside container: supervisorctl -c /root/sml/supervisord.sml.conf start mosquitto\n"
            "  On host:          mosquitto -c sml/mosquitto.conf -d\n"
            "  Unit tests only:  pytest sml/mpss/sim/tests/ --ignore=sml/mpss/sim/tests/test_integration.py",
            returncode=2,
        )


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

def _make_config() -> MpssConfig:
    return MpssConfig(
        broker=BrokerConfig(host=BROKER_HOST, port=BROKER_PORT,
                            client_id="mpss-it-sim", keepalive_s=10),
        reconnect=ReconnectConfig(initial_ms=200, max_ms=2_000,
                                  multiplier=2.0, jitter_pct=0.0),
        debug=DebugConfig(live_spy=False, log_level="WARNING"),
    )


def _make_sim_subsystem(role="dev") -> SimSubsystem:
    return SimSubsystem(
        installed_sim=SimCard(id="sim_card_001", iccid=_ICCID, imsi=_IMSI, home_plmn="46000"),
        role=role,
    )


class _Probe:
    """A test-side paho client that collects responses and events."""

    def __init__(self):
        self._client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id="mpss-it-sim-probe",
        )
        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        self.messages: List[dict] = []
        self._lock = threading.Lock()

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        client.subscribe("mp/rsp/#", qos=1)
        client.subscribe("mp/ind/#", qos=1)

    def _on_message(self, client, userdata, msg):
        try:
            body = json.loads(msg.payload.decode("utf-8"))
        except Exception:
            return
        with self._lock:
            self.messages.append({"topic": msg.topic, "body": body})

    def start(self):
        self._client.connect(BROKER_HOST, BROKER_PORT, 10)
        self._client.loop_start()
        time.sleep(0.2)

    def stop(self):
        self._client.loop_stop()
        self._client.disconnect()

    def send(self, topic: str, data: dict, corr_id: str = "0001") -> None:
        env = {
            "v": 1, "corrId": corr_id,
            "ts": int(time.time() * 1000),
            "src": _SRC, "data": data,
        }
        self._client.publish(topic, json.dumps(env).encode(), qos=1)

    def send_raw(self, topic: str, data: dict) -> None:
        """For ctrl/cmd/action/** topics -- raw JSON, no envelope."""
        self._client.publish(topic, json.dumps(data).encode(), qos=1)

    def wait_for(self, predicate, timeout_s=5.0):
        deadline = time.time() + timeout_s
        while time.time() < deadline:
            with self._lock:
                for m in self.messages:
                    if predicate(m):
                        return m
            time.sleep(0.05)
        return None

    def clear(self):
        with self._lock:
            self.messages.clear()


def _wait_for_state(client: MqttClient, target: str, timeout_s=5.0):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if client.state_fn.__name__ == target:
            return
        time.sleep(0.05)
    raise AssertionError(f"AO never reached {target}; last={client.state_fn.__name__}")


@pytest.fixture
def stack():
    """Full MqttClient + SimSubsystem + probe subscriber."""
    cfg = _make_config()
    probe = _Probe()
    probe.start()

    ss = _make_sim_subsystem(role="dev")
    client = MqttClient(cfg)
    client.register_subsystem(ss)
    client.start()
    _wait_for_state(client, "smfn_operational")
    time.sleep(0.3)  # let SimSubsystem subscribe and publish boot events
    probe.clear()

    yield client, ss, probe

    client.request_stop()
    client.wait_until_stopped(timeout_s=3.0)
    probe.stop()


# ---------------------------------------------------------------------------
# Card + subscription RPCs
# ---------------------------------------------------------------------------

def test_get_state_over_broker(stack):
    client, ss, probe = stack
    probe.send(TOPIC_REQ_GET_STATE, {"slot": 1}, corr_id="aa01")
    rsp = probe.wait_for(lambda m: m["topic"] == TOPIC_RSP_GET_STATE)
    assert rsp is not None, "No get_state response received"
    assert rsp["body"]["data"]["cardState"] == "PRESENT"
    assert rsp["body"]["data"]["appState"] == "READY"


def test_get_iccid_over_broker(stack):
    client, ss, probe = stack
    probe.send(TOPIC_REQ_GET_ICCID, {"slot": 1}, corr_id="aa02")
    rsp = probe.wait_for(lambda m: m["topic"] == TOPIC_RSP_GET_ICCID)
    assert rsp is not None, "No get_iccid response received"
    assert rsp["body"]["data"]["iccid"] == _ICCID


def test_get_imsi_over_broker(stack):
    client, ss, probe = stack
    probe.send(TOPIC_REQ_GET_IMSI, {"slot": 1}, corr_id="aa03")
    rsp = probe.wait_for(lambda m: m["topic"] == TOPIC_RSP_GET_IMSI)
    assert rsp is not None, "No get_imsi response received"
    assert rsp["body"]["data"]["imsi"] == _IMSI


def test_set_power_lifecycle_over_broker(stack):
    client, ss, probe = stack

    probe.send(TOPIC_REQ_SET_POWER, {"slot": 1, "powerOn": False}, corr_id="bb01")
    rsp = probe.wait_for(lambda m: m["topic"] == TOPIC_RSP_SET_POWER)
    assert rsp is not None, "No set_power response received"
    assert "error" not in rsp["body"]

    evt = probe.wait_for(
        lambda m: m["topic"] == TOPIC_IND_CARD_STATE and m["body"]["data"]["cardState"] == "ABSENT"
    )
    assert evt is not None, "No card_state ABSENT indication after power off"

    probe.clear()
    probe.send(TOPIC_REQ_SET_POWER, {"slot": 1, "powerOn": True}, corr_id="bb02")
    evt = probe.wait_for(
        lambda m: m["topic"] == TOPIC_IND_CARD_STATE and m["body"]["data"]["cardState"] == "PRESENT"
    )
    assert evt is not None, "No card_state PRESENT indication after power on"


# ---------------------------------------------------------------------------
# Action-injected hotswap
# ---------------------------------------------------------------------------

def test_hotswap_action_over_broker(stack):
    client, ss, probe = stack
    from generated.python.ctrl_topics import action as action_topics

    probe.send_raw(action_topics.sim.hotswap.req,
                   {"iccid": "8986011234567890999", "imsi": "460000999999999"})

    evt = probe.wait_for(lambda m: m["topic"] == TOPIC_IND_SUB_INFO_CHANGED)
    assert evt is not None, "No sub_info_changed indication after hotswap action"
    assert evt["body"]["data"]["iccid"] == "8986011234567890999"

    probe.clear()
    probe.send(TOPIC_REQ_GET_ICCID, {"slot": 1}, corr_id="cc01")
    rsp = probe.wait_for(lambda m: m["topic"] == TOPIC_RSP_GET_ICCID)
    assert rsp is not None
    assert rsp["body"]["data"]["iccid"] == "8986011234567890999"


# ---------------------------------------------------------------------------
# Subsystem-ready retained events -- one topic per manager, published
# independently.
# ---------------------------------------------------------------------------

_READY_TOPICS = (TOPIC_IND_SUBSYS_READY_CARD, TOPIC_IND_SUBSYS_READY_SUB)


def test_subsystem_ready_retained_events():
    """Start SimSubsystem against live broker; verify retained ready events."""
    cfg = _make_config()
    received_ready: dict[str, bool] = {}
    ready_lock = threading.Lock()
    all_ready = threading.Event()

    probe = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="mpss-it-sim-ready-probe",
    )

    def on_connect(client, userdata, flags, rc, properties):
        for t in _READY_TOPICS:
            client.subscribe(t, qos=1)

    def on_message(client, userdata, msg):
        try:
            body = json.loads(msg.payload.decode())
        except Exception:
            return
        with ready_lock:
            received_ready[msg.topic] = body.get("data", {}).get("ready", False)
            if all(received_ready.get(t) is True for t in _READY_TOPICS):
                all_ready.set()

    probe.on_connect = on_connect
    probe.on_message = on_message
    probe.connect(BROKER_HOST, BROKER_PORT, 10)
    probe.loop_start()

    ss = _make_sim_subsystem(role="dev")
    client = MqttClient(cfg)
    client.register_subsystem(ss)
    client.start()

    try:
        _wait_for_state(client, "smfn_operational")
        assert all_ready.wait(timeout=5.0), (
            f"Not all subsystems published ready=true within 5s; "
            f"received: {received_ready}"
        )

        client.request_stop()
        client.wait_until_stopped(timeout_s=3.0)
        time.sleep(0.3)

        not_ready_received: dict[str, bool] = {}
        not_ready_evt = threading.Event()

        def on_message2(c, userdata, msg):
            try:
                body = json.loads(msg.payload.decode())
            except Exception:
                return
            not_ready_received[msg.topic] = body.get("data", {}).get("ready", True)
            if all(not_ready_received.get(t) is False for t in _READY_TOPICS):
                not_ready_evt.set()

        probe2 = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id="mpss-it-sim-ready-probe2",
        )

        def on_connect2(client2, userdata, flags, rc, properties):
            for t in _READY_TOPICS:
                client2.subscribe(t, qos=1)

        probe2.on_connect = on_connect2
        probe2.on_message = on_message2
        probe2.connect(BROKER_HOST, BROKER_PORT, 10)
        probe2.loop_start()
        not_ready_evt.wait(timeout=3.0)
        probe2.loop_stop()
        probe2.disconnect()

        assert all(not_ready_received.get(t) is False for t in _READY_TOPICS), (
            f"Expected all ready=false after stop; got: {not_ready_received}"
        )
    finally:
        if not client.wait_until_stopped(timeout_s=0.1):
            client.request_stop()
            client.wait_until_stopped(timeout_s=2.0)
        probe.loop_stop()
        probe.disconnect()
