#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""`sml.tools.alias` -- short-name shortcuts for common invocations.

    sml alias                        # list available aliases
    sml alias <name> [extra_args...] # run the aliased command

Each entry in aliases.yaml is exactly one of:

  <name>:                              # forwards to an sml/tools/<tool>
    tool: <tool-dir-name>               # package's main(argv), in-process
    args: [<fixed argv>]

  <name>:                              # runs an arbitrary command
    shell: "<command> [fixed args...]"  # via subprocess, no shell=True
    cwd: "<optional working directory>"

`tool` and `shell` are mutually exclusive on a given entry. `extra_args`
given on the command line are appended as literal argv elements after
the fixed args/command in both cases.
"""
from __future__ import annotations

import importlib
import shlex
import subprocess
import sys
from pathlib import Path

import yaml

_TOOLS_DIR = Path(__file__).resolve().parent.parent
_ALIASES_FILE = Path(__file__).resolve().parent / "aliases.yaml"


def load_aliases() -> dict:
    with _ALIASES_FILE.open("r", encoding="utf-8") as fh:
        return yaml.safe_load(fh) or {}


def _resolve_paths(args: list[str]) -> list[str]:
    """Fixed `tool` args in aliases.yaml are file paths written relative
    to sml/tools/ (e.g. mqttcli/commands/step.yaml) -- resolve them to
    absolute paths so aliases work regardless of the caller's cwd."""
    resolved = []
    for arg in args:
        candidate = _TOOLS_DIR / arg
        resolved.append(str(candidate) if candidate.is_file() else arg)
    return resolved


def _describe(entry: dict) -> str:
    if "tool" in entry:
        return "sml " + " ".join([entry["tool"], *entry.get("args", [])])
    return entry.get("shell", "<invalid alias entry>")


def _print_list(aliases: dict) -> None:
    print("Usage: sml alias <name> [extra_args...]")
    print()
    print("Available aliases:")
    for name in sorted(aliases):
        print(f"  {name} -> {_describe(aliases[name])}")


def _run_tool(entry: dict, extra_args: list[str]) -> int:
    tool_name = entry["tool"]
    tool_argv = [*_resolve_paths(entry.get("args", [])), *extra_args]
    tool_main = importlib.import_module(f"sml.tools.{tool_name}.__main__").main
    return tool_main(tool_argv)


def _run_shell(name: str, entry: dict, extra_args: list[str]) -> int:
    shell_cmd = entry["shell"]
    try:
        argv_cmd = shlex.split(shell_cmd) + extra_args
    except ValueError as exc:
        print(f"ERROR: alias {name!r}: malformed 'shell' command: {exc}", file=sys.stderr)
        return 1

    cwd = entry.get("cwd")
    if cwd is not None:
        cwd = str(_TOOLS_DIR / cwd) if not Path(cwd).is_absolute() else cwd

    try:
        result = subprocess.run(argv_cmd, cwd=cwd)
    except FileNotFoundError as exc:
        print(f"ERROR: alias {name!r}: {exc}", file=sys.stderr)
        return 1
    return result.returncode


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    aliases = load_aliases()

    if not argv:
        _print_list(aliases)
        return 0

    name, extra_args = argv[0], argv[1:]
    entry = aliases.get(name)
    if entry is None:
        print(f"ERROR: unknown alias {name!r}", file=sys.stderr)
        print(file=sys.stderr)
        _print_list(aliases)
        return 1

    has_tool = "tool" in entry
    has_shell = "shell" in entry
    if has_tool == has_shell:
        print(f"ERROR: alias {name!r} must have exactly one of 'tool' or 'shell'",
              file=sys.stderr)
        return 1

    if has_tool:
        return _run_tool(entry, extra_args)
    return _run_shell(name, entry, extra_args)


if __name__ == "__main__":
    sys.exit(main())
