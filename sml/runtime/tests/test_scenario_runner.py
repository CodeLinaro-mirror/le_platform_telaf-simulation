# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for sml.runtime.scenario_runner.ScenarioRunner -- no broker
required.

Every test builds its own tiny devices/environments/scenario YAML trio
under ``tmp_path`` (``ScenarioRunner.load()`` requires
``setup.devices_config`` / ``environment_config`` to resolve real
catalog entries via ``sml.runtime.loader`` -- there is no "just
initial_state + timeline" shortcut).

Since T-11 the runner is a miros Active Object; the AO dispatch thread
runs handlers asynchronously. Tests poll state via ``wait_for_state``
and RPC responses via ``wait_for_response`` rather than assert
synchronously.
"""
from __future__ import annotations

import json
import logging
import threading
import time
from unittest.mock import MagicMock

import pytest
import yaml
from miros import Event, signals

from generated.python.ctrl_topics import scenario as scenario_topics
from sml.runtime.action_dispatcher import ActionDispatcher
from sml.runtime.loader import LoaderError
from sml.runtime.scenario_runner import ScenarioRunner


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

class _MockPublish:
    """Thread-safe publish spy (AO thread calls it; test thread reads it)."""

    def __init__(self):
        self._lock = threading.Lock()
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        entry = {"topic": topic, "payload": json.loads(payload),
                 "qos": qos, "retain": retain}
        with self._lock:
            self.calls.append(entry)

    def reset(self):
        with self._lock:
            self.calls.clear()

    def snapshot(self) -> list:
        with self._lock:
            return list(self.calls)

    def last(self, topic):
        with self._lock:
            for c in reversed(self.calls):
                if c["topic"] == topic:
                    return c
        return None


def _wait_for_state(runner: ScenarioRunner, target: str,
                    timeout_s: float = 3.0) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if runner.state_fn is not None and runner.state_fn.__name__ == target:
            return
        time.sleep(0.01)
    got = runner.state_fn.__name__ if runner.state_fn else None
    raise AssertionError(f"AO never reached {target}; last state was {got}")


def _wait_for_response(pub: _MockPublish, topic: str, timeout_s: float = 2.0):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        c = pub.last(topic)
        if c is not None:
            return c
        time.sleep(0.01)
    raise AssertionError(f"no response on {topic} within {timeout_s}s")


def _wait_for_dispatch_calls(mock, n: int, timeout_s: float = 2.0) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if mock.dispatch_action.call_count >= n:
            return
        time.sleep(0.01)
    raise AssertionError(
        f"dispatch_action called {mock.dispatch_action.call_count}, expected >= {n}"
    )


@pytest.fixture
def pub():
    return _MockPublish()


@pytest.fixture
def data_dispatcher():
    d = MagicMock()
    d.dispatch_action.return_value = True
    # dispatch_action is called from the AO thread -- MagicMock's
    # internal recording is thread-safe enough for these tests.
    return d


@pytest.fixture
def dispatcher(data_dispatcher):
    return ActionDispatcher(domains={"data": data_dispatcher})


_DEVICES_DOC = {
    "version": "1.0",
    "modems": [
        {"id": "modem_0_online", "type": "primary", "supported_rats": ["LTE"],
         "ecall_capable": False, "state": "online"},
    ],
    "sim_slots": [
        {"id": "sim_slot_0_inserted_sim001", "physical_type": "physical",
         "slot_id": 1, "state": "inserted", "installed_sim": "sim_card_001"},
    ],
    "sim_cards": [
        {"id": "sim_card_001", "iccid": "8986011234567890123",
         "imsi": "310150123456789", "home_plmn": "310150"},
    ],
    "data_profiles": [],
    "interface_presets": [],
    "call_timing_presets": [],
    "ip_presets": [],
}

_ENVIRONMENTS_DOC = {
    "version": "1.0",
    "cells": [
        {"id": "cell_urban_A", "plmn": "310150", "rat": "LTE", "default_rsrp_dbm": -95},
    ],
    "signal_models": [
        {"id": "stable_urban", "kind": "static", "variance_db": 0},
    ],
}


def _write_config_tree(tmp_path, timeline=None, initial_state=None, name="t"):
    scenarios_dir = tmp_path / "scenarios"
    scenarios_dir.mkdir(exist_ok=True)
    (tmp_path / "devices.yaml").write_text(yaml.safe_dump(_DEVICES_DOC))
    (tmp_path / "environments.yaml").write_text(yaml.safe_dump(_ENVIRONMENTS_DOC))

    doc = {
        "version": "1.0",
        "name": name,
        "setup": {
            "devices_config": "devices.yaml",
            "environment_config": "environments.yaml",
            "duration": "30s",
        },
        "initial_state": initial_state or {},
        "timeline": timeline or [],
    }
    p = scenarios_dir / f"{name}.yaml"
    p.write_text(yaml.safe_dump(doc))
    return p


def _start_paused(runner: ScenarioRunner, pub, sub=None, unsub=None):
    """Start the runner and immediately pin it in ``smfn_paused``.

    Autoplay is unavoidable in the new design (start() posts Start which
    transits idle -> running), so tests that want direct control over
    the timeline queue a Pause behind Start and wait for the paused
    state before poking the runner further.
    """
    runner.start(pub, sub or MagicMock(), unsub or MagicMock())
    runner.post_fifo(Event(signal=signals.Pause))
    _wait_for_state(runner, "smfn_paused")


# ---------------------------------------------------------------------------
# load() / initial_state
# ---------------------------------------------------------------------------

def test_load_missing_file_raises(dispatcher):
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    with pytest.raises(LoaderError):
        runner.load("/nonexistent/path.yaml")


def test_load_applies_modems_and_sim_slots(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path, initial_state={
        "modems": ["modem_0_online"],
        "sim_slots": ["sim_slot_0_inserted_sim001"],
    })
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    assert runner.modem_runtimes["modem_0_online"].state == "online"
    assert runner.sim_slot_runtimes["sim_slot_0_inserted_sim001"].state == "inserted"
    assert runner.sim_slot_runtimes["sim_slot_0_inserted_sim001"].installed_sim.id == "sim_card_001"


def test_load_unresolvable_id_raises(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path, initial_state={
        "modems": ["modem_nonexistent"],
    })
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    with pytest.raises(LoaderError):
        runner.load(p)


def test_load_applies_radio(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path, initial_state={
        "radio": {"serving_cell": "cell_urban_A", "signal_model": "stable_urban"},
    })
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)  # must not raise


def test_load_invalid_enum_state_raises(tmp_path, dispatcher):
    scenarios_dir = tmp_path / "scenarios"
    scenarios_dir.mkdir(exist_ok=True)
    bad_devices_doc = {
        "version": "1.0",
        "modems": [
            {"id": "modem_0_online", "type": "primary", "supported_rats": ["LTE"],
             "ecall_capable": False, "state": "banana"},
        ],
        "sim_slots": [],
        "sim_cards": [],
        "data_profiles": [],
        "interface_presets": [],
        "call_timing_presets": [],
        "ip_presets": [],
    }
    (tmp_path / "devices.yaml").write_text(yaml.safe_dump(bad_devices_doc))
    (tmp_path / "environments.yaml").write_text(yaml.safe_dump(_ENVIRONMENTS_DOC))
    doc = {
        "version": "1.0",
        "name": "t",
        "setup": {
            "devices_config": "devices.yaml",
            "environment_config": "environments.yaml",
            "duration": "30s",
        },
        "initial_state": {},
        "timeline": [],
    }
    p = scenarios_dir / "t.yaml"
    p.write_text(yaml.safe_dump(doc))

    runner = ScenarioRunner(action_dispatcher=dispatcher)
    with pytest.raises(LoaderError):
        runner.load(p)


def test_load_warns_on_unset_optional_slot_field(tmp_path, dispatcher, caplog):
    scenarios_dir = tmp_path / "scenarios"
    scenarios_dir.mkdir(exist_ok=True)
    devices_doc = dict(_DEVICES_DOC, sim_slots=[
        {"id": "sim_slot_0_removed", "physical_type": "physical",
         "slot_id": 1, "state": "removed"},
    ])
    (tmp_path / "devices.yaml").write_text(yaml.safe_dump(devices_doc))
    (tmp_path / "environments.yaml").write_text(yaml.safe_dump(_ENVIRONMENTS_DOC))
    doc = {
        "version": "1.0",
        "name": "t",
        "setup": {
            "devices_config": "devices.yaml",
            "environment_config": "environments.yaml",
            "duration": "30s",
        },
        "initial_state": {"sim_slots": ["sim_slot_0_removed"]},
        "timeline": [],
    }
    p = scenarios_dir / "t.yaml"
    p.write_text(yaml.safe_dump(doc))

    runner = ScenarioRunner(action_dispatcher=dispatcher)
    with caplog.at_level(logging.WARNING):
        runner.load(p)
    assert any(
        record.levelno == logging.WARNING and "installed_sim" in record.message
        for record in caplog.records
    )


def test_load_empty_timeline_ok(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path)
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    assert runner._timeline == []


def test_load_persistent_field_defaults_empty(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path)
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    assert runner.persistent == []


def test_load_persistent_field_set_from_devices_doc(tmp_path, dispatcher):
    scenarios_dir = tmp_path / "scenarios"
    scenarios_dir.mkdir(exist_ok=True)
    devices_doc = dict(_DEVICES_DOC, persistent=["data_profiles"])
    (tmp_path / "devices.yaml").write_text(yaml.safe_dump(devices_doc))
    (tmp_path / "environments.yaml").write_text(yaml.safe_dump(_ENVIRONMENTS_DOC))
    doc = {
        "version": "1.0",
        "name": "t",
        "setup": {
            "devices_config": "devices.yaml",
            "environment_config": "environments.yaml",
            "duration": "30s",
        },
        "initial_state": {},
        "timeline": [],
    }
    p = scenarios_dir / "t.yaml"
    p.write_text(yaml.safe_dump(doc))

    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    assert runner.persistent == ["data_profiles"]


# ---------------------------------------------------------------------------
# step / seek (via ctrl/cmd/scenario RPCs; direct .step() is gone)
# ---------------------------------------------------------------------------

def _runner_with_timeline(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "10s", "action": "data.force_serv_state", "args": {"networkRat": "WCDMA"}},
        {"at": "20s", "action": "data.force_call_drop", "args": {"profileId": 1}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    return runner


def test_step_executes_in_order_via_rpc(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        pub.reset()
        runner.handle_message(scenario_topics.step.req,
                              json.dumps({"steps": 1}).encode())
        rsp = _wait_for_response(pub, scenario_topics.step.rsp)
        assert rsp["payload"]["executed_steps"] == 1
        assert rsp["payload"]["reached_end"] is False
        _wait_for_dispatch_calls(data_dispatcher, 1)
        data_dispatcher.dispatch_action.assert_called_once_with(
            "data.force_serv_state", {"networkRat": "WCDMA"})
    finally:
        runner.stop()


def test_step_n_greater_than_remaining_stops_at_end(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        pub.reset()
        runner.handle_message(scenario_topics.step.req,
                              json.dumps({"steps": 5}).encode())
        rsp = _wait_for_response(pub, scenario_topics.step.rsp)
        assert rsp["payload"]["executed_steps"] == 2
        assert rsp["payload"]["reached_end"] is True
        _wait_for_state(runner, "smfn_done")
        assert data_dispatcher.dispatch_action.call_count == 2
    finally:
        runner.stop()


def test_seek_fires_all_steps_up_to_target(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        pub.reset()
        runner.handle_message(scenario_topics.seek.req,
                              json.dumps({"at": "15s"}).encode())
        rsp = _wait_for_response(pub, scenario_topics.seek.rsp)
        assert rsp["payload"]["reached_at_ms"] == 15000
        _wait_for_dispatch_calls(data_dispatcher, 1)
        data_dispatcher.dispatch_action.assert_called_once_with(
            "data.force_serv_state", {"networkRat": "WCDMA"})
    finally:
        runner.stop()


def test_seek_backward_is_noop(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        runner.handle_message(scenario_topics.seek.req,
                              json.dumps({"at": "15s"}).encode())
        _wait_for_response(pub, scenario_topics.seek.rsp)
        _wait_for_dispatch_calls(data_dispatcher, 1)
        data_dispatcher.reset_mock()
        pub.reset()

        runner.handle_message(scenario_topics.seek.req,
                              json.dumps({"at": "5s"}).encode())
        rsp = _wait_for_response(pub, scenario_topics.seek.rsp)
        assert rsp["payload"]["reached_at_ms"] == 15000  # clock stayed put
        # No further dispatch calls -- give the AO a moment to prove it.
        time.sleep(0.1)
        data_dispatcher.dispatch_action.assert_not_called()
    finally:
        runner.stop()


def test_unknown_action_domain_skipped_not_raised(tmp_path, pub, dispatcher, data_dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "1s", "action": "sim.hotswap", "args": {"target": "sim_slot_0_inserted_sim001"}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    _start_paused(runner, pub)
    try:
        runner.handle_message(scenario_topics.step.req,
                              json.dumps({"steps": 1}).encode())
        rsp = _wait_for_response(pub, scenario_topics.step.rsp)
        assert rsp["payload"]["executed_steps"] == 1
        data_dispatcher.dispatch_action.assert_not_called()
    finally:
        runner.stop()


def test_dispatcher_not_owning_action_logged_not_raised(tmp_path, pub, dispatcher, data_dispatcher):
    data_dispatcher.dispatch_action.return_value = False
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        runner.handle_message(scenario_topics.step.req,
                              json.dumps({"steps": 1}).encode())
        rsp = _wait_for_response(pub, scenario_topics.step.rsp)
        assert rsp["payload"]["executed_steps"] == 1
    finally:
        runner.stop()


# ---------------------------------------------------------------------------
# start()/stop() auto-play, pause/resume/abort
# ---------------------------------------------------------------------------

def test_start_autoplay_fires_step_on_wall_clock(tmp_path, pub, dispatcher, data_dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "50ms", "action": "data.force_serv_state", "args": {"networkRat": "WCDMA"}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    try:
        runner.start(pub, MagicMock(), MagicMock())
        _wait_for_dispatch_calls(data_dispatcher, 1, timeout_s=1.0)
        _wait_for_state(runner, "smfn_done")
        data_dispatcher.dispatch_action.assert_called_once_with(
            "data.force_serv_state", {"networkRat": "WCDMA"})
    finally:
        runner.stop()


def test_pause_prevents_autoplay_firing(tmp_path, pub, dispatcher, data_dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "50ms", "action": "data.force_serv_state", "args": {"networkRat": "WCDMA"}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    try:
        _start_paused(runner, pub)
        time.sleep(0.2)
        data_dispatcher.dispatch_action.assert_not_called()
    finally:
        runner.stop()


def test_resume_continues_autoplay(tmp_path, pub, dispatcher, data_dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "50ms", "action": "data.force_serv_state", "args": {"networkRat": "WCDMA"}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    try:
        _start_paused(runner, pub)
        runner.handle_message(scenario_topics.resume.req, b"{}")
        _wait_for_state(runner, "smfn_running")
        _wait_for_dispatch_calls(data_dispatcher, 1, timeout_s=1.0)
        data_dispatcher.dispatch_action.assert_called_once()
    finally:
        runner.stop()


def test_abort_stops_further_firing_even_after_resume_attempt(tmp_path, pub, dispatcher, data_dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "50ms", "action": "data.force_serv_state", "args": {"networkRat": "WCDMA"}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    try:
        runner.start(pub, MagicMock(), MagicMock())
        runner.handle_message(scenario_topics.abort.req, b"{}")
        _wait_for_state(runner, "smfn_done")
        # resume after abort must not resurrect playback
        runner.handle_message(scenario_topics.resume.req, b"{}")
        time.sleep(0.2)
        data_dispatcher.dispatch_action.assert_not_called()
    finally:
        runner.stop()


def test_progress_published_retained_on_start(tmp_path, pub, dispatcher):
    p = _write_config_tree(tmp_path, timeline=[
        {"at": "30s", "action": "data.force_serv_state", "args": {"networkRat": "WCDMA"}},
    ])
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    try:
        runner.start(pub, MagicMock(), MagicMock())
        _wait_for_state(runner, "smfn_running")
        # Poll for the running progress payload.
        deadline = time.time() + 1.0
        call = None
        while time.time() < deadline:
            c = pub.last("mp/sys/0/state/config/scenario_progress")
            if c and c["payload"]["state"] == "running":
                call = c
                break
            time.sleep(0.01)
        assert call is not None, "no running progress ever published"
        assert call["retain"] is True
    finally:
        runner.stop()


# ---------------------------------------------------------------------------
# ctrl/cmd/scenario/** message handlers
# ---------------------------------------------------------------------------

def test_owns_registered_topics(dispatcher):
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    try:
        assert runner.owns_topic(scenario_topics.step.req)
        assert runner.owns_topic(scenario_topics.seek.req)
        assert runner.owns_topic(scenario_topics.pause.req)
        assert runner.owns_topic(scenario_topics.resume.req)
        assert runner.owns_topic(scenario_topics.abort.req)
        assert not runner.owns_topic("ctrl/cmd/scenario/unrelated")
    finally:
        runner.stop()


def test_step_rpc_publishes_response(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        pub.reset()
        runner.handle_message(scenario_topics.step.req,
                              json.dumps({"steps": 1}).encode())
        call = _wait_for_response(pub, scenario_topics.step.rsp)
        assert call["payload"]["executed_steps"] == 1
        assert call["payload"]["current_time_ms"] == 10000
        assert call["payload"]["reached_end"] is False
    finally:
        runner.stop()


def test_seek_rpc_publishes_response(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        pub.reset()
        runner.handle_message(scenario_topics.seek.req,
                              json.dumps({"at": "15s"}).encode())
        call = _wait_for_response(pub, scenario_topics.seek.rsp)
        assert call["payload"]["reached_at_ms"] == 15000
    finally:
        runner.stop()


def test_pause_resume_abort_oneway_no_response(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    runner.start(pub, MagicMock(), MagicMock())
    try:
        _wait_for_state(runner, "smfn_running")
        pub.reset()
        runner.handle_message(scenario_topics.pause.req, b"{}")
        _wait_for_state(runner, "smfn_paused")
        assert all(c["topic"] != "ctrl/cmd/scenario/pause/rsp" for c in pub.snapshot())
    finally:
        runner.stop()


def test_invalid_step_payload_dropped(tmp_path, pub, dispatcher, data_dispatcher):
    runner = _runner_with_timeline(tmp_path, dispatcher)
    _start_paused(runner, pub)
    try:
        pub.reset()
        runner.handle_message(scenario_topics.step.req,
                              json.dumps({"steps": 0}).encode())
        # Give the AO a moment; must not produce a step.rsp.
        time.sleep(0.1)
        assert pub.last(scenario_topics.step.rsp) is None
        data_dispatcher.dispatch_action.assert_not_called()
    finally:
        runner.stop()


def test_unowned_topic_ignored(dispatcher):
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    try:
        assert runner.handle_message("ctrl/cmd/scenario/unrelated", b"{}") is False
    finally:
        runner.stop()


def test_start_subscribes_all_owned_topics(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path)
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    sub_mock = MagicMock()
    try:
        runner.start(MagicMock(), sub_mock, MagicMock())
        subscribed = {c.args[0] for c in sub_mock.call_args_list}
        expected = {
            scenario_topics.step.req, scenario_topics.seek.req,
            scenario_topics.pause.req, scenario_topics.resume.req,
            scenario_topics.abort.req,
        }
        assert subscribed == expected
    finally:
        runner.stop()


def test_stop_unsubscribes_all_owned_topics(tmp_path, dispatcher):
    p = _write_config_tree(tmp_path)
    runner = ScenarioRunner(action_dispatcher=dispatcher)
    runner.load(p)
    unsub_mock = MagicMock()
    runner.start(MagicMock(), MagicMock(), unsub_mock)
    runner.stop()
    unsubscribed = {c.args[0] for c in unsub_mock.call_args_list}
    expected = {
        scenario_topics.step.req, scenario_topics.seek.req,
        scenario_topics.pause.req, scenario_topics.resume.req,
        scenario_topics.abort.req,
    }
    assert unsubscribed == expected


# ---------------------------------------------------------------------------
# Race regression: run a 20-step scenario 20x consecutively; never hang,
# never miss a step. Hits the deferred-post_fifo + cancel_event path
# repeatedly.
# ---------------------------------------------------------------------------

def test_race_regression_twenty_by_twenty(tmp_path, dispatcher, data_dispatcher):
    timeline = [
        {"at": f"{5 + i * 3}ms",
         "action": "data.force_serv_state",
         "args": {"networkRat": "WCDMA"}}
        for i in range(20)
    ]
    p = _write_config_tree(tmp_path, timeline=timeline)

    for run_ix in range(20):
        data_dispatcher.dispatch_action.reset_mock()
        pub = _MockPublish()
        runner = ScenarioRunner(action_dispatcher=dispatcher)
        runner.load(p)
        runner.start(pub, MagicMock(), MagicMock())
        try:
            _wait_for_state(runner, "smfn_done", timeout_s=3.0)
        finally:
            runner.stop()
        assert data_dispatcher.dispatch_action.call_count == 20, (
            f"run {run_ix}: expected 20 dispatches, got "
            f"{data_dispatcher.dispatch_action.call_count}"
        )
