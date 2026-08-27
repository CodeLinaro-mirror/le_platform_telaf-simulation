# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Configuration loader for telaf-simulation MQTT client processes.

Generalizes what used to be `sml/mpss/config.py`: the top-level YAML key and
the default `client_id` template are parametrized by process name, so mpss
and apss (and any future process) share one implementation. `sml/mpss/config.py`
now re-exports this module as a compatibility shim -- see that file's header.
"""
from __future__ import annotations

import socket
from dataclasses import dataclass, field, fields
from pathlib import Path
from typing import Any, Optional

import yaml


class ConfigError(RuntimeError):
    """Raised for any unrecoverable problem in a process's config.yaml."""


_VALID_TRANSPORTS = frozenset({"tcp", "uds"})
_VALID_INSTRUMENTATION = frozenset({"off", "on", "verbose"})


@dataclass
class BrokerConfig:
    host: str = "localhost"
    port: int = 1883
    client_id: str = ""  # empty => process-specific default filled by load_config()
    keepalive_s: int = 60
    transport: str = "uds"
    socket_path: str = "/tmp/simula-mqtt.sock"


@dataclass
class ReconnectConfig:
    initial_ms: int = 500
    max_ms: int = 30_000
    multiplier: float = 2.0
    jitter_pct: float = 0.0
    connect_timeout_ms: int = 5_000   # max time to wait for CONNACK in Connecting


@dataclass
class DebugConfig:
    live_spy: bool = False  # legacy; prefer `instrumentation`
    log_level: str = "INFO"
    instrumentation: str = "off"  # "off" | "on" | "verbose"; see sml/common/instrumentation.py


@dataclass
class ProcessConfig:
    broker: BrokerConfig = field(default_factory=BrokerConfig)
    reconnect: ReconnectConfig = field(default_factory=ReconnectConfig)
    debug: DebugConfig = field(default_factory=DebugConfig)
    scenario: Optional[str] = None
    """Path (relative to sml/) to a config/scenarios/*.yaml file for the
    Scenario Runner to load at startup. None/absent disables it."""


def _default_client_id(process_name: str) -> str:
    return f"sml-{process_name}-${{HOSTNAME}}"


def _expand_hostname(client_id: str) -> str:
    if "${HOSTNAME}" in client_id:
        return client_id.replace("${HOSTNAME}", socket.gethostname())
    return client_id


def _field_names(dc_type: type) -> set[str]:
    return {f.name for f in fields(dc_type)}


def _populate(dc_type: type, raw: Any, section_label: str):
    if raw is None:
        return dc_type()
    if not isinstance(raw, dict):
        raise ConfigError(
            f"section '{section_label}' must be a mapping, got {type(raw).__name__}"
        )
    allowed = _field_names(dc_type)
    unknown = set(raw) - allowed
    if unknown:
        raise ConfigError(
            f"section '{section_label}' has unknown keys: {sorted(unknown)}; "
            f"allowed: {sorted(allowed)}"
        )
    return dc_type(**raw)


def _read_yaml_mapping(config_path: Path) -> Optional[dict]:
    """Parse a YAML file into a mapping. Returns None if empty; raises
    ConfigError on parse failure or a non-mapping top level."""
    try:
        with config_path.open("r", encoding="utf-8") as fh:
            raw = yaml.safe_load(fh)
    except yaml.YAMLError as exc:
        raise ConfigError(f"failed to parse {config_path}: {exc}") from exc
    if raw is None:
        return None
    if not isinstance(raw, dict):
        raise ConfigError(f"{config_path}: top-level YAML must be a mapping")
    return raw


def _deep_merge(base: dict, override: dict) -> dict:
    """Recursively merge `override` onto `base`; override wins. Nested
    mappings merge key-by-key; scalars/lists replace wholesale. Neither
    input is mutated."""
    merged = dict(base)
    for key, val in override.items():
        existing = merged.get(key)
        if isinstance(existing, dict) and isinstance(val, dict):
            merged[key] = _deep_merge(existing, val)
        else:
            merged[key] = val
    return merged


