# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for `sml.tools.mqttcli` -- pubfile parsing, envelope decision,
schema validation, formatter, and `sub`'s default topic. No broker.
"""
from __future__ import annotations

import json
from pathlib import Path

import jsonschema
import pytest
import yaml

from sml.tools.mqttcli.format import format_message
from sml.tools.mqttcli.pubfile import PubFileError, load_pubfile, parse_interval_ms, resolve_schema_id


def _write_yaml(tmp_path: Path, doc: dict) -> Path:
    path = tmp_path / "cmd.yaml"
    path.write_text(yaml.safe_dump(doc), encoding="utf-8")
    return path


# --- pubfile parse -----------------------------------------------------


def test_pubfile_single_object(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ctrl/cmd/action/data/force_serv_state",
        "data": {"serviceState": "IN_SERVICE", "networkRat": "LTE"},
    })
    result = load_pubfile(path)
    assert len(result.specs) == 1
    assert result.specs[0].topic == "ctrl/cmd/action/data/force_serv_state"
    assert result.interval_ms is None


def test_pubfile_list_sequence(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ctrl/cmd/action/data/force_serv_state",
        "interval": "500ms",
        "data": [
            {"serviceState": "OUT_OF_SERVICE"},
            {"serviceState": "IN_SERVICE"},
        ],
    })
    result = load_pubfile(path)
    assert len(result.specs) == 2
    assert result.interval_ms == 500.0


def test_pubfile_missing_topic_raises(tmp_path):
    path = _write_yaml(tmp_path, {"data": {"a": 1}})
    with pytest.raises(PubFileError, match="topic"):
        load_pubfile(path)


def test_pubfile_bad_interval_raises(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ctrl/cmd/action/data/force_serv_state",
        "interval": "500 milliseconds",
        "data": [{"serviceState": "IN_SERVICE"}, {"serviceState": "OUT_OF_SERVICE"}],
    })
    with pytest.raises(PubFileError, match="interval"):
        load_pubfile(path)


# --- envelope decision ---------------------------------------------------


def test_envelope_ap_req_wraps(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ap/req/data/start_data_call",
        "data": {"profileId": 1, "ipFamily": "IPV4V6", "opType": "DATA_LOCAL", "slot": 0},
    })
    result = load_pubfile(path, src="cli-test")
    payload = result.specs[0].payload
    assert payload["v"] == 1
    assert payload["src"] == "cli-test"
    assert re_corr_id_ok(payload["corrId"])
    assert payload["data"]["profileId"] == 1


def test_envelope_ctrl_cmd_raw(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ctrl/cmd/action/data/force_serv_state",
        "data": {"serviceState": "IN_SERVICE"},
    })
    result = load_pubfile(path)
    assert result.specs[0].payload == {"serviceState": "IN_SERVICE"}


def test_envelope_explicit_override(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ap/req/data/start_data_call",
        "envelope": False,
        "data": {"profileId": 1, "ipFamily": "IPV4V6", "opType": "DATA_LOCAL", "slot": 0},
    })
    result = load_pubfile(path)
    assert "v" not in result.specs[0].payload


def test_envelope_unknown_topic_wraps(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "some/unknown/topic",
        "data": {"foo": "bar"},
    })
    result = load_pubfile(path)
    assert result.specs[0].payload["v"] == 1


def re_corr_id_ok(corr_id: str) -> bool:
    import re
    return bool(re.match(r"^[0-9a-f]{4,8}$", corr_id))


# --- validation -----------------------------------------------------------


def test_validation_known_topic_bad_payload_hard_fails(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "ctrl/cmd/action/data/force_serv_state",
        "data": {"serviceState": "NOT_A_REAL_STATE"},
    })
    with pytest.raises(PubFileError, match="failed validation"):
        load_pubfile(path)


def test_validation_unknown_topic_sends_raw_no_error(tmp_path):
    path = _write_yaml(tmp_path, {
        "topic": "totally/unknown/topic",
        "envelope": False,
        "data": {"anything": "goes"},
    })
    result = load_pubfile(path)
    assert result.specs[0].payload == {"anything": "goes"}


def test_resolve_schema_id():
    assert resolve_schema_id("ctrl/cmd/action/data/force_serv_state") == "action.data.force_serv_state.req"
    assert resolve_schema_id("ap/req/data/start_data_call") == "data.start_data_call.req"
    assert resolve_schema_id("mp/rsp/data/start_data_call") is None


# --- formatter -------------------------------------------------------------


def test_formatter_envelope_dict_contains_corrid_src():
    body = {"v": 1, "corrId": "abcd", "ts": 1234, "src": "mpss",
            "data": {"foo": "bar"}}
    out = format_message("mp/ind/data/serv_state", json.dumps(body).encode(), qos=1)
    assert "abcd" in out
    assert "mpss" in out


def test_formatter_raw_dict_pretty_json():
    body = {"serviceState": "IN_SERVICE"}
    out = format_message("ctrl/cmd/action/data/force_serv_state", json.dumps(body).encode(), qos=1)
    assert "IN_SERVICE" in out


def test_formatter_non_json_bytes_shown_safely():
    out = format_message("some/topic", b"\xff\xfe not json", qos=1)
    assert "undecodable" in out


# --- sub topic default -----------------------------------------------------


def test_sub_default_topic_is_hash():
    from sml.tools.mqttcli.__main__ import build_parser
    args = build_parser().parse_args(["sub"])
    assert args.topics == []


# --- sub -v/--verbose default -----------------------------------------------


def test_sub_verbose_defaults_to_false():
    from sml.tools.mqttcli.__main__ import build_parser
    args = build_parser().parse_args(["sub"])
    assert args.verbose is False


def test_sub_verbose_flag_sets_true():
    from sml.tools.mqttcli.__main__ import build_parser
    args = build_parser().parse_args(["sub", "-v"])
    assert args.verbose is True

