# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side eCall Active Object (EcallManagerAO).

Mirrors ``telux::tel::ICallManager``'s eCall surface plus ``IPhone``'s
``set/getECallOperatingMode`` -- see CallManagerServerImpl.cpp (the eCall
RPCs bolted onto DialerService) and PhoneManagerServerImpl.cpp
(SetECallOperatingMode/GetECallOperatingMode), whose gRPC handlers this
AO's RPC handlers reproduce field-for-field. The per-call HLAP protocol
timing (EcallStateMachine.cpp) is ported into ``ecall.session.EcallSession``,
owned and driven synchronously by this AO -- see that module's docstring
for the pacing-vs-expiry timing adaptation.

State topology is the same Off -> Operating.{Starting,Ready} -> Stopping
shell as ``sml.mpss.radio.phone.RadioPhoneAO`` -- see that module's
docstring for the two-hop-INIT rationale, which applies verbatim here.

World State (flat attributes, same convention as every other domain):
  - ``_config``: global EcallConfig (mute-rx-audio, num type/overridden
    number, canned-MSD, GNSS interval, T2/T7/T9 timer values, MSD
    version) -- mirrors CallManagerServerImpl::SetConfig/GetConfig. Has
    no phoneId key, matching the old proto's phoneId-less
    SetConfigRequest/GetConfig(Empty).
  - ``_operating_mode`` / ``_hlap_timer_status`` / ``_hlap_timer_config``:
    per-phoneId dicts -- mirrors PhoneManagerServerImpl's per-slot
    eCallOperatingMode state and CallManagerServerImpl's per-slot
    ecallHlapTimerStatus/eCallConfig[T10Timer] state.
  - ``_sessions``: at most one active ``EcallSession`` per phoneId --
    mirrors taf_ecall's isIdle()/getInProgressCalls() guard against a
    second concurrent eCall on the same phone.
