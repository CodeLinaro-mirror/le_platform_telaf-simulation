# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS radio sub-package.

Provides :class:`RadioSubsystem`, the MPSS-side mirror of
`telaf-simulation/pa/telaf-pa-simula/component/tel/` -- the simulated
`telux::tel` radio surface (IPhoneManager/IPhone, tel::IServingSystemManager,
INetworkSelectionManager). Structurally identical to
:class:`sml.mpss.data.DataSubsystem` (Off -> Operating.{Starting,Ready} ->
Stopping, same three-line rationale in that module's docstring for why entry/
exit rather than a `_started: bool` guard).

Usage (from ``sml/mpss/__main__.py``)::

    rs = RadioSubsystem(slot_id=target_slot_runtime.sim_slot.slot_id,
                         role=_read_whoami(),
                         radio_seed=resolve_radio_seed(runner.radio_runtime))
    client.register_subsystem(rs)
    client.start()
"""
from __future__ import annotations

import logging
import os
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.mpss import instrumentation as _instr

from sml.config.models import RadioSeed

_log = logging.getLogger("sml.mpss.radio")


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


class RadioSubsystem(ActiveObject):
    """Coordinates all MPSS-side radio Active Objects for one slot.

    Lifecycle (called by :class:`~sml.mpss.mqtt_client.MqttClient`)::

        rs.start(publish_fn, subscribe_fn, unsubscribe_fn)
        # ... messages dispatched via rs.handle_message(topic, payload) ...
        rs.stop()
    """

    def __init__(self, slot_id: int = 1, role: Optional[str] = None,
                 radio_seed: Optional[RadioSeed] = None) -> None:
        super().__init__("RadioSubsystem")
        self._slot_id = slot_id
        self._role = role or _read_whoami()
        # From the active scenario's initial_state.radio block (see
        # sml/runtime/loader.py's resolve_radio_seed()); None boots
        # RadioPhoneAO/RadioServingSystemAO with their own built-in
        # defaults. RadioNetworkSelectionAO has no cell/signal state, so it
        # doesn't need this.
        self._radio_seed = radio_seed

        self._pending_start_args: Optional[tuple] = None
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None  # bypasses AO event queue

        self._phone_ao = None
        self._serving_system_ao = None
        self._network_selection_ao = None
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
        ``resubscribe``; nothing is stopped or recreated -- a subsystem AO's
        lifetime is the process lifetime, because ``stop()`` joins its dispatch
        thread and a joined AO cannot be revived.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def handle_message(self, topic: str, payload: bytes) -> None:
        """Route an inbound MQTT message to the appropriate sub-AO.

        Fire-and-forget: posts a ``MessageReceived`` event into this AO's
        fifo and returns immediately -- mirrors DataSubsystem.handle_message.
        """
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    def owns_topic(self, topic: str) -> bool:
        """Return True if any sub-AO owns this topic. Called (thread-safely,
        lock-free -- only frozen sets are read) by MqttClient before it
        posts a message."""
        for ao in (self._phone_ao, self._serving_system_ao, self._network_selection_ao,
                   self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                return True
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        """Proxy to the radio-domain Action Dispatcher's own dispatch_action.

        Mirrors DataSubsystem.dispatch_action -- lets a ScenarioRunner hold a
        reference to this RadioSubsystem at construction time (before
        ``start()`` has created ``_action_dispatcher``).
        """
        if self._action_dispatcher is None:
            _log.debug("RadioSubsystem dispatch_action(%s) dropped -- action dispatcher not started",
                       canonical_name)
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.mpss.radio.phone import RadioPhoneAO
        from sml.mpss.radio.serving_system import RadioServingSystemAO
        from sml.mpss.radio.network_selection import RadioNetworkSelectionAO
        from sml.mpss.radio.action_dispatcher import RadioActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._direct_publish_fn = direct_publish_fn or publish_fn

        mpss_src = f"mpss-{self._role}-{os.getpid()}"

        self._phone_ao = RadioPhoneAO(slot=self._slot_id, mpss_src=mpss_src,
                                       radio_seed=self._radio_seed)
        self._serving_system_ao = RadioServingSystemAO(slot=self._slot_id, mpss_src=mpss_src,
                                                         radio_seed=self._radio_seed)
        self._network_selection_ao = RadioNetworkSelectionAO(slot=self._slot_id, mpss_src=mpss_src)

        self._phone_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)
        self._serving_system_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)
        self._network_selection_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)

        self._action_dispatcher = RadioActionDispatcher(
            serving_system_ao=self._serving_system_ao,
            phone_ao=self._phone_ao,
        )
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("RadioSubsystem started (slot=%d, src=%s)", self._slot_id, mpss_src)

    def _fanout_message(self, topic: str, payload: bytes) -> None:
        for ao in (self._phone_ao, self._serving_system_ao, self._network_selection_ao,
                   self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                try:
                    ao.handle_message(topic, payload)
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO handler raised on %s: %s", topic, exc)
                return
        _log.debug("RadioSubsystem: no sub-AO owns %s", topic)

    def _fanout_resubscribe(self) -> None:
        """Re-issue subscriptions on every sub-AO, in start order.

        Each child's ``resubscribe`` only posts into its own fifo (the action
        dispatcher, a plain class, runs inline), so this returns without
        blocking on any of them.
        """
        for ao in (self._phone_ao, self._serving_system_ao,
                   self._network_selection_ao, self._action_dispatcher):
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
        # Switch every sub-AO to direct (synchronous) publish before
        # stopping it, so its own Ready-exit hook's subsys_ready(ready=False)
        # (see phone.py/serving_system.py/network_selection.py's
        # _do_exit_ready) reaches the broker before the MQTT client
        # disconnects -- mirrors DataSubsystem._do_stop's rationale. Each of
        # the three radio sub-AOs publishes its own retained subsys_ready
        # topic (radio.yaml: "each manager maps to an independently-booting
        # subsystem... rather than collapsing into one domain-wide signal"),
        # so all three need the swap, not just one. Without this, the async
        # event-queue path is processed after the AO has already left
        # Operational, leaving a stale retained ready=true in the broker's
        # cache after a restart -- any tooling querying readiness while MPSS
        # is actually down gets a false "available".
        if self._direct_publish_fn:
            for ao in (self._phone_ao, self._serving_system_ao, self._network_selection_ao):
                if ao is not None:
                    ao._publish_fn = self._direct_publish_fn
        for ao in (self._action_dispatcher, self._network_selection_ao,
                   self._serving_system_ao, self._phone_ao):
            if ao is not None:
                try:
                    ao.stop()
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO stop failed: %s", exc)
        _log.info("RadioSubsystem stopped")


# ---------------------------------------------------------------------------
# HSM state handlers -- identical topology to sml.mpss.data.DataSubsystem;
# see that module's docstring for the full rationale.
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
        _log.debug("RadioSubsystem: Resubscribe dropped -- not Ready")
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


__all__ = ["RadioSubsystem"]
