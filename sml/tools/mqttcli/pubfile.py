# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""`sml.tools.mqttcli pub` YAML command loader.

One file = one command. The compact form is
{topic, qos?, retain?, envelope?, interval?, data}. `data` may be a single
object (one publish) or a list (a same-topic sequence). The expanded form is
{qos?, retain?, envelope?, interval?, messages}, where each message has
{topic, data, qos?, retain?, envelope?}. Envelope wrapping follows the prefix
rule (mp/req/* wraps, ctrl/cmd/* raw, unknown wraps) unless `envelope:`
overrides it. Known topics are validated against the generated JSON Schemas
before anything is sent -- a bad payload must never partially publish a
sequence.
"""
from __future__ import annotations

import re
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Optional

import jsonschema
import yaml

# `generated.python.*` is imported as a top-level package (see
# sml/config/tools/validate_scenario.py for the same pattern) -- it only
# resolves when sml/ itself is on sys.path, which `python3 -m sml.tools.
# mqttcli` does not guarantee (only the repo root is). Insert it once, if
# missing, before the import below.
_SML_ROOT = Path(__file__).resolve().parents[2]
if str(_SML_ROOT) not in sys.path:
    sys.path.insert(0, str(_SML_ROOT))

from generated.python.ctrl_validators import SCHEMAS as CTRL_SCHEMAS
from generated.python.validators import SCHEMAS as PROD_SCHEMAS, ValidationError
from sml.common.envelope import _next_corr_id, resolve_schema_id

_MERGED_SCHEMAS = {**PROD_SCHEMAS, **CTRL_SCHEMAS}

_INTERVAL_RE = re.compile(r"^\d+(\.\d+)?(ms|s|m)$")
_INTERVAL_UNIT_SCALE = {"ms": 1.0, "s": 1000.0, "m": 60_000.0}


class PubFileError(ValueError):
    """Raised for a malformed pub YAML file (missing topic, bad interval,
    schema validation failure, ...)."""


@dataclass(frozen=True)
class PublishSpec:
    topic: str
    payload: dict
    qos: int
    retain: bool


def _should_envelope(topic: str, explicit: Optional[bool]) -> bool:
    if explicit is not None:
        return explicit
    if topic.startswith("ctrl/cmd/"):
        return False
    return True  # mp/req/* and unknown topics wrap


def parse_interval_ms(interval: str) -> float:
    if not _INTERVAL_RE.match(interval):
        raise PubFileError(
            f"`interval` value {interval!r} doesn't match ^\\d+(\\.\\d+)?(ms|s|m)$"
        )
    num_match = re.match(r"^\d+(\.\d+)?", interval)
    value = float(num_match.group())
    unit = interval[len(num_match.group()):]
    return value * _INTERVAL_UNIT_SCALE[unit]


def _wrap_envelope(src: str, data: dict) -> dict:
    return {
        "v": 1,
        "corrId": _next_corr_id(),
        "ts": int(time.time() * 1000),
        "src": src,
        "data": data,
    }


def _validate(topic: str, data: dict) -> None:
    schema_id = resolve_schema_id(topic)
    if schema_id is None or schema_id not in _MERGED_SCHEMAS:
        return  # unknown topic: send raw, no error
    try:
        jsonschema.validate(data, _MERGED_SCHEMAS[schema_id])
    except ValidationError as exc:
        raise PubFileError(
            f"payload for {topic!r} (schema {schema_id!r}) failed validation: {exc.message}"
        ) from exc


def _build_spec(
    path: Path,
    context: str,
    topic: Any,
    data: Any,
    qos: int,
    retain: bool,
    envelope_override: Optional[bool],
    src: str,
) -> PublishSpec:
    if not isinstance(topic, str) or not topic:
        raise PubFileError(f"{path}: {context}: 'topic' is required and must be a non-empty string")
    if data is None:
        raise PubFileError(f"{path}: {context}: 'data' is required")
    if not isinstance(data, dict):
        raise PubFileError(f"{path}: {context}: 'data' must be a mapping, got {data!r}")

    _validate(topic, data)
    wrap = _should_envelope(topic, envelope_override)
    payload = _wrap_envelope(src, data) if wrap else data
    return PublishSpec(topic=topic, payload=payload, qos=qos, retain=retain)


@dataclass(frozen=True)
class PubFile:
    specs: list[PublishSpec]
    interval_ms: Optional[float]


def load_pubfile(path: Path, src: str = "cli") -> PubFile:
    """Load+validate one pub YAML file, returning one PublishSpec per
    message to send in order (plus the optional inter-send interval).
    Raises PubFileError on any malformed input or schema validation failure
    -- always before anything is sent."""
    with Path(path).open("r", encoding="utf-8") as fh:
        raw = yaml.safe_load(fh)

    if not isinstance(raw, dict):
        raise PubFileError(f"{path}: top-level YAML must be a mapping")

    qos = raw.get("qos", 1)
    retain = raw.get("retain", False)
    envelope_override = raw.get("envelope")
    interval_ms = parse_interval_ms(raw["interval"]) if "interval" in raw else None

    if "messages" in raw:
        if "topic" in raw or "data" in raw:
            raise PubFileError(f"{path}: use either 'messages' or top-level 'topic'/'data', not both")
        messages = raw["messages"]
        if not isinstance(messages, list) or not messages:
            raise PubFileError(f"{path}: 'messages' must be a non-empty list")
        specs = []
        for idx, message in enumerate(messages):
            context = f"messages[{idx}]"
            if not isinstance(message, dict):
                raise PubFileError(f"{path}: {context} must be a mapping, got {message!r}")
            specs.append(_build_spec(
                path,
                context,
                message.get("topic"),
                message.get("data"),
                message.get("qos", qos),
                message.get("retain", retain),
                message.get("envelope", envelope_override),
                src,
            ))
        return PubFile(specs=specs, interval_ms=interval_ms)

    topic = raw.get("topic")
    data = raw.get("data")
    items: list[Any] = data if isinstance(data, list) else [data]
    specs = []
    for idx, item in enumerate(items):
        specs.append(_build_spec(path, f"data[{idx}]", topic, item, qos, retain, envelope_override, src))
    return PubFile(specs=specs, interval_ms=interval_ms)


__all__ = [
    "PubFile",
    "PubFileError",
    "PublishSpec",
    "load_pubfile",
    "parse_interval_ms",
    "resolve_schema_id",
]
