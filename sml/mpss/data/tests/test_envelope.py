# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for sml/mpss/envelope.py — no broker required."""
from __future__ import annotations

from sml.mpss.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    validate_envelope,
)


# ---------------------------------------------------------------------------
# validate_envelope
# ---------------------------------------------------------------------------

def _good_env(**overrides) -> dict:
    base = {"v": 1, "corrId": "00ab", "ts": 1718000000000, "src": "mpss-dev-123", "data": {}}
    base.update(overrides)
    return base


def test_valid_envelope_returns_no_errors():
    assert validate_envelope(_good_env()) == []


def test_wrong_version_returns_error():
    errs = validate_envelope(_good_env(v=2))
    assert any("v must be 1" in e for e in errs)


def test_bad_corr_id_returns_error():
    errs = validate_envelope(_good_env(corrId="ZZ"))
    assert any("corrId" in e for e in errs)


def test_missing_ts_returns_error():
    d = _good_env()
    del d["ts"]
    errs = validate_envelope(d)
    assert any("ts" in e for e in errs)


def test_non_dict_returns_error():
    errs = validate_envelope("not a dict")
    assert errs


# ---------------------------------------------------------------------------
# Envelope builders
# ---------------------------------------------------------------------------

def test_build_success_envelope_fields():
    env = build_success_envelope("mpss-dev-1", "00ab", "pa-req-1", {"x": 1})
    assert env["v"] == 1
    assert env["corrId"] == "00ab"
    assert env["src"] == "mpss-dev-1"
    assert env["dest"] == "pa-req-1"
    assert env["data"] == {"x": 1}
    assert "error" not in env


def test_build_error_envelope_fields():
    env = build_error_envelope("mpss-dev-1", "00ab", "pa-req-1", "GENERIC_FAILURE", "oops")
    assert env["dest"] == "pa-req-1"
    assert "data" not in env
    assert env["error"]["code"] == "GENERIC_FAILURE"
    assert env["error"]["msg"] == "oops"


def test_build_event_envelope_no_error_field():
    env = build_event_envelope("mpss-dev-1", {"status": "CONNECTED"})
    assert "error" not in env
    assert "dest" not in env
    assert env["data"]["status"] == "CONNECTED"
