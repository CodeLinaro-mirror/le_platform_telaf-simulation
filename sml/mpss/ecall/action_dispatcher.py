# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""ECall-domain Action Dispatcher (domain-local; mirrors
sml.mpss.radio.action_dispatcher.RadioActionDispatcher).

Subscribes to every `ctrl/cmd/action/ecall/**` topic from the generated
action registry, validates the inbound payload, and calls the matching
:class:`~sml.mpss.ecall.call_manager.EcallManagerAO` force_* hook -- the
MQTT-era replacement for the old gRPC event-injector's InjectEvent path
(see control/registry/action_ecall.yaml's header comment).
"""
from __future__ import annotations

import json
import logging
from typing import Callable

import jsonschema

from generated.python.action_registry import ACTIONS
from generated.python.ctrl_validators import validate as validate_test_payload

_log = logging.getLogger("sml.mpss.ecall.action_dispatcher")

_TOPIC_TO_ACTION = {
    entry["topic"]: name for name, entry in ACTIONS.items() if name.startswith("ecall.")
}


class EcallActionDispatcher:
    """Routes `ctrl/cmd/action/ecall/**` messages to EcallManagerAO force_* hooks."""

    def __init__(self, call_manager_ao) -> None:
        self._call_manager_ao = call_manager_ao
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "ecall.force_ecall_timer_failure":
                lambda p: self._call_manager_ao.force_ecall_timer_failure(p["timer"]),
            "ecall.force_msd_pull_request":
                lambda p: self._call_manager_ao.force_msd_pull_request(p["phoneId"]),
            "ecall.force_hangup_from_psap":
                lambda p: self._call_manager_ao.force_hangup_from_psap(p["phoneId"]),
            "ecall.force_ecall_redial_mode":
                lambda p: self._call_manager_ao.force_ecall_redial_mode(
                    p["config"], p.get("succeedAtAttempt")
                ),
        }

    def start(self, subscribe_fn: Callable, unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("ecall action dispatcher started (%d action(s))", len(self._owned_topics))

    def resubscribe(self) -> None:
        """Not an Active Object -- runs on the caller's (EcallSubsystem AO)
        thread, same rationale as RadioActionDispatcher.resubscribe."""
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("ecall action dispatcher resubscribed (%d action(s))", len(self._owned_topics))

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
        except jsonschema.ValidationError as exc:
            _log.warning("action dispatcher: %s payload invalid: %s; dropping", canonical_name, exc)
            return True
        try:
            _log.debug("action dispatcher: %s payload=%s", canonical_name, data)
            self._dispatch[canonical_name](data)
        except Exception as exc:  # noqa: BLE001
            _log.error("action dispatcher: %s handler raised: %s", canonical_name, exc)
        else:
            _log.info("action dispatcher: %s applied", canonical_name)
        return True


__all__ = ["EcallActionDispatcher"]
