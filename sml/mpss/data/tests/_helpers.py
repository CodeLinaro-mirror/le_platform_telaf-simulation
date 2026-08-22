# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Shared test helpers for MPSS data AO tests.

Each AO has its own dispatch thread, so state transitions are
asynchronous.  ``start()`` posts an event and returns; the AO thread
runs ``_do_start`` + transitions asynchronously.  Tests must poll for
the target state.
"""
from __future__ import annotations

import time

def _settled_state(ao):
    """Return the AO's *settled* state handler, or None.

    Deliberately reads ``ao.state.fun``, NOT ``ao.state_fn``. miros'
    ``spy_on`` assigns ``chart.state_fn = fn`` on entry to *every* handler
    it invokes, including while an ENTRY_SIGNAL body is still executing --
    so ``state_fn`` reports the destination before that state's entry
    actions have finished. ``state.fun`` is only assigned at the end of
    ``dispatch()``, once the RTC step is complete.

    This matters for any state whose ENTRY does real work: polling
    ``state_fn`` lets a test proceed while e.g. ``DataSubsystem``'s
    ``_do_start()`` is still constructing sub-AOs, so the very next
    assertion races against a half-built object.
    """
    state = getattr(ao, "state", None)
    return getattr(state, "fun", None) if state is not None else None


def wait_for_state(ao, target: str, timeout: float = 1.0) -> None:
    """Block until the AO has *settled* in ``target``, or raise.

    Small polling interval keeps unit tests fast (~5 ms).  Mirrors the
    pattern already used at ``sml/mpss/tests/test_integration.py:181``.
    """
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
    """Poll ``predicate`` until true or timeout; return final result."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(interval)
    return predicate()


__all__ = ["wait_for_state", "wait_until"]
