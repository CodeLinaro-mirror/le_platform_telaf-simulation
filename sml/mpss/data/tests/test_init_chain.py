# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Regression guard: the D4 shape's Off -> Operating(->Starting->Ready)
chain must land on ``smfn_ready`` within a bounded time when the AO is
started.  If a future flattening drops the ``StartingDone`` bridge or
turns ``smfn_ready`` back into a sibling of ``smfn_starting`` under a
handler that trans() to it via INIT, this test fails loudly rather
than hanging CI.

Covers T-06 / T-07 / T-08 sub-AOs.  ``DataSubsystem`` itself has no
Starting substate (T-09) and is exercised by ``test_subsystem.py``.
"""
from __future__ import annotations

from unittest.mock import MagicMock

from sml.mpss.data.serving_system import DataServingSystemAO
from sml.mpss.data.profile import DataProfileAO
from sml.mpss.data.connection import DataConnectionAO
from sml.mpss.data.tests._helpers import wait_for_state
from sml.config.models import (
    CallTimingPresetSeed,
    InterfacePresetSeed,
    IpConfigSeed,
)


def test_serving_system_ao_reaches_ready():
    ao = DataServingSystemAO(slot=1, mpss_src="mpss-dev-1")
    ao.start(MagicMock(), MagicMock(), MagicMock())
    wait_for_state(ao, "smfn_ready", timeout=1.0)


def test_profile_ao_reaches_ready():
    ao = DataProfileAO(slot=1, seed_profiles=[], mpss_src="mpss-dev-1")
    ao.start(MagicMock(), MagicMock(), MagicMock())
    wait_for_state(ao, "smfn_ready", timeout=1.0)


def test_connection_ao_reaches_ready():
    ao = DataConnectionAO(
        slot=1,
        interface_preset=InterfacePresetSeed(ifname_prefix="rmnet_data", ifname_pool_size=4),
        call_timing_preset=CallTimingPresetSeed(),
        ip_config=IpConfigSeed(),
        mpss_src="mpss-dev-1",
    )
    ao.start(MagicMock(), MagicMock(), MagicMock())
    wait_for_state(ao, "smfn_ready", timeout=1.0)
