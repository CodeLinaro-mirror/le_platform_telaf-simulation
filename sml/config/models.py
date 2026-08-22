# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Pydantic models for sml/config/{devices,environments,scenarios}/*.yaml.

Every model derives from `StrictModel` (`extra="forbid"`): a stray or
misspelled field in a config YAML is a validation error at load time, not
a silently-dropped value. Cross-reference validation (does
`sim_slots.sim_slot_0.active_profile_id` actually name a `data_profiles`
entry?) is not done here -- that is sml/runtime/loader.py's job, which
walks these parsed models with knowledge of which id fields reference
which collection.

Device instances (`Modem`, `SimSlot`) bake their world-state in directly --
each catalog entry already carries the exact `state` (and, for sim_slots,
the exact `installed_sim`/preset ids) it represents. A scenario's
`initial_state` is therefore just a list of instance ids to activate; there
is no separate override block and no `allowed_states` membership check --
the `ModemStateEnum`/`SimSlotStateEnum` types enforce validity structurally.
"""
from __future__ import annotations

from enum import Enum
from typing import Optional

from pydantic import BaseModel, ConfigDict


class StrictModel(BaseModel):
    model_config = ConfigDict(extra="forbid")


# ---------------------------------------------------------------------------
# devices/*.yaml -- Modem + its state enum
# ---------------------------------------------------------------------------

class ModemStateEnum(str, Enum):
    online = "online"
    offline = "offline"


class RatEnum(str, Enum):
    GSM = "GSM"
    WCDMA = "WCDMA"
    LTE = "LTE"
    NR5G = "NR5G"


class Modem(StrictModel):
    id: str
    type: str
    supported_rats: list[RatEnum]
    ecall_capable: bool
    state: ModemStateEnum


# ---------------------------------------------------------------------------
# devices/*.yaml -- SimSlot + its state enum + the physical slot it lives in
# ---------------------------------------------------------------------------

class SlotId(int, Enum):
    """telux wire `SlotId` (`getSlotId()`), 1-based. Hardware supports
    exactly two physical sim slots -- this enum is the structural
    constraint, not just a type hint."""
    slot_1 = 1
    slot_2 = 2


class SimSlotStateEnum(str, Enum):
    inserted = "inserted"
    removed = "removed"


class SimSlot(StrictModel):
    id: str
    physical_type: str
    slot_id: SlotId
    state: SimSlotStateEnum
    installed_sim: Optional[str] = None
    active_profile_id: Optional[str] = None
    interface_preset_id: Optional[str] = None
    call_timing_preset_id: Optional[str] = None
    ip_preset_id: Optional[str] = None


class SimCard(StrictModel):
    id: str
    iccid: str
    imsi: str
    home_plmn: str


class DataProfile(StrictModel):
    """An APN/auth seed. Owned by a sim_slot, not a physical sim_card --
    telux's `getSlotId()` scopes profiles per-slot, not per-card."""
    id: str
    mpss_profile_id: int
    apn: str
    ip_family: str
    auth_type: str
    username: str = ""
    password: str = ""
    tech_type: str = "3GPP"


class InterfacePreset(StrictModel):
    id: str
    ifname_prefix: str = "rmnet_data"
    ifname_pool_size: int = 8


class CallTimingPreset(StrictModel):
    id: str
    call_connect_delay_ms: int = 200
    call_disconnect_delay_ms: int = 100


class IpPreset(StrictModel):
    id: str
    ipv4_addr: str
    ipv4_gateway: str
    ipv4_dns_primary: str
    ipv4_dns_secondary: str = ""
    ipv4_mtu: int = 1500
    ipv4_subnet_mask: str = "255.255.255.0"
    ipv6_addr: str
    ipv6_gateway: str
    ipv6_dns_primary: str
    ipv6_dns_secondary: str = ""
    ipv6_mtu: int = 1500
    ipv6_prefix_len: int = 64


class BitRateByRatPreset(StrictModel):
    id: str
    rates: dict[str, "BitRateEntry"]


class BitRateEntry(StrictModel):
    maxTxRate: int
    maxRxRate: int


