#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Lint sml/simula_test/registry/*.yaml against Sim-Test-Contract-Spec.md
§10.

Checks (per-file, §10.1):
    - top-level `kind` is `scenario` or `action`
    - `kind: scenario`: every command has method/kind/req_topic/req_payload;
      `kind: rpc` commands additionally need rsp_topic/rsp_payload
    - `kind: action`: top-level `domain` present; every action has
      method/req_topic/req_payload
    - topic prefix matches ctrl/cmd/scenario/ or ctrl/cmd/action/<domain>/
    - method/domain names are snake_case, topics have no wildcards/spaces
    - req_payload/rsp_payload paths exist under sml/simula_test/ and parse
      as JSON
    - no duplicate topic strings within a single registry file
    - action registry filename matches its own `domain` field

Cross-registry checks (§10.2):
    - no duplicate topic string across registry files
    - no duplicate canonical action name (`<domain>.<method>`) across files

Exit code 0 = all registries clean; 1 = at least one error found.

Usage:
    python3 sml/simula_test/tools/validate_registry.py
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import yaml

CONTRACT_ROOT = Path(__file__).resolve().parent.parent
REGISTRY_DIR = CONTRACT_ROOT / "registry"

_SNAKE_RE = re.compile(r"^[a-z][a-z0-9_]*$")
_WILDCARD_CHARS = frozenset("+#")

_SCENARIO_PREFIX = "ctrl/cmd/scenario/"
_ACTION_PREFIX = "ctrl/cmd/action/"


def _check_topic(topic, expected_prefix: str, errs: list[str], ctx: str) -> None:
    if not isinstance(topic, str) or not topic:
        errs.append(f"{ctx}: topic must be a non-empty string, got {topic!r}")
        return
    if not topic.startswith(expected_prefix):
        errs.append(f"{ctx}: topic {topic!r} must start with {expected_prefix!r}")
    bad = _WILDCARD_CHARS & set(topic)
    if bad:
        errs.append(f"{ctx}: topic {topic!r} contains MQTT wildcard(s) {sorted(bad)}")
    if " " in topic:
        errs.append(f"{ctx}: topic {topic!r} contains a space")


def _check_name(name, kind: str, errs: list[str]) -> None:
    if not isinstance(name, str) or not _SNAKE_RE.match(name):
        errs.append(f"{kind} name {name!r} must be snake_case")


