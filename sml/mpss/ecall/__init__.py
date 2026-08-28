# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS ecall sub-package.

Provides :class:`EcallSubsystem`, the MPSS-side mirror of
`telaf-simulation/pa/telaf-pa-simula/component/tel/` -- the simulated
`telux::tel` eCall surface (ICallManager's eCall RPCs + IPhone's
set/getECallOperatingMode). Structurally identical to
:class:`sml.mpss.radio.RadioSubsystem` (Off -> Operating.{Starting,Ready}
-> Stopping, same three-line rationale in that module's docstring), but
with a single business AO (:class:`~sml.mpss.ecall.call_manager.
EcallManagerAO`) rather than several, since every eCall RPC in scope
funnels through the one ICallManager/IPhone eCall surface.

Usage (from ``sml/mpss/__main__.py``)::

    es = EcallSubsystem(slot_id=target_slot_runtime.sim_slot.slot_id)
    client.register_subsystem(es)
    client.start()
"""
from __future__ import annotations

import logging
import os
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr

_log = logging.getLogger("sml.mpss.ecall")


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


class EcallSubsystem(ActiveObject):
    """Coordinates the MPSS-side ecall Active Object for one slot.

    Lifecycle (called by :class:`~sml.common.mqtt_client.MqttClient`)::

        es.start(publish_fn, subscribe_fn, unsubscribe_fn)
        # ... messages dispatched via es.handle_message(topic, payload) ...
        es.stop()
    """

    def __init__(self, slot_id: int = 1, role: Optional[str] = None) -> None:
        super().__init__("EcallSubsystem")
        self._slot_id = slot_id
        self._role = role or _read_whoami()

        self._pending_start_args: Optional[tuple] = None
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None  # bypasses AO event queue

        self._call_manager_ao = None
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

        Called by :class:`~sml.common.mqtt_client.MqttClient` on every entry into
        Operational after the first one. Live sessions/timers are untouched --
        a broker flap is not an eCall teardown, same rationale as
        DataConnectionAO's resubscribe.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

    def owns_topic(self, topic: str) -> bool:
        """Return True if any sub-AO or the action dispatcher owns `topic`.

        REQUIRED by :class:`~sml.common.mqtt_client.MqttClient`. Its
        ``_on_message`` router asks every registered subsystem ``owns_topic``
        first and only calls ``handle_message`` on the one that claims the
        topic -- a subsystem without this method raises AttributeError inside
        the router's try/except, which logs and moves on, so every inbound
        ``mp/req/ecall/**`` and ``ctrl/cmd/action/ecall/**`` message would be
        silently dropped as "not consumed".
        """
        for ao in (self._call_manager_ao, self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                return True
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        if self._action_dispatcher is None:
            _log.debug("EcallSubsystem dispatch_action(%s) dropped -- action dispatcher not started",
                       canonical_name)
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.mpss.ecall.call_manager import EcallManagerAO
        from sml.mpss.ecall.action_dispatcher import EcallActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._direct_publish_fn = direct_publish_fn or publish_fn

        mpss_src = f"mpss-{self._role}-{os.getpid()}"

        self._call_manager_ao = EcallManagerAO(slot=self._slot_id, mpss_src=mpss_src)
        self._call_manager_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)

        self._action_dispatcher = EcallActionDispatcher(call_manager_ao=self._call_manager_ao)
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("EcallSubsystem started (slot=%d, src=%s)", self._slot_id, mpss_src)

    def _fanout_message(self, topic: str, payload: bytes) -> None:
        for ao in (self._call_manager_ao, self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                try:
                    ao.handle_message(topic, payload)
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO handler raised on %s: %s", topic, exc)
                return
        _log.debug("EcallSubsystem: no sub-AO owns %s", topic)

    def _fanout_resubscribe(self) -> None:
        for ao in (self._call_manager_ao, self._action_dispatcher):
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
        # Swap to direct (synchronous) publish before stopping, so
        # EcallManagerAO's Ready-exit hook (_do_exit_ready's
        # subsys_ready(ready=False)) reaches the broker before the MQTT
        # client disconnects -- mirrors RadioSubsystem._do_stop's rationale.
        if self._direct_publish_fn and self._call_manager_ao is not None:
            self._call_manager_ao._publish_fn = self._direct_publish_fn
        for ao in (self._action_dispatcher, self._call_manager_ao):
            if ao is not None:
                try:
                    ao.stop()
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO stop failed: %s", exc)
        _log.info("EcallSubsystem stopped")


# ---------------------------------------------------------------------------
# HSM state handlers -- identical topology to sml.mpss.radio.RadioSubsystem;
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
        _log.debug("EcallSubsystem: Resubscribe dropped -- not Ready")
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


__all__ = ["EcallSubsystem"]
