# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Regression tests for the sim domain's wiring into the MPSS runtime.

Everything here pins a property that was ACTUALLY BROKEN and is invisible to
the per-AO unit tests (which drive `SimCardAO`/`SubscriptionAO` directly and so
bypass both the MqttClient router and the process entry point).

Covered:
  1. `SimSubsystem.owns_topic` exists and is honoured by MqttClient's router.
     Its absence made every inbound sim message a silent no-op.
  2. `sml/mpss/__main__.py` builds `SimSubsystem` from a name that is actually
     defined -- it referenced an undefined `slot_state`, a NameError on boot.
  3. Outbound rsp/ind payloads are schema-validated (architecture §9).
  4. Inbound request payloads are schema-validated before touching World State.
  5. Instrumentation reaches the sim AOs (architecture §7 / parity with data).

Together 1+2 are the difference between "the 7 taf_sim_* APIs answer" and "the
whole stack fails to start", so they are worth pinning independently of the
richer behavioural tests next door.
"""
from __future__ import annotations

import ast
import json
from pathlib import Path

import jsonschema
import pytest

from sml.config.models import SimCard
from sml.mpss import instrumentation as _instr
from sml.mpss.sim import SimSubsystem
from sml.mpss.sim.card import SimCardAO
from sml.mpss.sim.subscription import SubscriptionAO
from generated.python.topics import sim as topics_sim

_ICCID = "8986011234567890123"
_IMSI = "460000123456789"


class _MockPublish:
    def __init__(self):
        self.calls: list = []

    def __call__(self, topic, payload, qos, retain=False):
        self.calls.append({"topic": topic, "payload": json.loads(payload),
                           "qos": qos, "retain": retain})

    def reset(self):
        self.calls.clear()


def _envelope(data: dict, corr: str = "00ab") -> bytes:
    return json.dumps({
        "v": 1, "corrId": corr, "ts": 1718000000000,
        "src": "dcs-master-1234", "data": data,
    }).encode()


@pytest.fixture
def started_ss():
    ss = SimSubsystem(
        installed_sim=SimCard(id="sim_card_001", iccid=_ICCID, imsi=_IMSI,
                              home_plmn="46000"),
        role="dev",
    )
    ss.start(_MockPublish(), lambda t: None, lambda t: None)
    yield ss
    ss.stop()


@pytest.fixture(autouse=True)
def _restore_mode():
    prev = _instr.current_mode()
    yield
    _instr.set_current_mode(prev)


# ---------------------------------------------------------------------------
# 1. The subsystem protocol MqttClient actually calls
# ---------------------------------------------------------------------------

def test_sim_subsystem_satisfies_mqtt_client_protocol(started_ss):
    """MqttClient._on_message calls owns_topic() BEFORE handle_message().

    A subsystem missing owns_topic raises AttributeError inside the router's
    try/except, which logs and continues -- so every ap/req/sim/** message is
    dropped as "not consumed" and the PA waits forever. Pin the whole duck-typed
    protocol, not just this one method, since the router relies on all of it.
    """
    for method in ("start", "stop", "owns_topic", "handle_message"):
        assert callable(getattr(started_ss, method, None)), f"missing {method}"


@pytest.mark.parametrize("topic_attr", [
    "get_state", "set_power", "get_iccid", "get_imsi",
])
def test_owns_every_rpc_topic_backing_the_seven_apis(started_ss, topic_attr):
    """The 4 RPCs behind the 7 taf_sim_* APIs must all be claimed.

    taf_sim_GetState/IsReady -> get_state, SetPower -> set_power,
    GetICCID -> get_iccid, GetIMSI -> get_imsi. (AddNewStateHandler /
    RemoveNewStateHandler are listener registration, served by the card_state
    indication rather than an RPC.)
    """
    topic = getattr(topics_sim, topic_attr).req
    assert started_ss.owns_topic(topic)


def test_does_not_own_foreign_domain_topics(started_ss):
    """owns_topic must not over-claim: the router hands the message to the
    FIRST subsystem that says yes, so a greedy sim domain would starve data."""
    assert not started_ss.owns_topic("ap/req/data/start_data_call")
    assert not started_ss.owns_topic("mp/ind/data/call_state")


def test_router_delivers_to_sim_subsystem():
    """End-to-end through the real MqttClient routing helper."""
    from sml.mpss.config import BrokerConfig, DebugConfig, MpssConfig
    from sml.mpss.mqtt_client import MessageReceivedPayload, MqttClient

    cfg = MpssConfig(broker=BrokerConfig(transport="tcp"),
                     debug=DebugConfig(log_level="CRITICAL"))
    client = MqttClient(cfg)
    try:
        pub = _MockPublish()
        ss = SimSubsystem(
            installed_sim=SimCard(id="c", iccid=_ICCID, imsi=_IMSI,
                                  home_plmn="46000"),
            role="dev",
        )
        ss.start(pub, lambda t: None, lambda t: None)
        client.register_subsystem(ss)
        pub.reset()

        client._on_message(MessageReceivedPayload(
            topic=topics_sim.get_state.req,
            payload=_envelope({"slot": 1}), qos=1,
        ))

        rsp = [c for c in pub.calls if c["topic"] == topics_sim.get_state.rsp]
        assert rsp, "MqttClient did not route ap/req/sim/get_state to SimSubsystem"
        assert rsp[0]["payload"]["data"]["cardState"] == "PRESENT"
        ss.stop()
    finally:
        client.stop()


# ---------------------------------------------------------------------------
# 2. The process entry point must not reference undefined names
# ---------------------------------------------------------------------------

def test_main_builds_sim_subsystem_from_a_defined_name():
    """`__main__.py` referenced an undefined `slot_state` -> NameError on boot.

    Checked by AST rather than by importing+running main() (which would need a
    broker and a real scenario). Asserts the SimSubsystem(...) call site's
    keyword argument is rooted at a name bound somewhere in the module.
    """
    main_py = Path(__file__).resolve().parents[3] / "mpss" / "__main__.py"
    tree = ast.parse(main_py.read_text(encoding="utf-8"))

    bound = {
        node.id
        for assign in ast.walk(tree) if isinstance(assign, (ast.Assign, ast.AnnAssign))
        for target in ([assign.targets[0]] if isinstance(assign, ast.Assign) else [assign.target])
        for node in ast.walk(target) if isinstance(node, ast.Name)
    }
    bound |= {a.arg for fn in ast.walk(tree)
              if isinstance(fn, ast.FunctionDef) for a in fn.args.args}

    call = next(
        (n for n in ast.walk(tree)
         if isinstance(n, ast.Call) and isinstance(n.func, ast.Name)
         and n.func.id == "SimSubsystem"),
        None,
    )
    assert call is not None, "no SimSubsystem(...) construction found in __main__"

    for kw in call.keywords:
        roots = {n.id for n in ast.walk(kw.value) if isinstance(n, ast.Name)}
        undefined = roots - bound - {"SimSubsystem"}
        assert not undefined, (
            f"SimSubsystem({kw.arg}=...) references undefined name(s) {undefined} "
            "-- this is a NameError at MPSS startup"
        )


# ---------------------------------------------------------------------------
# 3 + 4. Schema validation on both directions (architecture §9)
# ---------------------------------------------------------------------------

def test_outbound_get_state_rsp_is_schema_validated(monkeypatch):
    """A schema-invalid outbound rsp must RAISE, not ship.

    The PA-side ModemBridge enforces the same schema and treats an invalid rsp
    exactly like a timeout, so shipping one converts an MPSS bug into an
    unexplained 30s stall in taf_sim_GetState.
    """
    pub = _MockPublish()
    ao = SimCardAO(slot=1, mpss_src="mpss-dev-1", iccid=_ICCID, imsi=_IMSI)
    ao.start(pub, lambda t: None, lambda t: None)

    # Force World State off-contract; "BANANA" is not in the cardState enum.
    ao.card_state = "BANANA"
    with pytest.raises(jsonschema.ValidationError):
        ao._handle_get_state({"corrId": "00ab", "src": "dcs-1", "data": {"slot": 1}})


def test_outbound_card_state_ind_is_schema_validated():
    pub = _MockPublish()
    ao = SimCardAO(slot=1, mpss_src="mpss-dev-1", iccid=_ICCID, imsi=_IMSI)
    ao.start(pub, lambda t: None, lambda t: None)

    ao.app_state = "NOT_A_REAL_APP_STATE"
    with pytest.raises(jsonschema.ValidationError):
        ao._publish_card_state()


def test_inbound_set_power_with_wrong_type_is_rejected(started_ss):
    """powerOn must be a boolean.

    Before inbound payload validation, `powerOn: "yes-please"` was truthy, so
    the card was left PRESENT/READY while the requester believed it had powered
    the card down -- the exact "state says X, wire says Y" divergence
    invariant (h) exists to prevent.
    """
    card = started_ss._card_ao
    before = (card.card_state, card.app_state, card.power_on)

    started_ss.handle_message(
        topics_sim.set_power.req,
        _envelope({"slot": 1, "powerOn": "yes-please"}),
    )

    assert (card.card_state, card.app_state, card.power_on) == before


def test_inbound_valid_set_power_still_works(started_ss):
    """The guard above must not block legitimate traffic (taf_sim_SetPower)."""
    card = started_ss._card_ao
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": False}),
    )
    # Powered down but still physically present -- keeping the card non-ABSENT
    # is what preserves the PA's cached ICard pointer so power-on stays routable.
    assert card.card_state == "PRESENT"
    assert card.app_state == "UNKNOWN"
    assert card.power_on is False

    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": True}),
    )
    assert card.card_state == "PRESENT"
    assert card.app_state == "READY"


def test_inbound_unknown_field_is_rejected(started_ss):
    """Schemas are additionalProperties:false -- a typo'd field must not pass
    silently (invariant (g)'s runtime counterpart)."""
    card = started_ss._card_ao
    before = (card.card_state, card.app_state)
    started_ss.handle_message(
        topics_sim.set_power.req,
        _envelope({"slot": 1, "powerOn": False, "powrOn": True}),
    )
    assert (card.card_state, card.app_state) == before


# ---------------------------------------------------------------------------
# 4b. Card power state must gate subscription identity (cross-AO wiring)
#
# CardManager and SubscriptionManager are independent wire managers, so nothing
# in either AO alone makes identity follow card power. The link is established
# in SimSubsystem._do_start, which is only exercised here.
# ---------------------------------------------------------------------------

def _iccid_imsi(ss) -> tuple:
    pub = ss._card_ao._publish_fn
    pub.reset()
    ss.handle_message(topics_sim.get_iccid.req, _envelope({"slot": 1}))
    ss.handle_message(topics_sim.get_imsi.req, _envelope({"slot": 1}))
    iccid = [c for c in pub.calls if c["topic"] == topics_sim.get_iccid.rsp]
    imsi = [c for c in pub.calls if c["topic"] == topics_sim.get_imsi.rsp]
    assert iccid and imsi, "identity RPCs must always be answered"
    return iccid[-1]["payload"]["data"]["iccid"], imsi[-1]["payload"]["data"]["imsi"]


def test_power_off_hides_identity_then_power_on_restores_it(started_ss):
    """setPower OFF -> ON must return the ORIGINAL card details.

    Reported bug: after a power cycle the ICCID/IMSI stayed empty forever,
    because the restore was suppressed once the hide had been recorded.
    """
    assert _iccid_imsi(started_ss) == (_ICCID, _IMSI)

    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": False}))
    assert _iccid_imsi(started_ss) == ("", "")

    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": True}))
    assert _iccid_imsi(started_ss) == (_ICCID, _IMSI)


def test_power_on_republishes_restored_identity(started_ss):
    """The PA caches identity from sub_info_changed, so the restore must be
    announced -- an RPC-only fix would leave the cached value empty."""
    pub = started_ss._card_ao._publish_fn
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": False}))
    pub.reset()
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": True}))

    ind = [c for c in pub.calls if c["topic"] == topics_sim.sub_info_changed.ind]
    assert ind, "power-on must re-announce identity via sub_info_changed"
    assert ind[-1]["payload"]["data"] == {"iccid": _ICCID, "imsi": _IMSI}


def test_hotswap_during_power_off_wins_after_power_on(started_ss):
    """If a hotswap happened while powered off, power-on shows the NEW card."""
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": False}))
    started_ss.dispatch_action("sim.hotswap", {
        "slot": 1, "iccid": "8986011234567890999", "imsi": "460000999999999",
    })
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": True}))

    assert _iccid_imsi(started_ss) == ("8986011234567890999", "460000999999999")


def test_absent_card_hides_identity_and_present_restores_it(started_ss):
    """force_card_state ABSENT/PRESENT gates identity the same way power does."""
    started_ss.dispatch_action("sim.force_card_state", {
        "slot": 1, "cardState": "ABSENT", "appState": "UNKNOWN"})
    assert _iccid_imsi(started_ss) == ("", "")

    started_ss.dispatch_action("sim.force_card_state", {
        "slot": 1, "cardState": "PRESENT", "appState": "READY"})
    assert _iccid_imsi(started_ss) == (_ICCID, _IMSI)


def test_power_off_never_reports_absent_for_an_installed_card(started_ss):
    """Guards the PA's cached ICard pointer against being nulled.

    `taf_pa_sim_SetPower` returns FAULT when `pa.managers.cards[slot]` is null,
    and that cache is nulled by an ABSENT report. If any card_state indication
    during a power-off says ABSENT, power-ON becomes undeliverable and the slot
    wedges at `absent` -- the exact symptom reported from tafSimIntTest.
    """
    pub = started_ss._card_ao._publish_fn
    pub.reset()
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": False}))

    states = [c["payload"]["data"] for c in pub.calls
              if c["topic"] == topics_sim.card_state.ind]
    assert states, "power-off must publish card_state"
    assert all(s["cardState"] != "ABSENT" for s in states), states


def test_force_card_state_keeps_power_on_consistent(started_ss):
    """Forcing PRESENT/READY must not leave power_on stale at False.

    Otherwise the card advertises READY while power_on is False, and the next
    set_power(True) looks like a no-op -- a state/wire divergence.
    """
    card = started_ss._card_ao
    started_ss.dispatch_action("sim.force_power", {"slot": 1, "powerOn": False})
    assert card.power_on is False

    started_ss.dispatch_action("sim.force_card_state", {
        "slot": 1, "cardState": "PRESENT", "appState": "READY"})
    assert card.power_on is True, "power_on must follow a forced READY state"


def test_hotswap_emits_exactly_three_card_state_and_one_sub_info(started_ss):
    """Pins the documented hotswap contract: 3 card_state + 1 sub_info_changed.

    Both action_sim.yaml and action.sim.hotswap.req specify these counts. An
    extra (empty) sub_info_changed mid-swap makes the PA clear its cached ICCID,
    IMSI and phone number (tafSimCardImpl.cpp UpdateLocalSimState), so any
    listener sampling that window sees a blank card.
    """
    pub = started_ss._card_ao._publish_fn
    pub.reset()
    started_ss.dispatch_action("sim.hotswap", {
        "slot": 1, "iccid": "8986011234567890999", "imsi": "460000999999999"})

    card_states = [c["payload"]["data"] for c in pub.calls
                   if c["topic"] == topics_sim.card_state.ind]
    sub_infos = [c["payload"]["data"] for c in pub.calls
                 if c["topic"] == topics_sim.sub_info_changed.ind]

    assert len(card_states) == 3, card_states
    assert len(sub_infos) == 1, sub_infos
    # The single identity event carries the NEW card, never empty strings.
    assert sub_infos[0] == {"iccid": "8986011234567890999",
                            "imsi": "460000999999999"}


def test_hotswap_never_publishes_empty_identity(started_ss):
    """No indication during a swap may carry an empty ICCID."""
    pub = started_ss._card_ao._publish_fn
    pub.reset()
    started_ss.dispatch_action("sim.hotswap", {
        "slot": 1, "iccid": "8986011234567890999", "imsi": "460000999999999"})

    sub_infos = [c["payload"]["data"] for c in pub.calls
                 if c["topic"] == topics_sim.sub_info_changed.ind]
    assert all(s["iccid"] for s in sub_infos), sub_infos


def test_power_cycle_after_hotswap_restores_swapped_identity(started_ss):
    """State must stay coherent when the two fixes interact."""
    started_ss.dispatch_action("sim.hotswap", {
        "slot": 1, "iccid": "8986011234567890999", "imsi": "460000999999999"})
    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": False}))
    assert _iccid_imsi(started_ss) == ("", "")

    started_ss.handle_message(
        topics_sim.set_power.req, _envelope({"slot": 1, "powerOn": True}))
    assert _iccid_imsi(started_ss) == ("8986011234567890999", "460000999999999")


# ---------------------------------------------------------------------------
# 5. Instrumentation parity with the data domain (architecture §7)
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("mode,trace,spy", [
    ("off", False, False),
    ("on", True, False),
    ("verbose", True, True),
])
@pytest.mark.parametrize("factory", [
    lambda: SimCardAO(slot=1, mpss_src="mpss-dev-1", iccid=_ICCID, imsi=_IMSI),
    lambda: SubscriptionAO(slot=1, mpss_src="mpss-dev-1", iccid=_ICCID, imsi=_IMSI),
    lambda: SimSubsystem(installed_sim=None, role="dev"),
])
def test_every_sim_ao_applies_current_mode(factory, mode, trace, spy):
    """Each sim AO must honour the startup-resolved mode, like every data AO.

    Without this the sim domain stays silent when an operator explicitly asked
    for instrumentation -- and a stuck card state is invisible in the one place
    it would be obvious.
    """
    _instr.set_current_mode(mode)
    ao = factory()
    assert ao.live_trace is trace
    assert ao.live_spy is spy
