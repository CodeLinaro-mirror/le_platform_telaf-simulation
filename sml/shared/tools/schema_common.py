#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Shared helpers for gen_python.py / gen_cpp.py.

Loads every registry/*.yaml plus the schemas/**/*.json they reference and
flattens them into the two structures both generators render from:
    - `registries`: list of {domain, rpcs, indications} dicts (topics.py.j2
      / topics.h.j2 iterate this directly, unmodified from validate_registry.py's
      view of the YAML).
    - `messages`: one entry per req/rsp/ind payload, each carrying
      `topic_id` (dot-form, e.g. "data.start_data_call.req"), `class_name`
      (PascalCase, e.g. "DataStartDataCallReq"), `title`, `schema` (raw
      JSON Schema dict) and `fields` (list of {name, type, required} for
      the dataclass/struct emitters -- `type` is the *target-language*
      type already resolved by `_py_type`/`_cpp_type`).

Known simplification (documented in payloads.h.j2's own header comment):
  nested "object" and "array of object" schema fields collapse to an
  opaque dict (Python) / nlohmann::json (C++) rather than a recursively
  generated nested struct. Every schema in schemas/data/ that has such a
  field (list_data_call_rsp, call_state_ind, query_profile_rsp, ...) is
  read fine by hand-written code that treats them as JSON objects anyway,
  so this does not block PA/MPSS from consuming the generated dataclasses.
"""
from __future__ import annotations

import json
import re
from pathlib import Path

import yaml

CONTRACT_ROOT = Path(__file__).resolve().parent.parent

_SNAKE_TOKEN_RE = re.compile(r"[^a-zA-Z0-9]+")


def to_pascal_case(snake: str) -> str:
    return "".join(w.capitalize() for w in _SNAKE_TOKEN_RE.split(snake) if w)


def load_registries() -> list[dict]:
    registry_dir = CONTRACT_ROOT / "registry"
    out = []
    for path in sorted(registry_dir.glob("*.yaml")):
        doc = yaml.safe_load(path.read_text())
        doc.setdefault("rpcs", [])
        doc.setdefault("indications", [])
        out.append(doc)
    return out


def _py_field_type(spec: dict) -> str:
    t = spec.get("type")
    if t == "integer":
        return "int"
    if t == "string":
        return "str"
    if t == "boolean":
        return "bool"
    if t == "array":
        items = spec.get("items", {})
        if items.get("type") == "object":
            return "list[dict]"
        return f"list[{_py_field_type(items)}]" if items else "list"
    if t == "object":
        return "dict"
    return "object"


def _cpp_field_type(spec: dict) -> str:
    t = spec.get("type")
    if t == "integer":
        return "int64_t"
    if t == "string":
        return "std::string"
    if t == "boolean":
        return "bool"
    if t == "array":
        items = spec.get("items", {})
        if items.get("type") == "object":
            return "nlohmann::json"
        return f"std::vector<{_cpp_field_type(items)}>" if items else "nlohmann::json"
    if t == "object":
        return "nlohmann::json"
    return "nlohmann::json"


def _fields_from_schema(schema: dict, type_fn) -> list[dict]:
    required = set(schema.get("required", []))
    fields = []
    for name, spec in (schema.get("properties") or {}).items():
        fields.append({
            "name": name,
            "type": type_fn(spec),
            "required": name in required,
        })
    # Python dataclasses (and, for symmetry, the C++ struct emitter) require
    # every required (non-defaulted) field before any optional one -- JSON
    # Schema's `properties` has no such ordering constraint, so re-sort here
    # rather than relying on schema authors to happen to list fields in the
    # right order (see start_data_call_req.json's `ifname`, which sits
    # between two required fields in the JSON source).
    fields.sort(key=lambda f: not f["required"])
    return fields


def build_messages(registries: list[dict], lang: str) -> list[dict]:
    """lang: 'python' or 'cpp' -- selects the field-type mapping."""
    type_fn = _py_field_type if lang == "python" else _cpp_field_type
    messages: list[dict] = []
    seen_topic_ids: set[str] = set()

    def _add(domain: str, name: str, suffix: str, payload_rel_path: str) -> None:
        topic_id = f"{domain}.{name}.{suffix}"
        if topic_id in seen_topic_ids:
            return
        seen_topic_ids.add(topic_id)
        schema_path = CONTRACT_ROOT / payload_rel_path
        schema = json.loads(schema_path.read_text())
        messages.append({
            "topic_id": topic_id,
            "class_name": to_pascal_case(f"{domain}_{name}_{suffix}"),
            "title": schema.get("title", topic_id),
            "schema": schema,
            "fields": _fields_from_schema(schema, type_fn),
        })

    for reg in registries:
        domain = reg["domain"]
        for r in reg["rpcs"]:
            _add(domain, r["method"], "req", r["req_payload"])
            _add(domain, r["method"], "rsp", r["rsp_payload"])
        for ind in reg["indications"]:
            _add(domain, ind["event"], "ind", ind["payload"])

    return messages


__all__ = ["build_messages", "load_registries", "to_pascal_case", "CONTRACT_ROOT"]
