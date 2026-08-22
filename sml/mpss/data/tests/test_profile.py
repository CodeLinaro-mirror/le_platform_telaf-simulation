# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for ProfileStore and DataProfileAO — no broker required.

Field names on the wire mirror telux::data::ProfileParams exactly
(profileName/apn/userName/password/techPref/authType/ipFamilyType/apnTypes/
emergencyAllowed/clatEnabled); the profile object's id field is `id`, not
`profileId` (see DataProfileManager.cpp's wireToDataProfile).
"""
from __future__ import annotations

import json
from unittest.mock import MagicMock

from sml.mpss.data.profile import DataProfileAO, ProfileStore
from sml.mpss.data.tests._helpers import wait_for_state, wait_until
from sml.config.models import SeedProfile
from generated.python.topics import data as topics_data


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _seed(store: ProfileStore, n: int = 1) -> None:
    store.seed([
        SeedProfile(profileId=i, apn=f"apn{i}", ipFamily="IPV4V6",
                    authType="NONE", username="", password="",
                    techType="3GPP", isDefault=(i == 1))
        for i in range(1, n + 1)
    ])


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload)})

    def reset(self):
        self.calls.clear()


def _make_ao(pub):
    seed_profile = SeedProfile(profileId=1, apn="apn1", ipFamily="IPV4V6",
                               authType="NONE", username="", password="",
                               techType="3GPP", isDefault=True)
    ao = DataProfileAO(slot=1, seed_profiles=[seed_profile], mpss_src="mpss-dev-1")
    sub_mock = MagicMock()
    ao.start(pub, sub_mock)
    wait_for_state(ao, "smfn_ready")
    pub.reset()
    return ao


def _req(ao: DataProfileAO, topic: str, rsp_topic: str, data: dict) -> dict:
    """Send a request and return the published response payload."""
    pub = ao._publish_fn
    before = len(pub.calls)
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data
    }).encode()
    ao.handle_message(topic, msg)
    wait_until(lambda: any(c["topic"] == rsp_topic for c in pub.calls[before:]),
               timeout=1.0)
    new_calls = pub.calls[before:]
    rsp_calls = [c for c in new_calls if c["topic"] == rsp_topic]
    assert rsp_calls, f"no response published on {rsp_topic} for {topic}"
    return rsp_calls[0]["payload"]


# ---------------------------------------------------------------------------
# ProfileStore unit tests
# ---------------------------------------------------------------------------

class TestProfileStore:
    def test_empty_store_query(self):
        s = ProfileStore()
        assert s.query_all() == []

    def test_seed_and_query(self):
        s = ProfileStore()
        _seed(s, 2)
        assert len(s.query_all()) == 2

    def test_get_existing(self):
        s = ProfileStore()
        _seed(s)
        p = s.get_by_id(1)
        assert p is not None
        assert p["apn"] == "apn1"
        assert p["id"] == 1

    def test_get_nonexistent(self):
        s = ProfileStore()
        assert s.get_by_id(99) is None

    def test_create_assigns_lowest_free_id(self):
        s = ProfileStore()
        _seed(s, 3)
        s.delete(2)
        pid, err = s.create({"apn": "new", "ipFamilyType": "IPV4V6", "authType": "AUTH_NONE"})
        assert err is None
        assert pid == 2

    def test_create_full_store_returns_no_resources(self):
        s = ProfileStore()
        for i in range(1, 33):
            s._profiles[i] = None  # type: ignore[assignment]  # fill slots
        _, err = s.create({})
        assert err == "NO_RESOURCES"

    def test_modify_updates_fields(self):
        s = ProfileStore()
        _seed(s)
        updated, err = s.modify(1, {"apn": "newapn"})
        assert err is None
        assert updated["apn"] == "newapn"
        assert s.get_by_id(1)["apn"] == "newapn"

    def test_modify_nonexistent_returns_invalid(self):
        s = ProfileStore()
        _, err = s.modify(99, {"apn": "x"})
        assert err == "INVALID_ARGUMENTS"

    def test_delete_removes_profile(self):
        s = ProfileStore()
        _seed(s)
        err = s.delete(1)
        assert err is None
        assert s.get_by_id(1) is None

    def test_delete_nonexistent_returns_invalid(self):
        s = ProfileStore()
        err = s.delete(99)
        assert err == "INVALID_ARGUMENTS"

    def test_query_filtered_by_apn(self):
        s = ProfileStore()
        _seed(s, 3)
        out = s.query_filtered({"apn": "apn2"})
        assert len(out) == 1
        assert out[0]["apn"] == "apn2"

    def test_query_filtered_empty_filter_returns_all(self):
        s = ProfileStore()
        _seed(s, 2)
        assert len(s.query_filtered({})) == 2


# ---------------------------------------------------------------------------
# ProfileStore persistence tests
# ---------------------------------------------------------------------------

class TestProfileStorePersist:
    def test_seed_writes_persist_file(self, tmp_path):
        p = tmp_path / "data_profiles.json"
        s = ProfileStore(persist_path=p)
        _seed(s, 2)
        assert p.exists()

    def test_create_modify_delete_update_persist_file(self, tmp_path):
        p = tmp_path / "data_profiles.json"
        s = ProfileStore(persist_path=p)
        _seed(s, 1)

        pid, err = s.create({"apn": "new", "ipFamilyType": "IPV4V6", "authType": "AUTH_NONE"})
        assert err is None
        snapshot = json.loads(p.read_text())
        assert any(pr["id"] == pid and pr["apn"] == "new" for pr in snapshot["profiles"])

        s.modify(pid, {"apn": "modified"})
        snapshot = json.loads(p.read_text())
        assert any(pr["id"] == pid and pr["apn"] == "modified" for pr in snapshot["profiles"])

        s.delete(pid)
        snapshot = json.loads(p.read_text())
        assert not any(pr["id"] == pid for pr in snapshot["profiles"])

    def test_second_instance_loads_prior_instance_snapshot(self, tmp_path):
        p = tmp_path / "data_profiles.json"
        s1 = ProfileStore(persist_path=p)
        _seed(s1, 1)
        s1.create({"apn": "restored", "ipFamilyType": "IPV4V6", "authType": "AUTH_NONE"})

        s2 = ProfileStore(persist_path=p)
        assert s2.load(p) is True
        assert any(pr["apn"] == "restored" for pr in s2.query_all())

    def test_load_missing_file_returns_false(self, tmp_path):
        p = tmp_path / "nope.json"
        s = ProfileStore(persist_path=p)
        assert s.load(p) is False

    def test_load_corrupt_file_returns_false(self, tmp_path):
        p = tmp_path / "data_profiles.json"
        p.write_text("{not valid json")
        s = ProfileStore(persist_path=p)
        assert s.load(p) is False

    def test_no_persist_path_no_file_written(self, tmp_path):
        s = ProfileStore()
        _seed(s, 1)
        assert list(tmp_path.iterdir()) == []


# ---------------------------------------------------------------------------
# DataProfileAO unit tests
# ---------------------------------------------------------------------------

class TestDataProfileAO:
    def test_request_profile_list_returns_seed_profile(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        rsp = _req(ao, topics_data.request_profile_list.req, topics_data.request_profile_list.rsp, {})
        assert "data" in rsp
        assert len(rsp["data"]["profiles"]) == 1

    def test_query_by_id_returns_single_profile(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        rsp = _req(ao, topics_data.query_profile.req, topics_data.query_profile.rsp, {"profileId": 1})
        assert "data" in rsp
        assert rsp["data"]["profiles"][0]["apn"] == "apn1"

    def test_query_by_nonexistent_id_returns_empty_list(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        rsp = _req(ao, topics_data.query_profile.req, topics_data.query_profile.rsp, {"profileId": 99})
        assert rsp["data"]["profiles"] == []

    def test_query_filtered_by_apn(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        _req(ao, topics_data.create_profile.req, topics_data.create_profile.rsp,
             {"profileName": "p2", "apn": "apn2", "userName": "", "password": "",
              "techPref": "TP_3GPP", "authType": "AUTH_NONE", "ipFamilyType": "IPV4V6",
              "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED", "clatEnabled": False, "slot": 1})
        rsp = _req(ao, topics_data.query_profile.req, topics_data.query_profile.rsp, {"apn": "apn2"})
        assert len(rsp["data"]["profiles"]) == 1

    def test_create_returns_profile_id(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        rsp = _req(ao, topics_data.create_profile.req, topics_data.create_profile.rsp,
                   {"profileName": "p", "apn": "x", "userName": "", "password": "",
                    "techPref": "TP_3GPP", "authType": "AUTH_NONE", "ipFamilyType": "IPV4",
                    "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED", "clatEnabled": False, "slot": 1})
        assert "data" in rsp
        assert isinstance(rsp["data"]["profileId"], int)

    def test_create_emits_changed_event(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        before = len(pub.calls)
        _req(ao, topics_data.create_profile.req, topics_data.create_profile.rsp,
             {"profileName": "p", "apn": "x", "userName": "", "password": "",
              "techPref": "TP_3GPP", "authType": "AUTH_NONE", "ipFamilyType": "IPV4",
              "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED", "clatEnabled": False, "slot": 1})
        changed = [c for c in pub.calls[before:] if c["topic"] == topics_data.profile_changed.ind]
        assert len(changed) == 1
        assert changed[0]["payload"]["data"]["event"] == "CREATE"

    def test_modify_updates_and_emits_changed(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        before = len(pub.calls)
        rsp = _req(ao, topics_data.modify_profile.req, topics_data.modify_profile.rsp,
                   {"profileId": 1, "profileName": "p1", "apn": "newapn", "userName": "",
                    "password": "", "techPref": "TP_3GPP", "authType": "AUTH_NONE",
                    "ipFamilyType": "IPV4V6", "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED",
                    "clatEnabled": False, "slot": 1})
        assert "data" in rsp
        changed = [c for c in pub.calls[before:] if c["topic"] == topics_data.profile_changed.ind]
        assert len(changed) == 1
        assert changed[0]["payload"]["data"]["event"] == "MODIFY"

    def test_modify_missing_profile_id_dropped_at_schema_gate(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        bad = json.dumps({
            "v": 1, "corrId": "00ab", "ts": 1718000000000, "src": "dcs-master-1234",
            "data": {"profileName": "p1", "apn": "x", "userName": "", "password": "",
                     "techPref": "TP_3GPP", "authType": "AUTH_NONE", "ipFamilyType": "IPV4V6",
                     "apnTypes": 0, "emergencyAllowed": "UNSPECIFIED", "clatEnabled": False, "slot": 1},
        }).encode()
        before = len(pub.calls)
        ao.handle_message(topics_data.modify_profile.req, bad)
        assert len(pub.calls) == before  # no response published

    def test_delete_removes_profile_and_emits_changed(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        before = len(pub.calls)
        _req(ao, topics_data.delete_profile.req, topics_data.delete_profile.rsp,
             {"profileId": 1, "techPref": "TP_3GPP", "slot": 1})
        changed = [c for c in pub.calls[before:] if c["topic"] == topics_data.profile_changed.ind]
        assert changed[0]["payload"]["data"]["event"] == "DELETE"
        rsp = _req(ao, topics_data.query_profile.req, topics_data.query_profile.rsp, {"profileId": 1})
        assert rsp["data"]["profiles"] == []

    def test_invalid_envelope_dropped(self):
        pub = _MockPublish()
        ao = _make_ao(pub)
        bad = json.dumps({"v": 99, "corrId": "00ab", "ts": 1, "src": "x", "data": {}}).encode()
        before = len(pub.calls)
        ao.handle_message(topics_data.query_profile.req, bad)
        assert len(pub.calls) == before  # no response published


# ---------------------------------------------------------------------------
# DataProfileAO persistence tests
# ---------------------------------------------------------------------------

class TestDataProfileAOPersist:
    def test_starts_from_persist_file_skipping_seed(self, tmp_path):
        persist_path = tmp_path / "data_profiles.json"
        prior_store = ProfileStore(persist_path=persist_path)
        prior_store.seed([])
        prior_store.create({"apn": "from_persist", "ipFamilyType": "IPV4V6", "authType": "AUTH_NONE"})

        seed_profile = SeedProfile(profileId=1, apn="from_seed", ipFamily="IPV4V6",
                                   authType="NONE", username="", password="",
                                   techType="3GPP", isDefault=True)
        ao = DataProfileAO(slot=1, seed_profiles=[seed_profile], mpss_src="mpss-dev-1",
                           persist_path=persist_path)
        pub = _MockPublish()
        ao.start(pub, MagicMock())
        wait_for_state(ao, "smfn_ready")

        rsp = _req(ao, topics_data.request_profile_list.req, topics_data.request_profile_list.rsp, {})
        apns = [p["apn"] for p in rsp["data"]["profiles"]]
        assert "from_persist" in apns
        assert "from_seed" not in apns

    def test_starts_from_seed_when_no_persist_file(self, tmp_path):
        persist_path = tmp_path / "data_profiles.json"
        seed_profile = SeedProfile(profileId=1, apn="from_seed", ipFamily="IPV4V6",
                                   authType="NONE", username="", password="",
                                   techType="3GPP", isDefault=True)
        ao = DataProfileAO(slot=1, seed_profiles=[seed_profile], mpss_src="mpss-dev-1",
                           persist_path=persist_path)
        pub = _MockPublish()
        ao.start(pub, MagicMock())
        wait_for_state(ao, "smfn_ready")

        rsp = _req(ao, topics_data.request_profile_list.req, topics_data.request_profile_list.rsp, {})
        apns = [p["apn"] for p in rsp["data"]["profiles"]]
        assert apns == ["from_seed"]

    def test_starts_from_seed_when_persist_file_corrupt(self, tmp_path):
        persist_path = tmp_path / "data_profiles.json"
        persist_path.write_text("{not valid json")

        seed_profile = SeedProfile(profileId=1, apn="from_seed", ipFamily="IPV4V6",
                                   authType="NONE", username="", password="",
                                   techType="3GPP", isDefault=True)
        ao = DataProfileAO(slot=1, seed_profiles=[seed_profile], mpss_src="mpss-dev-1",
                           persist_path=persist_path)
        pub = _MockPublish()
        ao.start(pub, MagicMock())  # must not raise
        wait_for_state(ao, "smfn_ready")

        rsp = _req(ao, topics_data.request_profile_list.req, topics_data.request_profile_list.rsp, {})
        apns = [p["apn"] for p in rsp["data"]["profiles"]]
        assert apns == ["from_seed"]

    def test_no_persist_path_behaves_as_before(self):
        # Existing zero-persist-path behavior (regression guard).
        pub = _MockPublish()
        ao = _make_ao(pub)
        rsp = _req(ao, topics_data.request_profile_list.req, topics_data.request_profile_list.rsp, {})
        assert len(rsp["data"]["profiles"]) == 1
