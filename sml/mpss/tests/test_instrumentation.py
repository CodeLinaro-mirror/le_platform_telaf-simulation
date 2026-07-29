# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for sml.mpss.instrumentation.resolve_mode and helpers."""
from __future__ import annotations

import textwrap
from pathlib import Path
from types import SimpleNamespace

import pytest

from sml.mpss import instrumentation as instr
from sml.mpss.config import ConfigError, DebugConfig, MpssConfig, load_config


def _cfg(instrumentation: str = "off", live_spy: bool = False) -> MpssConfig:
    return MpssConfig(debug=DebugConfig(instrumentation=instrumentation, live_spy=live_spy))


# --- resolve_mode ----------------------------------------------------------- #

def test_resolve_mode_default_is_off(monkeypatch):
    monkeypatch.delenv(instr.ENV_VAR, raising=False)
    assert instr.resolve_mode(_cfg()) == "off"


def test_resolve_mode_reads_config_field(monkeypatch):
    monkeypatch.delenv(instr.ENV_VAR, raising=False)
    assert instr.resolve_mode(_cfg(instrumentation="on")) == "on"
    assert instr.resolve_mode(_cfg(instrumentation="verbose")) == "verbose"


def test_resolve_mode_env_wins_over_config(monkeypatch):
    monkeypatch.setenv(instr.ENV_VAR, "verbose")
    assert instr.resolve_mode(_cfg(instrumentation="off")) == "verbose"


def test_resolve_mode_env_case_insensitive(monkeypatch):
    monkeypatch.setenv(instr.ENV_VAR, "VERBOSE")
    assert instr.resolve_mode(_cfg()) == "verbose"


def test_resolve_mode_invalid_env_raises(monkeypatch):
    monkeypatch.setenv(instr.ENV_VAR, "bogus")
    with pytest.raises(ValueError, match="invalid instrumentation mode"):
        instr.resolve_mode(_cfg())


def test_resolve_mode_legacy_live_spy_upgrades_to_verbose(monkeypatch):
    """live_spy=True with default instrumentation='off' auto-upgrades."""
    monkeypatch.delenv(instr.ENV_VAR, raising=False)
    assert instr.resolve_mode(_cfg(live_spy=True)) == "verbose"


def test_resolve_mode_explicit_config_beats_legacy_live_spy(monkeypatch):
    monkeypatch.delenv(instr.ENV_VAR, raising=False)
    # Explicit instrumentation="on" wins over live_spy=True.
    assert instr.resolve_mode(_cfg(instrumentation="on", live_spy=True)) == "on"


# --- apply_mode / describe -------------------------------------------------- #

def test_apply_mode_off_clears_flags():
    ao = SimpleNamespace(live_trace=True, live_spy=True)
    instr.apply_mode(ao, "off")
    assert ao.live_trace is False and ao.live_spy is False


def test_apply_mode_on_enables_trace_only():
    ao = SimpleNamespace(live_trace=False, live_spy=True)
    instr.apply_mode(ao, "on")
    assert ao.live_trace is True and ao.live_spy is False


def test_apply_mode_verbose_enables_both():
    ao = SimpleNamespace(live_trace=False, live_spy=False)
    instr.apply_mode(ao, "verbose")
    assert ao.live_trace is True and ao.live_spy is True


def test_describe_mentions_mode():
    for m in ("off", "on", "verbose"):
        assert m in instr.describe(m)


def test_current_mode_roundtrip():
    prev = instr.current_mode()
    try:
        instr.set_current_mode("verbose")
        assert instr.current_mode() == "verbose"
    finally:
        instr.set_current_mode(prev)


# --- config load validation ------------------------------------------------- #

def test_load_config_accepts_instrumentation_field(tmp_path: Path):
    cfg_file = tmp_path / "config.yaml"
    cfg_file.write_text(textwrap.dedent("""\
        mpss:
          debug:
            instrumentation: verbose
    """))
    cfg = load_config(cfg_file)
    assert cfg.debug.instrumentation == "verbose"


def test_load_config_rejects_invalid_instrumentation(tmp_path: Path):
    cfg_file = tmp_path / "config.yaml"
    cfg_file.write_text(textwrap.dedent("""\
        mpss:
          debug:
            instrumentation: chatty
    """))
    with pytest.raises(ConfigError, match="debug.instrumentation"):
        load_config(cfg_file)


def test_load_config_default_instrumentation_is_off():
    cfg = MpssConfig()
    assert cfg.debug.instrumentation == "off"


# --- F-02: apply_current_mode wired into every AO --------------------------- #

# Every AO ctor calls _instr.apply_mode(self, _instr.current_mode()), which reads the mode
# published by set_current_mode() at startup. These tests build a real AO of
# each type under each mode and assert miros' live_trace/live_spy landed.

import time

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
    from sml.mpss.mqtt_client import MqttClient
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
