# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS Director entry point.

Loads config, starts the MqttClient AO, blocks until SIGTERM/SIGINT,
then asks the AO to stop and waits a short grace period.
"""
from __future__ import annotations

import logging
import signal
import sys
import threading
from pathlib import Path

from sml.mpss.mqtt_client import MqttClient
from sml.mpss.config import ConfigError, load_config
from sml.mpss.data import DataSubsystem
from sml.mpss import instrumentation as _instr
from sml.mpss.sim import SimSubsystem
from sml.runtime.action_dispatcher import ActionDispatcher
from sml.runtime.loader import (
    LoaderError,
    resolve_bitrate_by_rat,
    resolve_call_timing_preset,
    resolve_interface_preset,
    resolve_ip_config,
    resolve_qos_presets,
    resolve_seed_profiles,
    resolve_throttle_presets,
    resolve_throughput_presets,
)
from sml.runtime.scenario_runner import ScenarioRunner


_SHUTDOWN_GRACE_S = 2.0
_SML_ROOT = Path(__file__).resolve().parents[1]
# Named Docker volume (see up_simulation.sh's `/persist` mount) rather than
# _SML_ROOT, so persisted state doesn't land in the host bind-mount owned by
# the container's root user.
_PERSIST_ROOT = Path("/persist")


def _install_signal_handlers(shutdown: threading.Event) -> None:
    def _handler(signum, _frame):
        logging.getLogger("sml.mpss").info(
            "received signal %d, requesting shutdown", signum
        )
        shutdown.set()

    signal.signal(signal.SIGTERM, _handler)
    signal.signal(signal.SIGINT, _handler)


def main() -> int:
    try:
        cfg = load_config()
    except ConfigError as exc:
        print(f"ERROR: invalid mpss config: {exc}", file=sys.stderr)
        return 2

    logging.basicConfig(
        level=getattr(logging, cfg.debug.log_level.upper(), logging.INFO),
        format="%(asctime)s %(name)s %(levelname)s %(message)s",
    )
    log = logging.getLogger("sml.mpss")
    log.info("sml.mpss starting (client_id=%s, broker=%s:%d)",
             cfg.broker.client_id, cfg.broker.host, cfg.broker.port)

    # Resolve miros instrumentation mode (env > cfg.debug.instrumentation >
    # legacy live_spy > off) and publish it so every AO ctor picks it up via
    # _instr.apply_mode(self, _instr.current_mode()) -- the single mechanism, no side channel.
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
        runner = ScenarioRunner(action_dispatcher=dispatcher)
        try:
            runner.load(scenario_path)
        except LoaderError as exc:
            print(f"ERROR: failed to load scenario {cfg.scenario!r}: {exc}", file=sys.stderr)
            return 2

        # MPSS currently only manages the referenced sim slot whose SlotId
        # (telux getSlotId()) is 1 -- the DDS/permanent slot.
        TARGET_SLOT_ID = 1   # MPSS data domain currently manages SlotId 1 only
        target_slot_runtime = next(
            (s for s in runner.sim_slot_runtimes.values() if s.sim_slot.slot_id == TARGET_SLOT_ID),
            None,
        )
        if target_slot_runtime is None:
            log.warning("no referenced sim_slot with slot_id=%d; data domain seeds with defaults",
                        TARGET_SLOT_ID)
        persist_path = (
            _PERSIST_ROOT / "mpss" / "data" / "slot1" / "data_profiles.json"
            if "data_profiles" in runner.persistent else None
        )
        data_subsystem = DataSubsystem(
            slot_id=target_slot_runtime.sim_slot.slot_id if target_slot_runtime else TARGET_SLOT_ID,
            seed_profiles=resolve_seed_profiles(target_slot_runtime, runner.devices) if target_slot_runtime else [],
            interface_preset=resolve_interface_preset(target_slot_runtime) if target_slot_runtime else None,
            call_timing_preset=resolve_call_timing_preset(target_slot_runtime) if target_slot_runtime else None,
            ip_config=resolve_ip_config(target_slot_runtime) if target_slot_runtime else None,
            persist_path=persist_path,
            bitrate_by_rat=resolve_bitrate_by_rat(runner.devices),
            throughput_presets=resolve_throughput_presets(runner.devices),
            qos_presets=resolve_qos_presets(runner.devices),
            throttle_presets=resolve_throttle_presets(runner.devices),
        )
        dispatcher.register_domain("data", data_subsystem)
        client.register_subsystem(data_subsystem)

        sim_subsystem = SimSubsystem(
            # Same resolved SimSlotRuntime the data domain seeds from, so the
            # card the sim domain reports and the slot the data domain runs
            # calls on can never disagree. `installed_sim` is optional (a
            # `removed` slot legitimately has none) -> ABSENT/UNKNOWN card.
            installed_sim=target_slot_runtime.installed_sim if target_slot_runtime else None,
        )
        dispatcher.register_domain("sim", sim_subsystem)
        client.register_subsystem(sim_subsystem)

        client.register_subsystem(runner)
        log.info("scenario runner registered (mpss.scenario=%s)", cfg.scenario)
    else:
        log.info("no mpss.scenario configured; data/sim domains not started")

    client.start()

    # Block until a signal handler fires. wait() with no timeout lets
    # Python deliver signals to the main thread.
    while not shutdown.is_set():
        shutdown.wait(timeout=1.0)

    log.info("sml.mpss shutdown requested, asking AO to stop")
    client.request_stop()
    if not client.wait_until_stopped(timeout_s=_SHUTDOWN_GRACE_S):
        log.warning("AO did not stop cleanly within %.1fs; exiting anyway",
                    _SHUTDOWN_GRACE_S)
    log.info("sml.mpss stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
