# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for the MPSS MQTT client HSM.

These tests construct `MqttClient` WITHOUT starting its dispatch thread.
We exercise the state machine by hand via `dispatch()` and assert state
transitions, backoff math, and side effects on a mocked paho client.

No network I/O. Safe to run anywhere.
"""
from __future__ import annotations

import socket
from pathlib import Path
from unittest.mock import MagicMock, patch

import pytest
import yaml
from miros import Event, signals

from sml.mpss.config import (
    BrokerConfig,
    ConfigError,
    DebugConfig,
    MpssConfig,
    ReconnectConfig,
    load_config,
)
from sml.mpss.mqtt_client import (
    LinkDownPayload,
    LinkUpPayload,
    MessageReceivedPayload,
    MqttClient,
    _rc_to_int,
    smfn_connecting,
    smfn_off,
    smfn_operational,
    smfn_reconnecting,
    smfn_stopping,
    validate_publish_topic,
)


# --- Fixtures --------------------------------------------------------------


@pytest.fixture
def cfg() -> MpssConfig:
    return MpssConfig(
        broker=BrokerConfig(host="127.0.0.1", port=1883,
                            client_id="unit-test", keepalive_s=30,
                            transport="tcp"),
        reconnect=ReconnectConfig(initial_ms=500, max_ms=30_000,
                                  multiplier=2.0, jitter_pct=0.0),
        debug=DebugConfig(live_spy=False, log_level="WARNING"),
    )


@pytest.fixture
def client(cfg, monkeypatch):
    """An MqttClient that has NOT started its dispatch thread and whose
    paho client creation is patched to yield a MagicMock."""
    mock_paho_cls = MagicMock()
    instance = MagicMock(name="paho.Client.instance")
    mock_paho_cls.return_value = instance
    monkeypatch.setattr("sml.mpss.mqtt_client.mqtt.Client", mock_paho_cls)
    # Also stub CallbackAPIVersion lookup so VERSION2 attribute access works.
    return MqttClient(cfg)


def _dispatch(c: MqttClient, signal: int, payload=None) -> None:
    """Synchronously dispatch a signal into the AO without using the thread."""
    ev = Event(signal=signal, payload=payload) if payload is not None else Event(signal=signal)
    c.dispatch(ev)


def _state_name(c: MqttClient) -> str:
    """Return the current state function's __name__ — useful for asserts."""
    return c.state_fn.__name__


# --- 8.2 Initial state ----------------------------------------------------


def test_initial_state_is_off_and_no_paho_client(client):
    client.start_at(smfn_off)
    assert _state_name(client) == "smfn_off"
    assert client._paho is None


# --- 8.3 SIG_START transitions to Connecting and calls connect_async ------


def test_start_signal_transitions_to_connecting(client, cfg):
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    assert _state_name(client) == "smfn_connecting"
    client._paho.connect_async.assert_called_once_with(
        cfg.broker.host, cfg.broker.port, cfg.broker.keepalive_s
    )


# --- 8.4 CONNACK enters Operational and subscribes ------------------------


def test_connack_enters_operational(client):
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, payload=LinkUpPayload(rc=0))
    assert _state_name(client) == "smfn_operational"


# --- 8.5 Link drop from Operational transitions to Reconnecting -----------


def test_link_drop_from_operational_transitions_to_reconnecting(client):
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, payload=LinkUpPayload(rc=0))
    assert _state_name(client) == "smfn_operational"
    _dispatch(client, signals.LinkDown, payload=LinkDownPayload(rc=7))
    assert _state_name(client) == "smfn_reconnecting"
    assert client._failure_count == 1


# --- 8.6 Backoff sequence -------------------------------------------------


def test_backoff_sequence_matches_spec(cfg):
    # Direct compute, bypassing AO thread / paho.
    c = MqttClient.__new__(MqttClient)
    c._config = cfg
    c._failure_count = 0
    c._current_delay_ms = cfg.reconnect.initial_ms

    delays = []
    for _ in range(8):
        c._failure_count += 1
        delays.append(c._compute_next_delay_ms())

    # Expected: 500, 1000, 2000, 4000, 8000, 16000, 30000, 30000
    assert delays == [500, 1000, 2000, 4000, 8000, 16000, 30000, 30000]


# --- 8.7 Counter reset after recovery -------------------------------------


def test_counter_reset_on_successful_connect(client):
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    # Drive 3 failures
    for _ in range(3):
        _dispatch(client, signals.LinkUp, payload=LinkUpPayload(rc=5))
        # smfn_reconnecting on entry calls _schedule_reconnect_timer which
        # would post a deferred event; we don't run that timer here.
        # Simulate the timer firing to transition back to Connecting:
        _dispatch(client, signals.ReconnectTimer)
    assert client._failure_count == 3
    # Now a successful connect should reset.
    _dispatch(client, signals.LinkUp, payload=LinkUpPayload(rc=0))
    assert _state_name(client) == "smfn_operational"
    assert client._failure_count == 0
    assert client._current_delay_ms == client._config.reconnect.initial_ms


