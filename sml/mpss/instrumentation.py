# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS instrumentation control surface (miros live_spy / live_trace).

Instrumentation mode is FIXED AT STARTUP by config.yaml's `debug.instrumentation`
field, optionally overridden by the ``TELAF_MPSS_INSTRUMENTATION`` env var. It is
NOT switchable at runtime -- a runtime-switch hook would need a third
``ctrl/cmd/**`` target class, which violates invariant (c) (two disjoint control
channels: scenario/** and action/**).

Modes:
    off      -- live_trace=False, live_spy=False. Default.
    on       -- live_trace=True, live_spy=False. Records state transitions.
    verbose  -- live_trace=True, live_spy=True. Records every dispatch (chatty).

This module is a pure helpers module -- NOT a miros subchart / Active Object.
It is a conceptual instrumentation control surface, not an AO.

Usage:
    mode = resolve_mode(cfg)          # startup
    log.info(describe(mode))
    set_current_mode(mode)            # publish to constructors that opt in
    apply_mode(ao, mode)              # for every AO instance built downstream
"""
from __future__ import annotations

import os
from typing import Literal, Optional

Mode = Literal["off", "on", "verbose"]

VALID_MODES: tuple[Mode, ...] = ("off", "on", "verbose")

ENV_VAR = "TELAF_MPSS_INSTRUMENTATION"

_current_mode: Mode = "off"


def _validate(value: str, *, source: str) -> Mode:
    lowered = value.lower()
    if lowered not in VALID_MODES:
        raise ValueError(
            f"invalid instrumentation mode {value!r} from {source}; "
            f"expected one of {list(VALID_MODES)}"
        )
    return lowered  # type: ignore[return-value]


def resolve_mode(cfg) -> Mode:
    """Resolve the effective instrumentation mode at startup.

    Precedence:
      1. ``TELAF_MPSS_INSTRUMENTATION`` env var (case-insensitive).
      2. ``cfg.debug.instrumentation`` (validated on config load).
      3. Legacy fallback: if ``cfg.debug.live_spy`` is True and
         ``instrumentation`` is the default "off", auto-upgrade to "verbose".
      4. "off".

    Raises ValueError on an invalid env-var value.
    """
    env_val = os.environ.get(ENV_VAR)
    if env_val is not None and env_val.strip():
        return _validate(env_val.strip(), source=f"env {ENV_VAR}")

    cfg_val = getattr(cfg.debug, "instrumentation", "off")
    cfg_mode = _validate(str(cfg_val), source="cfg.debug.instrumentation")

    # Legacy fallback: live_spy=True + instrumentation not explicitly raised
    # -> treat as verbose so old configs keep working.
    if cfg_mode == "off" and bool(getattr(cfg.debug, "live_spy", False)):
        return "verbose"
    return cfg_mode


def apply_mode(ao, mode: Mode) -> None:
    """Set miros ``live_trace`` / ``live_spy`` attributes on an AO."""
    if mode == "off":
        ao.live_trace = False
        ao.live_spy = False
    elif mode == "on":
        ao.live_trace = True
        ao.live_spy = False
    elif mode == "verbose":
        ao.live_trace = True
        ao.live_spy = True
    else:  # pragma: no cover - resolve_mode validates
        raise ValueError(f"invalid mode {mode!r}")


def describe(mode: Mode) -> str:
    """Return a human-readable startup-banner line for the given mode."""
    detail = {
        "off":     "miros instrumentation: off (live_trace=False, live_spy=False)",
        "on":      "miros instrumentation: on (live_trace=True, live_spy=False) -- transitions logged",
        "verbose": "miros instrumentation: verbose (live_trace=True, live_spy=True) -- every dispatch logged",
    }
    return detail[mode]


def set_current_mode(mode: Mode) -> None:
    """Publish the resolved mode so AO constructors can read it at build time."""
    global _current_mode
    _current_mode = mode


def current_mode() -> Mode:
    """Return the mode set by :func:`set_current_mode` (defaults to "off")."""
    return _current_mode


__all__ = [
    "ENV_VAR",
    "Mode",
    "VALID_MODES",
    "apply_mode",
    "current_mode",
    "describe",
    "resolve_mode",
    "set_current_mode",
]
