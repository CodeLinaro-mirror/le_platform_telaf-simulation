# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS sim sub-package.

Provides :class:`SimSubsystem`, which implements the MPSS-side of the
WireSchema v1 MQTT contract for SIM card state/power and subscription
(ICCID/IMSI) services -- mirrors :class:`sml.mpss.data.DataSubsystem`.

Usage (from ``sml/mpss/__main__.py``)::

    ss = SimSubsystem(installed_sim=runner.sim_slot_states["sim_slot_0"].installed_sim,
                       role=_read_whoami())
    client.register_subsystem(ss)
    client.start()
"""
from __future__ import annotations

import logging
import os
from typing import Callable, Optional

from miros import Factory, Event, return_status, signals, spy_on
from miros.hsm import HsmWithQueues

from sml.config.models import SimCard
from sml.mpss import instrumentation as _instr

_log = logging.getLogger("sml.mpss.sim")

SIG_START = "Start"
SIG_STOP = "Stop"


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


class SimSubsystem(Factory):
    """Coordinates all MPSS-side sim Active Objects for one slot.

    Off → Ready → Stopping -- same 3-state shape as
    :class:`~sml.mpss.data.DataSubsystem` (no `Starting`; `Ready`'s entry
    creates and starts the two sub-AOs + Action Dispatcher).

    Lifecycle (called by :class:`~sml.mpss.mqtt_client.MqttClient`)::

        ss.start(publish_fn, subscribe_fn, unsubscribe_fn)
        # ... messages dispatched via ss.handle_message(topic, payload) ...
        ss.stop()
    """

    def __init__(
        self,
        installed_sim: Optional[SimCard] = None,
        role: Optional[str] = None,
    ) -> None:
        super().__init__("SimSubsystem")
        self._installed_sim = installed_sim
        self._role = role or _read_whoami()

        self._pending_start_args: Optional[tuple] = None
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None

        self._card_ao = None
        self._subscription_ao = None
        self._action_dispatcher = None

        self.nest(smfn_off, parent=None)
        self.nest(smfn_ready, parent=None)
        self.nest(smfn_stopping, parent=None)
        HsmWithQueues.start_at(self, smfn_off)
        # Startup-resolved instrumentation mode -- see SimCardAO's ctor.
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
        self.dispatch(Event(signal=signals.SIG_START))

    def stop(self) -> None:
        self.dispatch(Event(signal=signals.SIG_STOP))

    def resubscribe(self) -> None:
        """Re-establish this subsystem's broker state after an MQTT reconnect.

        Called by :class:`~sml.mpss.mqtt_client.MqttClient` on every entry into
        Operational after the first one. Fans out to each sub-AO's own
        ``resubscribe``; nothing is stopped or recreated -- a subsystem's
        lifetime is the process lifetime, because ``stop()`` is terminal
        (``smfn_stopping`` has no path back to Ready).
        """
        self.dispatch(Event(signal=signals.SIG_RESUBSCRIBE))

    def owns_topic(self, topic: str) -> bool:
        """Return True if any sub-AO or the action dispatcher owns `topic`.

        REQUIRED by :class:`~sml.mpss.mqtt_client.MqttClient`. Its
        ``_on_message`` router asks every registered subsystem ``owns_topic``
        first and only calls ``handle_message`` on the one that claims the
        topic -- a subsystem without this method raises AttributeError inside
        the router's try/except, which logs and moves on, so every inbound
        ``ap/req/sim/**`` and ``ctrl/cmd/action/sim/**`` message would be
        silently dropped as "not consumed". That is precisely the failure mode
        invariant (d) warns about: the PA waits forever on a get_state /
        get_iccid / get_imsi response that MPSS received but never routed.
        Mirrors DataSubsystem.owns_topic.
        """
        for ao in (self._card_ao, self._subscription_ao, self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                return True
        return False

    def handle_message(self, topic: str, payload: bytes) -> bool:
        """Route an inbound MQTT message to the appropriate sub-AO.

        Returns True if the message was handled, False otherwise.
        """
        if self.state_fn.__name__ != "smfn_ready":
            return False
        for ao in (self._card_ao, self._subscription_ao, self._action_dispatcher):
            if ao is not None and ao.owns_topic(topic):
                ao.handle_message(topic, payload)
                return True
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        """Proxy to the sim-domain Action Dispatcher's own dispatch_action.

        Mirrors DataSubsystem.dispatch_action -- lets a ScenarioRunner hold
        a reference to this SimSubsystem before start() has created
        `_action_dispatcher`.
        """
        if self._action_dispatcher is None:
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        from sml.mpss.sim.card import SimCardAO
        from sml.mpss.sim.subscription import SubscriptionAO
        from sml.mpss.sim.action_dispatcher import SimActionDispatcher

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._direct_publish_fn = direct_publish_fn or publish_fn

        mpss_src = f"mpss-{self._role}-{os.getpid()}"

        iccid = self._installed_sim.iccid if self._installed_sim else ""
        imsi = self._installed_sim.imsi if self._installed_sim else ""

        self._card_ao = SimCardAO(slot=1, mpss_src=mpss_src, iccid=iccid, imsi=imsi)
        self._subscription_ao = SubscriptionAO(slot=1, mpss_src=mpss_src, iccid=iccid, imsi=imsi)

        self._card_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)
        self._subscription_ao.start(publish_fn, subscribe_fn, unsubscribe_fn)

        # Card state and subscription identity are separate wire managers, but a
        # powered-down/absent card must not expose identity. Wire this AFTER both
        # AOs are started (so a publish_fn exists), otherwise the first
        # hide/restore would be swallowed during construction.
        self._subscription_ao.set_card_available(self._card_ao.identity_available)
        self._card_ao.set_state_changed_callback(self._subscription_ao.set_card_available)

        self._action_dispatcher = SimActionDispatcher(
            card_ao=self._card_ao,
            subscription_ao=self._subscription_ao,
        )
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("SimSubsystem started (slot=1, src=%s)", mpss_src)

    def _fanout_resubscribe(self) -> None:
        """Re-issue subscriptions on every sub-AO, in start order."""
        for ao in (self._card_ao, self._subscription_ao, self._action_dispatcher):
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
        if self._direct_publish_fn:
            # Switch to direct (synchronous) publish so ready=false reaches
            # the broker before paho disconnects -- mirrors DataSubsystem's
            # equivalent shutdown-ordering fix for DataServingSystemAO.
            for ao in (self._card_ao, self._subscription_ao):
                if ao is not None:
                    ao._publish_fn = self._direct_publish_fn
        for ao in (self._action_dispatcher, self._subscription_ao, self._card_ao):
            if ao is not None:
                try:
                    ao.stop()
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO stop failed: %s", exc)
        _log.info("SimSubsystem stopped")


# ---------------------------------------------------------------------------
# HSM state handlers.
#
#     top
#     ├── smfn_off
#     ├── smfn_ready
#     └── smfn_stopping
# ---------------------------------------------------------------------------

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SIG_START:
        status = chart.trans(smfn_ready)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.SIG_STOP:
        status = chart.trans(smfn_stopping)
    elif e.signal == signals.SIG_RESUBSCRIBE:
        chart._fanout_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_stopping(chart, e):
    """Terminal: stops every sub-AO on entry. No path back to Ready."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["SimSubsystem"]