# --- 8.8 SIG_STOP while Reconnecting cancels timer & transitions to Stopping --


def test_stop_while_reconnecting_goes_straight_to_stopping(client):
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, payload=LinkUpPayload(rc=5))
    assert _state_name(client) == "smfn_reconnecting"
    _dispatch(client, signals.Stop)
    assert _state_name(client) == "smfn_stopping"
    # paho disconnect was attempted as part of clean shutdown
    client._paho.disconnect.assert_called()


# --- 8.9 validate_publish_topic rejects wildcards -------------------------


def test_validate_publish_topic_rejects_wildcards():
    with pytest.raises(ValueError):
        validate_publish_topic("foo/+/bar")
    with pytest.raises(ValueError):
        validate_publish_topic("foo/#")
    # Valid topic should not raise
    validate_publish_topic("sml/mpss/heartbeat")


# --- 8.10 ${HOSTNAME} expansion in client_id ------------------------------


def test_hostname_expansion_in_client_id(monkeypatch, tmp_path):
    monkeypatch.setattr(socket, "gethostname", lambda: "master")
    cfg_file = tmp_path / "c.yaml"
    cfg_file.write_text(yaml.safe_dump({
        "mpss": {"broker": {"client_id": "sml-mpss-${HOSTNAME}"}}
    }))
    cfg = load_config(cfg_file)
    assert cfg.broker.client_id == "sml-mpss-master"


# --- 8.11 missing config returns defaults; malformed YAML aborts ----------


def test_missing_config_returns_defaults(tmp_path):
    missing = tmp_path / "nope.yaml"
    cfg = load_config(missing)
    assert cfg.broker.host == "localhost"
    assert cfg.broker.port == 1883
    assert cfg.reconnect.initial_ms == 500


def test_malformed_yaml_raises_config_error(tmp_path):
    bad = tmp_path / "bad.yaml"
    bad.write_text("mpss: {broker: {host: localhost, port: 1883\n")  # truncated
    with pytest.raises(ConfigError):
        load_config(bad)


def test_unknown_key_raises_config_error(tmp_path):
    bad = tmp_path / "bad.yaml"
    bad.write_text(yaml.safe_dump({
        "mpss": {"broker": {"host": "localhost"}, "nonsense": True}
    }))
    with pytest.raises(ConfigError) as excinfo:
        load_config(bad)
    assert "nonsense" in str(excinfo.value)


# --- 8.12 callback discipline — does not synchronously change state -------


def test_paho_callback_only_posts_event_without_changing_state_sync(client):
    """Invoking a paho callback directly must enqueue an event, NOT
    transition the HSM synchronously inside the callback."""
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    assert _state_name(client) == "smfn_connecting"

    # Fire the paho on_connect callback as paho's loop thread would.
    before_state = _state_name(client)
    client._cb_on_connect(client._paho, None, None, 0, None)
    # State must NOT have changed yet — the dispatch happens when the AO
    # thread runs. Since we never started it, state is still Connecting.
    assert _state_name(client) == before_state == "smfn_connecting"


# --- regression: paho 2.x ReasonCode is not int-convertible directly ------


class _FakeReasonCode:
    """Mimics paho.mqtt.reasoncodes.ReasonCode for unit tests."""
    def __init__(self, value: int):
        self.value = value
        self.is_failure = value != 0


def test_rc_to_int_accepts_reason_code_object():
    rc_ok = _FakeReasonCode(0)
    rc_bad = _FakeReasonCode(135)
    assert _rc_to_int(rc_ok) == 0
    assert _rc_to_int(rc_bad) == 135
    # Legacy / test int passthrough still works.
    assert _rc_to_int(0) == 0
    assert _rc_to_int(7) == 7
    # None defaults to 0 (clean disconnect path).
    assert _rc_to_int(None) == 0


def test_cb_on_connect_handles_reason_code_object(client):
    """Regression: calling the callback with a ReasonCode object must
    not raise; we observed `TypeError: int() argument must be ... not
    'ReasonCode'` against live mosquitto."""
    client.start_at(smfn_off)
    _dispatch(client, signals.Start)
    # Must not raise:
    client._cb_on_connect(client._paho, None, None, _FakeReasonCode(0), None)


# --- sub-AO integration tests (Task 9.5) ---------------------------------


def test_subsystem_start_called_on_operational_entry(client):
    """register_subsystem: start() called when AO enters Operational."""
    mock_ds = MagicMock()
    mock_ds.handle_message = MagicMock(return_value=False)
    client.register_subsystem(mock_ds)
    client.start_at(smfn_off)
    client._paho = MagicMock()
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, LinkUpPayload(rc=0))
    mock_ds.start.assert_called_once()


