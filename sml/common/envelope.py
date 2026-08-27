# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""WireSchema v1 envelope helpers, shared by mpss and apss.

Both processes build/parse envelopes identically; only the inbound topic
prefixes differ (mpss: `mp/req/**`, `ctrl/cmd/**`; apss: `ap/req/**`,
`ap/ff/**`, plus the cross-process indication `mp/ind/wakeup/event` it
consumes from mpss -- D11/O1-b wakeup relay). `resolve_schema_id` merges
both prefix tables; the two topic namespaces never overlap, so this is
safe for both processes without any per-caller parametrization.
"""
from __future__ import annotations

import json
import logging
import re
import time
from typing import Callable, Optional

from generated.python.validators import ValidationError, validate as validate_payload

# Pattern used in WireSchema envelope `corrId` field.
_CORR_ID_RE = re.compile(r"^[0-9a-f]{4,8}$")

# The one cross-process indication apss consumes that isn't its own request.
_WAKEUP_EVENT_TOPIC = "mp/ind/wakeup/event"
_WAKEUP_EVENT_SCHEMA_ID = "wakeup.event.ind"

_PREFIX_SUFFIX = (
    ("mp/req/", "req"),
    ("ap/req/", "req"),
    ("ap/ff/", "ff"),
    ("ctrl/cmd/", "req"),
)


def resolve_schema_id(topic: str) -> Optional[str]:
    """Map a wire topic to its generated-schema id (`domain.method.req`/`.ff`).

    Shared by every business AO's `_dispatch_message` and by
    `sml.tools.mqttcli.pubfile` -- one conversion rule, per invariant (g).
    """
    if topic == _WAKEUP_EVENT_TOPIC:
        return _WAKEUP_EVENT_SCHEMA_ID
    for prefix, suffix in _PREFIX_SUFFIX:
        if topic.startswith(prefix):
            return topic[len(prefix):].replace("/", ".") + f".{suffix}"
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
    `mp/rsp/#`/`ap/rsp/#` wildcard and filters on this field before even
    consulting `corrId`.
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

    Used by every mpss/apss business AO.
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
    except ValidationError as exc:
        log.warning("%s: payload schema invalid on %s: %s; dropping", label, topic, exc)
        return
    try:
        handler(msg)
    except Exception as exc:  # noqa: BLE001
        log.error("%s: handler %s raised: %s", label, topic, exc)
    else:
        log.info("%s: dispatched %s (corrId=%s)", label, topic, msg.get("corrId"))


__all__ = [
    "build_error_envelope",
    "build_event_envelope",
    "build_success_envelope",
    "dispatch_inbound",
    "resolve_schema_id",
    "validate_envelope",
]
