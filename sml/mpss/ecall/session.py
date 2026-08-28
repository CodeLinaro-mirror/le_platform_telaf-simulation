# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side eCall session -- ported from
sdk/simulation/services/sdk-simulation-server/tel/EcallStateMachine.cpp.

Threadless: constructed and driven synchronously by the owning
``EcallManagerAO``'s own thread, exactly like ``sml.mpss.data.connection.
CallSession``. Not a ``miros`` chart -- the original state machine's
``onEnter``/``onExit`` bodies are long, branchy, synchronous procedures
(they call back into the service and immediately keep going), not
reactive per-event handlers, so a plain Python class with one method per
state entry is the more faithful shape; a ``@spy_on`` HSM would force an
artificial event round-trip into logic that was never event-driven at
that granularity in the original.

Timing adaptation (documented, not a behavior change): the original
threads block on ``std::this_thread::sleep_for(1000ms)`` purely to pace
consecutive bookkeeping calls (no branch of the state machine depends on
those particular delays -- they only affect message *timing*, never
*content* or *ordering*). A single-threaded Active Object cannot block
like that without stalling every other RPC on the same phoneId, so those
pacing sleeps collapse to immediate, synchronously-ordered calls here.
The delays that ARE load-bearing -- T2/T5/T6/T7/T9/T10 HLAP timer
*expiry*, which the original schedules via ``CallManagerServerImpl::
startTimer()`` and waits for asynchronously -- are preserved as real
scheduled timers via the owning ``EcallManagerAO``'s
``post_fifo(..., deferred=True)``, never a blocking sleep. See
``EcallManagerAO._schedule_hlap_timer``/``_on_hlap_timer_expiry``.
"""
from __future__ import annotations

import logging
from typing import Callable, Optional, Set

_log = logging.getLogger("sml.mpss.ecall.session")

# EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP re-entering
# CallConversation (mid-call MSD re-pull) needs to remember "we're doing a
# pull", exactly like the original's eventId_ member.
EVENT_MSD_PULL = "MSD_PULL_REQUEST_FROM_PSAP"


class SessionCallbacks:
    """Bound closures an ``EcallSession`` calls back into -- the direct
    analog of the original's ``ecallStateMachine->getCallservice()->...``
    (``CallManagerServerImpl`` methods) and ``getEcallOperatingMode()``.
    All are provided by the owning ``EcallManagerAO``.
    """

    def __init__(
        self,
        change_call_state: Callable[[str, str], None],
        start_timer: Callable[[str], None],
        send_event: Callable[[str, str], None],
        expiry_timer: Callable[[str], None],
        msd_transmission_status: Callable[[str], None],
        get_operating_mode: Callable[[], str],
        on_terminal: Callable[[], None],
        start_redial_timer: Callable[[int], None],
        on_redial: Callable[[bool, str], None],
    ) -> None:
        self.change_call_state = change_call_state
        self.start_timer = start_timer
        self.send_event = send_event
        self.expiry_timer = expiry_timer
        self.msd_transmission_status = msd_transmission_status
        self.get_operating_mode = get_operating_mode
        self.on_terminal = on_terminal
        self.start_redial_timer = start_redial_timer
        self.on_redial = on_redial


class EcallSession:
    """One in-progress eCall. At most one is ever active per phoneId --
    ``EcallManagerAO`` enforces that (mirrors taf_ecall's isIdle() /
    getInProgressCalls() guard against a second concurrent eCall).
    """

    def __init__(
        self,
        phone_id: int,
        call_index: int,
        remote_party_number: str,
        is_msd_transmitted: bool,
        is_ng_ecall: bool,
        is_custom_number_ecall: bool,
        direction: str,
        callbacks: SessionCallbacks,
        failed_timers: Optional[Set[str]] = None,
        redial_mode: Optional[str] = None,
        redial_succeed_at: Optional[int] = None,
        redial_time_gaps: Optional[list] = None,
    ) -> None:
        self.phone_id = phone_id
        self.call_index = call_index
        self.remote_party_number = remote_party_number
        self.is_msd_transmitted = is_msd_transmitted
        self.is_ng_ecall = is_ng_ecall
        self.is_custom_number_ecall = is_custom_number_ecall
        self.direction = direction
        self._cb = callbacks
        # EcallStateMachine::result_ / parseVectortoString -- the test-config
        # "configureFailureForRegulatoryECall" strings (T5FAILED/T6FAILED/
        # T7FAILED) that force a real HLAP-timer-expiry failure path instead
        # of the immediate-success bookkeeping path.
        self._failed_timers = failed_timers or set()
        # EcallStateMachine::ModemRedial's config test-hook (SUCCESS/
        # CALLORIG/CALLDROP), set only via the force_ecall_redial_mode
        # control action. redial_mode is None (dial normally) unless a
        # test explicitly configured CALLORIG/CALLDROP for this call.
        self.redial_mode = redial_mode
        self.redial_succeed_at = redial_succeed_at
        self.redial_time_gaps = redial_time_gaps or []
        self._redial_attempt = 0
        self.update_in_progress = False
        self._event_id: Optional[str] = None  # mirrors eventId_
        self.state = "CallConnect"  # set before _enter_call_connect() runs

    # ------------------------------------------------------------------
    # Public entry points -- mirrors EcallStateMachine::onEvent's dispatch
    # to whichever state is current, gated the same way each state's
    # onEvent gates it (only meaningful while the matching state is
    # active; a stale call elsewhere is a silent no-op like the original).
    # ------------------------------------------------------------------

    def start(self) -> None:
        """Mirrors EcallStateMachine::start()."""
        self.state = "CallConnect"
        _log.debug("ecall session %d: start -> state=CallConnect", self.call_index)
        self._enter_call_connect()

    def hangup(self, from_psap: bool = False) -> None:
        """HANGUP_REQUEST_FROM_USER / _FROM_PSAP -- handled identically by
        every state that isn't already terminal/PSAPCallback (the original
        has no PSAPCallback::onEvent hangup branch either)."""
        if self.state in ("CallConnect", "DecodeSendMSD", "CRCCheckonMSD", "DecodeMSD",
                           "CallConversation"):
            self._cb.change_call_state("CALL_ENDED", self.remote_party_number)
            if not self.is_custom_number_ecall:
                self._cb.send_event("T2", "stop")
            self._transition_to_psap_callback()
        elif self.state == "Redialing":
            # Redialing has no equivalent in the original EcallStateMachine,
            # so it was missing here entirely -- ending the call while
            # mid-redial-loop fell through to the else branch below and did
            # nothing, leaving state stuck at "Redialing" forever. Both
            # _handle_end_ecall and _run_session_hangup in EcallManagerAO
            # only release the call slot once state == "PSAPCallback", so
            # that stuck state leaked the session and the scheduled redial
            # timer, and left the phoneId reporting DEVICE_IN_USE
            # indefinitely. T2 is never running here (CALLORIG enters
            # Redialing before T2 starts; CALLDROP enters it only after
            # T2's own expiry), so there's nothing to stop -- unlike the
            # branch above. The still-outstanding scheduled redial timer
            # itself is harmless: on_redial_timer_expiry() already no-ops
            # once state != "Redialing", same as every HLAP timer's
            # expiry-after-hangup case.
            self._cb.change_call_state("CALL_ENDED", self.remote_party_number)
            self._transition_to_psap_callback()
        else:
            _log.debug("ecall session %d: hangup ignored in state=%s", self.call_index, self.state)

    def msd_pull_request(self) -> None:
        """MSD_PULL_REQUEST_FROM_PSAP -- backs taf_ecall_SendMsd (via
        updateECallMsd). Only meaningful while DecodeSendMSD hasn't already
        run past it, or once settled in CallConversation (a mid-call
        re-pull); mirrors the original's per-state onEvent gating."""
        self._event_id = EVENT_MSD_PULL
        if self.state == "CallConversation":
            self.update_in_progress = True
            if self.is_ng_ecall:
                if not self.is_custom_number_ecall:
                    self._cb.msd_transmission_status("OUTBAND_MSD_TRANSMISSION_STARTED")
                self._cb.msd_transmission_status("OUTBAND_MSD_TRANSMISSION_SUCCESS")
                self.update_in_progress = False
                # stays in CallConversation
            else:
                self._transition_to("DecodeSendMSD", self._enter_decode_send_msd)
        else:
            _log.debug("ecall session %d: msd_pull_request while state=%s (no-op)",
                       self.call_index, self.state)

    def network_deregistration_request(self) -> None:
        """ON_NETWORK_DEREGISTRATION_REQUEST -- backs
        taf_ecall_TerminateRegistration. Only meaningful in PSAPCallback
        with T10 running (guarded by the caller -- EcallManagerAO checks
        T9 INACTIVE / T10 ACTIVE before calling this, mirroring
        CallManagerServerImpl::RequestNetworkDeregistration's guard)."""
        if self.state == "PSAPCallback" and not self.is_custom_number_ecall:
            _log.debug("ecall session %d: network_deregistration_request -- stopping T10", self.call_index)
            self._cb.send_event("T10", "stop")

    def on_hlap_timer_expiry(self, timer: str) -> None:
        """ON_TIMER_EXPIRY -- delivered by EcallManagerAO when a real
        scheduled HLAP timer (see module docstring) fires."""
        _log.debug("ecall session %d: on_hlap_timer_expiry timer=%s state=%s",
                   self.call_index, timer, self.state)
        if self.state == "DecodeSendMSD" and timer == "T5" and "T5" in self._failed_timers:
            self._cb.expiry_timer("T5")
            self._cb.msd_transmission_status("MSD_TRANSMISSION_FAILURE")
            self._transition_to("CallConversation", self._enter_call_conversation)
        elif self.state == "CRCCheckonMSD" and timer == "T7":
            self._cb.expiry_timer("T7")
            self._cb.msd_transmission_status("MSD_TRANSMISSION_FAILURE")
            self._transition_to("CallConversation", self._enter_call_conversation)
        elif self.state == "DecodeMSD" and timer == "T6" and not self.is_custom_number_ecall \
                and not self.is_ng_ecall:
            self._cb.expiry_timer("T6")
            self._cb.msd_transmission_status("MSD_TRANSMISSION_FAILURE")
            self._transition_to("CallConversation", self._enter_call_conversation)
        elif self.state == "CallConversation" and timer == "T2" and not self.is_custom_number_ecall:
            self._cb.expiry_timer("T2")
            self._cb.change_call_state("CALL_ENDED", self.remote_party_number)
            if self.redial_mode == "CALLDROP":
                self._enter_redialing()
            else:
                self._transition_to_psap_callback()
        elif self.state == "PSAPCallback" and timer == "T9":
            if not self.is_custom_number_ecall:
                self._cb.expiry_timer("T9")
                if self._cb.get_operating_mode() != "ECALL_ONLY":
                    self.state = "Terminal"
                    self._cb.on_terminal()
                else:
                    self._cb.start_timer("T10")
        elif self.state == "PSAPCallback" and timer == "T10":
            self._cb.expiry_timer("T10")
        else:
            _log.debug("ecall session %d: timer=%s expiry ignored in state=%s",
                       self.call_index, timer, self.state)

    # ------------------------------------------------------------------
    # State entry procedures -- see module docstring re: collapsed pacing.
    # ------------------------------------------------------------------

    def _transition_to(self, state: str, enter_fn: Callable[[], None]) -> None:
        _log.debug("ecall session %d: %s -> %s", self.call_index, self.state, state)
        self.state = state
        enter_fn()

    def _transition_to_psap_callback(self) -> None:
        self._transition_to("PSAPCallback", self._enter_psap_callback)

    def _enter_call_connect(self) -> None:
        self._cb.change_call_state("CALL_DIALING", self.remote_party_number)
        if self.redial_mode == "CALLORIG":
            # Mirrors the old CallConnect::onEnter()'s `if (config ==
            # "CALLORIG") changeState(PSAPCallback)` branch: the very first
            # dial attempt itself fails, before ever reaching ALERTING/ACTIVE.
            self._cb.change_call_state("CALL_ENDED", self.remote_party_number)
            self._enter_redialing()
            return
        self._cb.change_call_state("CALL_ALERTING", self.remote_party_number)
        if not self.is_custom_number_ecall:
            self._cb.start_timer("T2")
        # CallConnect::onExit -- runs as part of this same commit, before
        # DecodeSendMSD::onEnter, exactly as the original's changeState()
        # sequencing (onExit of the old state, then onEnter of the new one).
        if not self.is_custom_number_ecall and not self.is_ng_ecall and self.is_msd_transmitted:
            if "T5" not in self._failed_timers:
                self._cb.send_event("T5", "start")
            else:
                self._cb.start_timer("T5")
        self._transition_to("DecodeSendMSD", self._enter_decode_send_msd)

    def _enter_decode_send_msd(self) -> None:
        if not self.is_ng_ecall:  # CS eCall
            if self.is_msd_transmitted:
                if self._event_id == EVENT_MSD_PULL:
                    if not self.is_custom_number_ecall:
                        self._cb.msd_transmission_status("START_RECEIVED")
                    self._cb.msd_transmission_status("MSD_TRANSMISSION_STARTED")
                    self._transition_to("CRCCheckonMSD", self._enter_crc_check_on_msd)
                else:
                    self._cb.msd_transmission_status("MSD_TRANSMISSION_STARTED")
                    self._cb.change_call_state("CALL_ACTIVE", self.remote_party_number)
                    if not self.is_custom_number_ecall:
                        self._cb.msd_transmission_status("START_RECEIVED")
                        if "T5" not in self._failed_timers:
                            self._cb.send_event("T5", "stop")
                            self._transition_to("CRCCheckonMSD", self._enter_crc_check_on_msd)
                        # else: T5FAILED -- stay here; wait for the real T5
                        # expiry started in CallConnect's onExit (see
                        # on_hlap_timer_expiry's DecodeSendMSD/T5 branch).
                    else:
                        self._transition_to("CRCCheckonMSD", self._enter_crc_check_on_msd)
            else:  # CS eCall, MSD not transmitted
                if not self.is_custom_number_ecall and self._event_id == EVENT_MSD_PULL:
                    self._cb.msd_transmission_status("MSD_TRANSMISSION_STARTED")
                # Unconditional tail -- mirrors the original's second,
                # overriding changeState(CallConversation) call in this
                # branch (see sml-pa-ecall-architecture.md's port notes for
                # this specific edge case).
                self._cb.change_call_state("CALL_ACTIVE", self.remote_party_number)
                self._transition_to("CallConversation", self._enter_call_conversation)
        else:  # NG/IMS eCall
            if self.is_msd_transmitted:
                if not self.is_custom_number_ecall:
                    self._cb.msd_transmission_status("OUTBAND_MSD_TRANSMISSION_STARTED")
                self._cb.change_call_state("CALL_ACTIVE", self.remote_party_number)
                self._transition_to("DecodeMSD", self._enter_decode_msd)
            else:
                self._transition_to("CallConversation", self._enter_call_conversation)

    def _enter_crc_check_on_msd(self) -> None:
        if "T7" not in self._failed_timers:
            if not self.is_custom_number_ecall:
                self._cb.send_event("T7", "start")
            self._transition_to("DecodeMSD", self._enter_decode_msd)
            # CRCCheckonMSD::onExit -- runs immediately after the transition
            # commits, same sequencing rationale as CallConnect's onExit above.
            if not self.is_custom_number_ecall:
                self._cb.send_event("T7", "stop")
                self._cb.msd_transmission_status("LL_ACK_RECEIVED")
        else:
            self._cb.start_timer("T7")  # wait for real expiry -- see on_hlap_timer_expiry

    def _enter_decode_msd(self) -> None:
        if not self.is_ng_ecall:  # CS eCall
            if "T6" not in self._failed_timers:
                if not self.is_custom_number_ecall:
                    self._cb.send_event("T6", "start")
                self._cb.msd_transmission_status("MSD_TRANSMISSION_SUCCESS")
                if not self.is_custom_number_ecall:
                    self._cb.send_event("T6", "stop")
                if self._event_id == EVENT_MSD_PULL:
                    self.update_in_progress = False
                self._transition_to("CallConversation", self._enter_call_conversation)
            else:
                if not self.is_custom_number_ecall:
                    self._cb.start_timer("T6")  # wait for real expiry
        else:  # NG eCall
            self._cb.msd_transmission_status("OUTBAND_MSD_TRANSMISSION_SUCCESS")
            if self._event_id == EVENT_MSD_PULL:
                self.update_in_progress = False
            self._transition_to("CallConversation", self._enter_call_conversation)

    def _enter_call_conversation(self) -> None:
        pass  # CallConversation::onEnter is a no-op in the original.

    def _enter_psap_callback(self) -> None:
        if not self.is_custom_number_ecall:
            self._cb.start_timer("T9")  # wait for real expiry

    def _enter_redialing(self) -> None:
        """ModemRedial::onEnter()'s entry, ported onto EcallManagerAO's
        non-blocking scheduled-timer primitive instead of the original's
        blocking sleep_for() loop -- see module docstring's timing-
        adaptation rationale (redial gaps run up to 180s and this AO is
        single-threaded, so blocking here would stall every other RPC).
        Reached from _enter_call_connect() (CALLORIG: fails before ever
        reaching ALERTING/ACTIVE) or on_hlap_timer_expiry's T2-in-
        CallConversation branch (CALLDROP: fails after MSD ack)."""
        self.state = "Redialing"
        self._redial_attempt = 1
        _log.debug("ecall session %d: -> Redialing attempt=1 delay=%d",
                   self.call_index, self.redial_time_gaps[0])
        self._cb.start_redial_timer(self.redial_time_gaps[0])

    def on_redial_timer_expiry(self) -> None:
        """Delivered by EcallManagerAO when a scheduled redial timer fires
        -- the event-driven replacement for one iteration of the original
        ModemRedial::onEnter()'s for-loop. The original has no branch for
        an attempt succeeding mid-loop (every attempt fails through to
        MAX_REDIAL_ATTEMPTED); redial_succeed_at is new: if this attempt
        matches it, the call connects instead of failing again and no
        further attempts are scheduled."""
        if self.state != "Redialing":
            return
        self._cb.change_call_state("CALL_DIALING", self.remote_party_number)
        if self._redial_attempt == self.redial_succeed_at:
            self._cb.change_call_state("CALL_ACTIVE", self.remote_party_number)
            self._cb.on_redial(False, "CALL_CONNECTED")
            self._transition_to("DecodeSendMSD", self._enter_decode_send_msd)
            return
        self._cb.change_call_state("CALL_ENDED", self.remote_party_number)
        reason = "CALL_ORIG_FAILURE" if self.redial_mode == "CALLORIG" else "CALL_DROP"
        if self._redial_attempt < len(self.redial_time_gaps):
            self._cb.on_redial(True, reason)
            self._redial_attempt += 1
            self._cb.start_redial_timer(self.redial_time_gaps[self._redial_attempt - 1])
        else:
            self._cb.on_redial(False, "MAX_REDIAL_ATTEMPTED")
            self._transition_to_psap_callback()


__all__ = ["EcallSession", "SessionCallbacks", "EVENT_MSD_PULL"]
