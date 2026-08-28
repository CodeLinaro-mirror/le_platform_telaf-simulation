# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for SettingsStore and NetSettingsAO. No broker required."""
from __future__ import annotations

import time

from sml.mpss.net.settings import NetSettingsAO, SettingsStore
from sml.mpss.net.tests._helpers import MockPublish, boot, err, inds, ok, send
from generated.python.topics import net_settings as T


def _ao(pub, **kw) -> NetSettingsAO:
    return boot(NetSettingsAO(slot_id=1, mpss_src="mpss-dev-1", **kw),
                pub, "smfn_set_ready")


class TestSettingsStore:
    def test_wwan_defaults_to_allowed(self):
        """The SDK documents 'allow' as the default, so an unqueried slot must
        not read as disallowed."""
        assert SettingsStore().is_wwan_allowed(1) is True

    def test_wwan_is_per_slot(self):
        store = SettingsStore()
        store.wwan_allowed[1] = False
        assert store.is_wwan_allowed(1) is False
        assert store.is_wwan_allowed(2) is True


class TestBackhaulPreference:
    def test_set_then_read_preserves_order(self):
        """Order is the entire meaning of this list -- first is most preferred."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_backhaul_pref.req, {"backhaulPref": ["ETH", "USB", "WWAN"]})
        ok(pub, T.set_backhaul_pref.rsp)
        pub.reset()
        send(ao, T.request_backhaul_pref.req, {})
        assert ok(pub, T.request_backhaul_pref.rsp)["backhaulPref"] == \
            ["ETH", "USB", "WWAN"]

    def test_duplicate_backhaul_rejected(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_backhaul_pref.req, {"backhaulPref": ["ETH", "ETH"]})
        assert err(pub, T.set_backhaul_pref.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_empty_list_allowed(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_backhaul_pref.req, {"backhaulPref": []})
        ok(pub, T.set_backhaul_pref.rsp)


class TestBandInterference:
    def test_enable_requires_config(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_band_interference.req, {"enable": True})
        assert err(pub, T.set_band_interference.rsp)["code"] == "INVALID_ARGUMENTS"

    def test_disable_without_config_allowed(self):
        """The SDK permits a null config when disabling."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_band_interference.req, {"enable": False})
        ok(pub, T.set_band_interference.rsp)

    def test_disable_clears_config(self):
        """requestBandInterferenceConfig must report no config when disabled --
        the SDK says 'set to nullptr if interference management is disabled'."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_band_interference.req,
             {"enable": True, "config": {"priority": "N79"}})
        send(ao, T.set_band_interference.req, {"enable": False})
        pub.reset()
        send(ao, T.request_band_interference.req, {})
        data = ok(pub, T.request_band_interference.rsp)
        assert data["isEnabled"] is False
        assert "config" not in data

    def test_enabled_config_round_trips(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_band_interference.req,
             {"enable": True,
              "config": {"priority": "WLAN", "wlanWaitTimeInSec": 10,
                         "n79WaitTimeInSec": 20}})
        pub.reset()
        send(ao, T.request_band_interference.req, {})
        data = ok(pub, T.request_band_interference.rsp)
        assert data["config"]["priority"] == "WLAN"
        assert data["config"]["wlanWaitTimeInSec"] == 10


class TestWwanConnectivity:
    def test_set_publishes_indication_on_change(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_wwan_connectivity.req, {"slot": 1, "allow": False})
        ok(pub, T.set_wwan_connectivity.rsp)
        got = inds(pub, T.wwan_connectivity_changed.ind)
        assert len(got) == 1
        assert got[0]["payload"]["data"] == {"slot": 1, "isConnectivityAllowed": False}

    def test_no_indication_when_value_unchanged(self):
        """Re-asserting the current value is a no-op; a spurious callback would
        look like a real transition to a listener."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_wwan_connectivity.req, {"slot": 1, "allow": True})
        ok(pub, T.set_wwan_connectivity.rsp)
        assert inds(pub, T.wwan_connectivity_changed.ind) == []

    def test_request_reports_per_slot(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_wwan_connectivity.req, {"slot": 1, "allow": False})
        pub.reset()
        send(ao, T.request_wwan_connectivity.req, {"slot": 2})
        assert ok(pub, T.request_wwan_connectivity.rsp)["isAllowed"] is True

    def test_force_action_publishes_indication(self):
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_wwan_connectivity({"slot": 1, "isConnectivityAllowed": False})
        time.sleep(0.1)
        assert len(inds(pub, T.wwan_connectivity_changed.ind)) == 1
        assert ao._store.is_wwan_allowed(1) is False


class TestMacSec:
    def test_round_trip(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_macsec_state.req, {"enable": True})
        ok(pub, T.set_macsec_state.rsp)
        pub.reset()
        send(ao, T.request_macsec_state.req, {})
        assert ok(pub, T.request_macsec_state.rsp)["enabled"] is True


class TestSwitchBackhaul:
    def test_switch_succeeds(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.switch_backhaul.req,
             {"source": {"backhaul": "WWAN"}, "dest": {"backhaul": "WLAN"}})
        ok(pub, T.switch_backhaul.rsp)

    def test_identical_source_and_dest_rejected(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.switch_backhaul.req,
             {"source": {"backhaul": "WWAN"}, "dest": {"backhaul": "WWAN"}})
        assert err(pub, T.switch_backhaul.rsp)["code"] == "INVALID_ARGUMENTS"


