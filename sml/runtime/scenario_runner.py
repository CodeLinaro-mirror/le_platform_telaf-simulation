# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Scenario Runner: miros Active Object playing a scenario timeline.

Loads a ``sml/config/scenarios/*.yaml`` via :mod:`sml.runtime.loader`
(yaml -> pydantic validate + cross-reference resolution), applies
``initial_state`` at load time (invariant b: World State is initialized
only via ``scenario.initial_state``), and drives the ``timeline`` from
its own AO dispatch thread using miros' deferred ``post_fifo`` -- no
wall-clock timer objects, no reentrant mutex.

Timeline actions and manual ``ctrl/cmd/action/**`` injections converge
on the same :class:`~sml.runtime.action_dispatcher.ActionDispatcher`
(invariant e: one action-dispatch path).

State graph (all handlers `smfn_*` below)::

    top
    ├── smfn_idle       -- initial state; Start -> running (or done if empty)
    ├── smfn_running    -- schedules next StepDue as a deferred post_fifo
    ├── smfn_paused     -- cancel_event() on entry; Resume -> running
    └── smfn_done       -- terminal

Registers as an ``MqttClient`` subsystem: ``start / stop / handle_message
/ owns_topic``. ``handle_message`` is fire-and-forget (posts a
``MessageReceived`` event and returns True/False for topic ownership
only -- it cannot report whether the payload was actually valid or
handled; that decision runs later on the AO thread).
"""
from __future__ import annotations

import json
import logging
import re
import uuid as _uuid
from pathlib import Path
from typing import Callable, Optional

import jsonschema
from miros import Event, ActiveObject, return_status, signals, spy_on

from sml.mpss import instrumentation as _instr
from sml.runtime.action_dispatcher import ActionDispatcher
from sml.config.models import DevicesDoc
from sml.runtime.loader import (
    load_devices_doc, load_environments_doc, load_scenario_doc, resolve_initial_state,
)
from sml.runtime.world_state import ModemRuntime, RadioRuntime, SimSlotRuntime
from generated.python.ctrl_topics import scenario as scenario_topics
from generated.python.ctrl_validators import validate as validate_test_payload

_log = logging.getLogger("sml.runtime.scenario_runner")

_AT_RE = re.compile(r"^\d+(\.\d+)?(ms|s|m)$")
_AT_UNIT_SCALE = {"ms": 1.0, "s": 1000.0, "m": 60_000.0}


# Simulator self-introspection topic: hardcoded, not defined by any
# registry. Retained so a late-attaching observer sees current progress
# immediately on subscribe.
TOPIC_SCENARIO_PROGRESS = "mp/sys/0/state/config/scenario_progress"

_OWNED_TOPICS = frozenset({
    scenario_topics.step.req, scenario_topics.seek.req,
    scenario_topics.pause.req, scenario_topics.resume.req, scenario_topics.abort.req,
})

def _parse_at(at: str) -> float:
    """Parse a timeline/seek ``at`` string (e.g. ``"10s"``, ``"500ms"``) to ms."""
    if not _AT_RE.match(at):
        raise ValueError(f"`at` value {at!r} doesn't match ^\\d+(\\.\\d+)?(ms|s|m)$")
    num_match = re.match(r"^\d+(\.\d+)?", at)
    value = float(num_match.group())
    unit = at[len(num_match.group()):]
    return value * _AT_UNIT_SCALE[unit]


class ScenarioRunner(ActiveObject):
    """Loads one scenario file, applies ``initial_state``, plays its timeline.

    Usage::

        dispatcher = ActionDispatcher(domains={"data": data_subsystem})
        runner = ScenarioRunner(action_dispatcher=dispatcher)
        runner.load(scenario_path)          # applies initial_state right away
        client.register_subsystem(runner)   # start() begins autoplay
    """

    def __init__(self, action_dispatcher: ActionDispatcher,
                 name: str = "ScenarioRunner") -> None:
        super().__init__(name)
        self._action_dispatcher = action_dispatcher
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None

        self._scenario_name = ""
        self._timeline: list[dict] = []  # sorted [{at_ms, action, args}]
        self._next_idx = 0
        self._current_time_ms = 0.0
        self._aborted = False
        # post_fifo() returns the timer thread's name, which miros sets to a
        # uuid.UUID object (not a str).
        self._step_due_uuid: Optional[_uuid.UUID] = None
        self._started = False

        self.modem_runtimes: dict[str, ModemRuntime] = {}
        self.sim_slot_runtimes: dict[str, SimSlotRuntime] = {}
        self.radio_runtime: Optional[RadioRuntime] = None
        self.persistent: list[str] = []
        self.devices: Optional[DevicesDoc] = None

        self.start_at(smfn_idle)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Loading
    # ------------------------------------------------------------------

    def load(self, path: Path | str) -> None:
        """Parse ``path``, apply ``initial_state`` immediately, stage ``timeline``.

        Does not start autoplay -- call before registering as an
        ``MqttClient`` subsystem; ``start()`` begins playback.
        """
        p = Path(path)
        scenario = load_scenario_doc(p)

        config_root = p.resolve().parent.parent  # scenarios/../ -> config/
        devices = load_devices_doc(config_root / scenario.setup.devices_config)
        environments = load_environments_doc(config_root / scenario.setup.environment_config)

        self.modem_runtimes, self.sim_slot_runtimes, self.radio_runtime = resolve_initial_state(
            scenario, devices, environments
        )
        self.persistent = devices.persistent
        self.devices = devices

        self._scenario_name = scenario.name
        self._timeline = self._stage_timeline(scenario.timeline)
        self._next_idx = 0
        self._current_time_ms = 0.0
        _log.info("scenario %r loaded (%d timeline step(s))",
                  self._scenario_name, len(self._timeline))

    def _stage_timeline(self, timeline: list) -> list[dict]:
        steps = [
            {"at_ms": _parse_at(step.at), "action": step.action, "args": step.args}
            for step in timeline
        ]
        steps.sort(key=lambda s: s["at_ms"])
        return steps

    # ------------------------------------------------------------------
    # MqttClient subsystem protocol
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Optional[Callable] = None,
              direct_publish_fn: Optional[Callable] = None) -> None:
        """Wire up broker fns, subscribe, post Start event."""
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in _OWNED_TOPICS:
            subscribe_fn(topic)
        self._started = True
        self.post_fifo(Event(signal=signals.Start))
        _log.info("scenario runner started (%r)", self._scenario_name)

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained progress.

        Called by :class:`~sml.mpss.mqtt_client.MqttClient` on every entry into
        Operational after the first one. The runner keeps running across the
        flap -- the timeline is wall-clock driven and a broker outage is not a
        scenario abort, so no timer is cancelled or rescheduled here; only what
        the broker forgot is rebuilt.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def stop(self) -> None:
        """Signal the AO to stop via the HSM (walks EXIT actions) and
        unsubscribe owned topics.

        Idempotent: safe to call before/after ``start()``.
        """
        if self._started:
            self.post_fifo(Event(signal=signals.Stop))
            self._started = False
        if self._unsubscribe_fn:
            for topic in _OWNED_TOPICS:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def owns_topic(self, topic: str) -> bool:
        """Synchronous, lock-free -- reads a module-level ``frozenset``."""
        return topic in _OWNED_TOPICS

    def handle_message(self, topic: str, payload: bytes) -> bool:
        """Fire-and-forget: post a ``MessageReceived`` event onto the AO.

        Returns True iff we own ``topic`` (payload accepted for async
        processing). Since dispatch happens on the AO thread later, the
        return value **cannot** report whether the payload validated or
        the RPC ultimately produced a response -- callers must poll for
        the response topic.
        """
        if topic not in _OWNED_TOPICS:
            return False
        self.post_fifo(
            Event(signal=signals.MessageReceived, payload=(topic, payload))
        )
        return True

    # ------------------------------------------------------------------
    # AO-thread helpers -- called from state handlers only
    # ------------------------------------------------------------------

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish retained progress.

        The retained ``scenario/progress`` has to be re-sent in case the broker
        restarted rather than merely dropping our session. The label is derived
        from the live state so a resubscribe never mis-reports the runner as
        paused while it is mid-timeline.
        """
        if self._subscribe_fn is None:
            return
        for topic in _OWNED_TOPICS:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        if self._aborted:
            label = "aborted"
        elif self.state_fn.__name__ == "smfn_running":
            label = "running"
        elif self.state_fn.__name__ == "smfn_done":
            label = "done"
        else:
            label = "paused"
        self._publish_progress(label)
        _log.info("scenario runner resubscribed (%r)", self._scenario_name)

    def _dispatch_one(self, step: dict) -> None:
        # Invariant e: same ActionDispatcher path as manual ctrl/cmd/action.
        if not self._action_dispatcher.dispatch_action(step["action"], step["args"]):
            _log.warning("scenario runner: action %r not handled by any domain",
                         step["action"])

    def _schedule_next_step_due(self) -> None:
        """Post a deferred ``StepDue`` for the next timeline entry.

        No-op past the last step. Cancels any pending StepDue first so
        we never queue two.
        """
        self._cancel_step_due()
        if self._next_idx >= len(self._timeline):
            return
        remaining_ms = self._timeline[self._next_idx]["at_ms"] - self._current_time_ms
        delay_s = max(0.0, remaining_ms / 1000.0)
        self._step_due_uuid = self.post_fifo(
            Event(signal=signals.StepDue),
            times=1, period=delay_s, deferred=True,
        )

    def _cancel_step_due(self) -> None:
        """Release the pending (or already-fired) StepDue timer slot.

        miros allocates a ``posted_events_queue`` entry (deque, maxlen 500)
        per deferred post_fifo and never reclaims it when the timer thread
        fires and exits -- only cancel_event() pops it. So this must be
        called even after StepDue has been received, or we leak one slot per
        executed timeline step and eventually hit
        ActiveObjectOutOfPostedEventResources.
        """
        if self._step_due_uuid is None:
            return
        try:
            self.cancel_event(self._step_due_uuid)
        except Exception:  # noqa: BLE001 - already fired / unknown uuid
            pass
        self._step_due_uuid = None

    def _do_step_sync(self, n: int) -> tuple[int, bool]:
        """Consume up to ``n`` timeline steps immediately (AO-thread).

        Cancels the pending StepDue; caller (state handler) reschedules
        via a fresh entry into ``smfn_running`` if still running.
        """
        self._cancel_step_due()
        executed = 0
        while executed < n and self._next_idx < len(self._timeline):
            s = self._timeline[self._next_idx]
            self._current_time_ms = s["at_ms"]
            self._dispatch_one(s)
            self._next_idx += 1
            executed += 1
        return executed, self._next_idx >= len(self._timeline)

    def _do_seek_sync(self, at: str) -> float:
        """Advance the virtual clock to ``at``, firing every intermediate step.

        Seeking backward is a no-op (fired actions cannot be un-fired).
        """
        self._cancel_step_due()
        target_ms = _parse_at(at)
        if target_ms >= self._current_time_ms:
            while (self._next_idx < len(self._timeline)
                   and self._timeline[self._next_idx]["at_ms"] <= target_ms):
                self._dispatch_one(self._timeline[self._next_idx])
                self._next_idx += 1
            self._current_time_ms = target_ms
        return self._current_time_ms

    # ------------------------------------------------------------------
    # ctrl/cmd/scenario/** message dispatch -- AO-thread
    # ------------------------------------------------------------------

    def _on_message(self, envelope: tuple) -> None:
        topic, payload = envelope
        try:
            data = json.loads(payload.decode("utf-8")) if payload else {}
        except Exception:
            _log.warning("scenario runner: bad JSON on %s; dropping", topic)
            return

        if topic == scenario_topics.step.req:
            self._rpc_step(data)
        elif topic == scenario_topics.seek.req:
            self._rpc_seek(data)
        elif topic == scenario_topics.pause.req:
            if self._validate("scenario.pause.req", data):
                self.post_fifo(Event(signal=signals.Pause))
        elif topic == scenario_topics.resume.req:
            if self._validate("scenario.resume.req", data):
                self.post_fifo(Event(signal=signals.Resume))
        elif topic == scenario_topics.abort.req:
            if self._validate("scenario.abort.req", data):
                self._aborted = True
                self.post_fifo(Event(signal=signals.Stop))

    def _rpc_step(self, data: dict) -> None:
        if not self._validate("scenario.step.req", data):
            return
        executed, reached_end = self._do_step_sync(data.get("steps", 1))
        self._publish_rsp(scenario_topics.step.rsp, "scenario.step.rsp", {
            "executed_steps": executed,
            "current_time_ms": int(self._current_time_ms),
            "reached_end": reached_end,
        })
        if reached_end:
            # Natural completion via RPC: transition to done.
            self.post_fifo(Event(signal=signals.Stop))
        else:
            # Re-arm the timer if the current state auto-plays.
            if self.state_fn.__name__ == "smfn_running":
                self._schedule_next_step_due()

    def _rpc_seek(self, data: dict) -> None:
        if not self._validate("scenario.seek.req", data):
            return
        reached_at_ms = self._do_seek_sync(data["at"])
        self._publish_rsp(scenario_topics.seek.rsp, "scenario.seek.rsp",
                          {"reached_at_ms": int(reached_at_ms)})
        if self._next_idx >= len(self._timeline):
            self.post_fifo(Event(signal=signals.Stop))
        elif self.state_fn.__name__ == "smfn_running":
            self._schedule_next_step_due()

    def _validate(self, schema_id: str, data: dict) -> bool:
        try:
            validate_test_payload(schema_id, data)
        except jsonschema.ValidationError as exc:
            _log.warning("scenario runner: %s invalid: %s; dropping", schema_id, exc)
            return False
        return True

    def _publish_rsp(self, topic: str, schema_id: str, payload: dict) -> None:
        validate_test_payload(schema_id, payload)
        if self._publish_fn:
            self._publish_fn(topic, json.dumps(payload).encode(), 1, False)

    # ------------------------------------------------------------------
    # Progress introspection
    # ------------------------------------------------------------------

    def _publish_progress(self, state_label: str) -> None:
        if not self._publish_fn:
            return
        payload = {
            "scenario_name": self._scenario_name,
            "current_time_ms": int(self._current_time_ms),
            "state": state_label,
            "reached_end": self._next_idx >= len(self._timeline),
        }
        self._publish_fn(TOPIC_SCENARIO_PROGRESS,
                         json.dumps(payload).encode(), 1, True)


# ---------------------------------------------------------------------------
# HSM state handlers.
#
# Each `smfn_*` is a miros state handler with signature
# `(chart, e) -> return_status`. `chart` is the ScenarioRunner instance.
# ---------------------------------------------------------------------------


@spy_on
def smfn_idle(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._publish_progress("paused")
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        if not chart._timeline or chart._next_idx >= len(chart._timeline):
            status = chart.trans(smfn_done)
        else:
            status = chart.trans(smfn_running)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_done)
    elif e.signal == signals.MessageReceived:
        chart._on_message(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_running(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._schedule_next_step_due()
        chart._publish_progress("running")
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._cancel_step_due()
        status = return_status.HANDLED
    elif e.signal == signals.StepDue:
        # Reclaim the fired timer's slot in miros' posted_events_queue; a bare
        # `_step_due_uuid = None` would leak it.
        chart._cancel_step_due()
        if chart._next_idx >= len(chart._timeline):
            status = chart.trans(smfn_done)
        else:
            s = chart._timeline[chart._next_idx]
            chart._current_time_ms = s["at_ms"]
            chart._dispatch_one(s)
            chart._next_idx += 1
            chart._publish_progress("running")
            if chart._next_idx >= len(chart._timeline):
                status = chart.trans(smfn_done)
            else:
                chart._schedule_next_step_due()
                status = return_status.HANDLED
    elif e.signal == signals.Pause:
        status = chart.trans(smfn_paused)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_done)
    elif e.signal == signals.MessageReceived:
        chart._on_message(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_paused(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._cancel_step_due()
        chart._publish_progress("aborted" if chart._aborted else "paused")
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Resume:
        if chart._aborted:
            status = return_status.HANDLED  # aborted resume is a no-op
        elif chart._next_idx >= len(chart._timeline):
            status = chart.trans(smfn_done)
        else:
            status = chart.trans(smfn_running)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_done)
    elif e.signal == signals.MessageReceived:
        chart._on_message(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_done(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._cancel_step_due()
        chart._publish_progress("aborted" if chart._aborted else "done")
        chart.stop()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        # Late-arriving RPC on a finished scenario is dropped silently.
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Ready-entry already called chart.stop(), so this normally never
        # arrives; drop it rather than re-subscribing a dead runner.
        _log.debug("scenario runner: Resubscribe dropped -- scenario finished")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["ScenarioRunner"]