class ThroughputPreset(StrictModel):
    id: str
    profile_id: int
    ul_throughput: int
    ul_max_throughput: int
    ul_queue_size: int
    dl_throughput: int


class QosPreset(StrictModel):
    id: str
    profile_id: int
    qos_id: int
    tx_max_rate: int
    tx_min_rate: int
    rx_max_rate: int
    rx_min_rate: int


class ThrottlePreset(StrictModel):
    id: str
    profile_id: int
    apn: str
    ipv4_time: int
    ipv6_time: int
    mcc: str
    mnc: str


class DevicesDoc(StrictModel):
    version: str
    persistent: list[str] = []
    modems: list[Modem] = []
    sim_slots: list[SimSlot] = []
    sim_cards: list[SimCard] = []
    data_profiles: list[DataProfile] = []
    interface_presets: list[InterfacePreset] = []
    call_timing_presets: list[CallTimingPreset] = []
    ip_presets: list[IpPreset] = []
    bitrate_by_rat_presets: list[BitRateByRatPreset] = []
    throughput_presets: list[ThroughputPreset] = []
    qos_presets: list[QosPreset] = []
    throttle_presets: list[ThrottlePreset] = []


# ---------------------------------------------------------------------------
# environments/*.yaml
# ---------------------------------------------------------------------------

class Cell(StrictModel):
    id: str
    plmn: str
    rat: str
    default_rsrp_dbm: int


class SignalModel(StrictModel):
    id: str
    kind: str
    variance_db: int


class EnvironmentsDoc(StrictModel):
    version: str
    cells: list[Cell] = []
    signal_models: list[SignalModel] = []


# ---------------------------------------------------------------------------
# scenarios/*.yaml
# ---------------------------------------------------------------------------

class ScenarioSetup(StrictModel):
    devices_config: str
    environment_config: str
    duration: str


class RadioInitialState(StrictModel):
    serving_cell: str
    signal_model: str


class ScenarioInitialState(StrictModel):
    """List of baked device-instance ids to activate at load time.

    Each id names a fully-formed `Modem`/`SimSlot` catalog entry (see
    devices/*.yaml) -- state, installed_sim, and presets are already baked
    into that entry, so a scenario has nothing to override, only to select.
    """
    modems: list[str] = []
    sim_slots: list[str] = []
    radio: Optional[RadioInitialState] = None


class TimelineStep(StrictModel):
    at: str
    action: str
    args: dict = {}


class ScenarioDoc(StrictModel):
    version: str
    name: str
    setup: ScenarioSetup
    initial_state: ScenarioInitialState = ScenarioInitialState()
    timeline: list[TimelineStep] = []


# ---------------------------------------------------------------------------
# Seed dataclasses handed to sml/mpss/data/* sub-AOs after resolution.
# Moved here from the old sml/mpss/scenario.py per the Phase 1/2 split --
# loader.py resolves a SimSlot's active_profile_id/*_preset_id fields into
# these directly, no other hand-written mapping function involved.
# ---------------------------------------------------------------------------

class SeedProfile(StrictModel):
    profileId: int = 1
    apn: str = "internet"
    ipFamily: str = "IPV4V6"
    authType: str = "NONE"
    username: str = ""
    password: str = ""
    techType: str = "3GPP"
    isDefault: bool = True


class InterfacePresetSeed(StrictModel):
    ifname_prefix: str = "rmnet_data"
    ifname_pool_size: int = 8


class CallTimingPresetSeed(StrictModel):
    call_connect_delay_ms: int = 200
    call_disconnect_delay_ms: int = 100


class IpConfigSeed(StrictModel):
    ipv4_addr: str = "10.0.0.1"
    ipv4_gateway: str = "10.0.0.254"
    ipv4_dns_primary: str = "8.8.8.8"
    ipv4_dns_secondary: str = "8.8.4.4"
    ipv4_mtu: int = 1500
    ipv4_subnet_mask: str = "255.255.255.0"
    ipv6_addr: str = "2001:db8::1"
    ipv6_gateway: str = "2001:db8::fffe"
    ipv6_dns_primary: str = "2001:4860:4860::8888"
    ipv6_dns_secondary: str = ""
    ipv6_mtu: int = 1500
    ipv6_prefix_len: int = 64
