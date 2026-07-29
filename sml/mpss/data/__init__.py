# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS data sub-package.

Provides :class:`DataSubsystem`, which implements the MPSS-side of the
WireSchema v1 MQTT contract for data call, profile, throttle, and serving
system services.

Usage (from ``sml/mpss/__main__.py``)::

    ds = DataSubsystem(slot_id=target_slot_runtime.sim_slot.slot_id,
                       seed_profiles=runner.resolved_seed_profiles,
                       interface_preset=runner.resolved_interface_preset,
                       call_timing_preset=runner.resolved_call_timing_preset,
                       ip_config=runner.resolved_ip_config,
                       role=_read_whoami())
    client.register_subsystem(ds)
    client.start()
"""
from __future__ import annotations

import logging
import os
from pathlib import Path
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.config.models import CallTimingPresetSeed, InterfacePresetSeed, IpConfigSeed
from sml.mpss import instrumentation as _instr

_log = logging.getLogger("sml.mpss.data")


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


class DataSubsystem(ActiveObject):
    """Coordinates all MPSS-side data Active Objects for one or more slots.

    Off → Ready → Stopping -- `Ready`'s entry creates and starts the three
    sub-AOs + Action Dispatcher (what used to be gated by `self._started:
    bool`); exit stops them symmetrically, so there's no separate
    `if self._started` guard against a double start/stop -- being outside
    `Ready` already means "not started" (see `start`/`stop` below, which
    just dispatch signals and let the chart's own topology enforce the
    idempotency the plain-class predecessor implemented by hand).

    Lifecycle (called by :class:`~sml.mpss.mqtt_client.MqttClient`)::

        ds.start(publish_fn, subscribe_fn, unsubscribe_fn)
        # ... messages dispatched via ds.handle_message(topic, payload) ...
        ds.stop()
    """

    def __init__(
        self,
        slot_id: int = 1,
        seed_profiles: Optional[list] = None,
        interface_preset: Optional[InterfacePresetSeed] = None,
        call_timing_preset: Optional[CallTimingPresetSeed] = None,
        ip_config: Optional[IpConfigSeed] = None,
        role: Optional[str] = None,
        persist_path: Optional[Path] = None,
        bitrate_by_rat: Optional[dict] = None,
        throughput_presets: Optional[dict] = None,
        qos_presets: Optional[dict] = None,
        throttle_presets: Optional[dict] = None,
    ) -> None:
        super().__init__("DataSubsystem")
        self._slot_id = slot_id
        self._seed_profiles = seed_profiles or []
        self._interface_preset = interface_preset or InterfacePresetSeed()
        self._call_timing_preset = call_timing_preset or CallTimingPresetSeed()
        self._ip_config = ip_config or IpConfigSeed()
        self._role = role or _read_whoami()
        self._persist_path = persist_path
        self._bitrate_by_rat = bitrate_by_rat or {}
        self._throughput_presets = throughput_presets or {}
        self._qos_presets = qos_presets or {}
        self._throttle_presets = throttle_presets or {}

        self._pending_start_args: Optional[tuple] = None
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None  # bypasses AO event queue

        self._serving_system_ao = None
        self._profile_ao = None
        self._connection_ao = None
        self._action_dispatcher = None

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
        """Re-establish this subsystem's broker state after an MQTT reconnect.

        Called by :class:`~sml.mpss.mqtt_client.MqttClient` on every entry into
        Operational after the first one. Fans out to each sub-AO's own
        ``resubscribe`` (see ``_fanout_resubscribe``); nothing is stopped or
        recreated -- a subsystem AO's lifetime is the process lifetime, because
        ``stop()`` joins its dispatch thread and a joined AO cannot be revived.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def handle_message(self, topic: str, payload: bytes) -> None:
        """Route an inbound MQTT message to the appropriate sub-AO.

        Fire-and-forget: posts a ``MessageReceived`` event into
        this AO's fifo and returns immediately.  The old ``bool`` return
        is gone -- callers (``MqttClient._on_message``) now consult
        ``owns_topic`` before posting.
        """
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    def owns_topic(self, topic: str) -> bool:
        """Return True if any sub-AO or the action-dispatcher owns
        this topic.  Called (thread-safely, lock-free -- only frozen
        sets are read) by ``MqttClient`` before it posts a message."""
        for ao in (self._profile_ao, self._connection_ao,
                   self._serving_system_ao, self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                return True
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        """Proxy to the data-domain Action Dispatcher's own dispatch_action.

        Lets a :class:`~sml.runtime.scenario_runner.ScenarioRunner` hold a
        reference to this :class:`DataSubsystem` at construction time (before
        ``start()`` has created ``_action_dispatcher``) -- register
        DataSubsystem before ScenarioRunner with MqttClient so this isn't
        called before ``start()`` has run.
        """
        if self._action_dispatcher is None:
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.mpss.data.serving_system import DataServingSystemAO
        from sml.mpss.data.profile import DataProfileAO
        from sml.mpss.data.connection import DataConnectionAO
        from sml.mpss.data.action_dispatcher import DataActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        # direct_publish_fn bypasses the AO event queue; used during shutdown
        # so ready=false is sent synchronously before paho disconnects.
        self._direct_publish_fn = direct_publish_fn or publish_fn

        mpss_src = f"mpss-{self._role}-{os.getpid()}"

        self._connection_ao = DataConnectionAO(
            slot=self._slot_id,
            interface_preset=self._interface_preset,
            call_timing_preset=self._call_timing_preset,
            ip_config=self._ip_config,
            mpss_src=mpss_src,
            bitrate_by_rat=self._bitrate_by_rat,
            throughput_presets=self._throughput_presets,
            qos_presets=self._qos_presets,
            throttle_presets=self._throttle_presets,
            get_network_rat_fn=lambda: self._serving_system_ao.network_rat,
        )
        self._serving_system_ao = DataServingSystemAO(
            slot=self._slot_id, mpss_src=mpss_src,
            on_rat_changed=self._connection_ao.on_network_rat_changed,
        )
        self._profile_ao = DataProfileAO(
            slot=self._slot_id,
            seed_profiles=self._seed_profiles,
            mpss_src=mpss_src,
            persist_path=self._persist_path,
        )

        self._serving_system_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)
        self._profile_ao.start(publish_fn, subscribe_fn)
        self._connection_ao.start(publish_fn, subscribe_fn)

        self._action_dispatcher = DataActionDispatcher(
            serving_system_ao=self._serving_system_ao,
            connection_ao=self._connection_ao,
        )
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("DataSubsystem started (slot=%d, src=%s)", self._slot_id, mpss_src)

    def _fanout_message(self, topic: str, payload: bytes) -> None:
        """Route a MessageReceived event to the sub-AO that owns the
        topic.  Each sub-AO's ``handle_message`` is fire-and-forget;
        it posts into its own fifo and returns."""
        for ao in (self._profile_ao, self._connection_ao,
                   self._serving_system_ao, self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                try:
                    ao.handle_message(topic, payload)
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO handler raised on %s: %s", topic, exc)
                return
        _log.debug("DataSubsystem: no sub-AO owns %s", topic)

    def _fanout_resubscribe(self) -> None:
        """Re-issue subscriptions on every sub-AO, in start order.

        Each child's ``resubscribe`` only posts an event into its own fifo (the
        action dispatcher, a plain class, runs inline), so this returns without
        blocking on any of them.
        """
        for ao in (self._serving_system_ao, self._profile_ao,
                   self._connection_ao, self._action_dispatcher):
            if ao is None:
                continue
            fn = getattr(ao, "resubscribe", None)
            if fn is None:
                _log.warning("sub-AO %s has no resubscribe(); its subscriptions "
                             "are not restored", type(ao).__name__)
                continue
            try:
                fn()
            except Exception as exc:  # noqa: BLE001
                _log.error("sub-AO resubscribe failed: %s", exc)

    def _do_stop(self) -> None:
        if self._serving_system_ao is not None and self._direct_publish_fn:
            # Switch to direct (synchronous) publish so the tel-ready=false
            # published from DataServingSystemAO's own Ready-exit hook
            # reaches the broker before paho disconnects (the async
            # event-queue path would be processed after the AO has already
            # left Operational).
            self._serving_system_ao._publish_fn = self._direct_publish_fn
        for ao in (self._action_dispatcher, self._connection_ao, self._profile_ao, self._serving_system_ao):
            if ao is not None:
                try:
                    ao.stop()
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO stop failed: %s", exc)
        _log.info("DataSubsystem stopped")


# ---------------------------------------------------------------------------
# HSM state handlers.
#
# HSM state handlers.
#
# DataSubsystem: Operating -> Starting -> Ready,
# matching the sub-AOs it manages.
#
#     top
#     |-- smfn_off
#     |-- smfn_operating
#     |     INIT -> smfn_starting
#     |     SIG_STOP -> smfn_stopping
#     |     SIG_MESSAGE_RECEIVED: defer
#     |     |-- smfn_starting
#     |     |     ENTRY: _do_start(); post_fifo(StartingDone)
#     |     |     SIG_STARTING_DONE -> smfn_ready
#     |     |-- smfn_ready
#     |           ENTRY: recall_all()
#     |           SIG_MESSAGE_RECEIVED: _fanout_message()
#     `-- smfn_stopping
# ---------------------------------------------------------------------------

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_operating(chart, e):
    """Composite parent: defers MessageReceived until Ready."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Starting subscribes every sub-AO anyway; a reconnect racing the
        # initial start needs nothing extra.
        _log.debug("DataSubsystem: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    """Spins up all sub-AOs on entry; posts StartingDone for the sibling
    trans to Ready."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_ready)
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    """Fans out inbound messages to whichever sub-AO owns the topic."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._fanout_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._fanout_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_stopping(chart, e):
    """Terminal: stops every sub-AO on entry. No path back to Operating."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        chart.stop()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["DataSubsystem"]
