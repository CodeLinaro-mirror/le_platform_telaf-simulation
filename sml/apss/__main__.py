# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""APSS Director entry point.

Mirrors `sml/mpss/__main__.py`'s wiring shape. Loads config, starts the
MqttClient AO with PowerSubsystem + ScenarioRunner registered, blocks until
SIGTERM/SIGINT, then asks the AO to stop and waits a short grace period.
"""
from __future__ import annotations

import logging
import signal
import sys
import threading
from pathlib import Path

from sml.common.mqtt_client import MqttClient
from sml.common.config import ConfigError, load_config
from sml.apss.power import PowerSubsystem
from sml.apss.power.action_dispatcher import PowerActionDispatcher
from sml.common import instrumentation as _instr
from sml.runtime.action_dispatcher import ActionDispatcher
from sml.runtime.loader import resolve_power_seed
from sml.runtime.scenario_runner import ScenarioRunner


_SHUTDOWN_GRACE_S = 2.0
_SML_ROOT = Path(__file__).resolve().parents[1]


def _install_signal_handlers(shutdown: threading.Event) -> None:
    def _handler(signum, _frame):
        logging.getLogger("sml.apss").info(
            "received signal %d, requesting shutdown", signum
        )
        shutdown.set()

    signal.signal(signal.SIGTERM, _handler)
    signal.signal(signal.SIGINT, _handler)


def main() -> int:
    try:
        cfg = load_config("apss", _SML_ROOT / "apss" / "config.yaml")
    except ConfigError as exc:
        print(f"ERROR: invalid apss config: {exc}", file=sys.stderr)
        return 2

    logging.basicConfig(
        level=getattr(logging, cfg.debug.log_level.upper(), logging.INFO),
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )
    log = logging.getLogger("sml.apss")
    log.info("sml.apss starting (client_id=%s, broker=%s:%d)",
             cfg.broker.client_id, cfg.broker.host, cfg.broker.port)

    try:
        mode = _instr.resolve_mode(cfg)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    _instr.set_current_mode(mode)
    log.info(_instr.describe(mode))

    shutdown = threading.Event()
    _install_signal_handlers(shutdown)

    client = MqttClient(cfg)

    if cfg.scenario:
        scenario_path = _SML_ROOT / cfg.scenario
        dispatcher = ActionDispatcher(domains={})
        runner = ScenarioRunner(action_dispatcher=dispatcher, process_name="apss")
        try:
            runner.load(scenario_path)
        except Exception as exc:  # noqa: BLE001 - LoaderError from sml.runtime.loader
            print(f"ERROR: failed to load scenario {cfg.scenario!r}: {exc}", file=sys.stderr)
            return 2

        power_seed = resolve_power_seed(runner.power_runtime)
        power_subsystem = PowerSubsystem(
            local_machine_name=power_seed.local_machine_name if power_seed else "mdm",
            timeout_ms=power_seed.timeout_ms if power_seed else 5000,
            ftimeout_ms=power_seed.ftimeout_ms if power_seed else 36_000_000,
            shutdown_trigger_en=power_seed.shutdown_trigger_en if power_seed else True,
            interconnect_supports_autosuspend=(
                power_seed.interconnect_supports_autosuspend if power_seed else False
            ),
        )
        if power_seed:
            for machine_name in power_seed.machines:
                power_subsystem.add_machine({"machineName": machine_name})

        dispatcher.register_domain("power", PowerActionDispatcher(power_subsystem))
        client.register_subsystem(power_subsystem)
        client.register_subsystem(runner)
        log.info("power domain + scenario runner registered (apss.scenario=%s)", cfg.scenario)
    else:
        power_subsystem = PowerSubsystem()
        client.register_subsystem(power_subsystem)
        log.info("no apss.scenario configured; power domain seeds with defaults")

    client.start()

    while not shutdown.is_set():
        shutdown.wait(timeout=1.0)

    log.info("sml.apss shutdown requested, asking AO to stop")
    client.request_stop()
    if not client.wait_until_stopped(timeout_s=_SHUTDOWN_GRACE_S):
        log.warning("AO did not stop cleanly within %.1fs; exiting anyway",
                    _SHUTDOWN_GRACE_S)
    log.info("sml.apss stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
