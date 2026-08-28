# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side network subsystem for ``taf_net.api``.

The Linux-local APIs stay in ``tafNetSvc``. The simulated TelSDK families are
implemented by five child AOs: VLAN, L2TP, NAT, SOCKS, and settings/IP config.
Each child owns its state and publishes its own retained readiness indication.
"""
from __future__ import annotations

import logging
import os
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr

_log = logging.getLogger("sml.mpss.net")


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


class NetSubsystem(ActiveObject):
    """Coordinates the five network child AOs and the action dispatcher.

    The subsystem owns one ``net`` domain handle; each child owns its state and
    readiness indication.
    """

    def __init__(
        self,
        slot_id: int = 1,
        role: Optional[str] = None,
        seed_vlans: Optional[list] = None,
        seed_vlan_bindings: Optional[list] = None,
        seed_l2tp_tunnels: Optional[list] = None,
        seed_l2tp_bindings: Optional[list] = None,
        seed_nat_entries: Optional[list] = None,
        seed_backhaul_pref: Optional[list] = None,
    ) -> None:
        super().__init__("NetSubsystem")
        self._slot_id = slot_id
        self._role = role or _read_whoami()
        self._seed_vlans = seed_vlans or []
        self._seed_vlan_bindings = seed_vlan_bindings or []
        self._seed_l2tp_tunnels = seed_l2tp_tunnels or []
        self._seed_l2tp_bindings = seed_l2tp_bindings or []
        self._seed_nat_entries = seed_nat_entries or []
        self._seed_backhaul_pref = seed_backhaul_pref or []

        self._pending_start_args: Optional[tuple] = None
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._direct_publish_fn: Optional[Callable] = None

        self._vlan_ao = None
        self._l2tp_ao = None
        self._nat_ao = None
        self._socks_ao = None
        self._settings_ao = None
        self._action_dispatcher = None

        self.start_at(smfn_net_off)
        _instr.apply_mode(self, _instr.current_mode())

    @property
    def _children(self) -> tuple:
        """Return all child AOs used for topic and action fanout."""
        return (self._vlan_ao, self._l2tp_ao, self._nat_ao, self._socks_ao,
                self._settings_ao, self._action_dispatcher)

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable,
              direct_publish_fn: Optional[Callable] = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn,
                                    direct_publish_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        self.post_fifo(Event(signal=signals.Resubscribe))

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    def owns_topic(self, topic: str) -> bool:
        for ao in self._children:
            if ao is not None and ao.owns_topic(topic):
                return True
        return False

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        """Dispatch a scenario action through the net-domain dispatcher."""
        if self._action_dispatcher is None:
            return False
        return self._action_dispatcher.dispatch_action(canonical_name, data)

    def _do_start(self) -> None:
        from sml.mpss.net.action_dispatcher import NetActionDispatcher
        from sml.mpss.net.l2tp import NetL2tpAO
        from sml.mpss.net.nat import NetNatAO
        from sml.mpss.net.settings import NetSettingsAO
        from sml.mpss.net.socks import NetSocksAO
        from sml.mpss.net.vlan import NetVlanAO

        publish_fn, subscribe_fn, unsubscribe_fn, direct_publish_fn = \
            self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        self._direct_publish_fn = direct_publish_fn or publish_fn

        mpss_src = f"mpss-{self._role}-{os.getpid()}"

        self._vlan_ao = NetVlanAO(
            slot=self._slot_id,
            mpss_src=mpss_src,
            seed_vlans=self._seed_vlans,
            seed_bindings=self._seed_vlan_bindings,
        )
        self._l2tp_ao = NetL2tpAO(
            mpss_src=mpss_src,
            seed_tunnels=self._seed_l2tp_tunnels,
            seed_bindings=self._seed_l2tp_bindings,
        )
        self._nat_ao = NetNatAO(
            mpss_src=mpss_src, seed_entries=self._seed_nat_entries,
        )
        self._socks_ao = NetSocksAO(mpss_src=mpss_src)
        self._settings_ao = NetSettingsAO(
            slot_id=self._slot_id, mpss_src=mpss_src,
            backhaul_pref=self._seed_backhaul_pref,
        )

        for ao in (self._vlan_ao, self._l2tp_ao, self._nat_ao, self._socks_ao,
                   self._settings_ao):
            ao.start(publish_fn, subscribe_fn, unsubscribe_fn)

        self._action_dispatcher = NetActionDispatcher(
            vlan_ao=self._vlan_ao, settings_ao=self._settings_ao,
        )
        self._action_dispatcher.start(subscribe_fn, unsubscribe_fn)

        _log.info("NetSubsystem started (slot=%d, src=%s, 5 famil(ies))",
                  self._slot_id, mpss_src)

    def _fanout_message(self, topic: str, payload: bytes) -> None:
        for ao in self._children:
            if ao is not None and ao.owns_topic(topic):
                try:
                    ao.handle_message(topic, payload)
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO handler raised on %s: %s", topic, exc)
                return
        _log.debug("NetSubsystem: no sub-AO owns %s", topic)

    def _fanout_resubscribe(self) -> None:
        for ao in self._children:
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
        # Switch every child to direct (synchronous) publish so the retained
        # ready=false from each Ready-exit hook reaches the broker before paho
        # disconnects -- the async event-queue path would be processed after the
        # AO has already left Operating.
        if self._direct_publish_fn:
            for ao in (self._vlan_ao, self._l2tp_ao, self._nat_ao,
                       self._socks_ao, self._settings_ao):
                if ao is not None:
                    ao._publish_fn = self._direct_publish_fn
        # Dispatcher first, then the AOs it forwards into, so no action can
        # arrive at an AO that has already begun tearing down.
        for ao in (self._action_dispatcher, self._settings_ao, self._socks_ao,
                   self._nat_ao, self._l2tp_ao, self._vlan_ao):
            if ao is not None:
                try:
                    ao.stop()
                except Exception as exc:  # noqa: BLE001
                    _log.error("sub-AO stop failed: %s", exc)
        _log.info("NetSubsystem stopped")


@spy_on
def smfn_net_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_net_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_net_operating(chart, e):
    """Composite parent: defers MessageReceived until Ready."""
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_net_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_net_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("NetSubsystem: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_net_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_net_ready)
    else:
        chart.temp.fun = smfn_net_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_net_ready(chart, e):
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
        chart.temp.fun = smfn_net_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_net_stopping(chart, e):
    """Terminal: stops every sub-AO on entry."""
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


__all__ = ["NetSubsystem"]
