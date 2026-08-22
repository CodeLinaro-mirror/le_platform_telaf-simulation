#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""`sml.tools` dispatcher -- routes `sml <tool> [args...]` to the matching
tool package under `sml/tools/<tool>/`.

    sml <tool> [args...]                    # via the `sml` shell wrapper
    python3 -m sml.tools <tool> [args...]   # equivalent, no wrapper needed

A "tool" is any immediate subdirectory of `sml/tools/` that has a
`__main__.py` exposing `main(argv: list[str] | None = None) -> int`. New
tools need no registration here -- dropping a conforming package under
`sml/tools/` makes it show up automatically. `sml` (or any unrecognized
tool name) with no/bad args prints the discovered tool list.

Existing per-tool invocations (e.g. `python3 -m sml.tools.mqttcli sub`)
are unaffected -- this module only adds a shared front door, it does not
change how any tool package works.
"""
from __future__ import annotations

import importlib
import sys
from pathlib import Path

_TOOLS_DIR = Path(__file__).resolve().parent


def discover_tools() -> list[str]:
    """Return sorted names of immediate sml/tools/<name>/ packages that
    expose a __main__.py, i.e. every tool `sml <name>` can dispatch to."""
    names = []
    for entry in _TOOLS_DIR.iterdir():
        if entry.is_dir() and (entry / "__main__.py").exists():
            names.append(entry.name)
    return sorted(names)


def _print_usage(tools: list[str]) -> None:
    print("Usage: sml <tool> [args...]")
    print()
    print("Available tools:")
    for name in tools:
        print(f"  {name}")


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    tools = discover_tools()

    if not argv:
        _print_usage(tools)
        return 0

    tool_name, tool_argv = argv[0], argv[1:]
    if tool_name not in tools:
        print(f"ERROR: unknown tool {tool_name!r}", file=sys.stderr)
        print(file=sys.stderr)
        _print_usage(tools)
        return 1

    tool_main = importlib.import_module(f"sml.tools.{tool_name}.__main__").main
    return tool_main(tool_argv)


if __name__ == "__main__":
    sys.exit(main())
