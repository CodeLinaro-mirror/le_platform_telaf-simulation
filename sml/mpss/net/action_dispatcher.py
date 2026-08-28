# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Net-domain Action Dispatcher (domain-local; the top-level router in
sml/runtime/action_dispatcher.py forwards to this one by canonical-name
prefix).

Structurally identical to
:class:`~sml.mpss.data.action_dispatcher.DataActionDispatcher`: subscribes to
every ``ctrl/cmd/action/net/**`` topic from the generated action registry,
validates the inbound payload against that action's JSON Schema, and calls the
matching World State mutator on a child AO. Both trigger paths -- manual
``mosquitto_pub`` and Scenario Runner timeline steps -- converge on the single
``dispatch_action`` table so behaviour can't diverge between them.

Actions are oneway: there is no response topic, so schema failures and unknown
targets are logged and dropped.
"""
from __future__ import annotations

import json
import logging
from typing import Callable

import jsonschema

from generated.python.action_registry import ACTIONS
from generated.python.ctrl_validators import validate as validate_test_payload

_log = logging.getLogger("sml.mpss.net.action_dispatcher")

_TOPIC_TO_ACTION = {
    entry["topic"]: name for name, entry in ACTIONS.items() if name.startswith("net.")
}


class NetActionDispatcher:
    """Routes ``ctrl/cmd/action/net/**`` messages to World State mutators."""

    def __init__(self, vlan_ao, settings_ao=None) -> None:
        self._vlan_ao = vlan_ao
        self._settings_ao = settings_ao
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "net.force_vlan_hw_accel": lambda p: self._vlan_ao.force_vlan_hw_accel(p),
            "net.force_wwan_connectivity":
                lambda p: self._settings_ao.force_wwan_connectivity(p),
            "net.force_dds": lambda p: self._settings_ao.force_dds(p),
            "net.force_ippt_config": lambda p: self._settings_ao.force_ippt_config(p),
            "net.force_ip_config": lambda p: self._settings_ao.force_ip_config(p),
        }

    def start(self, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("net action dispatcher started (%d action(s))",
                  len(self._owned_topics))

    def resubscribe(self) -> None:
        """Re-issue every action subscription after an MQTT reconnect.

        Not an Active Object -- no fifo, no state to rebuild -- so this runs on
        the caller's thread (the NetSubsystem AO thread). Publishes nothing:
        actions are inbound-only.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("net action dispatcher resubscribed (%d action(s))",
                  len(self._owned_topics))

    def stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        canonical_name = _TOPIC_TO_ACTION.get(topic)
        if canonical_name is None:
            return
        try:
            data = json.loads(payload.decode("utf-8"))
        except Exception:
            _log.warning("action dispatcher: bad JSON on %s; dropping", topic)
            return
        self.dispatch_action(canonical_name, data)

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        """Validate+run one action by canonical name.

        Returns False if this dispatcher doesn't own ``canonical_name``, so the
        top-level router can try another domain.
        """
        if canonical_name not in self._dispatch:
            return False
        schema_id = f"action.{canonical_name}.req"
        try:
            validate_test_payload(schema_id, data)
        except jsonschema.ValidationError as exc:
            _log.warning("action dispatcher: %s payload invalid: %s; dropping",
                         canonical_name, exc)
            return True
        try:
            self._dispatch[canonical_name](data)
        except Exception as exc:  # noqa: BLE001
            _log.error("action dispatcher: %s handler raised: %s", canonical_name, exc)
        return True


__all__ = ["NetActionDispatcher"]