def _build_config(raw: dict, source: Path, process_name: str) -> ProcessConfig:
    """Validate a merged raw mapping and build ProcessConfig."""
    root = raw.get(process_name)
    if root is None:
        raise ConfigError(f"{source}: missing top-level '{process_name}' key")
    if not isinstance(root, dict):
        raise ConfigError(f"{source}: '{process_name}' must be a mapping")

    allowed_top = {"broker", "reconnect", "debug", "scenario"}
    unknown_top = set(root) - allowed_top
    if unknown_top:
        raise ConfigError(
            f"{source}: unknown keys under '{process_name}': {sorted(unknown_top)}; "
            f"allowed: {sorted(allowed_top)}"
        )

    scenario_raw = root.get("scenario")
    if scenario_raw is not None and not isinstance(scenario_raw, str):
        raise ConfigError(f"{source}: '{process_name}.scenario' must be a string path")

    cfg = ProcessConfig(
        broker=_populate(BrokerConfig, root.get("broker"), f"{process_name}.broker"),
        reconnect=_populate(ReconnectConfig, root.get("reconnect"), f"{process_name}.reconnect"),
        debug=_populate(DebugConfig, root.get("debug"), f"{process_name}.debug"),
        scenario=scenario_raw,
    )
    if cfg.broker.transport not in _VALID_TRANSPORTS:
        raise ConfigError(
            f"broker.transport={cfg.broker.transport!r} is invalid; "
            f"allowed: {sorted(_VALID_TRANSPORTS)}"
        )
    # YAML 1.1 ("Norway problem"): bare `off`/`on`/`yes`/`no` in YAML parse to
    # Python bool, so `instrumentation: off` becomes `False`. Coerce bool
    # aliases before validating so operators can write the natural form.
    raw_instr = cfg.debug.instrumentation
    if isinstance(raw_instr, bool):
        raw_instr = "verbose" if raw_instr else "off"
    instr = str(raw_instr).lower()
    if instr not in _VALID_INSTRUMENTATION:
        raise ConfigError(
            f"debug.instrumentation={cfg.debug.instrumentation!r} is invalid; "
            f"allowed: {sorted(_VALID_INSTRUMENTATION)}"
        )
    cfg.debug.instrumentation = instr
    if not cfg.broker.client_id:
        cfg.broker.client_id = _default_client_id(process_name)
    cfg.broker.client_id = _expand_hostname(cfg.broker.client_id)
    return cfg


def load_config(process_name: str, path: Path | None = None) -> ProcessConfig:
    """Load config from YAML; return defaults if the file is missing.

    A sibling ``config.local.yaml`` (gitignored) in the same directory as the
    base file is deep-merged over it when present -- local keys win per-section,
    absent local keys leave the base untouched. The local-override lookup keys
    off the resolved base path's directory, so an explicit ``path`` (as passed
    by tests, typically into a private tmp dir) only picks up a local file
    sitting beside *that* path.

    `process_name` selects the top-level YAML key (e.g. "mpss", "apss") and the
    default `client_id` template (`sml-{process_name}-${HOSTNAME}`).

    Raises ConfigError on malformed YAML, unknown keys, or a bad merge result.
    """
    if path is None:
        raise ConfigError("load_config() requires an explicit path in sml/common")
    config_path = path

    base_raw = _read_yaml_mapping(config_path) if config_path.exists() else None

    local_path = config_path.parent / "config.local.yaml"
    local_raw = _read_yaml_mapping(local_path) if local_path.exists() else None

    if base_raw is None and local_raw is None:
        cfg = ProcessConfig()
        cfg.broker.client_id = _expand_hostname(_default_client_id(process_name))
        return cfg

    merged = _deep_merge(base_raw or {}, local_raw or {})
    return _build_config(merged, config_path, process_name)


__all__ = [
    "BrokerConfig",
    "ConfigError",
    "DebugConfig",
    "ProcessConfig",
    "ReconnectConfig",
    "load_config",
]
