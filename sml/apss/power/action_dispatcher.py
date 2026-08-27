# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Power-domain Action Dispatcher (invariant e: manual `ctrl/cmd/action/**`
injection and scenario-timeline actions share this one dispatch path).
"""
from __future__ import annotations

import json
import logging
from typing import Callable

from generated.python.action_registry import ACTIONS
from generated.python.ctrl_validators import ValidationError, validate as validate_test_payload

_log = logging.getLogger("sml.apss.power.action_dispatcher")

_TOPIC_TO_ACTION = {
    entry["topic"]: name for name, entry in ACTIONS.items() if name.startswith("power.")
}


class PowerActionDispatcher:
    """Routes `ctrl/cmd/action/power/**` messages to PowerSubsystem mutators."""

    def __init__(self, power_subsystem) -> None:
        self._power_subsystem = power_subsystem
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "power.add_machine": lambda p: self._power_subsystem.add_machine(p),
            "power.remove_machine": lambda p: self._power_subsystem.remove_machine(p),
            "power.force_slave_ack": lambda p: self._power_subsystem.force_slave_ack(p), # only for test
            "power.suspend_system": lambda p: self._power_subsystem.suspend_system(p),
            "power.resume_system": lambda p: self._power_subsystem.resume_system(p),
        }

    def start(self, subscribe_fn: Callable, unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("power action dispatcher started (%d action(s))", len(self._owned_topics))

    def resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("power action dispatcher resubscribed (%d action(s))",
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
        if canonical_name not in self._dispatch:
            return False
        schema_id = f"action.{canonical_name}.req"
        try:
            validate_test_payload(schema_id, data)
        except ValidationError as exc:
            _log.warning("action dispatcher: %s payload invalid: %s; dropping", canonical_name, exc)
            return True
        try:
            self._dispatch[canonical_name](data)
        except Exception as exc:  # noqa: BLE001
            _log.error("action dispatcher: %s handler raised: %s", canonical_name, exc)
        else:
            _log.info("action dispatcher: %s applied", canonical_name)
        return True


__all__ = ["PowerActionDispatcher"]