class TestDds:
    def test_switch_to_other_slot_publishes_indication(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.request_dds_switch.req, {"slot": 2, "ddsType": "PERMANENT"})
        ok(pub, T.request_dds_switch.rsp)
        assert len(inds(pub, T.dds_changed.ind)) == 1

    def test_permanent_switch_to_current_permanent_rejected(self):
        """The SDK documents OPERATION_NOT_ALLOWED here -- there is nothing to
        switch."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.request_dds_switch.req, {"slot": 1, "ddsType": "PERMANENT"})
        assert err(pub, T.request_dds_switch.rsp)["code"] == "OPERATION_NOT_ALLOWED"

    def test_temporary_switch_to_current_slot_allowed(self):
        """Only the permanent-to-already-permanent case is disallowed."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.request_dds_switch.req, {"slot": 1, "ddsType": "TEMPORARY"})
        ok(pub, T.request_dds_switch.rsp)

    def test_request_current(self):
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.request_current_dds.req, {})
        data = ok(pub, T.request_current_dds.rsp)
        assert data == {"slot": 1, "ddsType": "PERMANENT"}

    def test_force_dds_publishes_indication(self):
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_dds({"slot": 2, "ddsType": "TEMPORARY"})
        time.sleep(0.1)
        assert len(inds(pub, T.dds_changed.ind)) == 1
        assert ao._store.dds_slot == 2


class TestSyncMirrors:
    """The retained mirrors backing the six synchronous ErrorCode APIs."""

    def test_both_mirrors_published_retained_on_ready(self):
        pub = MockPublish()
        ao = NetSettingsAO(slot_id=1, mpss_src="mpss-dev-1")
        ao.start(pub, __import__("unittest.mock", fromlist=["MagicMock"]).MagicMock())
        from sml.mpss.data.tests._helpers import wait_for_state
        wait_for_state(ao, "smfn_set_ready")
        time.sleep(0.05)
        for topic in (T.ippt_config_sync.ind, T.ip_config_sync.ind):
            got = inds(pub, topic)
            assert len(got) == 1, f"{topic} not published on Ready"
            # Retained is the whole point: a PA attaching later must seed its
            # write-through mirror from ground truth, not local defaults.
            assert got[0]["retain"] is True
        ao.stop()

    def test_ippt_entries_upsert_not_replace(self):
        """A single-entry write must not clear the rest of the table."""
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_ippt_config({"entries": [
            {"profileId": 1, "vlanId": -1, "slot": 1, "ipptOpr": "ENABLE"}]})
        ao.force_ippt_config({"entries": [
            {"profileId": 2, "vlanId": -1, "slot": 1, "ipptOpr": "ENABLE"}]})
        time.sleep(0.1)
        assert len(ao._store.ippt_entries) == 2

    def test_ippt_same_key_overwrites(self):
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_ippt_config({"entries": [
            {"profileId": 1, "vlanId": -1, "slot": 1, "ipptOpr": "ENABLE"}]})
        ao.force_ippt_config({"entries": [
            {"profileId": 1, "vlanId": -1, "slot": 1, "ipptOpr": "DISABLE"}]})
        time.sleep(0.1)
        assert len(ao._store.ippt_entries) == 1
        assert list(ao._store.ippt_entries.values())[0]["ipptOpr"] == "DISABLE"

    def test_nat_toggle_without_entries(self):
        """natEnabled is optional so NAT can be toggled without restating the
        whole entry table."""
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_ippt_config({"entries": [
            {"profileId": 1, "vlanId": -1, "slot": 1, "ipptOpr": "ENABLE"}]})
        ao.force_ippt_config({"natEnabled": False})
        time.sleep(0.1)
        assert ao._store.ippt_nat_enabled is False
        assert len(ao._store.ippt_entries) == 1

    def test_ip_config_keyed_on_iface_family_vlan(self):
        """IPv4 and IPv6 on one interface are separate configs: the SDK requires
        setIpConfig to be called once per family."""
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_ip_config({"entries": [
            {"ifType": "ETH", "ipFamily": "IPV4", "ipType": "STATIC_IP",
             "ipOpr": "ENABLE"},
            {"ifType": "ETH", "ipFamily": "IPV6", "ipType": "DYNAMIC_IP",
             "ipOpr": "ENABLE"}]})
        time.sleep(0.1)
        assert len(ao._store.ip_config_entries) == 2

    def test_mirror_republished_after_mutation(self):
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_ip_config({"entries": [
            {"ifType": "ETH", "ipFamily": "IPV4", "ipType": "STATIC_IP",
             "ipOpr": "ENABLE"}]})
        time.sleep(0.1)
        got = inds(pub, T.ip_config_sync.ind)
        assert len(got) == 1
        assert len(got[0]["payload"]["data"]["entries"]) == 1


class TestFactoryReset:
    def test_clears_every_setting(self):
        """Keeping any setting would leave a state the factory never shipped."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.set_backhaul_pref.req, {"backhaulPref": ["ETH"]})
        send(ao, T.set_macsec_state.req, {"enable": True})
        ao.force_ippt_config({"natEnabled": False})
        time.sleep(0.1)
        pub.reset()
        send(ao, T.restore_factory_settings.req, {"opType": "DATA_LOCAL"})
        ok(pub, T.restore_factory_settings.rsp)
        assert ao._store.backhaul_pref == []
        assert ao._store.macsec_enabled is False
        assert ao._store.ippt_nat_enabled is True

    def test_republishes_mirrors(self):
        """A PA holding a stale mirror must be told the table was wiped."""
        pub = MockPublish()
        ao = _ao(pub)
        send(ao, T.restore_factory_settings.req, {"opType": "DATA_LOCAL"})
        assert len(inds(pub, T.ippt_config_sync.ind)) == 1
        assert len(inds(pub, T.ip_config_sync.ind)) == 1