def _check_payload_path(rel_path, field: str, errs: list[str]) -> None:
    if not isinstance(rel_path, str) or not rel_path:
        errs.append(f"{field} must be a non-empty string, got {rel_path!r}")
        return
    p = CONTRACT_ROOT / rel_path
    if not p.exists():
        errs.append(f"{field} {rel_path!r} does not exist (resolved: {p})")
        return
    try:
        json.loads(p.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        errs.append(f"{field} {rel_path!r} is not valid JSON: {exc}")


def _validate_scenario_file(path: Path, doc: dict) -> tuple[list[str], set[str]]:
    errs: list[str] = []
    seen_topics: set[str] = set()
    commands = doc.get("commands") or []
    if not commands:
        errs.append(f"{path.name}: `commands` is empty")

    for i, c in enumerate(commands):
        ctx = f"{path.name}: commands[{i}]"
        if not isinstance(c, dict):
            errs.append(f"{ctx}: entry must be a mapping")
            continue
        for field in ("method", "kind", "req_topic", "req_payload"):
            if field not in c:
                errs.append(f"{ctx}: missing required field `{field}`")
        if "method" in c:
            _check_name(c["method"], f"{ctx} method", errs)
        cmd_kind = c.get("kind")
        if cmd_kind not in ("rpc", "oneway"):
            errs.append(f"{ctx}: kind must be 'rpc' or 'oneway', got {cmd_kind!r}")
        if cmd_kind == "rpc":
            for field in ("rsp_topic", "rsp_payload"):
                if field not in c:
                    errs.append(f"{ctx}: kind:rpc missing required field `{field}`")
        if "req_topic" in c:
            _check_topic(c["req_topic"], _SCENARIO_PREFIX, errs, ctx)
            if c["req_topic"] in seen_topics:
                errs.append(f"{ctx}: duplicate topic {c['req_topic']!r}")
            seen_topics.add(c["req_topic"])
        if "rsp_topic" in c:
            _check_topic(c["rsp_topic"], _SCENARIO_PREFIX, errs, ctx)
            if c["rsp_topic"] in seen_topics:
                errs.append(f"{ctx}: duplicate topic {c['rsp_topic']!r}")
            seen_topics.add(c["rsp_topic"])
        if "req_payload" in c:
            _check_payload_path(c["req_payload"], f"{ctx} req_payload", errs)
        if cmd_kind == "rpc" and "rsp_payload" in c:
            _check_payload_path(c["rsp_payload"], f"{ctx} rsp_payload", errs)
        qos = c.get("qos", 1)
        if qos not in (0, 1, 2):
            errs.append(f"{ctx}: qos must be 0, 1, or 2, got {qos!r}")

    return errs, seen_topics


def _validate_action_file(path: Path, doc: dict) -> tuple[list[str], set[str], set[str]]:
    errs: list[str] = []
    seen_topics: set[str] = set()
    canonical_names: set[str] = set()

    domain = doc.get("domain")
    if not isinstance(domain, str) or not domain:
        errs.append(f"{path.name}: missing or empty top-level `domain`")
        domain = domain or ""
    else:
        expected_stem = f"action_{domain}"
        if path.stem != expected_stem:
            errs.append(
                f"{path.name}: filename doesn't match domain {domain!r} "
                f"(expected {expected_stem}.yaml)"
            )

    actions = doc.get("actions") or []
    if not actions:
        errs.append(f"{path.name}: `actions` is empty")

    expected_prefix = f"{_ACTION_PREFIX}{domain}/"
    for i, a in enumerate(actions):
        ctx = f"{path.name}: actions[{i}]"
        if not isinstance(a, dict):
            errs.append(f"{ctx}: entry must be a mapping")
            continue
        for field in ("method", "req_topic", "req_payload"):
            if field not in a:
                errs.append(f"{ctx}: missing required field `{field}`")
        if "method" in a:
            _check_name(a["method"], f"{ctx} method", errs)
            canonical_names.add(f"{domain}.{a['method']}")
        if "req_topic" in a:
            _check_topic(a["req_topic"], expected_prefix, errs, ctx)
            if a["req_topic"] in seen_topics:
                errs.append(f"{ctx}: duplicate topic {a['req_topic']!r}")
            seen_topics.add(a["req_topic"])
        if "req_payload" in a:
            _check_payload_path(a["req_payload"], f"{ctx} req_payload", errs)
        qos = a.get("qos", 1)
        if qos not in (0, 1, 2):
            errs.append(f"{ctx}: qos must be 0, 1, or 2, got {qos!r}")

    return errs, seen_topics, canonical_names


def validate_registry_file(path: Path) -> tuple[list[str], set[str], set[str]]:
    """Return (errors, topics_used, canonical_action_names)."""
    try:
        doc = yaml.safe_load(path.read_text(encoding="utf-8"))
    except yaml.YAMLError as exc:
        return [f"{path.name}: YAML parse error: {exc}"], set(), set()

    if not isinstance(doc, dict):
        return [f"{path.name}: top-level document must be a mapping"], set(), set()

    kind = doc.get("kind")
    if kind == "scenario":
        errs, topics = _validate_scenario_file(path, doc)
        return errs, topics, set()
    if kind == "action":
        errs, topics, names = _validate_action_file(path, doc)
        return errs, topics, names
    return [f"{path.name}: top-level `kind` must be 'scenario' or 'action', got {kind!r}"], set(), set()


def main() -> int:
    if not REGISTRY_DIR.is_dir():
        print(f"registry dir not found: {REGISTRY_DIR}", file=sys.stderr)
        return 1

    registry_files = sorted(REGISTRY_DIR.glob("*.yaml"))
    if not registry_files:
        print(f"no registry/*.yaml files found under {REGISTRY_DIR}", file=sys.stderr)
        return 1

    all_errs: list[str] = []
    topic_owners: dict[str, str] = {}
    name_owners: dict[str, str] = {}

    for path in registry_files:
        errs, topics, names = validate_registry_file(path)
        if errs:
            all_errs.extend(errs)
        else:
            print(f"OK: {path.name}")

        for t in topics:
            if t in topic_owners and topic_owners[t] != path.name:
                all_errs.append(
                    f"cross-registry: topic {t!r} defined in both "
                    f"{topic_owners[t]} and {path.name}"
                )
            else:
                topic_owners[t] = path.name

        for n in names:
            if n in name_owners and name_owners[n] != path.name:
                all_errs.append(
                    f"cross-registry: canonical action name {n!r} defined in both "
                    f"{name_owners[n]} and {path.name}"
                )
            else:
                name_owners[n] = path.name

    if all_errs:
        print("\nFAILED:", file=sys.stderr)
        for e in all_errs:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print(f"\nsim-test-lint: {len(registry_files)} registry file(s) clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
