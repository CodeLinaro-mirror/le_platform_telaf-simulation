# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for UDS-related config fields and validation."""
from __future__ import annotations

import os
import textwrap
from pathlib import Path

import pytest

from sml.mpss.config import BrokerConfig, ConfigError, MpssConfig, load_config
from sml.mpss.mqtt_client import MqttClient


# --------------------------------------------------------------------------- #
# 4.1: Default BrokerConfig has UDS fields                                    #
# --------------------------------------------------------------------------- #

def test_broker_config_default_transport():
    cfg = BrokerConfig()
    assert cfg.transport == "uds"
    assert cfg.socket_path == "/tmp/simula-mqtt.sock"


# --------------------------------------------------------------------------- #
# 4.2: config.yaml with transport: tcp produces TCP config                    #
# --------------------------------------------------------------------------- #

def test_load_config_tcp_transport(tmp_path: Path):
    yaml_content = textwrap.dedent("""\
        mpss:
          broker:
            host: localhost
            port: 1883
            client_id: test-client
            keepalive_s: 60
            transport: tcp
            socket_path: /tmp/simula-mqtt.sock
    """)
    cfg_file = tmp_path / "config.yaml"
    cfg_file.write_text(yaml_content)

    cfg = load_config(cfg_file)
    assert cfg.broker.transport == "tcp"


def test_load_config_tcp_paho_no_unix_transport(tmp_path: Path):
    """When transport=tcp, _ensure_paho_client must use standard mqtt.Client not _UnixMqttClient."""
    yaml_content = textwrap.dedent("""\
        mpss:
          broker:
            host: localhost
            port: 1883
            client_id: test-client
            keepalive_s: 60
            transport: tcp
    """)
    cfg_file = tmp_path / "config.yaml"
    cfg_file.write_text(yaml_content)

    cfg = load_config(cfg_file)
    assert cfg.broker.transport == "tcp"

    from sml.mpss.mqtt_client import MqttClient, _UnixMqttClient
    client = MqttClient(cfg)
    client._ensure_paho_client()
    assert not isinstance(client._paho, _UnixMqttClient), \
        "TCP transport must NOT use _UnixMqttClient"


# --------------------------------------------------------------------------- #
# 4.3: Invalid transport value raises ConfigError                             #
# --------------------------------------------------------------------------- #

def test_load_config_invalid_transport(tmp_path: Path):
    yaml_content = textwrap.dedent("""\
        mpss:
          broker:
            transport: websocket
    """)
    cfg_file = tmp_path / "config.yaml"
    cfg_file.write_text(yaml_content)

    with pytest.raises(ConfigError, match="broker.transport"):
        load_config(cfg_file)


# --------------------------------------------------------------------------- #
# 4.4: Unknown broker key raises ConfigError                                  #
# --------------------------------------------------------------------------- #

def test_load_config_unknown_broker_key(tmp_path: Path):
    yaml_content = textwrap.dedent("""\
        mpss:
          broker:
            unknown_key: foo
    """)
    cfg_file = tmp_path / "config.yaml"
    cfg_file.write_text(yaml_content)

    with pytest.raises(ConfigError, match="unknown_key"):
        load_config(cfg_file)


# --------------------------------------------------------------------------- #
# 4.4b: Sibling config.local.yaml deep-merges over the base (local wins)      #
# --------------------------------------------------------------------------- #

def test_load_config_local_override_merges(tmp_path: Path):
    """A config.local.yaml beside the base file overrides it per-section:
    local scenario wins, untouched base sections (broker) survive."""
    base = tmp_path / "config.yaml"
    base.write_text(textwrap.dedent("""\
        mpss:
          broker:
            transport: tcp
            client_id: base-client
          scenario: config/scenarios/baseline.yaml
    """))
    local = tmp_path / "config.local.yaml"
    local.write_text(textwrap.dedent("""\
        mpss:
          scenario: config/scenarios/data_apn_bringup.yaml
    """))

    cfg = load_config(base)
    # local scenario wins
    assert cfg.scenario == "config/scenarios/data_apn_bringup.yaml"
    # base broker section untouched by the override
    assert cfg.broker.transport == "tcp"
    assert cfg.broker.client_id == "base-client"


# --------------------------------------------------------------------------- #
# 4.5: Integration test connects via UDS when socket exists                   #
# --------------------------------------------------------------------------- #

def test_integration_uds_transport_skips_when_no_socket():
    """If /tmp/simula-mqtt.sock is absent, skip. If present, verify _UnixMqttClient connects."""
    socket_path = "/tmp/simula-mqtt.sock"
    if not os.path.exists(socket_path):
        pytest.skip(f"UDS socket {socket_path} not present (broker not running)")

    from sml.mpss.mqtt_client import _UnixMqttClient

    cfg = MpssConfig(
        broker=BrokerConfig(
            transport="uds",
            socket_path=socket_path,
            client_id="test-uds-client",
            keepalive_s=5,
        )
    )
    client = MqttClient(cfg)
    client._ensure_paho_client()
    assert client._paho is not None
    assert isinstance(client._paho, _UnixMqttClient)
