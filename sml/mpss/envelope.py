# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""WireSchema v1 envelope helpers.

The three business AOs (serv/profile/connection) build/parse envelopes
through these functions.  Field-level validators beyond what's here
(e.g. PA-side test fixtures) live in their own modules.
"""
from __future__ import annotations

import json
import logging
import re
import time
from typing import Callable, Optional

import jsonschema

from generated.python.validators import validate as validate_payload

# Pattern used in WireSchema envelope `corrId` field.
_CORR_ID_RE = re.compile(r"^[0-9a-f]{4,8}$")


def resolve_schema_id(topic: str) -> Optional[str]:
    """Map a wire topic to its generated-schema id (`domain.method.req`).

    Shared by the three business AOs' `_dispatch_message` and by
    `sml.tools.mqttcli.pubfile` -- one conversion rule, per invariant (g).
    """
    if topic.startswith("ap/req/"):
        return topic[len("ap/req/"):].replace("/", ".") + ".req"
    if topic.startswith("ctrl/cmd/"):
        return topic[len("ctrl/cmd/"):].replace("/", ".") + ".req"
    return None


def validate_envelope(msg) -> list[str]:
    """Validate a parsed JSON envelope dict against WireSchema v1 rules.

    Returns a list of error strings (empty = valid).
    """
    errs: list[str] = []
    if not isinstance(msg, dict):
        return ["message body is not a JSON object"]
    if msg.get("v") != 1:
        errs.append(f"envelope v must be 1, got {msg.get('v')!r}")
    corr = msg.get("corrId")
    if not isinstance(corr, str) or not _CORR_ID_RE.match(corr):
        errs.append(f"corrId must match ^[0-9a-f]{{4,8}}$, got {corr!r}")
    if not isinstance(msg.get("ts"), int):
        errs.append("ts must be an integer")
    if not isinstance(msg.get("src"), str):
        errs.append("src must be a string")
    return errs


def _next_corr_id() -> str:
    import random
    return f"{random.randint(0, 0xFFFF):04x}"


def build_success_envelope(src: str, corr_id: str, dest: str, data: dict) -> dict:
    """Build a success response envelope.

    `dest` echoes the requester's `src` -- PA subscribes to the shared
    `mp/rsp/#` wildcard and filters on this field before even consulting
    `corrId` (see Envelope.hpp's doc comment on the rsp envelope shape).
    """
    return {
        "v": 1,
        "corrId": corr_id,
        "ts": int(time.time() * 1000),
        "src": src,
        "dest": dest,
        "data": data,
    }


def build_error_envelope(src: str, corr_id: str, dest: str, code: str, msg: str = "") -> dict:
    err: dict = {"code": code}
    if msg:
        err["msg"] = msg
    return {
        "v": 1,
        "corrId": corr_id,
        "ts": int(time.time() * 1000),
        "src": src,
        "dest": dest,
        "error": err,
    }


def build_event_envelope(src: str, data: dict) -> dict:
    return {
        "v": 1,
        "corrId": _next_corr_id(),
        "ts": int(time.time() * 1000),
        "src": src,
        "data": data,
    }


def dispatch_inbound(
    topic: str,
    payload: bytes,
    handlers: dict[str, Callable],
    log: logging.Logger,
    label: str,
) -> None:
    """Shared inbound-message dispatch: JSON parse, envelope validate,
    payload schema validate, then call the registered handler.

    Used by connection, profile, and serving_system AOs.
    """
    handler = handlers.get(topic)
    if handler is None:
        return
    try:
        msg = json.loads(payload.decode("utf-8"))
    except Exception:
        log.warning("%s: bad JSON on %s; dropping", label, topic)
        return
    errs = validate_envelope(msg)
    if errs:
        log.warning("%s: envelope errors on %s: %s; dropping", label, topic, errs)
        return
    try:
        validate_payload(resolve_schema_id(topic), msg.get("data") or {})
    except jsonschema.ValidationError as exc:
        log.warning("%s: payload schema invalid on %s: %s; dropping", label, topic, exc)
        return
    try:
        handler(msg)
    except Exception as exc:  # noqa: BLE001
        log.error("%s: handler %s raised: %s", label, topic, exc)


__all__ = [
    "build_error_envelope",
    "build_event_envelope",
    "build_success_envelope",
    "dispatch_inbound",
    "resolve_schema_id",
    "validate_envelope",
]
