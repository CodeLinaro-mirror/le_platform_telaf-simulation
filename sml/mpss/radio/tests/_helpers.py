# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Shared test helpers for MPSS radio AO tests -- same pattern as
sml/mpss/data/tests/_helpers.py."""
from __future__ import annotations

import time


def _settled_state(ao):
    state = getattr(ao, "state", None)
    return getattr(state, "fun", None) if state is not None else None


def wait_for_state(ao, target: str, timeout: float = 1.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        fn = _settled_state(ao)
        if fn is not None and fn.__name__ == target:
            return
        time.sleep(0.005)
    fn = _settled_state(ao)
    last = fn.__name__ if fn is not None else "<none>"
    raise AssertionError(f"AO never reached {target}; last={last}")


def wait_until(predicate, timeout: float = 1.0, interval: float = 0.005) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(interval)
    return predicate()


__all__ = ["wait_for_state", "wait_until"]
