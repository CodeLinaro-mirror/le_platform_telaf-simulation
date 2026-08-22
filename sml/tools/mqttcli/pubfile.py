# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""`sml.tools.mqttcli pub` YAML command loader.

One file = one command: {topic, qos?, retain?, envelope?, interval?, data}.
`data` may be a single object (one publish) or a list (a sequence, fired in
order with optional `interval` delay between sends). Envelope wrapping
follows the prefix rule (ap/req/* wraps, ctrl/cmd/* raw, unknown wraps)
unless the file's `envelope:` key overrides it. Known topics are validated
against the generated JSON Schemas before anything is sent -- a bad payload
must never partially publish a sequence.
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
from generated.python.validators import SCHEMAS as PROD_SCHEMAS
from sml.mpss.envelope import _next_corr_id, resolve_schema_id

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
    return True  # ap/req/* and unknown topics wrap


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
    except jsonschema.ValidationError as exc:
        raise PubFileError(
            f"payload for {topic!r} (schema {schema_id!r}) failed validation: {exc.message}"
        ) from exc


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

    topic = raw.get("topic")
    if not isinstance(topic, str) or not topic:
        raise PubFileError(f"{path}: 'topic' is required and must be a non-empty string")

    qos = raw.get("qos", 1)
    retain = raw.get("retain", False)
    envelope_override = raw.get("envelope")
    data = raw.get("data")

    if data is None:
        raise PubFileError(f"{path}: 'data' is required")

    items: list[Any] = data if isinstance(data, list) else [data]
    for item in items:
        if not isinstance(item, dict):
            raise PubFileError(f"{path}: each 'data' entry must be a mapping, got {item!r}")

    interval_ms = parse_interval_ms(raw["interval"]) if "interval" in raw else None

    wrap = _should_envelope(topic, envelope_override)
    specs = []
    for item in items:
        _validate(topic, item)
        payload = _wrap_envelope(src, item) if wrap else item
        specs.append(PublishSpec(topic=topic, payload=payload, qos=qos, retain=retain))
    return PubFile(specs=specs, interval_ms=interval_ms)


__all__ = [
    "PubFile",
    "PubFileError",
    "PublishSpec",
    "load_pubfile",
    "parse_interval_ms",
    "resolve_schema_id",
]
