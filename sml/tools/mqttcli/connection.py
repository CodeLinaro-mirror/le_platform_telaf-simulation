# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Plain paho.mqtt connection helper for `sml.tools.mqttcli` CLI commands.

Not an Active Object -- `sub`/`pub` are short-lived scripts, not the MPSS
process, so there's no HSM/reconnect-backoff machinery here. Reuses the
uds-via-AF_UNIX trick from `sml.mpss.mqtt_client._UnixMqttClient` and the
`sml.mpss.config` broker settings so both the CLI and the real MPSS
process agree on where the broker lives by default.
"""
from __future__ import annotations

import argparse
from dataclasses import replace

import paho.mqtt.client as mqtt

from sml.mpss.config import BrokerConfig, load_config
from sml.mpss.mqtt_client import _UnixMqttClient


def add_connection_args(parser: argparse.ArgumentParser) -> None:
    """Register --host/--port/--socket/--transport overrides on `parser`."""
    parser.add_argument("--host", default=None, help="broker host (tcp transport)")
    parser.add_argument("--port", type=int, default=None, help="broker port (tcp transport)")
    parser.add_argument("--socket", default=None, help="broker UDS path (uds transport)")
    parser.add_argument("--transport", choices=("tcp", "uds"), default=None,
                        help="override transport from config")


def resolve_broker_config(args: argparse.Namespace) -> BrokerConfig:
    """Merge CLI overrides onto `load_config().broker`."""
    broker = load_config().broker
    overrides = {}
    if args.transport is not None:
        overrides["transport"] = args.transport
    if args.host is not None:
        overrides["host"] = args.host
    if args.port is not None:
        overrides["port"] = args.port
    if args.socket is not None:
        overrides["socket_path"] = args.socket
    return replace(broker, **overrides) if overrides else broker


def connect_client(broker: BrokerConfig, client_id: str) -> mqtt.Client:
    """Build+connect (blocking) a plain paho client for `broker`. Caller owns
    loop_start()/loop_stop()/disconnect()."""
    if broker.transport == "uds":
        client = _UnixMqttClient(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=client_id,
        )
        client._unix_socket_path = broker.socket_path
        client.connect(broker.host, broker.port, broker.keepalive_s)
    else:
        client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=client_id,
        )
        client.connect(broker.host, broker.port, broker.keepalive_s)
    return client


def make_client_id(prefix: str) -> str:
    import os
    return f"{prefix}-{os.getpid()}"


__all__ = [
    "add_connection_args",
    "connect_client",
    "make_client_id",
    "resolve_broker_config",
]
