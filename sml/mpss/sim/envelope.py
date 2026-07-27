# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Deprecated compatibility shim; prefer :mod:`sml.mpss.envelope`."""
from __future__ import annotations

from sml.mpss.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
    resolve_schema_id,
    validate_envelope,
)
__all__ = [
    "build_error_envelope",
    "build_event_envelope",
    "build_success_envelope",
    "dispatch_inbound",
    "resolve_schema_id",
    "validate_envelope",
]

