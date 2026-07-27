# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Routes SIM control actions to the card and subscription AOs."""
from __future__ import annotations

import json
import logging
from typing import Callable

import jsonschema

from generated.python.action_registry import ACTIONS
from generated.python.ctrl_validators import validate as validate_test_payload

_log = logging.getLogger("sml.mpss.sim.action_dispatcher")

_SIM_ACTION_TOPICS = {
    name: entry["topic"] for name, entry in ACTIONS.items() if name.startswith("sim.")
}

_TOPIC_TO_ACTION = {
    entry["topic"]: name for name, entry in ACTIONS.items() if name.startswith("sim.")
}


class SimActionDispatcher:
    """Routes `ctrl/cmd/action/sim/**` messages to World State mutators."""

    def __init__(self, card_ao, subscription_ao) -> None:
        self._card_ao = card_ao
        self._subscription_ao = subscription_ao
        self._owned_topics = set(_TOPIC_TO_ACTION.keys())
        self._subscribe_fn: Callable | None = None
        self._unsubscribe_fn: Callable | None = None

        self._dispatch = {
            "sim.force_card_state": lambda p: self._card_ao.force_card_state(p),
            "sim.force_power": lambda p: self._card_ao.force_power(p),
            "sim.hotswap": self._hotswap,
        }

    # Composite actions.

    def _hotswap(self, data: dict) -> None:
        """Publish the remove, detect, ready, identity-update sequence.

        Emits exactly 3 card_state + 1 sub_info_changed, synchronously.

        ORDERING IS LOAD-BEARING: sub_info_changed comes LAST, after the card is
        READY -- not between removal and re-insertion as earlier revisions did.
        The PA only copies identity into its cache when the card is present:
        tafPaSubscriptionListener::onSubscriptionInfoChanged reaches
        InitializeSimInfo() only for a non-null subscription on a live slot,
        while the ABSENT card_state ahead of it runs
        UpdateLocalSimState(simPtr, nullptr) and clears ICCID/IMSI/phoneNumber
        (tafSimCardImpl.cpp). Crucially the PRESENT path afterwards calls only
        CheckAndSendRefreshEvent() and never re-reads identity -- so an
        announcement made while the card is out is dropped and never recovered,
        leaving taf_sim_GetICCID()/GetIMSI() serving the pre-swap card forever
        even though the wire carried the new one.
        """
        self._subscription_ao.begin_hotswap()
        try:
            self._card_ao.remove_card()
            self._card_ao.set_card_app_state("ABSENT", "UNKNOWN")
            self._card_ao.insert_card()
            self._card_ao.set_card_app_state("PRESENT", "UNKNOWN", power_on=True)
            self._card_ao.set_card_app_state("PRESENT", "READY")
        finally:
            self._subscription_ao.end_hotswap()
        # Announce only now: the card is PRESENT/READY, so the PA accepts and
        # caches it. Suppressing the intermediate availability-driven events
        # above keeps this the single identity indication for the whole swap.
        self._subscription_ao.hotswap(data)

    def start(self, subscribe_fn: Callable, unsubscribe_fn: Callable | None = None) -> None:
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("sim action dispatcher started (%d action(s))", len(self._owned_topics))

    def resubscribe(self) -> None:
        """Re-issue every action subscription after an MQTT reconnect.

        Not an Active Object -- runs inline on the caller's thread. Publishes
        nothing: actions are inbound-only.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        _log.info("sim action dispatcher resubscribed (%d action(s))",
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
        """Validate and run an action; return False when it is not owned."""
        if canonical_name not in self._dispatch:
            return False
        schema_id = f"action.{canonical_name}.req"
        try:
            validate_test_payload(schema_id, data)
        except jsonschema.ValidationError as exc:
            _log.warning("action dispatcher: %s payload invalid: %s; dropping", canonical_name, exc)
            return True
        # Direct Scenario Runner calls require the same slot guard as wire actions.
        slot = data.get("slot", 1)
        if slot != 1:
            _log.warning(
                "action dispatcher: %s rejected -- slot %s not supported "
                "(this simulation models slot 1 only)",
                canonical_name, slot,
            )
            return True
        try:
            self._dispatch[canonical_name](data)
        except Exception as exc:  # noqa: BLE001
            _log.error("action dispatcher: %s handler raised: %s", canonical_name, exc)
        return True


__all__ = ["SimActionDispatcher"]
