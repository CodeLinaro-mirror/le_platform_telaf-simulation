# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for ModemSettingsStore and NetModemSubsystem. No broker required.

These cover the three families that deliberately stayed on MPSS when the rest
of the net domain moved to APSS: WWAN connectivity gating, DDS switching and
band interference. Behaviour is carried over verbatim from the old
mpss/net/tests/test_settings.py so the move is provably behaviour-preserving --
only the owning process and the topic namespace changed.
"""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

from sml.mpss.data import DataSubsystem
from sml.mpss.data.tests._helpers import wait_for_state, wait_until
from sml.mpss.net_modem import ModemSettingsStore, NetModemSubsystem
from generated.python.topics import data as topics_data
from generated.python.topics import net_modem as T


class MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "retain": retain})

    def reset(self):
        self.calls.clear()


def _ao(pub, **kw) -> NetModemSubsystem:
    ao = NetModemSubsystem(slot_id=1, role="dev", **kw)
    ao.start(pub, MagicMock(), MagicMock())
    wait_for_state(ao, "smfn_modem_ready")
    pub.reset()
    return ao


def send(ao, topic, data):
    msg = json.dumps({
        "v": 1, "corrId": "00ab", "ts": 1718000000000,
        "src": "netsvc-master-1234", "data": data,
    }).encode()
    ao.handle_message(topic, msg)
    time.sleep(0.05)


def _rsp(pub, topic):
    return [c for c in pub.calls if c["topic"] == topic]


def ok(pub, topic):
    calls = _rsp(pub, topic)
    assert len(calls) == 1, f"expected 1 rsp on {topic}, got {len(calls)}"
    env = calls[0]["payload"]
    assert "error" not in env, f"unexpected error: {env.get('error')}"
    return env["data"]


def err(pub, topic):
    calls = _rsp(pub, topic)
    assert len(calls) == 1, f"expected 1 rsp on {topic}, got {len(calls)}"
    env = calls[0]["payload"]
    assert "error" in env, f"expected an error, got data={env.get('data')}"
    return env["error"]


def inds(pub, topic):
    return [c for c in pub.calls if c["topic"] == topic]


class TestOwnership:
    """The split must be real: MPSS owns these topics and only these."""

    def test_every_owned_topic_is_mp_prefixed(self):
        pub = MockPublish()
        ao = _ao(pub)
        for topic in ao._owned_topics:
            assert topic.startswith("mp/req/"), \
                f"net_modem subscribed to non-mp topic {topic}"

    def test_does_not_own_ap_local_settings_topics(self):
        """Backhaul preference, MacSec, IPPT and IP config belong to APSS."""
        from generated.python.topics import net_settings as TS
        pub = MockPublish()
        ao = _ao(pub)
        for topic in (TS.set_backhaul_pref.req, TS.set_macsec_state.req,
                      TS.switch_backhaul.req, TS.set_ippt_config.req,
                      TS.set_ip_config.req,
                      TS.restore_factory_settings.req):
            assert not ao.owns_topic(topic), \
                f"net_modem must not own AP-local topic {topic}"

    def test_readiness_is_retained(self):
        """The PA gates its Ready shell on this retained indication, so a PA
        that attaches late must still learn the subsystem is up."""
        pub = MockPublish()
        ao = NetModemSubsystem(slot_id=1, role="dev")
        ao.start(pub, MagicMock(), MagicMock())
        wait_for_state(ao, "smfn_modem_ready")
        time.sleep(0.05)
        got = inds(pub, T.subsys_ready_net_modem.ind)
        assert len(got) == 1
        assert got[0]["retain"] is True
        assert got[0]["payload"]["data"] == {"ready": True, "status": "AVAILABLE"}
        ao.stop()


class TestModemSettingsStore:
    def test_wwan_defaults_to_allowed(self):
        """The SDK documents 'allow' as the default, so an unqueried slot must
        not read as disallowed."""
        assert ModemSettingsStore().is_wwan_allowed(1) is True

    def test_wwan_is_per_slot(self):
        store = ModemSettingsStore()
        store.wwan_allowed[1] = False
        assert store.is_wwan_allowed(1) is False
        assert store.is_wwan_allowed(2) is True


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
        assert ok(pub, T.request_current_dds.rsp) == {"slot": 1,
                                                      "ddsType": "PERMANENT"}

    def test_force_dds_publishes_indication(self):
        pub = MockPublish()
        ao = _ao(pub)
        ao.force_dds({"slot": 2, "ddsType": "TEMPORARY"})
        time.sleep(0.1)
        assert len(inds(pub, T.dds_changed.ind)) == 1
        assert ao._store.dds_slot == 2


class TestActionDispatch:
    """Scenario-timeline actions must reach the modem AO under the new domain."""

    def test_dispatch_force_dds_by_canonical_name(self):
        pub = MockPublish()
        ao = _ao(pub)
        handled = ao.dispatch_action("net_modem.force_dds",
                                     {"slot": 2, "ddsType": "PERMANENT"})
        time.sleep(0.1)
        assert handled is True
        assert len(inds(pub, T.dds_changed.ind)) == 1

    def test_dispatch_force_wwan_by_canonical_name(self):
        pub = MockPublish()
        ao = _ao(pub)
        handled = ao.dispatch_action("net_modem.force_wwan_connectivity",
                                     {"slot": 1, "isConnectivityAllowed": False})
        time.sleep(0.1)
        assert handled is True
        assert len(inds(pub, T.wwan_connectivity_changed.ind)) == 1

    def test_foreign_action_is_declined(self):
        """Returning False is what lets the top-level router try another
        domain instead of silently swallowing the step."""
        pub = MockPublish()
        ao = _ao(pub)
        assert ao.dispatch_action("net.force_vlan_hw_accel",
                                  {"state": "ACTIVE"}) is False


class TestWwanConnectivityCrossDomain:
    """The contract net_modem.yaml promises but this AO cannot keep alone.

    ``net_modem.yaml`` states that disallowing WWAN "tears down active WWAN
    calls ... the data domain then publishes NET_NO_NET for each". This AO only
    owns the permission flag -- the sessions it governs live in the data domain
    -- so every other test in this file can pass while data calls stay
    CONNECTED. That is exactly the hole this class closes.

    Both AOs are instantiated and the indication is relayed **verbatim**: the
    envelope bytes that ``NetModemSubsystem`` actually published are handed to
    ``DataSubsystem`` unmodified, standing in for the broker. A hand-built
    payload would not catch a field-name or schema drift between publisher and
    consumer, which is the most likely way this contract silently breaks.
    """

    @staticmethod
    def _start_both():
        """Bring up a real net_modem AO and a real data AO on the same slot."""
        nm_pub = MockPublish()
        nm = _ao(nm_pub)

        data_pub = MockPublish()
        ds = DataSubsystem(slot_id=1)
        ds.start(data_pub, MagicMock(), MagicMock())
        # DataSubsystem builds sub-AOs in its Starting entry; poll until the
        # connection AO is actually consuming before sending it anything.
        assert wait_until(lambda: ds.owns_topic(topics_data.start_data_call.req),
                          timeout=5.0), "data subsystem never became ready"
        time.sleep(0.2)
        data_pub.reset()
        return nm, nm_pub, ds, data_pub

    @staticmethod
    def _send(ao, topic, data, pause=0.3):
        ao.handle_message(topic, json.dumps({
            "v": 1, "corrId": "00ab", "ts": 1718000000000,
            "src": "dcs-master-1234", "data": data,
        }).encode())
        time.sleep(pause)

    @classmethod
    def _start_call(cls, ds, profile_id=1):
        cls._send(ds, topics_data.start_data_call.req,
                  {"profileId": profile_id, "ipFamily": "IPV4V6", "ifname": "",
                   "opType": "DATA_LOCAL", "slot": 1})

    @staticmethod
    def _call_states(data_pub):
        return [c["payload"]["data"] for c in data_pub.calls
                if c["topic"] == topics_data.call_state.ind]

    @classmethod
    def _relay_wwan_indication(cls, nm_pub, ds):
        """Hand net_modem's published indication to data, as the broker would.

        Returns the relayed payload so a caller can assert on what actually
        crossed the boundary.
        """
        published = [c for c in nm_pub.calls
                     if c["topic"] == T.wwan_connectivity_changed.ind]
        assert published, "net_modem published no wwan_connectivity_changed indication"
        env = published[-1]["payload"]
        ds.handle_message(T.wwan_connectivity_changed.ind,
                          json.dumps(env).encode())
        time.sleep(0.3)
        return env["data"]

    def test_disable_wwan_tears_down_active_call_with_no_net(self):
        nm, nm_pub, ds, data_pub = self._start_both()
        try:
            self._start_call(ds)
            assert wait_until(lambda: any(s["status"] == "CONNECTED"
                                          for s in self._call_states(data_pub)),
                              timeout=3.0), "call never reached CONNECTED"

            # Disable through the real RPC, not by poking the store.
            nm_pub.reset()
            self._send(nm, T.set_wwan_connectivity.req, {"slot": 1, "allow": False})

            data_pub.reset()
            relayed = self._relay_wwan_indication(nm_pub, ds)
            assert relayed == {"slot": 1, "isConnectivityAllowed": False}

            states = self._call_states(data_pub)
            assert any(s["status"] == "NO_NET" for s in states), (
                f"data domain left the call up after WWAN was disabled: {states}")
            # NETWORK_DROP, not the clean-stop reason -- this is what tells the
            # PA the network dropped the PDN rather than the client stopping it.
            no_net = [s for s in states if s["status"] == "NO_NET"]
            assert no_net[0]["end_reason"] == {"type": 0x02, "code": 206}
        finally:
            ds.stop()
            nm.stop()

    def test_start_data_call_rejected_while_wwan_disabled(self):
        nm, nm_pub, ds, data_pub = self._start_both()
        try:
            nm_pub.reset()
            self._send(nm, T.set_wwan_connectivity.req, {"slot": 1, "allow": False})
            self._relay_wwan_indication(nm_pub, ds)

            data_pub.reset()
            self._start_call(ds, profile_id=9)

            rsps = [c["payload"] for c in data_pub.calls
                    if c["topic"] == topics_data.start_data_call.rsp]
            assert rsps, "start_data_call produced no response"
            assert "error" in rsps[0], (
                f"new call allowed while WWAN disabled: {rsps[0].get('data')}")
            assert rsps[0]["error"]["code"] == "INVALID_OPERATION"
            # And nothing was actually brought up.
            assert not any(s["status"] in ("CONNECTING", "CONNECTED")
                           for s in self._call_states(data_pub))
        finally:
            ds.stop()
            nm.stop()

    def test_every_active_call_on_the_slot_is_torn_down(self):
        """The registry promises NET_NO_NET "for each" call, so one indication
        per session -- not a single summary event."""
        nm, nm_pub, ds, data_pub = self._start_both()
        try:
            for pid in (1, 2, 3):
                self._start_call(ds, profile_id=pid)
            assert wait_until(
                lambda: len({s["profileId"] for s in self._call_states(data_pub)
                             if s["status"] == "CONNECTED"}) == 3,
                timeout=4.0), "not all three calls reached CONNECTED"

            nm_pub.reset()
            self._send(nm, T.set_wwan_connectivity.req, {"slot": 1, "allow": False})

            data_pub.reset()
            self._relay_wwan_indication(nm_pub, ds)

            dropped = {s["profileId"] for s in self._call_states(data_pub)
                       if s["status"] == "NO_NET"}
            assert dropped == {1, 2, 3}, f"only {dropped} were torn down"
        finally:
            ds.stop()
            nm.stop()

    def test_call_allowed_again_after_wwan_re_enabled(self):
        """Re-enabling must lift the gate; a torn-down PDN is re-established by
        the client, not restored automatically."""
        nm, nm_pub, ds, data_pub = self._start_both()
        try:
            nm_pub.reset()
            self._send(nm, T.set_wwan_connectivity.req, {"slot": 1, "allow": False})
            self._relay_wwan_indication(nm_pub, ds)

            nm_pub.reset()
            self._send(nm, T.set_wwan_connectivity.req, {"slot": 1, "allow": True})
            self._relay_wwan_indication(nm_pub, ds)

            data_pub.reset()
            self._start_call(ds, profile_id=4)

            rsps = [c["payload"] for c in data_pub.calls
                    if c["topic"] == topics_data.start_data_call.rsp]
            assert rsps and "error" not in rsps[0], (
                f"call still refused after WWAN re-enabled: {rsps}")
        finally:
            ds.stop()
            nm.stop()

    def test_force_action_also_drives_teardown(self):
        """The scenario-timeline action must have the same reach as the RPC --
        it is the path a test scenario uses to model a network-side change."""
        nm, nm_pub, ds, data_pub = self._start_both()
        try:
            self._start_call(ds)
            assert wait_until(lambda: any(s["status"] == "CONNECTED"
                                          for s in self._call_states(data_pub)),
                              timeout=3.0)

            nm_pub.reset()
            nm.force_wwan_connectivity({"slot": 1, "isConnectivityAllowed": False})
            time.sleep(0.2)

            data_pub.reset()
            self._relay_wwan_indication(nm_pub, ds)

            assert any(s["status"] == "NO_NET" for s in self._call_states(data_pub))
        finally:
            ds.stop()
            nm.stop()

    def test_disabling_other_slot_leaves_our_calls_up(self):
        """The permission is slot-scoped; a slot-2 change must not drop slot-1
        calls."""
        nm, nm_pub, ds, data_pub = self._start_both()
        try:
            self._start_call(ds)
            assert wait_until(lambda: any(s["status"] == "CONNECTED"
                                          for s in self._call_states(data_pub)),
                              timeout=3.0)

            nm_pub.reset()
            self._send(nm, T.set_wwan_connectivity.req, {"slot": 2, "allow": False})

            data_pub.reset()
            self._relay_wwan_indication(nm_pub, ds)

            states = [s["status"] for s in self._call_states(data_pub)]
            assert "NO_NET" not in states, (
                f"slot-2 disable tore down a slot-1 call: {states}")
        finally:
            ds.stop()
            nm.stop()
