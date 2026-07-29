# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Data-domain Action Dispatcher (domain-local; the top-level router in
sml/runtime/action_dispatcher.py forwards to this one by canonical-name
prefix, see that module's docstring).

Subscribes to every `ctrl/cmd/action/data/**` topic from the generated
action registry, validates the inbound payload against that action's JSON
Schema, and calls the matching World State mutator on
:class:`~sml.mpss.data.serving_system.DataServingSystemAO` or
:class:`~sml.mpss.data.connection.DataConnectionAO`. This is a *manual*
entry point equivalent to what the Scenario Runner calls when a timeline
step fires the same canonical action name -- both paths must converge on
the same dispatch table so behavior doesn't diverge between "triggered by
mosquitto_pub" and "triggered by scenario timeline".

Unlike the production RPCs, `ctrl/cmd/action/**` payloads are raw JSON
objects matching the action's req schema directly -- no WireSchema v1
envelope (no v/corrId/src/dest wrapper). Schema validation failures and
unknown profile ids are logged and dropped; there is no response topic to
report an error on (every action is oneway per spec).
"""
from __future__ import annotations

import json
import logging
from typing import Callable

import jsonschema

from generated.python.action_registry import ACTIONS
from generated.python.ctrl_validators import validate as validate_test_payload

_log = logging.getLogger("sml.mpss.data.action_dispatcher")

# This dispatcher only owns the data-domain slice of ACTIONS (canonical
# names prefixed "data."); phone/sim/network/sms dispatchers are P4
# follow-up work once those domains have MPSS implementations.
_DATA_ACTION_TOPICS = {
    name: entry["topic"] for name, entry in ACTIONS.items() if name.startswith("data.")
}


# This dispatcher only owns the data-domain slice of ACTIONS (canonical
# names prefixed "data."); phone/sim/network/sms dispatchers are P4
# follow-up work once those domains have MPSS implementations. Indexed
# topic -> canonical name, the direction handle_message() needs to look up.
_TOPIC_TO_ACTION = {
    entry["topic"]: name for name, entry in ACTIONS.items() if name.startswith("data.")
}


class DataActionDispatcher:
    """Routes `ctrl/cmd/action/data/**` messages to World State mutators."""

    def __init__(self, serving_system_ao, connection_ao) -> None:
        self._serving_system_ao = serving_system_ao
        self._connection_ao = connection_ao
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "data.force_serv_state": lambda p: self._serving_system_ao.force_serv_state(p),
            "data.force_roaming": lambda p: self._serving_system_ao.force_roaming(p),
            "data.force_nr_icon_type": lambda p: self._serving_system_ao.force_nr_icon_type(p),
            "data.force_call_drop": lambda p: self._connection_ao.force_call_drop(p["profileId"]),
            "data.force_throughput": lambda p: self._connection_ao.force_throughput(p),
            "data.force_qos": lambda p: self._connection_ao.force_qos(p),
            "data.force_hw_accel": lambda p: self._connection_ao.force_hw_accel(p),
            "data.force_throttle": lambda p: self._connection_ao.force_throttle(p),
        }

    def start(self, subscribe_fn: Callable, unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("data action dispatcher started (%d action(s))", len(self._owned_topics))

    def resubscribe(self) -> None:
        """Re-issue every action subscription after an MQTT reconnect.

        Not an Active Object -- there is no fifo to post into and no state to
        rebuild, so the loop runs on the caller's thread (the DataSubsystem AO
        thread). Publishes nothing: actions are inbound-only.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("data action dispatcher resubscribed (%d action(s))",
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
        """Validate+run one action by canonical name (e.g. "data.force_roaming").

        Shared by :meth:`handle_message` (manual ``mosquitto_pub``) and the
        Scenario Runner (timeline steps), so both trigger paths converge on
        this single table -- manual and automatic actions are deliberately
        kept on one dispatch path. Returns False if this dispatcher doesn't
        own `canonical_name` -- callers should try another domain's
        dispatcher or log "unknown action".
        """
        if canonical_name not in self._dispatch:
            return False
        schema_id = f"action.{canonical_name}.req"
        try:
            validate_test_payload(schema_id, data)
        except jsonschema.ValidationError as exc:
            _log.warning("action dispatcher: %s payload invalid: %s; dropping", canonical_name, exc)
            return True
        try:
            self._dispatch[canonical_name](data)
        except Exception as exc:  # noqa: BLE001
            _log.error("action dispatcher: %s handler raised: %s", canonical_name, exc)
        return True


__all__ = ["DataActionDispatcher"]
