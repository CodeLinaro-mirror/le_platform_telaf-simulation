# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for SettingsStore and NetSettingsAO. No broker required.

Covers only the AP-local half of IDataSettingsManager. The modem-owned
families (WWAN connectivity, DDS, band interference) are tested in
sml/mpss/net_modem/tests/test_net_modem.py.
"""
from __future__ import annotations

import time

from sml.apss.net.settings import NetSettingsAO, SettingsStore
from sml.apss.net.tests._helpers import MockPublish, boot, err, inds, ok, send
from generated.python.topics import net_settings as T


def _ao(pub, **kw) -> NetSettingsAO:
    return boot(NetSettingsAO(slot_id=1, apss_src="apss-dev-1", **kw),
                pub, "smfn_set_ready")


class TestSettingsStore:
    """The AP-local store must not retain any modem-owned state.

    These are the tests that make the APSS/MPSS split real rather than
    cosmetic: a leftover field or an orphaned subscription is exactly how a
    "moved" domain quietly ends up with two owners.
    """

    def test_store_holds_no_modem_state(self):
        store = SettingsStore()
        for attr in ("dds_slot", "dds_type", "wwan_allowed", "band_enabled",
                     "band_config"):
            assert not hasattr(store, attr), \
                f"SettingsStore must not carry modem-owned {attr!r}"

    def test_ao_does_not_own_modem_topics(self):
        from generated.python.topics import net_modem as TM
        pub = MockPublish()
        ao = _ao(pub)
        for topic in (TM.set_wwan_connectivity.req,
                      TM.request_wwan_connectivity.req,
                      TM.set_band_interference.req,
                      TM.request_band_interference.req,
                      TM.request_dds_switch.req,
                      TM.request_current_dds.req):
            assert not ao.owns_topic(topic), \
                f"APSS settings AO must not own modem topic {topic}"

    def test_ao_has_no_modem_force_entry_points(self):
        """force_wwan_connectivity / force_dds belong to the net_modem
        dispatcher on MPSS; leaving them here would let an APSS action mutate
        modem ground truth."""
        pub = MockPublish()
        ao = _ao(pub)
        assert not hasattr(ao, "force_wwan_connectivity")
        assert not hasattr(ao, "force_dds")

    def test_every_owned_topic_is_ap_prefixed(self):
        """APSS must never subscribe on the mp/* namespace."""
        pub = MockPublish()
        ao = _ao(pub)
        for topic in ao._owned_topics:
            assert topic.startswith("ap/req/"), \
                f"APSS settings AO subscribed to non-ap topic {topic}"



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


class TestSyncMirrors:
    """The retained mirrors backing the six synchronous ErrorCode APIs."""

    def test_both_mirrors_published_retained_on_ready(self):
        pub = MockPublish()
        ao = NetSettingsAO(slot_id=1, apss_src="apss-dev-1")
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

