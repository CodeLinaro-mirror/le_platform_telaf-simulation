# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""APSS power sub-package.

Provides :class:`PowerSubsystem`, the apss-side of the WireSchema v1 power
domain contract (`ap/req/power/**`, `ap/ff/power/**`, `ap/ind/power/**`) plus
the wakeup relay (`mp/ind/wakeup/event` -> `ap/ind/power/wakeup`).

Usage (from ``sml/apss/__main__.py``)::

    ps = PowerSubsystem(local_machine_name="mdm", role=_read_whoami())
    client.register_subsystem(ps)
    client.start()
"""
from __future__ import annotations

import logging
import os
from collections import deque
from dataclasses import dataclass
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr
from sml.common.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import power as topics_power
from generated.python.topics import wakeup as topics_wakeup
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.apss.power")

ALL_MACHINES = "ALL_MACHINES"
LOCAL_MACHINE = "LOCAL_MACHINE"

_TELAF_CGROUP_FREEZE_PATH = "/sys/fs/cgroup/telaf/cgroup.freeze"


def _read_whoami() -> str:
    home = os.environ.get("HOME", "")
    if not home:
        return "dev"
    try:
        with open(os.path.join(home, ".whoami"), encoding="utf-8") as fh:
            role = fh.read().strip()
            return role if role else "dev"
    except OSError:
        return "dev"


@dataclass(frozen=True)
class _QueuedRequest:
    """One queued `power.set_state.req` (invariant e: manual and timeline
    actions share this same queue -- there is no separate fast path)."""

    machine_name: str
    state: str
    corr_id: str
    src: str


class PowerSubsystem(ActiveObject):
    """Coordinates the apss-side power/TCU-activity domain and the wakeup relay.
    """

    def __init__(
        self,
        local_machine_name: str = "mdm",
        role: Optional[str] = None,
        timeout_ms: int = 5000,
        ftimeout_ms: int = 36_000_000,
        shutdown_trigger_en: bool = True,
        interconnect_supports_autosuspend: bool = False,
    ) -> None:
        super().__init__("PowerSubsystem")
        self._local_machine_name = local_machine_name
        self._role = role or _read_whoami()
        self._timeout_ms = timeout_ms
        self._ftimeout_ms = ftimeout_ms
        self._shutdown_trigger_en = shutdown_trigger_en
        self._interconnect_supports_autosuspend = interconnect_supports_autosuspend

        self._machines_state: dict[str, str] = {
            local_machine_name: "RESUME",
            ALL_MACHINES: "RESUME",
        }
        self._registered_machines: set[str] = {local_machine_name}
        self._request_queue: deque[_QueuedRequest] = deque()

        self._pending_target: Optional[str] = None   # resolved machine name or ALL_MACHINES
        self._pending_state: Optional[str] = None
        self._pending_expected: set[str] = set()
        self._pending_acked: set[str] = set()
        self._pending_nacked: set[str] = set()
        self._pending_corr: Optional[dict] = None     # {corrId, src} of the set_state req
        self._pending_timer_uuid = None
        self._ftimeout_timer_uuid = None

        self._pending_start_args: Optional[tuple] = None
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None
        self._action_dispatcher = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (called from MqttClient)
    # ------------------------------------------------------------------

    def start(
        self,
        publish_fn: Callable,
        subscribe_fn: Callable,
        unsubscribe_fn: Callable,
        direct_publish_fn: Optional[Callable] = None,
    ) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        self.post_fifo(Event(signal=signals.Resubscribe))

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

    def owns_topic(self, topic: str) -> bool:
        if topic in self._owned_topics:
            return True
        if self._action_dispatcher is not None and self._action_dispatcher.owns_topic(topic):
            return True
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        if self._action_dispatcher is None:
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.apss.power.action_dispatcher import PowerActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._direct_publish_fn = direct_publish_fn or publish_fn

        self._apss_src = f"apss-{self._role}-{os.getpid()}"

        mapping = {
            topics_power.set_state.req: self._handle_set_state,
            topics_power.get_machine_names.req: self._handle_get_machine_names,
            topics_power.slave_ack.req: self._handle_slave_ack,  # ff topic, misleadingly-named attr
            topics_wakeup.event.ind: self._handle_wakeup_event,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)

        self._action_dispatcher = PowerActionDispatcher(self)
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("PowerSubsystem started (local_machine=%s, src=%s)",
                  self._local_machine_name, self._apss_src)

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        if self._action_dispatcher is not None:
            self._action_dispatcher.resubscribe()
        self._publish_subsys_ready("tcu", ready=True)
        self._publish_subsys_ready("wakeup", ready=True)
        _log.info("PowerSubsystem resubscribed")

    def _do_stop(self) -> None:
        self._cancel_pending_timer()
        self._cancel_ftimeout_timer()
        if self._direct_publish_fn:
            self._publish_fn = self._direct_publish_fn
        try:
            self._publish_subsys_ready("tcu", ready=False)
            self._publish_subsys_ready("wakeup", ready=False)
        except Exception as exc:  # noqa: BLE001
            _log.warning("failed to publish power ready=false on stop: %s", exc)
        if self._action_dispatcher is not None:
            try:
                self._action_dispatcher.stop()
            except Exception as exc:  # noqa: BLE001
                _log.error("power action dispatcher stop failed: %s", exc)
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)
        _log.info("PowerSubsystem stopped")

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        if self._action_dispatcher is not None and self._action_dispatcher.owns_topic(topic):
            self._action_dispatcher.handle_message(topic, payload)
            return
        dispatch_inbound(topic, payload, self._handlers, _log, "power AO")

    # ------------------------------------------------------------------
    # Timer helpers -- post_fifo(..., deferred=True); no real threads
    # (per invariant h and plan.md's explicit "miros deferred timers" note).
    # ------------------------------------------------------------------

    def _schedule_ack_timeout(self) -> None:
        self._cancel_pending_timer()
        delay_s = self._timeout_ms / 1000.0
        self._pending_timer_uuid = self.post_fifo(
            Event(signal=signals.PowerAckTimeout),
            times=1, period=delay_s, deferred=True,
        )

    def _cancel_pending_timer(self) -> None:
        if self._pending_timer_uuid is not None:
            try:
                self.cancel_event(uuid=self._pending_timer_uuid)
            except Exception as exc:  # noqa: BLE001
                _log.debug("cancel_event(%s) raised: %s", self._pending_timer_uuid, exc)
            self._pending_timer_uuid = None

    def _schedule_ftimeout_retry(self) -> None:
        self._cancel_ftimeout_timer()
        delay_s = self._ftimeout_ms / 1000.0
        self._ftimeout_timer_uuid = self.post_fifo(
            Event(signal=signals.PowerFTimeout),
            times=1, period=delay_s, deferred=True,
        )

    def _cancel_ftimeout_timer(self) -> None:
        if self._ftimeout_timer_uuid is not None:
            try:
                self.cancel_event(uuid=self._ftimeout_timer_uuid)
            except Exception as exc:  # noqa: BLE001
                _log.debug("cancel_event(%s) raised: %s", self._ftimeout_timer_uuid, exc)
            self._ftimeout_timer_uuid = None

    # ------------------------------------------------------------------
    # RPC / ff handlers
    # ------------------------------------------------------------------

    def _resolve_targets(self, machine_name: str) -> list[str]:
        """Expand ALL_MACHINES/LOCAL_MACHINE sentinels to concrete names."""
        if machine_name == ALL_MACHINES:
            return sorted(self._registered_machines)
        if machine_name == LOCAL_MACHINE:
            return [self._local_machine_name]
        return [machine_name]

    def _handle_set_state(self, msg: dict) -> None:
        data = msg.get("data") or {}
        state = data.get("state")
        machine_name = data.get("machineName")
        corr_id, src = msg["corrId"], msg["src"]

        # SHUTDOWN is non-interruptible: while it is the in-flight head of
        # the queue, every new request is rejected at intake -- it never
        # gets a chance to queue behind it.
        if self._request_queue and self._request_queue[0].state == "SHUTDOWN":
            env = build_success_envelope(
                self._apss_src, corr_id, src,
                {"ok": False, "errorMessage": "shutdown in progress, cannot be interrupted"},
            )
            self._pub_rsp(topics_power.set_state.rsp, "power.set_state.rsp", env)
            return

        if not self._should_enqueue(machine_name, state):
            env = build_success_envelope(
                self._apss_src, corr_id, src,
                {"ok": False, "errorMessage": "duplicate of already-queued/current state for this target"},
            )
            self._pub_rsp(topics_power.set_state.rsp, "power.set_state.rsp", env)
            return

        was_empty = not self._request_queue
        self._request_queue.append(_QueuedRequest(machine_name, state, corr_id, src))
        if was_empty:
            self._process_next_queued()

    def _should_enqueue(self, machine_name: str, state: str) -> bool:
        targets = self._resolve_targets(machine_name)
        if not self._request_queue:
            return any(self._machines_state.get(t, "UNKNOWN") != state for t in targets)
        return any(self._last_queued_state(t) != state for t in targets)

    def _last_queued_state(self, target: str) -> str:
        for req in reversed(self._request_queue):
            if target in self._resolve_targets(req.machine_name):
                return req.state
        return self._machines_state.get(target, "UNKNOWN")

    def _process_next_queued(self) -> None:
        if not self._request_queue:
            return
        req = self._request_queue[0]
        self._pending_target = req.machine_name
        self._pending_state = req.state
        self._pending_corr = {"corrId": req.corr_id, "src": req.src}

        targets = self._resolve_targets(req.machine_name)
        changed = {t for t in targets if self._machines_state.get(t, "UNKNOWN") != req.state}

        for t in changed:
            self._pub_ind(topics_power.state_update.ind, "power.state_update.ind",
                          {"state": req.state, "machineName": t})

        if req.state == "RESUME":
            # RESUME is fire-and-forget: no ACK aggregation (SDK: not called
            # for transitioning to resumed state).
            self._pending_expected = set()
            self._pending_acked = set()
            self._pending_nacked = set()
            self._finish_set_state_round(status="COMPLETE")
            return

        # SUSPEND / SHUTDOWN: every changed target is awaited, including the
        # local machine -- it also runs its own slave listener and must ack
        # itself like any other registered machine.
        awaited = changed
        self._pending_expected = awaited
        self._pending_acked = set()
        self._pending_nacked = set()

        if not awaited:
            self._finish_set_state_round(status="COMPLETE")
            return

        self._schedule_ack_timeout()

    def _handle_get_machine_names(self, msg: dict) -> None:
        env = build_success_envelope(
            self._apss_src, msg["corrId"], msg["src"],
            {"machineNames": sorted(self._registered_machines)},
        )
        self._pub_rsp(topics_power.get_machine_names.rsp, "power.get_machine_names.rsp", env)

    def _handle_slave_ack(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self._record_ack(data.get("machineName"), data.get("response"))

    def _handle_wakeup_event(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self._pub_ind(topics_power.wakeup.ind, "power.wakeup.ind", {
            "wakeupType": "QMI",
            "qmiWakeupInfo": {
                "serviceId": data.get("serviceId"),
                "sourceNodeId": data.get("sourceNodeId"),
                "destinationNodeId": data.get("destinationNodeId"),
                "isMsgIdValid": data.get("isMsgIdValid"),
                "msgId": data.get("msgId"),
                "isPIDValid": data.get("isPIDValid"),
                "pid": data.get("pid"),
                "isProcessNameValid": data.get("isProcessNameValid"),
                "processName": data.get("processName"),
            },
        })

    # ------------------------------------------------------------------
    # ACK aggregation
    # ------------------------------------------------------------------

    def _record_ack(self, machine_name: str, response: str) -> None:
        if self._pending_target is None or machine_name not in self._pending_expected:
            return
        if response == "ACK":
            self._pending_acked.add(machine_name)
            if self._pending_acked >= self._pending_expected:
                self._cancel_pending_timer()
                self._finish_set_state_round(status="COMPLETE")
        else:
            self._pending_nacked.add(machine_name)
            # First NACK finalizes immediately -- stragglers are not
            # awaited, they land in `unresponsive` at finalize time.
            self._cancel_pending_timer()
            self._finish_set_state_round(status="COMPLETE")

    def _on_ack_timeout(self) -> None:
        if self._pending_target is None:
            return
        self._pending_timer_uuid = None
        self._finish_set_state_round(status="TIMEOUT")

    def _finish_set_state_round(self, status: str) -> None:
        machine_name = self._pending_target
        state = self._pending_state
        corr = self._pending_corr
        expected = self._pending_expected
        acked = self._pending_acked
        nacked = self._pending_nacked
        noack = expected - acked - nacked

        unresponsive = [{"clientName": m, "machineName": m} for m in sorted(noack)]
        nack_list = [{"clientName": m, "machineName": m} for m in sorted(nacked)]

        if expected:
            self._pub_ind(topics_power.slave_ack_status.ind, "power.slave_ack_status.ind", {
                "status": status,
                "machineName": machine_name,
                "unresponsive": unresponsive,
                "nack": nack_list,
            })

        ok = not noack and not nacked
        self._apply_state_update(machine_name, state)

        if corr is not None:
            if ok:
                env = build_success_envelope(self._apss_src, corr["corrId"], corr["src"], {"ok": True})
            else:
                env = build_success_envelope(
                    self._apss_src, corr["corrId"], corr["src"],
                    {"ok": False, "errorMessage": "one or more machines nacked or did not respond"},
                )
            self._pub_rsp(topics_power.set_state.rsp, "power.set_state.rsp", env)

        involves_local = (
            machine_name in (ALL_MACHINES, LOCAL_MACHINE)
            or machine_name == self._local_machine_name
        )
        self._pending_target = None
        self._pending_state = None
        self._pending_expected = set()
        self._pending_acked = set()
        self._pending_nacked = set()
        self._pending_corr = None

        if not ok and involves_local:
            self._schedule_ftimeout_retry()

        if self._request_queue:
            self._request_queue.popleft()
        if self._request_queue:
            self._process_next_queued()

    def _apply_state_update(self, machine_name: str, state: str) -> None:
        if machine_name == ALL_MACHINES:
            for m in self._machines_state:
                self._machines_state[m] = state
        else:
            for t in self._resolve_targets(machine_name):
                self._machines_state[t] = state

    def _retry_local_confirm(self) -> None:
        """ftimeout: re-confirm the local machine's own recorded state by
        rebroadcasting it, without re-querying remotes (QCPMD parity)."""
        state = self._machines_state.get(self._local_machine_name, "UNKNOWN")
        self._pub_ind(topics_power.state_update.ind, "power.state_update.ind",
                      {"state": state, "machineName": self._local_machine_name})

    # ------------------------------------------------------------------
    # Machine registry mutators (invoked by PowerActionDispatcher)
    # ------------------------------------------------------------------

    def add_machine(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.PowerAddMachine, payload=data))

    def remove_machine(self, data: dict) -> None:
        self.post_fifo(Event(signal=signals.PowerRemoveMachine, payload=data))

    def force_slave_ack(self, data: dict) -> None:
        """Inject an ACK/NACK into the in-flight round without a real message."""
        self.post_fifo(Event(signal=signals.PowerForceSlaveAck, payload=data))

    def suspend_system(self, data: dict) -> None:
        """Freeze the telaf cgroup, simulating the whole system going to sleep."""
        self.post_fifo(Event(signal=signals.PowerSuspendSystem, payload=data))

    def resume_system(self, data: dict) -> None:
        """Thaw the telaf cgroup, simulating a hardware wakeup out of sleep."""
        self.post_fifo(Event(signal=signals.PowerResumeSystem, payload=data))

    def _apply_add_machine(self, data: dict) -> None:
        machine_name = data["machineName"]
        if machine_name in self._registered_machines:
            return
        self._registered_machines.add(machine_name)
        self._machines_state.setdefault(machine_name, "RESUME")
        self._pub_ind(topics_power.machine_update.ind, "power.machine_update.ind",
                     {"machineName": machine_name, "machineEvent": "AVAILABLE"})

    def _apply_remove_machine(self, data: dict) -> None:
        machine_name = data["machineName"]
        if machine_name not in self._registered_machines:
            return
        self._registered_machines.discard(machine_name)
        self._pub_ind(topics_power.machine_update.ind, "power.machine_update.ind",
                     {"machineName": machine_name, "machineEvent": "UNAVAILABLE"})

    def _apply_force_slave_ack(self, data: dict) -> None:
        self._record_ack(data["machineName"], data["response"])

    def _apply_suspend_system(self, data: dict) -> None:
        self._write_cgroup_freeze("1")

    def _apply_resume_system(self, data: dict) -> None:
        self._write_cgroup_freeze("0")

    @staticmethod
    def _write_cgroup_freeze(value: str) -> None:
        try:
            with open(_TELAF_CGROUP_FREEZE_PATH, "w", encoding="utf-8") as fh:
                fh.write(value)
        except OSError as exc:
            _log.error("cgroup freeze write(%s, %s) failed: %s",
                       _TELAF_CGROUP_FREEZE_PATH, value, exc)

    # ------------------------------------------------------------------
    # Envelope publish helpers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, __import__("json").dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._apss_src, data)
        self._publish_fn(topic, __import__("json").dumps(env).encode(), 1, retain)

    def _publish_subsys_ready(self, which: str, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        if which == "tcu":
            topic = topics_power.subsys_ready_tcu.ind
            schema_id = "power.subsys_ready_tcu.ind"
        else:
            topic = topics_power.subsys_ready_wakeup.ind
            schema_id = "power.subsys_ready_wakeup.ind"
        self._pub_ind(topic, schema_id, {"ready": ready, "status": status}, retain=True)


# ---------------------------------------------------------------------------
# HSM state handlers -- Off -> Operating(Starting -> Ready) -> Stopping,
# ---------------------------------------------------------------------------

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.Start:
        status = chart.trans(smfn_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_operating(chart, e):
    """Composite parent: defers MessageReceived until Ready."""
    status = return_status.UNHANDLED
    if e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal in (signals.PowerAddMachine, signals.PowerRemoveMachine,
                      signals.PowerForceSlaveAck, signals.PowerSuspendSystem,
                      signals.PowerResumeSystem):
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal in (signals.PowerAckTimeout, signals.PowerFTimeout):
        _log.debug("PowerSubsystem: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("PowerSubsystem: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_ready)
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._publish_subsys_ready("tcu", ready=True)
        chart._publish_subsys_ready("wakeup", ready=True)
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.PowerAckTimeout:
        chart._on_ack_timeout()
        status = return_status.HANDLED
    elif e.signal == signals.PowerFTimeout:
        chart._ftimeout_timer_uuid = None
        chart._retry_local_confirm()
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    elif e.signal == signals.PowerAddMachine:
        chart._apply_add_machine(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.PowerRemoveMachine:
        chart._apply_remove_machine(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.PowerForceSlaveAck:
        chart._apply_force_slave_ack(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.PowerSuspendSystem:
        chart._apply_suspend_system(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.PowerResumeSystem:
        chart._apply_resume_system(e.payload)
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_stopping(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        chart.stop()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["PowerSubsystem"]
