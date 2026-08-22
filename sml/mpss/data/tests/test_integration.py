# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Integration tests for the MPSS data sub-package.

Requires a live mosquitto broker on localhost:1883.
If the broker is unreachable the suite FAILS FAST (not skip).

Run with:
    pytest sml/mpss/data/tests/test_integration.py -v

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
from sml.mpss.data import DataSubsystem
from sml.config.models import CallTimingPresetSeed, InterfacePresetSeed, SeedProfile
from generated.python.topics import data as topics_data

BROKER_HOST = "localhost"
BROKER_PORT = 1883
_SRC = "test-dev-0001"  # used as the request src field

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
            "  Unit tests only:  pytest sml/mpss/data/tests/ --ignore=sml/mpss/data/tests/test_integration.py",
            returncode=2,
        )


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

def _make_config() -> MpssConfig:
    return MpssConfig(
        broker=BrokerConfig(host=BROKER_HOST, port=BROKER_PORT,
                            client_id="mpss-it-data", keepalive_s=10),
        reconnect=ReconnectConfig(initial_ms=200, max_ms=2_000,
                                  multiplier=2.0, jitter_pct=0.0),
        debug=DebugConfig(live_spy=False, log_level="WARNING"),
    )


def _make_data_subsystem(connect_ms=100, disconnect_ms=100, role="dev") -> DataSubsystem:
    return DataSubsystem(
        seed_profiles=[SeedProfile(profileId=1, apn="internet", ipFamily="IPV4V6",
                                   authType="NONE", username="", password="",
                                   techType="3GPP", isDefault=True)],
        interface_preset=InterfacePresetSeed(ifname_prefix="rmnet_data", ifname_pool_size=4),
        call_timing_preset=CallTimingPresetSeed(
            call_connect_delay_ms=connect_ms, call_disconnect_delay_ms=disconnect_ms,
        ),
        role=role,
    )


class _Probe:
    """A test-side paho client that collects responses and events."""

    def __init__(self):
        self._client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id="mpss-it-probe",
        )
        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        self.messages: List[dict] = []
        self._lock = threading.Lock()
        self._any_msg = threading.Event()

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        # Subscribe to all responses (shared wildcard, PA-side convention)
        # and indications from the data subsystem.
        client.subscribe("mp/rsp/#", qos=1)
        client.subscribe("mp/ind/#", qos=1)

    def _on_message(self, client, userdata, msg):
        try:
            body = json.loads(msg.payload.decode("utf-8"))
        except Exception:
            return
        with self._lock:
            self.messages.append({"topic": msg.topic, "body": body})
            self._any_msg.set()

    def start(self):
        self._client.connect(BROKER_HOST, BROKER_PORT, 10)
        self._client.loop_start()
        time.sleep(0.2)  # wait for subscriptions to settle

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

    def wait_for(self, predicate, timeout_s=5.0) -> dict | None:
        """Wait until a received message matches predicate, return it or None."""
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
            self._any_msg.clear()


def _wait_for_state(client: MqttClient, target: str, timeout_s=5.0):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if client.state_fn.__name__ == target:
            return
        time.sleep(0.05)
    raise AssertionError(f"AO never reached {target}; last={client.state_fn.__name__}")


@pytest.fixture
def stack():
    """Full MqttClient + DataSubsystem + probe subscriber."""
    cfg = _make_config()
    probe = _Probe()
    probe.start()

    ds = _make_data_subsystem(connect_ms=150, disconnect_ms=100, role="dev")
    client = MqttClient(cfg)
    client.register_subsystem(ds)
    client.start()
    _wait_for_state(client, "smfn_operational")
    time.sleep(0.3)  # let DataSubsystem subscribe and publish boot events
    probe.clear()

    yield client, ds, probe

    client.request_stop()
    client.wait_until_stopped(timeout_s=3.0)
    probe.stop()


# ---------------------------------------------------------------------------
# Full data call lifecycle
# ---------------------------------------------------------------------------

def test_data_call_start_stop_lifecycle(stack):
    client, ds, probe = stack

    # 1. send start
    probe.send(topics_data.start_data_call.req,
               {"profileId": 1, "ipFamily": "IPV4V6",
                "ifname": "", "opType": "DATA_LOCAL", "slot": 1},
               corr_id="aa01")

    # 2. expect response with CONNECTING
    rsp_connecting = probe.wait_for(
        lambda m: (m["topic"] == topics_data.start_data_call.rsp and
                   m["body"].get("data", {}).get("status") == "CONNECTING"),
        timeout_s=5.0,
    )
    assert rsp_connecting is not None, "No CONNECTING response received"
    ifname = rsp_connecting["body"]["data"]["ifname"]
    assert ifname.startswith("rmnet_data")

    # 3. wait for state event CONNECTED (after connect delay)
    evt_connected = probe.wait_for(
        lambda m: (m["topic"] == topics_data.call_state.ind and
                   m["body"].get("data", {}).get("status") == "CONNECTED"),
        timeout_s=5.0,
    )
    assert evt_connected is not None, "No CONNECTED event received"
    assert "if_address" in evt_connected["body"]["data"]["ipv4"]

    # 4. send stop
    probe.clear()
    probe.send(topics_data.stop_data_call.req,
               {"profileId": 1, "ipFamily": "IPV4V6",
                "ifname": ifname, "opType": "DATA_LOCAL", "slot": 1},
               corr_id="aa02")

    # 5. expect response with DISCONNECTING
    rsp_discon = probe.wait_for(
        lambda m: (m["topic"] == topics_data.stop_data_call.rsp and
                   m["body"].get("data", {}).get("status") == "DISCONNECTING"),
        timeout_s=5.0,
    )
    assert rsp_discon is not None, "No DISCONNECTING response received"

    # 6. wait for state event NO_NET
    evt_no_net = probe.wait_for(
        lambda m: (m["topic"] == topics_data.call_state.ind and
                   m["body"].get("data", {}).get("status") == "NO_NET"),
        timeout_s=5.0,
    )
    assert evt_no_net is not None, "No NO_NET event received"


