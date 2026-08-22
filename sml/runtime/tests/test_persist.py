# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for sml.runtime.persist -- atomic_write_json/read_json."""
from __future__ import annotations

from sml.runtime.persist import atomic_write_json, read_json


def test_write_then_read_roundtrip(tmp_path):
    p = tmp_path / "state.json"
    atomic_write_json(p, {"a": 1, "b": [1, 2, 3]})
    assert read_json(p) == {"a": 1, "b": [1, 2, 3]}


def test_read_missing_file_returns_none(tmp_path):
    assert read_json(tmp_path / "nope.json") is None


def test_read_corrupt_json_returns_none(tmp_path):
    p = tmp_path / "bad.json"
    p.write_text("{not valid json")
    assert read_json(p) is None


def test_write_overwrites_existing_file(tmp_path):
    p = tmp_path / "state.json"
    atomic_write_json(p, {"v": 1})
    atomic_write_json(p, {"v": 2})
    assert read_json(p) == {"v": 2}


def test_write_creates_missing_parent_dirs(tmp_path):
    p = tmp_path / "a" / "b" / "c" / "state.json"
    atomic_write_json(p, {"v": 1})
    assert read_json(p) == {"v": 1}
