#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Lint sml/simula_shared/registry/*.yaml against the spec in
Sim-Shared-Contract-and-CodeGen.md §4 and §14.1.

Checks:
    - top-level `domain` key present
    - every RPC has method / req_topic / rsp_topic / req_payload / rsp_payload
    - every indication has event / ind_topic / payload
    - `rpcs` and `indications` are not both empty
    - topic prefixes match ap/req/ , mp/rsp/ , mp/ind/
    - method/event names are snake_case, topic segments have no wildcards
    - every req_payload/rsp_payload/payload path exists under
      sml/simula_shared/ and parses as JSON
    - no duplicate topic strings within a single registry file

Exit code 0 = all registries clean; 1 = at least one error found.

Usage:
    python3 sml/simula_shared/tools/validate_registry.py
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

_REQ_PREFIX = "ap/req/"
_RSP_PREFIX = "mp/rsp/"
_IND_PREFIX = "mp/ind/"


class RegistryError(RuntimeError):
    """One or more validation errors found; message holds all of them."""


def _check_topic(topic: str, expected_prefix: str, domain: str, errs: list[str]) -> None:
    if not isinstance(topic, str) or not topic:
        errs.append(f"topic must be a non-empty string, got {topic!r}")
        return
    if not topic.startswith(expected_prefix):
        errs.append(f"topic {topic!r} must start with {expected_prefix!r}")
    if not topic.startswith(f"{expected_prefix}{domain}/"):
        errs.append(f"topic {topic!r} must start with {expected_prefix}{domain}/")
    bad = _WILDCARD_CHARS & set(topic)
    if bad:
        errs.append(f"topic {topic!r} contains MQTT wildcard(s) {sorted(bad)}")
    if " " in topic:
        errs.append(f"topic {topic!r} contains a space")


def _check_name(name: str, kind: str, errs: list[str]) -> None:
    if not isinstance(name, str) or not _SNAKE_RE.match(name):
        errs.append(f"{kind} name {name!r} must be snake_case")


def _check_payload_path(rel_path: str, field: str, errs: list[str]) -> None:
    if not isinstance(rel_path, str) or not rel_path:
        errs.append(f"{field} must be a non-empty string, got {rel_path!r}")
        return
    p = CONTRACT_ROOT / rel_path
    if not p.exists():
        errs.append(f"{field} {rel_path!r} does not exist (resolved: {p})")
        return
    try:
        json.loads(p.read_text())
    except json.JSONDecodeError as exc:
        errs.append(f"{field} {rel_path!r} is not valid JSON: {exc}")


def validate_registry_file(path: Path) -> list[str]:
    """Return a list of error strings (empty = file is clean)."""
    errs: list[str] = []
    try:
        doc = yaml.safe_load(path.read_text())
    except yaml.YAMLError as exc:
        return [f"{path.name}: YAML parse error: {exc}"]

    if not isinstance(doc, dict):
        return [f"{path.name}: top-level document must be a mapping"]

    domain = doc.get("domain")
    if not isinstance(domain, str) or not domain:
        errs.append(f"{path.name}: missing or empty top-level `domain`")
        domain = domain or ""

    rpcs = doc.get("rpcs") or []
    indications = doc.get("indications") or []
    if not rpcs and not indications:
        errs.append(f"{path.name}: `rpcs` and `indications` are both empty")

    seen_topics: set[str] = set()

    for i, r in enumerate(rpcs):
        ctx = f"{path.name}: rpcs[{i}]"
        if not isinstance(r, dict):
            errs.append(f"{ctx}: entry must be a mapping")
            continue
        for field in ("method", "req_topic", "rsp_topic", "req_payload", "rsp_payload"):
            if field not in r:
                errs.append(f"{ctx}: missing required field `{field}`")
        if "method" in r:
            _check_name(r["method"], f"{ctx} method", errs)
        if "req_topic" in r:
            _check_topic(r["req_topic"], _REQ_PREFIX, domain, errs)
            if r["req_topic"] in seen_topics:
                errs.append(f"{ctx}: duplicate topic {r['req_topic']!r}")
            seen_topics.add(r["req_topic"])
        if "rsp_topic" in r:
            _check_topic(r["rsp_topic"], _RSP_PREFIX, domain, errs)
            if r["rsp_topic"] in seen_topics:
                errs.append(f"{ctx}: duplicate topic {r['rsp_topic']!r}")
            seen_topics.add(r["rsp_topic"])
        if "req_payload" in r:
            _check_payload_path(r["req_payload"], f"{ctx} req_payload", errs)
        if "rsp_payload" in r:
            _check_payload_path(r["rsp_payload"], f"{ctx} rsp_payload", errs)
        qos = r.get("qos", 1)
        if qos not in (0, 1, 2):
            errs.append(f"{ctx}: qos must be 0, 1, or 2, got {qos!r}")

    for i, ind in enumerate(indications):
        ctx = f"{path.name}: indications[{i}]"
        if not isinstance(ind, dict):
            errs.append(f"{ctx}: entry must be a mapping")
            continue
        for field in ("event", "ind_topic", "payload"):
            if field not in ind:
                errs.append(f"{ctx}: missing required field `{field}`")
        if "event" in ind:
            _check_name(ind["event"], f"{ctx} event", errs)
        if "ind_topic" in ind:
            _check_topic(ind["ind_topic"], _IND_PREFIX, domain, errs)
            if ind["ind_topic"] in seen_topics:
                errs.append(f"{ctx}: duplicate topic {ind['ind_topic']!r}")
            seen_topics.add(ind["ind_topic"])
        if "payload" in ind:
            _check_payload_path(ind["payload"], f"{ctx} payload", errs)
        qos = ind.get("qos", 1)
        if qos not in (0, 1, 2):
            errs.append(f"{ctx}: qos must be 0, 1, or 2, got {qos!r}")
        retain = ind.get("retain", False)
        if not isinstance(retain, bool):
            errs.append(f"{ctx}: retain must be a bool, got {retain!r}")

    return errs


def main() -> int:
    if not REGISTRY_DIR.is_dir():
        print(f"registry dir not found: {REGISTRY_DIR}", file=sys.stderr)
        return 1

    registry_files = sorted(REGISTRY_DIR.glob("*.yaml"))
    if not registry_files:
        print(f"no registry/*.yaml files found under {REGISTRY_DIR}", file=sys.stderr)
        return 1

    all_errs: list[str] = []
    for path in registry_files:
        errs = validate_registry_file(path)
        if errs:
            all_errs.extend(errs)
        else:
            print(f"OK: {path.name}")

    if all_errs:
        print("\nFAILED:", file=sys.stderr)
        for e in all_errs:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print(f"\nsim-shared-lint: {len(registry_files)} registry file(s) clean")
    return 0


if __name__ == "__main__":
    sys.exit(main())
