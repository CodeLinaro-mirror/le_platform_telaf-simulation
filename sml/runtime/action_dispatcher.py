# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Top-level Action Dispatcher.

Routes a canonical action name (e.g. ``"data.force_roaming"``) to the
domain-local dispatcher registered for its prefix (``"data"``). This is
the parent of `sml/mpss/data/action_dispatcher.py`'s `DataActionDispatcher`
-- that one owns the data domain's own dispatch table (schema validation +
World State mutator lookup); this one just picks *which* domain-local
dispatcher gets a given action, keyed off the part of the name before the
first ``.``.

Not an MQTT subsystem itself -- each domain-local dispatcher still
subscribes to and validates its own `ctrl/cmd/action/<domain>/**` topics
directly. This class only implements the entry point
:meth:`~sml.runtime.scenario_runner.ScenarioRunner` (or any other caller)
uses when it has a canonical name and needs to reach the right domain.
"""
from __future__ import annotations

from typing import Any


class ActionDispatcher:
    """Routes `dispatch_action(canonical_name, data)` by domain prefix."""

    def __init__(self, domains: dict[str, Any]) -> None:
        self._domains = domains

    def register_domain(self, domain: str, dispatcher: Any) -> None:
        self._domains[domain] = dispatcher

    def dispatch_action(self, canonical_name: str, data: dict) -> bool:
        domain = canonical_name.split(".", 1)[0]
        dispatcher = self._domains.get(domain)
        if dispatcher is None:
            return False
        return dispatcher.dispatch_action(canonical_name, data)


__all__ = ["ActionDispatcher"]
