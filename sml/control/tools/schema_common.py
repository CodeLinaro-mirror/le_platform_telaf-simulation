#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Shared helpers for sml/simula_test/tools/gen_python.py.

Mirrors sml/simula_shared/tools/schema_common.py's `build_messages`
shape (topic_id / class_name / title / schema / fields), but flattened
over test_contract's two registry kinds (`scenario` commands, `action`
methods) instead of shared contract's uniform rpcs/indications. Also
builds the canonical-action-name -> {topic, payload class} table that
Sim-Test-Contract-Spec.md §7.3/§9 requires for action_registry.py.

Python-only per that spec §8.4 -- there is no cpp counterpart to this file.
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
    scenarios = []
    actions = []
    for path in sorted(registry_dir.glob("*.yaml")):
        doc = yaml.safe_load(path.read_text())
        if doc.get("kind") == "scenario":
            doc.setdefault("commands", [])
            scenarios.append(doc)
        elif doc.get("kind") == "action":
            doc.setdefault("actions", [])
            actions.append(doc)
    return {"scenarios": scenarios, "actions": actions}


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


def _fields_from_schema(schema: dict) -> list[dict]:
    required = set(schema.get("required", []))
    fields = []
    for name, spec in (schema.get("properties") or {}).items():
        fields.append({
            "name": name,
            "type": _py_field_type(spec),
            "required": name in required,
        })
    fields.sort(key=lambda f: not f["required"])
    return fields


def build_messages(registries: dict) -> list[dict]:
    """One entry per req/rsp payload across scenario commands and actions."""
    messages: list[dict] = []
    seen_topic_ids: set[str] = set()

    def _add(prefix: str, name: str, suffix: str, payload_rel_path: str) -> None:
        topic_id = f"{prefix}.{name}.{suffix}"
        if topic_id in seen_topic_ids:
            return
        seen_topic_ids.add(topic_id)
        schema_path = CONTRACT_ROOT / payload_rel_path
        schema = json.loads(schema_path.read_text())
        messages.append({
            "topic_id": topic_id,
            "class_name": to_pascal_case(f"{prefix}_{name}_{suffix}"),
            "title": schema.get("title", topic_id),
            "schema": schema,
            "fields": _fields_from_schema(schema),
        })

    for reg in registries["scenarios"]:
        for c in reg["commands"]:
            _add("scenario", c["method"], "req", c["req_payload"])
            if c["kind"] == "rpc":
                _add("scenario", c["method"], "rsp", c["rsp_payload"])

    for reg in registries["actions"]:
        domain = reg["domain"]
        for a in reg["actions"]:
            _add(f"action.{domain}", a["method"], "req", a["req_payload"])

    return messages


def build_action_entries(registries: dict) -> list[dict]:
    """canonical name (`<domain>.<method>`) -> topic + payload class name."""
    entries = []
    for reg in registries["actions"]:
        domain = reg["domain"]
        for a in reg["actions"]:
            entries.append({
                "canonical_name": f"{domain}.{a['method']}",
                "domain": domain,
                "method": a["method"],
                "topic": a["req_topic"],
                "class_name": to_pascal_case(f"action_{domain}_{a['method']}_req"),
                "schema_id": f"action.{domain}.{a['method']}.req",
            })
    return entries


__all__ = [
    "build_action_entries",
    "build_messages",
    "load_registries",
    "to_pascal_case",
    "CONTRACT_ROOT",
]