"""
from __future__ import annotations

import itertools
import json
import logging
from typing import Callable, Dict, Optional, Set

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr

from sml.common.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from sml.mpss.ecall.session import EcallSession, SessionCallbacks
from generated.python.topics import ecall as topics_ecall
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.ecall.call_manager")

# T5/T6 are fixed at 5000ms per EN 16062:2015 in the original
# (CallManagerServerImpl::startTimers: "timer expiry is set as per eCall
# specification to 5 secs"), not configurable. T2/T7/T9 default values and
# T10's default below are simulator test-friendly defaults (the original's
# own JSON-file defaults were not recoverable from this checkout) --
# overridable via set_config / update_hlap_timer respectively.
_FIXED_TIMER_MS = {"T5": 5000, "T6": 5000}
_DEFAULT_CONFIG = {
    "muteRxAudio": False,
    "numType": "DEFAULT",
    "overriddenNum": "",
    "useCannedMsd": False,
    "gnssUpdateInterval": 1000,
    "t2Timer": 10000,
    "t7Timer": 5000,
    "t9Timer": 15000,
    "msdVersion": 2,
}
_DEFAULT_T10_MS = 15000
_ALL_TIMERS = ("T2", "T5", "T6", "T7", "T9", "T10")

# Default redial timing tables, ms between successive dial attempts --
# matches the 3GPP TS22.001 Annex 6 spec table and the old simulator's
# json/system-state/tel/ICallManagerStateSlot1.json defaults. CALLORIG
# (origination failure): 10 entries. CALLDROP (drop before MSD ack): 2.
# Overridable via configure_ecall_redial, independently per config.
_DEFAULT_REDIAL_TIME_GAPS = {
    "CALLORIG": [5000, 60000, 60000, 60000, 180000, 180000, 180000, 180000, 180000, 180000],
    "CALLDROP": [5000, 60000],
}
_REDIAL_ATTEMPTS_BOUNDS = {"CALLORIG": (1, 10), "CALLDROP": (1, 2)}

_CALL_STATE_BY_ACTION = {
    "CALL_DIALING": "DIALING",
    "CALL_ALERTING": "ALERTING",
    "CALL_ACTIVE": "ACTIVE",
    "CALL_ENDED": "ENDED",
}


class EcallManagerAO(ActiveObject):
    """Publishes call_state/msd_transmission_status/hlap_timer_event/
    operating_mode events and answers ICallManager/IPhone eCall RPCs while
    Ready. See module docstring for the full state topology."""

    def __init__(self, slot: int, mpss_src: str,
                 is_ng_ecall_device: bool = False) -> None:
        super().__init__("EcallManagerAO")
        self._slot = slot
        self._mpss_src = mpss_src
        # Device-level CS-vs-NG bearer choice for *regulatory* eCalls
        # (TEST/MANUAL/AUTOMATIC) -- mirrors the old system's global
        # json "eCallType": "NGeCall"/"CS" device config, which has no
        # TELAF-facing setter in this migration's scope. type=PRIVATE
        # (StartPrivate) is always NG/IMS regardless of this flag, by
        # definition of its ICallManager::makeECall() overload.
        self._is_ng_ecall_device = is_ng_ecall_device

        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None
        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self._call_index_seq = itertools.count(1)
        self._sessions: Dict[int, EcallSession] = {}
        self._hlap_timer_uuids: Dict[tuple, str] = {}
        self._redial_timer_uuids: Dict[int, str] = {}

        self._config: dict = dict(_DEFAULT_CONFIG)
        self._operating_mode: Dict[int, str] = {}
        self._hlap_timer_status: Dict[int, Dict[str, str]] = {}
        self._hlap_timer_config: Dict[int, Dict[str, int]] = {}
        # Test-injected HLAP-timer-failure config, per taf_ecall's
        # configureFailureForRegulatoryECall (T5FAILED/T6FAILED/T7FAILED) --
        # set only via the force_ecall_timer_failure control action.
        self._failed_timers: Set[str] = set()
        self._redial_time_gaps: Dict[str, list] = {
            "CALLORIG": list(_DEFAULT_REDIAL_TIME_GAPS["CALLORIG"]),
            "CALLDROP": list(_DEFAULT_REDIAL_TIME_GAPS["CALLDROP"]),
        }
        # Test-injected redial-mode config, per EcallStateMachine::
        # ModemRedial's config test-hook (SUCCESS/CALLORIG/CALLDROP) --
        # set only via the force_ecall_redial_mode control action. None
        # means SUCCESS (dial normally, the default).
        self._redial_config: Optional[dict] = None

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Optional[Callable] = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state after a
        reconnect. Live sessions/timers are untouched -- a broker flap is
        not an eCall teardown, same rationale as DataConnectionAO's."""
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

    # -- force_* control-plane hooks (ctrl/cmd/action/ecall/* via
    #    EcallActionDispatcher) -- the MQTT-era replacement for the old
    #    gRPC event-injector's InjectEvent(filter="tel_call", event=...).
    def force_ecall_timer_failure(self, timer: str) -> None:
        """timer in {T5,T6,T7}. Mirrors setting
        configureFailureForRegulatoryECall=<timer>FAILED in the old JSON."""
        self.post_fifo(Event(signal=signals.ForceTimerFailure, payload=timer))

    def force_msd_pull_request(self, phone_id: int) -> None:
        """Mirrors the old event-injector's msdUpdateRequest -- PSAP asks
        for MSD without the PA having called send_msd first."""
        self.post_fifo(Event(signal=signals.SessionMsdPull, payload=phone_id))

    def force_hangup_from_psap(self, phone_id: int) -> None:
        self.post_fifo(Event(signal=signals.SessionHangupFromPsap, payload=phone_id))

    def force_ecall_redial_mode(self, config: str, succeed_at_attempt: Optional[int] = None) -> None:
        """config in {SUCCESS,CALLORIG,CALLDROP}. Mirrors setting
        EcallStateMachine::ModemRedial's config test-hook. This is a sticky
        override, same convention as _failed_timers/force_ecall_timer_failure
        -- it is NOT consumed by the next eCall and does not revert to
        SUCCESS on its own; it applies to every eCall started on every
        phoneId until a later force_ecall_redial_mode(config="SUCCESS")
        call clears it (see force_ecall_redial_restore.yaml). succeed_at_attempt
        (1-based, counting from the first redial retry) is new -- the
        original had no "attempt succeeds mid-loop" branch; None/omitted
        lets every configured attempt fail through to MAX_REDIAL_ATTEMPTED,
        matching the original's only modeled outcome. Note succeed_at_attempt
        is not bounds-checked against config's actual table length (10 for
        CALLORIG, 2 for CALLDROP) -- a value beyond the table just never
        matches and silently falls through to MAX_REDIAL_ATTEMPTED."""
        self.post_fifo(Event(signal=signals.ForceRedialMode, payload=(config, succeed_at_attempt)))

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        mapping = {
            topics_ecall.start_ecall.req: self._handle_start_ecall,
            topics_ecall.end_ecall.req: self._handle_end_ecall,
            topics_ecall.answer_ecall.req: self._handle_answer_ecall,
            topics_ecall.send_msd.req: self._handle_send_msd,
            topics_ecall.set_config.req: self._handle_set_config,
            topics_ecall.get_config.req: self._handle_get_config,
            topics_ecall.set_operating_mode.req: self._handle_set_operating_mode,
            topics_ecall.get_operating_mode.req: self._handle_get_operating_mode,
            topics_ecall.update_hlap_timer.req: self._handle_update_hlap_timer,
            topics_ecall.request_hlap_timer.req: self._handle_request_hlap_timer,
            topics_ecall.request_hlap_timer_status.req: self._handle_request_hlap_timer_status,
            topics_ecall.request_network_deregistration.req:
                self._handle_request_network_deregistration,
            topics_ecall.configure_ecall_redial.req: self._handle_configure_ecall_redial,
            topics_ecall.get_ecall_redial_config.req: self._handle_get_ecall_redial_config,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("ecall manager AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        self._publish_subsys_ready(ready=True)
        _log.info("ecall manager AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish ecall ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        for uuid in list(self._hlap_timer_uuids.values()):
            self._cancel_scheduled(uuid)
        self._hlap_timer_uuids.clear()
        for uuid in list(self._redial_timer_uuids.values()):
            self._cancel_scheduled(uuid)
        self._redial_timer_uuids.clear()
        self._sessions.clear()
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "ecall manager AO")

    def _do_resubscribe(self) -> None:
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("ecall manager AO resubscribed (slot=%d)", self._slot)

    # ------------------------------------------------------------------
    # Timer helpers -- rooted at post_fifo(period=..., deferred=True),
    # same discipline as DataConnectionAO's _schedule_call_connect.
    # ------------------------------------------------------------------

    def _timer_duration_ms(self, phone_id: int, timer: str) -> int:
        if timer in _FIXED_TIMER_MS:
            return _FIXED_TIMER_MS[timer]
        if timer == "T10":
            return self._hlap_timer_config.setdefault(phone_id, {}).get("T10", _DEFAULT_T10_MS)
        key = {"T2": "t2Timer", "T7": "t7Timer", "T9": "t9Timer"}.get(timer)
        return int(self._config.get(key, 0)) if key else 0

    def _schedule_hlap_expiry(self, phone_id: int, timer: str) -> None:
        self._cancel_scheduled(self._hlap_timer_uuids.pop((phone_id, timer), None))
        period_s = self._timer_duration_ms(phone_id, timer) / 1000.0
        if period_s <= 0:
            _log.debug("EcallManagerAO phoneId=%d timer=%s: firing immediately (period=0)",
                       phone_id, timer)
            self.post_fifo(Event(signal=signals.HlapTimerExpiry, payload=(phone_id, timer)))
            return
        _log.debug("EcallManagerAO phoneId=%d timer=%s: armed for %.3fs", phone_id, timer, period_s)
        uuid = self.post_fifo(
            Event(signal=signals.HlapTimerExpiry, payload=(phone_id, timer)),
            times=1, period=period_s, deferred=True,
        )
        self._hlap_timer_uuids[(phone_id, timer)] = uuid

    def _schedule_redial_expiry(self, phone_id: int, delay_ms: int) -> None:
        self._cancel_scheduled(self._redial_timer_uuids.pop(phone_id, None))
        period_s = delay_ms / 1000.0
        if period_s <= 0:
            _log.debug("EcallManagerAO phoneId=%d redial timer: firing immediately (period=0)",
                       phone_id)
            self.post_fifo(Event(signal=signals.RedialTimerExpiry, payload=phone_id))
            return
        _log.debug("EcallManagerAO phoneId=%d redial timer: armed for %.3fs", phone_id, period_s)
        uuid = self.post_fifo(
            Event(signal=signals.RedialTimerExpiry, payload=phone_id),
            times=1, period=period_s, deferred=True,
        )
        self._redial_timer_uuids[phone_id] = uuid

    def _cancel_scheduled(self, uuid: Optional[str]) -> None:
        if uuid is not None:
            try:
                self.cancel_event(uuid=uuid)
            except Exception as exc:  # noqa: BLE001
                _log.debug("cancel_event(%s) raised: %s", uuid, exc)

    def _clear_call_slot(self, phone_id: int) -> None:
        """Release simulator-owned per-call state for a completed eCall.

        The TelAF unit tests start the next eCall immediately after ending
        the previous one.  The real stack allows that once the call has ended,
        even if HLAP callback timers (T9/T10) are still being modeled.  Keeping
        the old session in _sessions makes the next start fail with
        DEVICE_IN_USE, which the PA then reports through a null ICall pointer.
        """
        self._sessions.pop(phone_id, None)
        for timer in _ALL_TIMERS:
            self._cancel_scheduled(self._hlap_timer_uuids.pop((phone_id, timer), None))
        self._cancel_scheduled(self._redial_timer_uuids.pop(phone_id, None))
        self._hlap_timer_status[phone_id] = self._default_timer_status()

    # ------------------------------------------------------------------
    # Session callback bindings -- the direct analog of
    # CallManagerServerImpl's startTimer/sendEvent/expiryTimer/
    # changeCallState/msdTransmissionStatus, and
    # PSAPCallback::getEcallOperatingMode.
    # ------------------------------------------------------------------

    def _make_session_callbacks(self, phone_id: int) -> SessionCallbacks:
        def _timer_status(timer: str, status: str) -> None:
            self._hlap_timer_status.setdefault(phone_id, self._default_timer_status())[timer] = status

        def start_timer(timer: str) -> None:
            _timer_status(timer, "ACTIVE")
            _log.debug("EcallManagerAO phoneId=%d: timer=%s STARTED", phone_id, timer)
            self._pub_ind(topics_ecall.hlap_timer_event.ind, "ecall.hlap_timer_event.ind",
                          {"phoneId": phone_id, "timer": timer, "action": "STARTED"})
            self._schedule_hlap_expiry(phone_id, timer)

        def send_event(timer: str, status: str) -> None:
            action = {"start": "STARTED", "stop": "STOPPED"}.get(status)
            if action is None:
                _log.error("EcallManagerAO send_event: invalid status=%s", status)
                return
            _timer_status(timer, "ACTIVE" if status == "start" else "INACTIVE")
            _log.debug("EcallManagerAO phoneId=%d: timer=%s %s", phone_id, timer, action)
            self._pub_ind(topics_ecall.hlap_timer_event.ind, "ecall.hlap_timer_event.ind",
                          {"phoneId": phone_id, "timer": timer, "action": action})

        def expiry_timer(timer: str) -> None:
            _timer_status(timer, "INACTIVE")
            _log.debug("EcallManagerAO phoneId=%d: timer=%s EXPIRED", phone_id, timer)
            self._pub_ind(topics_ecall.hlap_timer_event.ind, "ecall.hlap_timer_event.ind",
                          {"phoneId": phone_id, "timer": timer, "action": "EXPIRED"})

        def msd_transmission_status(status: str) -> None:
            _log.debug("EcallManagerAO phoneId=%d: msd_transmission_status=%s", phone_id, status)
            self._pub_ind(topics_ecall.msd_transmission_status.ind,
                          "ecall.msd_transmission_status.ind",
                          {"phoneId": phone_id, "status": status})

        def change_call_state(action: str, remote_party_number: str) -> None:
            session = self._sessions.get(phone_id)
            if session is None:
                return
            call_state = _CALL_STATE_BY_ACTION.get(action, "UNKNOWN")
            _log.debug("EcallManagerAO phoneId=%d: change_call_state action=%s callState=%s",
                       phone_id, action, call_state)
            self._pub_ind(topics_ecall.call_state.ind, "ecall.call_state.ind", {
                "callIndex": session.call_index,
                "phoneId": phone_id,
                "callState": call_state,
                "direction": session.direction,
                "remotePartyNumber": remote_party_number,
            })

        def get_operating_mode() -> str:
            return self._operating_mode.get(phone_id, "NORMAL")

        def on_terminal() -> None:
            self._clear_call_slot(phone_id)
            _log.info("ecall session on phoneId=%d reached Terminal", phone_id)

        def start_redial_timer(delay_ms: int) -> None:
            self._schedule_redial_expiry(phone_id, delay_ms)

        def on_redial(will_redial: bool, reason: str) -> None:
            _log.debug("EcallManagerAO phoneId=%d: on_redial willRedial=%s reason=%s",
                       phone_id, will_redial, reason)
            self._pub_ind(topics_ecall.redial.ind, "ecall.redial.ind",
                          {"phoneId": phone_id, "willRedial": will_redial, "reason": reason})

        return SessionCallbacks(
            change_call_state=change_call_state,
            start_timer=start_timer,
            send_event=send_event,
            expiry_timer=expiry_timer,
            msd_transmission_status=msd_transmission_status,
            get_operating_mode=get_operating_mode,
            on_terminal=on_terminal,
            start_redial_timer=start_redial_timer,
            on_redial=on_redial,
        )

    @staticmethod
    def _default_timer_status() -> Dict[str, str]:
        return {t: "INACTIVE" for t in _ALL_TIMERS}

    # ------------------------------------------------------------------
    # Publish helpers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _send_error(self, topic: str, msg: dict, code: str, detail: str = "") -> None:
        env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], code, detail)
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(topics_ecall.subsys_ready_call.ind, "ecall.subsys_ready_call.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("ecall subsystem ready=%s published (slot=%d)", ready, self._slot)

    # ------------------------------------------------------------------
    # RPC handlers -- each mirrors one CallManagerServerImpl/
    # PhoneManagerServerImpl eCall method.
    # ------------------------------------------------------------------

    def _resolve_call_target(self, ecall_type: str, data: dict) -> tuple:
        """Returns (remote_party_number, is_custom_number_ecall, is_ng_ecall).

        Mirrors CallManagerStub::createRequest<T>()'s "if (dialNumber != '')
        // Regulatory eCall" comment (a caller-supplied number, or a
        configured overriddenNum via set_config, makes this a
        "custom-numbered" eCall) and the device's CS/NG eCall type for
        the regulatory (non-PRIVATE) overloads.
        """
        if ecall_type == "PRIVATE":
            return data.get("dialNumber", ""), False, True
        remote = data.get("remotePartyNumber", "")
        if remote:
            return remote, True, self._is_ng_ecall_device
        if self._config.get("numType") == "OVERRIDDEN" and self._config.get("overriddenNum"):
            return self._config["overriddenNum"], True, self._is_ng_ecall_device
        return "", False, self._is_ng_ecall_device

    def _handle_start_ecall(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id = data["phoneId"]
        ecall_type = data["type"]
        is_msd_transmitted = data["isMsdTransmitted"]

        existing = self._sessions.get(phone_id)
        if existing is not None:
            if existing.state in ("PSAPCallback", "Terminal"):
                _log.info("EcallManagerAO replacing completed eCall session phoneId=%d state=%s",
                          phone_id, existing.state)
                self._clear_call_slot(phone_id)
            else:
                self._send_error(topics_ecall.start_ecall.rsp, msg, "DEVICE_IN_USE",
                                 "an eCall is already in progress on this phoneId")
                return

        remote_party_number, is_custom_number_ecall, is_ng_ecall = \
            self._resolve_call_target(ecall_type, data)
        call_index = next(self._call_index_seq)
        redial_mode = self._redial_config["mode"] if self._redial_config else None
        redial_succeed_at = self._redial_config["succeed_at_attempt"] if self._redial_config else None
        redial_time_gaps = self._redial_time_gaps.get(redial_mode, []) if redial_mode else []
        session = EcallSession(
            phone_id=phone_id, call_index=call_index,
            remote_party_number=remote_party_number,
            is_msd_transmitted=is_msd_transmitted, is_ng_ecall=is_ng_ecall,
            is_custom_number_ecall=is_custom_number_ecall, direction="MO",
            callbacks=self._make_session_callbacks(phone_id),
            failed_timers=set(self._failed_timers),
            redial_mode=redial_mode,
            redial_succeed_at=redial_succeed_at,
            redial_time_gaps=redial_time_gaps,
        )
        self._sessions[phone_id] = session
        _log.info("EcallManagerAO start_ecall phoneId=%d type=%s callIndex=%d ngEcall=%s custom=%s",
                  phone_id, ecall_type, call_index, is_ng_ecall, is_custom_number_ecall)

        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {
            "call": {
                "callIndex": call_index, "phoneId": phone_id, "callState": "DIALING",
                "direction": "MO", "remotePartyNumber": remote_party_number,
            }
        })
        self._pub_rsp(topics_ecall.start_ecall.rsp, "ecall.start_ecall.rsp", env)
        # Ack now, run the (ported) state machine after -- mirrors
        # CallManagerServerImpl::MakeECall kicking off handleStateMachine()
        # on a background task while the gRPC handler returns immediately.
        self.post_fifo(Event(signal=signals.SessionStart, payload=phone_id))

    def _handle_end_ecall(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id, call_index = data["phoneId"], data["callIndex"]
        session = self._sessions.get(phone_id)
        if session is None or session.call_index != call_index:
            self._send_error(topics_ecall.end_ecall.rsp, msg, "INVALID_ARGUMENTS", "unknown callIndex")
            return

        # End is intentionally handled before acking the RPC.  tafECallSvc's
        # StopECall() waits only for the PA command callback; if the simulator
        # replies first and emits CALL_ENDED later, the unit test can start the
        # next eCall while tafECallSvc still has ECALL_ACTIVE/REQUEST cached and
        # StartECall() returns LE_BUSY ("Already ecall in progress").
        session.hangup(from_psap=False)
        if session.state == "PSAPCallback":
            _log.info("EcallManagerAO releasing ended eCall session before end_ecall rsp "
                      "phoneId=%d callIndex=%d", phone_id, session.call_index)
            self._clear_call_slot(phone_id)

        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.end_ecall.rsp, "ecall.end_ecall.rsp", env)


    def _handle_answer_ecall(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id, call_index = data["phoneId"], data["callIndex"]
        session = self._sessions.get(phone_id)
        if session is None or session.call_index != call_index:
            self._send_error(topics_ecall.answer_ecall.rsp, msg, "INVALID_ARGUMENTS", "unknown callIndex")
            return
        if session.state != "Incoming":
            self._send_error(topics_ecall.answer_ecall.rsp, msg, "INVALID_STATE",
                             "call is not in an answerable (Incoming) state")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.answer_ecall.rsp, "ecall.answer_ecall.rsp", env)
        _log.info("EcallManagerAO answer_ecall phoneId=%d callIndex=%d", phone_id, call_index)
        self.post_fifo(Event(signal=signals.SessionStart, payload=phone_id))

    def _handle_send_msd(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id = data["phoneId"]
        session = self._sessions.get(phone_id)
        if session is None:
            self._send_error(topics_ecall.send_msd.rsp, msg, "INVALID_ARGUMENTS", "no active eCall")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.send_msd.rsp, "ecall.send_msd.rsp", env)
        _log.info("EcallManagerAO send_msd phoneId=%d", phone_id)
        self.post_fifo(Event(signal=signals.SessionMsdPull, payload=phone_id))

    def _handle_set_config(self, msg: dict) -> None:
        data = msg.get("data") or {}
        for wire_key, valid_key in (
            ("muteRxAudio", "isMuteRxAudioValid"), ("numType", "isNumTypeValid"),
            ("overriddenNum", "isOverriddenNumValid"), ("useCannedMsd", "isUseCannedMsdValid"),
            ("gnssUpdateInterval", "isGnssUpdateIntervalValid"),
            ("t2Timer", "isT2TimerValid"), ("t7Timer", "isT7TimerValid"),
            ("t9Timer", "isT9TimerValid"), ("msdVersion", "isMsdVersionValid"),
        ):
            if data.get(valid_key) and wire_key in data:
                self._config[wire_key] = data[wire_key]
        _log.info("EcallManagerAO set_config applied: %s", self._config)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.set_config.rsp, "ecall.set_config.rsp", env)

    def _handle_get_config(self, msg: dict) -> None:
        data = {f"is{k[0].upper()}{k[1:]}Valid": True for k in _DEFAULT_CONFIG}
        data.update(self._config)
        _log.debug("EcallManagerAO get_config requested -> %s", self._config)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_ecall.get_config.rsp, "ecall.get_config.rsp", env)

    def _handle_configure_ecall_redial(self, msg: dict) -> None:
        data = msg.get("data") or {}
        config = data.get("config")
        time_gap = data.get("timeGap") or []
        lo, hi = _REDIAL_ATTEMPTS_BOUNDS.get(config, (0, 0))
        if config not in _REDIAL_ATTEMPTS_BOUNDS or not (lo <= len(time_gap) <= hi):
            self._send_error(topics_ecall.configure_ecall_redial.rsp, msg, "INVALID_ARGUMENTS",
                             f"timeGap length must be {lo}-{hi} for config={config}")
            return
        self._redial_time_gaps[config] = list(time_gap)
        _log.info("EcallManagerAO configure_ecall_redial: config=%s timeGap=%s", config, time_gap)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.configure_ecall_redial.rsp, "ecall.configure_ecall_redial.rsp", env)

    def _handle_get_ecall_redial_config(self, msg: dict) -> None:
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {
            "callOrigTimeGap": self._redial_time_gaps["CALLORIG"],
            "callDropTimeGap": self._redial_time_gaps["CALLDROP"],
        })
        self._pub_rsp(topics_ecall.get_ecall_redial_config.rsp, "ecall.get_ecall_redial_config.rsp", env)

    def _handle_set_operating_mode(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id, mode = data["phoneId"], data["mode"]
        self._operating_mode[phone_id] = mode
        _log.info("EcallManagerAO set_operating_mode applied: phoneId=%d mode=%s", phone_id, mode)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.set_operating_mode.rsp, "ecall.set_operating_mode.rsp", env)
        # Mirrors PhoneManagerServerImpl's ECallModeInfoChangeEvent.
        self._pub_ind(topics_ecall.operating_mode.ind, "ecall.operating_mode.ind",
                      {"phoneId": phone_id, "mode": mode, "reason": "NORMAL"})

    def _handle_get_operating_mode(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id = data["phoneId"]
        mode = self._operating_mode.get(phone_id, "NORMAL")
        _log.debug("EcallManagerAO get_operating_mode requested phoneId=%d -> %s", phone_id, mode)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {"mode": mode})
        self._pub_rsp(topics_ecall.get_operating_mode.rsp, "ecall.get_operating_mode.rsp", env)

    def _handle_update_hlap_timer(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id, timer, duration = data["phoneId"], data["type"], data["timeDuration"]
        if timer != "T10":
            self._send_error(topics_ecall.update_hlap_timer.rsp, msg, "NOT_SUPPORTED",
                             "only T10 is supported by updateEcallHlapTimer")
            return
        self._hlap_timer_config.setdefault(phone_id, {})["T10"] = duration
        _log.info("EcallManagerAO update_hlap_timer phoneId=%d T10=%dms", phone_id, duration)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.update_hlap_timer.rsp, "ecall.update_hlap_timer.rsp", env)

    def _handle_request_hlap_timer(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id, timer = data["phoneId"], data["type"]
        if timer != "T10":
            self._send_error(topics_ecall.request_hlap_timer.rsp, msg, "NOT_SUPPORTED",
                             "only T10 is supported by requestEcallHlapTimer")
            return
        duration = self._hlap_timer_config.setdefault(phone_id, {}).get("T10", _DEFAULT_T10_MS)
        _log.debug("EcallManagerAO request_hlap_timer phoneId=%d T10=%dms", phone_id, duration)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                     {"timeDuration": duration})
        self._pub_rsp(topics_ecall.request_hlap_timer.rsp, "ecall.request_hlap_timer.rsp", env)

    def _handle_request_hlap_timer_status(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id = data["phoneId"]
        status = self._hlap_timer_status.get(phone_id, self._default_timer_status())
        payload = {t.lower(): status.get(t, "INACTIVE") for t in _ALL_TIMERS}
        _log.debug("EcallManagerAO request_hlap_timer_status phoneId=%d -> %s", phone_id, payload)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], payload)
        self._pub_rsp(topics_ecall.request_hlap_timer_status.rsp,
                      "ecall.request_hlap_timer_status.rsp", env)

    def _handle_request_network_deregistration(self, msg: dict) -> None:
        data = msg.get("data") or {}
        phone_id = data["phoneId"]
        status = self._hlap_timer_status.get(phone_id, self._default_timer_status())
        # Mirrors CallManagerServerImpl::RequestNetworkDeregistration's guard:
        # only proceeds if T9 is INACTIVE and T10 is ACTIVE.
        if status.get("T9") != "INACTIVE" or status.get("T10") != "ACTIVE":
            self._send_error(topics_ecall.request_network_deregistration.rsp, msg,
                             "INVALID_STATE", "T9 must be inactive and T10 active")
            return
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_ecall.request_network_deregistration.rsp,
                      "ecall.request_network_deregistration.rsp", env)
        _log.info("EcallManagerAO request_network_deregistration phoneId=%d", phone_id)
        session = self._sessions.get(phone_id)
        if session is not None:
            session.network_deregistration_request()

    # ------------------------------------------------------------------
    # Session-action / timer-expiry dispatch (run on this AO's thread)
    # ------------------------------------------------------------------

    def _run_session_start(self, phone_id: int) -> None:
        session = self._sessions.get(phone_id)
        if session is not None:
            session.start()

    def _run_session_hangup(self, phone_id: int, from_psap: bool = False) -> None:
        session = self._sessions.get(phone_id)
        if session is not None:
            session.hangup(from_psap=from_psap)
            if not from_psap and session.state == "PSAPCallback":
                _log.info("EcallManagerAO releasing ended eCall session phoneId=%d callIndex=%d",
                          phone_id, session.call_index)
                self._clear_call_slot(phone_id)

    def _run_session_msd_pull(self, phone_id: int) -> None:
        session = self._sessions.get(phone_id)
        if session is not None:
            session.msd_pull_request()

    def _run_hlap_timer_expiry(self, phone_id: int, timer: str) -> None:
        self._hlap_timer_uuids.pop((phone_id, timer), None)
        session = self._sessions.get(phone_id)
        if session is not None:
            session.on_hlap_timer_expiry(timer)

    def _run_redial_timer_expiry(self, phone_id: int) -> None:
        self._redial_timer_uuids.pop(phone_id, None)
        session = self._sessions.get(phone_id)
        if session is not None:
            session.on_redial_timer_expiry()

    def _apply_force_timer_failure(self, timer: str) -> None:
        if timer not in ("T5", "T6", "T7"):
            _log.warning("force_ecall_timer_failure: unsupported timer=%s", timer)
            return
        self._failed_timers.add(timer)
        _log.info("EcallManagerAO force_ecall_timer_failure: %s will now fail on next eCall", timer)

    def _apply_force_redial_mode(self, config: str, succeed_at_attempt: Optional[int]) -> None:
        if config not in ("SUCCESS", "CALLORIG", "CALLDROP"):
            _log.warning("force_ecall_redial_mode: unsupported config=%s", config)
            return
        if config != "SUCCESS" and succeed_at_attempt is not None:
            _, hi = _REDIAL_ATTEMPTS_BOUNDS[config]
            if not (1 <= succeed_at_attempt <= hi):
                _log.warning(
                    "force_ecall_redial_mode: succeedAtAttempt=%s is out of range 1-%d for "
                    "config=%s -- it will never match and every attempt will fail through to "
                    "MAX_REDIAL_ATTEMPTED", succeed_at_attempt, hi, config,
                )
        self._redial_config = None if config == "SUCCESS" else {
            "mode": config, "succeed_at_attempt": succeed_at_attempt,
        }
        _log.info(
            "EcallManagerAO force_ecall_redial_mode: config=%s succeedAtAttempt=%s applied "
            "(sticky until a later config=SUCCESS call)",
            config, succeed_at_attempt,
        )


