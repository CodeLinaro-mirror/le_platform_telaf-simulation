# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Radio-domain Action Dispatcher (domain-local; the top-level router in
sml/runtime/action_dispatcher.py forwards to this one by canonical-name
prefix -- mirrors sml.mpss.data.action_dispatcher.DataActionDispatcher /
sml.mpss.sim.action_dispatcher.SimActionDispatcher).

Subscribes to every `ctrl/cmd/action/radio/**` topic from the generated
action registry, validates the inbound payload against that action's JSON
Schema, and calls the matching World State mutator on
:class:`~sml.mpss.radio.serving_system.RadioServingSystemAO` or
:class:`~sml.mpss.radio.phone.RadioPhoneAO`. Manual injection
(mosquitto_pub) and scenario-timeline actions both converge on this one
dispatch table -- see DataActionDispatcher's docstring for the shared
rationale.
"""
from __future__ import annotations

import json
import logging
from typing import Callable

import jsonschema

from generated.python.action_registry import ACTIONS
from generated.python.ctrl_validators import validate as validate_test_payload

_log = logging.getLogger("sml.mpss.radio.action_dispatcher")

_RADIO_ACTION_TOPICS = {
    name: entry["topic"] for name, entry in ACTIONS.items() if name.startswith("radio.")
}

_TOPIC_TO_ACTION = {
    entry["topic"]: name for name, entry in ACTIONS.items() if name.startswith("radio.")
}


class RadioActionDispatcher:
    """Routes `ctrl/cmd/action/radio/**` messages to World State mutators."""

    def __init__(self, serving_system_ao, phone_ao) -> None:
        self._serving_system_ao = serving_system_ao
        self._phone_ao = phone_ao
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "radio.force_lte_cs_capability":
                lambda p: self._serving_system_ao.force_lte_cs_capability(p),
            "radio.force_sys_info":
                lambda p: self._serving_system_ao.force_sys_info(p),
            "radio.force_dc_status":
                lambda p: self._serving_system_ao.force_dc_status(p),
            "radio.force_signal_strength":
                lambda p: self._phone_ao.force_signal_strength(p),
        }

    def start(self, subscribe_fn: Callable, unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("radio action dispatcher started (%d action(s))", len(self._owned_topics))

    def resubscribe(self) -> None:
        """Re-issue every action subscription after an MQTT reconnect.

        Not an Active Object -- there is no fifo to post into and no state to
        rebuild, so the loop runs on the caller's thread (the RadioSubsystem AO
        thread). Publishes nothing: actions are inbound-only.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("radio action dispatcher resubscribed (%d action(s))",
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
        """Validate+run one action by canonical name (e.g. "radio.force_lte_cs_capability").

        Shared by :meth:`handle_message` (manual `mosquitto_pub`) and the
        Scenario Runner (timeline steps) -- see DataActionDispatcher's
        docstring. Returns False if this dispatcher doesn't own
        `canonical_name`.
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
        else:
            _log.info("action dispatcher: %s applied", canonical_name)
        return True


__all__ = ["RadioActionDispatcher"]
