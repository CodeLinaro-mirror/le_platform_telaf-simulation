#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Static scenario validator: pydantic parse + cross-reference resolution,
no MQTT broker, no scenario_runner instantiation.

Replaces `simula_config/tools/simu_config_lint.py`'s cross-reference role.
Unlike that lint script (a generic engine driven by a separate `refs.yaml`
rule table), this calls the exact same `sml.runtime.loader` functions the
real runtime uses to load a scenario -- there is no second implementation
of "what references what" to keep in sync.

Timeline `action:` existence in `control/registry/action_*.yaml` and
required/declared `args` fields are NOT checked here (out of loader.py's
scope, which only resolves devices/environments references).

Usage:
    python3 sml/config/tools/validate_scenario.py [scenario.yaml ...]

With no arguments, validates every file under sml/config/scenarios/*.yaml.
Exit code 0 = all clean; 1 = at least one scenario failed to load.
"""
from __future__ import annotations

import sys
from pathlib import Path

CONFIG_ROOT = Path(__file__).resolve().parent.parent
SML_ROOT = CONFIG_ROOT.parent
sys.path.insert(0, str(SML_ROOT.parent))  # repo root, for `import sml.*`
sys.path.insert(0, str(SML_ROOT))         # sml/, for `import generated.*`

from sml.runtime.loader import LoaderError, load_devices_doc, load_environments_doc, load_scenario_doc, resolve_initial_state  # noqa: E402


def validate_scenario_file(path: Path) -> list[str]:
    try:
        scenario = load_scenario_doc(path)
    except LoaderError as exc:
        return [str(exc)]

    config_root = path.resolve().parent.parent  # scenarios/../ -> config/
    try:
        devices = load_devices_doc(config_root / scenario.setup.devices_config)
        environments = load_environments_doc(config_root / scenario.setup.environment_config)
        resolve_initial_state(scenario, devices, environments)
    except LoaderError as exc:
        return [str(exc)]
    return []


def main(argv: list[str]) -> int:
    if argv:
        paths = [Path(a) for a in argv]
    else:
        paths = sorted((CONFIG_ROOT / "scenarios").glob("*.yaml"))
    if not paths:
        print(f"no scenario files found under {CONFIG_ROOT / 'scenarios'}", file=sys.stderr)
        return 1

    all_errs: list[str] = []
    for path in paths:
        errs = validate_scenario_file(path)
        if errs:
            all_errs.extend(f"{path.name}: {e}" for e in errs)
        else:
            print(f"OK: {path.name}")

    if all_errs:
        print("\nFAILED:", file=sys.stderr)
        for e in all_errs:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print(f"\nvalidate-scenario: {len(paths)} scenario file(s) clean")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
