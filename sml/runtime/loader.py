# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""YAML -> pydantic validation + cross-reference resolution.

Replaces the old `simula_config/refs.yaml` declarative rule table: instead
of a generic engine walking dotted paths against a separate rule file,
each `resolve_*` function here directly looks up the id it needs in the
relevant devices/environments catalog and raises :class:`LoaderError` if
that id doesn't exist. There is no silent default-value fallback -- an
unresolvable id is a load-time error, not a warning.
"""
from __future__ import annotations

import logging
from pathlib import Path
from typing import Optional, TypeVar

import pydantic
import yaml

from sml.config.models import (
    CallTimingPreset,
    CallTimingPresetSeed,
    DevicesDoc,
    EnvironmentsDoc,
    InterfacePreset,
    InterfacePresetSeed,
    IpConfigSeed,
    IpPreset,
    Modem,
    ScenarioDoc,
    SeedProfile,
    SimCard,
    SimSlot,
)
from sml.runtime.world_state import ModemRuntime, SimSlotRuntime

_T = TypeVar("_T")
_log = logging.getLogger("sml.runtime.loader")


class LoaderError(RuntimeError):
    """Raised for a YAML parse failure, schema violation, or unresolved id."""


def _read_yaml(path: Path) -> dict:
    try:
        raw = yaml.safe_load(path.read_text(encoding="utf-8"))
    except OSError as exc:
        raise LoaderError(f"cannot read {path}: {exc}") from exc
    except yaml.YAMLError as exc:
        raise LoaderError(f"failed to parse {path}: {exc}") from exc
    return raw or {}


def _validate(model: type[_T], raw: dict, path: Path) -> _T:
    try:
        return model.model_validate(raw)
    except pydantic.ValidationError as exc:
        raise LoaderError(f"{path}: {exc}") from exc


def load_devices_doc(path: Path) -> DevicesDoc:
    return _validate(DevicesDoc, _read_yaml(path), path)


def load_environments_doc(path: Path) -> EnvironmentsDoc:
    return _validate(EnvironmentsDoc, _read_yaml(path), path)


def load_scenario_doc(path: Path) -> ScenarioDoc:
    return _validate(ScenarioDoc, _read_yaml(path), path)


# ---------------------------------------------------------------------------
# Catalog lookups
# ---------------------------------------------------------------------------

def _index_by_id(items: list) -> dict:
    return {item.id: item for item in items}


def _lookup(index: dict, item_id: str, kind: str) -> object:
    entry = index.get(item_id)
    if entry is None:
        raise LoaderError(f"no matching {kind} with id {item_id!r}")
    return entry


# ---------------------------------------------------------------------------
# initial_state resolution
# ---------------------------------------------------------------------------

def resolve_initial_state(
    scenario: ScenarioDoc,
    devices: DevicesDoc,
    environments: EnvironmentsDoc,
) -> tuple[dict[str, ModemRuntime], dict[str, SimSlotRuntime]]:
    """Resolve `scenario.initial_state` against `devices`/`environments`.

    Returns `(modem_runtimes, sim_slot_runtimes)` keyed by their catalog id.
    `scenario.initial_state.modems`/`sim_slots` are just lists of instance
    ids -- each id's `Modem`/`SimSlot` catalog entry already carries its
    own baked `state` (and, for sim_slots, `installed_sim`/preset ids), so
    there is nothing to override here, only to look up and carry across.
    Raises :class:`LoaderError` on any id that doesn't resolve.

    Example return value (single online modem, single inserted sim_slot)::

        (
            {"modem_0_online": ModemRuntime(modem=Modem(id="modem_0_online", ...),
                                             state=ModemStateEnum.online)},
            {"sim_slot_0_inserted_sim001": SimSlotRuntime(
                sim_slot=SimSlot(id="sim_slot_0_inserted_sim001", ...),
                state=SimSlotStateEnum.inserted,
                installed_sim=SimCard(id="sim_card_001", ...),
                active_profile=None, interface_preset=None,
                call_timing_preset=None, ip_preset=None)},
        )
    """
    modems_idx = _index_by_id(devices.modems)
    sim_slots_idx = _index_by_id(devices.sim_slots)
    sim_cards_idx = _index_by_id(devices.sim_cards)
    data_profiles_idx = _index_by_id(devices.data_profiles)
    interface_presets_idx = _index_by_id(devices.interface_presets)
    call_timing_presets_idx = _index_by_id(devices.call_timing_presets)
    ip_presets_idx = _index_by_id(devices.ip_presets)
    cells_idx = _index_by_id(environments.cells)
    signal_models_idx = _index_by_id(environments.signal_models)

    modem_runtimes: dict[str, ModemRuntime] = {}
    for modem_id in scenario.initial_state.modems:
        modem: Modem = _lookup(modems_idx, modem_id, "devices.modems")
        modem_runtimes[modem_id] = ModemRuntime(modem=modem, state=modem.state)

    sim_slot_runtimes: dict[str, SimSlotRuntime] = {}
    for slot_id in scenario.initial_state.sim_slots:
        sim_slot: SimSlot = _lookup(sim_slots_idx, slot_id, "devices.sim_slots")

        installed_sim: Optional[SimCard] = None
        if sim_slot.installed_sim is not None:
            installed_sim = _lookup(sim_cards_idx, sim_slot.installed_sim, "devices.sim_cards")

        active_profile = _resolve_optional(
            sim_slot.active_profile_id, data_profiles_idx, "devices.data_profiles",
        )
        interface_preset = _resolve_optional(
            sim_slot.interface_preset_id, interface_presets_idx, "devices.interface_presets",
        )
        call_timing_preset = _resolve_optional(
            sim_slot.call_timing_preset_id, call_timing_presets_idx, "devices.call_timing_presets",
        )
        ip_preset = _resolve_optional(
            sim_slot.ip_preset_id, ip_presets_idx, "devices.ip_presets",
        )

        for name, value in (("installed_sim", installed_sim), ("active_profile_id", active_profile)):
            if value is None:
                _log.warning("sim_slot %r: optional field %r not set; left unset", slot_id, name)
        for name, value in (
            ("interface_preset_id", interface_preset),
            ("call_timing_preset_id", call_timing_preset),
            ("ip_preset_id", ip_preset),
        ):
            if value is None:
                _log.warning("sim_slot %r: optional field %r not set; using default", slot_id, name)

        sim_slot_runtimes[slot_id] = SimSlotRuntime(
            sim_slot=sim_slot,
            state=sim_slot.state,
            installed_sim=installed_sim,
            active_profile=active_profile,
            interface_preset=interface_preset,
            call_timing_preset=call_timing_preset,
            ip_preset=ip_preset,
        )

    if scenario.initial_state.radio is not None:
        radio = scenario.initial_state.radio
        _lookup(cells_idx, radio.serving_cell, "environments.cells")
        _lookup(signal_models_idx, radio.signal_model, "environments.signal_models")

    return modem_runtimes, sim_slot_runtimes


def _resolve_optional(item_id: Optional[str], index: dict, kind: str) -> Optional[object]:
    if item_id is None:
        return None
    return _lookup(index, item_id, kind)


# ---------------------------------------------------------------------------
# Seed dataclasses handed to sml/mpss/data/* sub-AOs
# ---------------------------------------------------------------------------

def resolve_seed_profiles(slot: SimSlotRuntime, devices: DevicesDoc) -> list[SeedProfile]:
    """Every `devices.data_profiles` entry, not just `slot.active_profile_id`
    -- seed a second profile so the test's default Phone1ProfileId=5 resolves
    without a runtime create_profile call. Only the slot's own active_profile_id
    gets isDefault=True; every other profile seeds with isDefault=False."""
    active_id = slot.sim_slot.active_profile_id
    return [
        SeedProfile(
            profileId=p.mpss_profile_id,
            apn=p.apn,
            ipFamily=p.ip_family,
            authType=p.auth_type,
            username=p.username,
            password=p.password,
            techType=p.tech_type,
            isDefault=(p.id == active_id),
        )
        for p in devices.data_profiles
    ]


def resolve_interface_preset(slot: SimSlotRuntime) -> InterfacePresetSeed:
    p: Optional[InterfacePreset] = slot.interface_preset
    if p is None:
        return InterfacePresetSeed()
    return InterfacePresetSeed(ifname_prefix=p.ifname_prefix, ifname_pool_size=p.ifname_pool_size)
    # Example: InterfacePresetSeed(ifname_prefix="rmnet_data", ifname_pool_size=8)


def resolve_call_timing_preset(slot: SimSlotRuntime) -> CallTimingPresetSeed:
    p: Optional[CallTimingPreset] = slot.call_timing_preset
    if p is None:
        return CallTimingPresetSeed()
    return CallTimingPresetSeed(
        call_connect_delay_ms=p.call_connect_delay_ms,
        call_disconnect_delay_ms=p.call_disconnect_delay_ms,
    )
    # Example: CallTimingPresetSeed(call_connect_delay_ms=200, call_disconnect_delay_ms=100)


def resolve_ip_config(slot: SimSlotRuntime) -> IpConfigSeed:
    p: Optional[IpPreset] = slot.ip_preset
    if p is None:
        return IpConfigSeed()
    return IpConfigSeed(
        ipv4_addr=p.ipv4_addr,
        ipv4_gateway=p.ipv4_gateway,
        ipv4_dns_primary=p.ipv4_dns_primary,
        ipv4_dns_secondary=p.ipv4_dns_secondary,
        ipv4_mtu=p.ipv4_mtu,
        ipv4_subnet_mask=p.ipv4_subnet_mask,
        ipv6_addr=p.ipv6_addr,
        ipv6_gateway=p.ipv6_gateway,
        ipv6_dns_primary=p.ipv6_dns_primary,
        ipv6_dns_secondary=p.ipv6_dns_secondary,
        ipv6_mtu=p.ipv6_mtu,
        ipv6_prefix_len=p.ipv6_prefix_len,
    )
    # Example: IpConfigSeed(ipv4_addr="10.0.0.1", ipv4_gateway="10.0.0.254",
    #                        ipv4_dns_primary="8.8.8.8", ..., ipv6_addr="2001:db8::1", ...)


# ---------------------------------------------------------------------------
# Device-level tuning presets (not per-slot -- looked up by RAT/profileId at
# request time, so these are exposed as plain id->model dicts rather than
# resolved against a SimSlotRuntime like the seed dataclasses above).
# ---------------------------------------------------------------------------

def resolve_bitrate_by_rat(devices: DevicesDoc) -> dict[str, tuple[int, int]]:
    """RAT string -> (maxTxRate, maxRxRate), from the first bitrate_by_rat
    preset (only one is expected)."""
    if not devices.bitrate_by_rat_presets:
        return {}
    preset = devices.bitrate_by_rat_presets[0]
    return {rat: (entry.maxTxRate, entry.maxRxRate) for rat, entry in preset.rates.items()}


def resolve_throughput_presets(devices: DevicesDoc) -> dict[int, dict]:
    """profileId -> {ulThroughput, ulMaxThroughput, ulQueueSize, dlThroughput}."""
    return {
        p.profile_id: {
            "ulThroughput": p.ul_throughput,
            "ulMaxThroughput": p.ul_max_throughput,
            "ulQueueSize": p.ul_queue_size,
            "dlThroughput": p.dl_throughput,
        }
        for p in devices.throughput_presets
    }


def resolve_qos_presets(devices: DevicesDoc) -> dict[int, dict]:
    """profileId -> {qosId, txMaxRate, txMinRate, rxMaxRate, rxMinRate}."""
    return {
        p.profile_id: {
            "qosId": p.qos_id,
            "txMaxRate": p.tx_max_rate,
            "txMinRate": p.tx_min_rate,
            "rxMaxRate": p.rx_max_rate,
            "rxMinRate": p.rx_min_rate,
        }
        for p in devices.qos_presets
    }


def resolve_throttle_presets(devices: DevicesDoc) -> dict[int, dict]:
    """profileId -> {apn, ipv4Time, ipv6Time, mcc, mnc}."""
    return {
        p.profile_id: {
            "apn": p.apn,
            "ipv4Time": p.ipv4_time,
            "ipv6Time": p.ipv6_time,
            "mcc": p.mcc,
            "mnc": p.mnc,
        }
        for p in devices.throttle_presets
    }


__all__ = [
    "LoaderError",
    "load_devices_doc",
    "load_environments_doc",
    "load_scenario_doc",
    "resolve_bitrate_by_rat",
    "resolve_call_timing_preset",
    "resolve_initial_state",
    "resolve_interface_preset",
    "resolve_ip_config",
    "resolve_qos_presets",
    "resolve_seed_profiles",
    "resolve_throttle_presets",
    "resolve_throughput_presets",
]