# ---------------------------------------------------------------------------
# HSM state handlers -- identical topology to RadioPhoneAO; see that
# module's docstring for the full rationale.
# ---------------------------------------------------------------------------

@spy_on
def smfn_off(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.Start:
        status = chart.trans(smfn_operating)
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_operating(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.INIT_SIGNAL:
        status = chart.trans(smfn_starting)
    elif e.signal == signals.Stop:
        status = chart.trans(smfn_stopping)
    elif e.signal == signals.MessageReceived:
        chart.defer(e)
        status = return_status.HANDLED
    elif e.signal in (signals.SessionStart, signals.SessionHangup, signals.SessionHangupFromPsap,
                       signals.SessionMsdPull, signals.HlapTimerExpiry, signals.ForceTimerFailure,
                       signals.RedialTimerExpiry, signals.ForceRedialMode):
        _log.debug("ecall manager AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        _log.debug("ecall manager AO: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_start()
        chart.post_fifo(Event(signal=signals.StartingDone))
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    elif e.signal == signals.StartingDone:
        status = chart.trans(smfn_ready)
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_ready(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_enter_ready()
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._do_exit_ready()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.SessionStart:
        chart._run_session_start(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.SessionHangup:
        chart._run_session_hangup(e.payload, from_psap=False)
        status = return_status.HANDLED
    elif e.signal == signals.SessionHangupFromPsap:
        chart._run_session_hangup(e.payload, from_psap=True)
        status = return_status.HANDLED
    elif e.signal == signals.SessionMsdPull:
        chart._run_session_msd_pull(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.HlapTimerExpiry:
        phone_id, timer = e.payload
        chart._run_hlap_timer_expiry(phone_id, timer)
        status = return_status.HANDLED
    elif e.signal == signals.RedialTimerExpiry:
        chart._run_redial_timer_expiry(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceTimerFailure:
        chart._apply_force_timer_failure(e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceRedialMode:
        config, succeed_at_attempt = e.payload
        chart._apply_force_redial_mode(config, succeed_at_attempt)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        chart._do_resubscribe()
        status = return_status.HANDLED
    else:
        chart.temp.fun = smfn_operating
        status = return_status.SUPER
    return status


@spy_on
def smfn_stopping(chart, e):
    status = return_status.UNHANDLED
    if e.signal == signals.ENTRY_SIGNAL:
        chart._do_stop()
        chart.stop()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


__all__ = ["EcallManagerAO"]
