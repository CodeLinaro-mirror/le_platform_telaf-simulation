# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Sanity: every AO module exposes smfn_* handlers (guard against silent
import regressions that would make handler introspection vacuously pass)."""
from __future__ import annotations

import importlib


_HANDLER_MODULES = [
    "sml.mpss.data",
    "sml.mpss.data.connection",
    "sml.mpss.data.profile",
    "sml.mpss.data.serving_system",
    "sml.runtime.scenario_runner",
]


def _iter_handlers():
    """Yield ``(module_name, fn)`` for every module-level ``smfn_*`` handler."""
    for mod_name in _HANDLER_MODULES:
        mod = importlib.import_module(mod_name)
        for attr, fn in vars(mod).items():
            if attr.startswith("smfn_") and callable(fn):
                yield mod_name, fn


def test_handler_modules_expose_handlers():
    """Sanity: introspection finds handlers."""
    found = list(_iter_handlers())
    assert len(found) >= 20, f"expected >=20 handlers, found {len(found)}"