def test_subsystem_not_stopped_on_disconnect(client):
    """register_subsystem: a link flap must NOT stop() the subsystem.

    stop() joins the subsystem's miros dispatch thread and a joined AO cannot
    be revived, so tearing subsystems down on LinkDown would kill them for the
    rest of the process -- stale retained ready=true in the broker plus every
    `*.req` unanswered forever. Teardown belongs to smfn_stopping only.
    """
    mock_ds = MagicMock()
    mock_ds.handle_message = MagicMock(return_value=False)
    client.register_subsystem(mock_ds)
    client.start_at(smfn_off)
    client._paho = MagicMock()
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, LinkUpPayload(rc=0))
    mock_ds.start.assert_called_once()
    _dispatch(client, signals.LinkDown, LinkDownPayload(rc=0))
    mock_ds.stop.assert_not_called()


def test_subsystem_resubscribed_not_restarted_on_reconnect(client):
    """A second entry into Operational resubscribes; it must not re-start()."""
    mock_ds = MagicMock()
    mock_ds.handle_message = MagicMock(return_value=False)
    client.register_subsystem(mock_ds)
    client.start_at(smfn_off)
    client._paho = MagicMock()
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, LinkUpPayload(rc=0))
    mock_ds.start.assert_called_once()
    mock_ds.resubscribe.assert_not_called()

    _dispatch(client, signals.LinkDown, LinkDownPayload(rc=0))
    # Reconnecting only reacts to its own backoff timer; drive it by hand so
    # we re-enter Connecting and then Operational a second time.
    _dispatch(client, signals.ReconnectTimer)
    _dispatch(client, signals.LinkUp, LinkUpPayload(rc=0))
    assert _state_name(client) == "smfn_operational"
    mock_ds.start.assert_called_once()  # still once -- never re-started
    mock_ds.resubscribe.assert_called_once()


def test_subsystem_stopped_on_process_shutdown(client):
    """stop() is called exactly once, on the way to process exit."""
    mock_ds = MagicMock()
    mock_ds.handle_message = MagicMock(return_value=False)
    client.register_subsystem(mock_ds)
    client.start_at(smfn_off)
    client._paho = MagicMock()
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, LinkUpPayload(rc=0))
    _dispatch(client, signals.Stop)
    mock_ds.stop.assert_called_once()


def test_subsystem_start_exception_does_not_crash_ao(client):
    """A sub-AO start() failure must not prevent AO from staying in Operational."""
    mock_ds = MagicMock()
    mock_ds.start.side_effect = RuntimeError("boom")
    mock_ds.handle_message = MagicMock(return_value=False)
    client.register_subsystem(mock_ds)
    client.start_at(smfn_off)
    client._paho = MagicMock()
    _dispatch(client, signals.Start)
    _dispatch(client, signals.LinkUp, LinkUpPayload(rc=0))
    # AO should still be in operational despite start() raising.
    assert _state_name(client) == "smfn_operational"


# --- regression: paho's dead loop thread must be reaped before reconnect ---


class _FakeThread:
    def __init__(self, alive: bool):
        self._alive = alive

    def is_alive(self) -> bool:
        return self._alive


def test_reconnect_reaps_dead_paho_loop_thread(client):
    """A finished loop thread is cleared so loop_start() can spawn a new one.

    paho only resets `_thread` inside `loop_stop()`, and `loop_start()` returns
    MQTT_ERR_INVAL (silently -- nothing raises) while `_thread` is non-None. Left
    unreaped, every reconnect queues a CONNECT that no network thread ever
    flushes and dies on the connect timeout instead, forever.
    """
    client.start_at(smfn_off)
    client._paho = MagicMock()
    client._paho._thread = _FakeThread(alive=False)
    _dispatch(client, signals.Start)
    client._paho.loop_stop.assert_called_once()
    client._paho.loop_start.assert_called_once()


def test_reconnect_does_not_join_a_live_paho_loop_thread(client):
    """A still-running loop thread is left alone -- loop_stop() would join it.

    `_initiate_connect` runs on the AO dispatch thread, so joining a live paho
    loop would stall every other event for as long as that loop keeps going.
    """
    client.start_at(smfn_off)
    client._paho = MagicMock()
    client._paho._thread = _FakeThread(alive=True)
    _dispatch(client, signals.Start)
    client._paho.loop_stop.assert_not_called()


def test_first_connect_does_not_call_loop_stop(client):
    """Fresh client: `_thread` is None, so there is nothing to reap."""
    client.start_at(smfn_off)
    client._paho = MagicMock()
    client._paho._thread = None
    _dispatch(client, signals.Start)
    client._paho.loop_stop.assert_not_called()
    client._paho.loop_start.assert_called_once()
