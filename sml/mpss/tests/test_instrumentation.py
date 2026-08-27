# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""F-02: verify every MPSS-side AO wires sml.common.instrumentation correctly.

Pure instrumentation-module unit tests (resolve_mode/apply_mode/describe/
current_mode, config validation) live in sml/common/tests/test_instrumentation.py.
This suite builds a real AO of each MPSS type under each mode and asserts
miros' live_trace/live_spy landed as instrumentation dictates.
"""
from __future__ import annotations

import time

import pytest

from sml.common import instrumentation as instr
from sml.common.config import ProcessConfig as MpssConfig
from sml.config.models import CallTimingPresetSeed, InterfacePresetSeed, IpConfigSeed


def _make_connection_ao():
    from sml.mpss.data.connection import DataConnectionAO
    return DataConnectionAO(
        slot=1,
        interface_preset=InterfacePresetSeed(ifname_prefix="rmnet_data", ifname_pool_size=4),
        call_timing_preset=CallTimingPresetSeed(),
        ip_config=IpConfigSeed(),
        mpss_src="mpss-dev-1",
    )


def _make_profile_ao():
    from sml.mpss.data.profile import DataProfileAO
    return DataProfileAO(slot=1, seed_profiles=[], mpss_src="mpss-dev-1")


def _make_serving_system_ao():
    from sml.mpss.data.serving_system import DataServingSystemAO
    return DataServingSystemAO(slot=1, mpss_src="mpss-dev-1")


def _make_subsystem():
    from sml.mpss.data import DataSubsystem
    return DataSubsystem(slot_id=1)


def _make_scenario_runner():
    from sml.runtime.action_dispatcher import ActionDispatcher
    from sml.runtime.scenario_runner import ScenarioRunner
    return ScenarioRunner(action_dispatcher=ActionDispatcher(domains={}))


def _make_mqtt_client():
    from sml.common.mqtt_client import MqttClient
    return MqttClient(MpssConfig())


_AO_FACTORIES = [
    _make_connection_ao,
    _make_profile_ao,
    _make_serving_system_ao,
    _make_subsystem,
    _make_scenario_runner,
    _make_mqtt_client,
]


@pytest.fixture(autouse=True)
def _restore_mode():
    prev = instr.current_mode()
    yield
    instr.set_current_mode(prev)


@pytest.mark.parametrize("factory", _AO_FACTORIES)
@pytest.mark.parametrize("mode,trace,spy", [
    ("off", False, False),
    ("on", True, False),
    ("verbose", True, True),
])
def test_every_ao_applies_current_mode(factory, mode, trace, spy):
    instr.set_current_mode(mode)
    ao = factory()
    assert ao.live_trace is trace, f"{factory.__name__} live_trace under {mode}"
    assert ao.live_spy is spy, f"{factory.__name__} live_spy under {mode}"


def test_data_domain_ao_emits_trace_when_on(capsys):
    """Observed-not-asserted-only: a data-domain AO (Connection) actually
    emits trace lines on its live dispatch when instrumentation is 'on',
    mirroring the parent plan's Wave-1 'PA now emits trace' gate on the
    MPSS side."""
    from sml.mpss.data.connection import DataConnectionAO
    from sml.mpss.data.tests._helpers import wait_for_state
    from unittest.mock import MagicMock

    instr.set_current_mode("on")
    ao = _make_connection_ao()
    assert ao.live_trace is True
    ao.start(MagicMock(), MagicMock())
    wait_for_state(ao, "smfn_ready")
    time.sleep(0.05)
    out = capsys.readouterr().out
    # miros live_trace prints a "<-" transition line per RTC step; reaching
    # smfn_ready means at least the Off->Operating->Ready walk was traced.
    assert "smfn_ready" in out, f"no trace observed on stdout:\n{out}"
    ao.stop()
