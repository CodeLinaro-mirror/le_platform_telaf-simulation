#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Generate generated/python/{ctrl_topics,ctrl_payloads,ctrl_validators,
action_registry}.py from sml/control/registry/*.yaml +
schemas/**/*.json.

Per Sim-Test-Contract-Spec.md §8: Python-only output, no C++ (PA never
touches ctrl/cmd/**). `generated/` is not checked into git.

Usage:
    python3 sml/control/tools/gen_python.py
"""
from __future__ import annotations

import sys
from pathlib import Path

from jinja2 import Environment, FileSystemLoader

from schema_common import CONTRACT_ROOT, build_action_entries, build_messages, load_registries

TEMPLATES_DIR = Path(__file__).resolve().parent / "templates"
OUT_DIR = CONTRACT_ROOT.parent / "generated" / "python"


def main() -> int:
    registries = load_registries()
    messages = build_messages(registries)
    entries = build_action_entries(registries)

    env = Environment(loader=FileSystemLoader(str(TEMPLATES_DIR)),
                       trim_blocks=True, lstrip_blocks=True)
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    (OUT_DIR / "ctrl_topics.py").write_text(
        env.get_template("ctrl_topics.py.j2").render(registries=registries)
    )
    (OUT_DIR / "ctrl_payloads.py").write_text(
        env.get_template("ctrl_payloads.py.j2").render(messages=messages)
    )
    (OUT_DIR / "ctrl_validators.py").write_text(
        env.get_template("ctrl_validators.py.j2").render(messages=messages)
    )
    (OUT_DIR / "action_registry.py").write_text(
        env.get_template("action_registry.py.j2").render(entries=entries)
    )

    print(f"generated: {OUT_DIR}/ctrl_topics.py, ctrl_payloads.py, "
          f"ctrl_validators.py, action_registry.py "
          f"({len(registries['scenarios'])} scenario registry, "
          f"{len(registries['actions'])} action domain(s), "
          f"{len(messages)} message(s), {len(entries)} action(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
