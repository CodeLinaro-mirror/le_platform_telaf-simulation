# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""net_modem-domain Action Dispatcher (domain-local; the top-level router in
sml/runtime/action_dispatcher.py forwards to this one by canonical-name
prefix).

Owns the two ``ctrl/cmd/action/net_modem/**`` actions -- force_wwan_connectivity
and force_dds -- which inject modem/network-side changes that have no AP-facing
request. They live here rather than in the APSS net dispatcher because the state
they mutate (WWAN gating, DDS) is modem ground truth, and an action must be
handled by whichever process owns the state it writes.

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

_log = logging.getLogger("sml.mpss.net_modem.action_dispatcher")

_TOPIC_TO_ACTION = {
    entry["topic"]: name
    for name, entry in ACTIONS.items()
    if name.startswith("net_modem.")
}


class NetModemActionDispatcher:
    """Routes ``ctrl/cmd/action/net_modem/**`` messages to World State mutators."""

    def __init__(self, modem_ao) -> None:
        self._modem_ao = modem_ao
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "net_modem.force_wwan_connectivity":
                lambda p: self._modem_ao.force_wwan_connectivity(p),
            "net_modem.force_dds": lambda p: self._modem_ao.force_dds(p),
        }

    def start(self, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("net_modem action dispatcher started (%d action(s))",
                  len(self._owned_topics))

    def resubscribe(self) -> None:
        """Re-issue every action subscription after an MQTT reconnect.

        Not an Active Object -- no fifo, no state to rebuild -- so this runs on
        the caller's thread (the NetModemSubsystem AO thread). Publishes
        nothing: actions are inbound-only.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("net_modem action dispatcher resubscribed (%d action(s))",
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
            _log.error("action dispatcher: %s handler raised: %s",
                       canonical_name, exc)
        return True


__all__ = ["NetModemActionDispatcher"]
