# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Modem / SIM-slot World State.

`ModemRuntime`/`SimSlotRuntime` hold references to the catalog objects they
describe (a `Modem`/`SimSlot`/`SimCard`/`DataProfile`/preset instance from
`sml/config/models.py`), not bare id strings -- a consumer that already has
a `SimSlotRuntime` can read `.active_profile.apn` directly instead of
re-indexing into a devices document by id.

`sml.runtime.loader.resolve_initial_state()` is what builds these; nothing
else constructs them.
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

from sml.config.models import (
    CallTimingPreset,
    Cell,
    DataProfile,
    InterfacePreset,
    IpPreset,
    Modem,
    ModemStateEnum,
    SignalModel,
    SimCard,
    SimSlot,
    SimSlotStateEnum,
)


@dataclass
class ModemRuntime:
    """Example (from `modem_0_online` in devices.yaml)::

        ModemRuntime(
            modem=Modem(id="modem_0_online", type="primary", ...),
            state=ModemStateEnum.online,
        )
    """
    modem: Modem
    state: ModemStateEnum


@dataclass
class SimSlotRuntime:
    """Example (from `sim_slot_0_inserted_sim001` in devices.yaml)::

        SimSlotRuntime(
            sim_slot=SimSlot(id="sim_slot_0_inserted_sim001", slot_id=SlotId.slot_1, ...),
            state=SimSlotStateEnum.inserted,
            installed_sim=SimCard(id="sim_card_001", iccid="898601...", ...),
            active_profile=DataProfile(id="profile_internet", apn="internet", ...),
            interface_preset=None,
            call_timing_preset=None,
            ip_preset=None,
        )
    """
    sim_slot: SimSlot
    state: SimSlotStateEnum
    installed_sim: Optional[SimCard] = None
    active_profile: Optional[DataProfile] = None
    interface_preset: Optional[InterfacePreset] = None
    call_timing_preset: Optional[CallTimingPreset] = None
    ip_preset: Optional[IpPreset] = None


@dataclass
class RadioRuntime:
    """Example (from a scenario's initial_state.radio block)::

        RadioRuntime(
            serving_cell=Cell(id="cell_urban_A", plmn="46000", rat="LTE",
                               default_rsrp_dbm=-85),
            signal_model=SignalModel(id="stable_urban", kind="variance", variance_db=3),
        )

    Holds the resolved catalog objects, not bare id strings, mirroring
    ModemRuntime/SimSlotRuntime above. sml.runtime.loader.resolve_radio_seed()
    flattens this into the RadioSeed shape sml.mpss.radio's AOs consume.
    """
    serving_cell: Cell
    signal_model: SignalModel


__all__ = ["ModemRuntime", "SimSlotRuntime", "RadioRuntime"]