# ---------------------------------------------------------------------------
# Profile lifecycle with changed events
# ---------------------------------------------------------------------------

def test_profile_lifecycle_with_changed_events(stack):
    client, ds, probe = stack

    # 1. create a new profile
    probe.send(topics_data.create_profile.req,
               {"profileName": "ent", "apn": "enterprise", "userName": "", "password": "",
                "techPref": "TP_3GPP", "authType": "AUTH_NONE", "ipFamilyType": "IPV4",
                "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED", "clatEnabled": False, "slot": 1},
               corr_id="bb01")

    rsp_create = probe.wait_for(
        lambda m: m["topic"] == topics_data.create_profile.rsp and "profileId" in m["body"].get("data", {}),
        timeout_s=5.0,
    )
    assert rsp_create is not None, "No create response"
    new_pid = rsp_create["body"]["data"]["profileId"]

    evt_added = probe.wait_for(
        lambda m: (m["topic"] == topics_data.profile_changed.ind and
                   m["body"]["data"].get("event") == "CREATE"),
        timeout_s=5.0,
    )
    assert evt_added is not None, "No profile-changed/CREATE event"

    # 2. query — new profile must appear
    probe.clear()
    probe.send(topics_data.query_profile.req, {}, corr_id="bb02")
    rsp_query = probe.wait_for(
        lambda m: m["topic"] == topics_data.query_profile.rsp and "profiles" in m["body"].get("data", {}),
        timeout_s=5.0,
    )
    assert rsp_query is not None
    pids = [p["id"] for p in rsp_query["body"]["data"]["profiles"]]
    assert new_pid in pids, f"new profile {new_pid} not in query result: {pids}"

    # 3. modify
    probe.clear()
    probe.send(topics_data.modify_profile.req,
               {"profileId": new_pid, "profileName": "ent", "apn": "modified", "userName": "",
                "password": "", "techPref": "TP_3GPP", "authType": "AUTH_NONE",
                "ipFamilyType": "IPV4", "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED",
                "clatEnabled": False, "slot": 1},
               corr_id="bb03")
    evt_modified = probe.wait_for(
        lambda m: (m["topic"] == topics_data.profile_changed.ind and
                   m["body"]["data"].get("event") == "MODIFY"),
        timeout_s=5.0,
    )
    assert evt_modified is not None, "No profile-changed/MODIFY event"

    # 4. delete
    probe.clear()
    probe.send(topics_data.delete_profile.req,
               {"profileId": new_pid, "techPref": "TP_3GPP", "slot": 1}, corr_id="bb04")
    evt_removed = probe.wait_for(
        lambda m: (m["topic"] == topics_data.profile_changed.ind and
                   m["body"]["data"].get("event") == "DELETE"),
        timeout_s=5.0,
    )
    assert evt_removed is not None, "No profile-changed/DELETE event"

    # 5. query — deleted profile must be gone
    probe.clear()
    probe.send(topics_data.query_profile.req, {}, corr_id="bb05")
    rsp_query2 = probe.wait_for(
        lambda m: m["topic"] == topics_data.query_profile.rsp and "profiles" in m["body"].get("data", {}),
        timeout_s=5.0,
    )
    assert rsp_query2 is not None
    pids2 = [p["id"] for p in rsp_query2["body"]["data"]["profiles"]]
    assert new_pid not in pids2, f"deleted profile {new_pid} still in query: {pids2}"


# ---------------------------------------------------------------------------
# Subsystem-ready retained events -- one topic per subsystem, published
# independently (not gated on the other two).
# ---------------------------------------------------------------------------

_READY_TOPICS = (topics_data.subsys_ready_data.ind, topics_data.subsys_ready_profile.ind, topics_data.subsys_ready_serv.ind)


def test_subsystem_ready_retained_events():
    """Start DataSubsystem against live broker; verify retained ready events."""
    cfg = _make_config()
    received_ready: dict[str, bool] = {}
    ready_lock = threading.Lock()
    all_ready = threading.Event()

    probe = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="mpss-it-ready-probe",
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

    ds = _make_data_subsystem(connect_ms=50, disconnect_ms=50, role="dev")
    client = MqttClient(cfg)
    client.register_subsystem(ds)
    client.start()

    try:
        _wait_for_state(client, "smfn_operational")
        assert all_ready.wait(timeout=5.0), (
            f"Not all subsystems published ready=true within 5s; "
            f"received: {received_ready}"
        )

        # Now stop and check ready=false
        client.request_stop()
        client.wait_until_stopped(timeout_s=3.0)
        time.sleep(0.3)

        # Re-subscribe to check retained messages are now ready=false
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
            client_id="mpss-it-ready-probe2",
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
