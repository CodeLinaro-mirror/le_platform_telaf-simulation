# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side IMS Active Object (RadioImsAO).

Mirrors ``telux::tel::IImsServingSystemManager`` and
``telux::tel::IImsSettingsManager`` 
Same Off -> Operating.{Starting,Ready} -> Stopping shell as every other MPSS
business AO. World-state lives in flat instance attributes, same convention
as serving_system.py/network_selection.py.
"""
from __future__ import annotations

import json
import logging
from typing import Callable, Optional

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.common import instrumentation as _instr
from sml.common.envelope import (
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from generated.python.topics import radio as topics_radio
from generated.python.validators import validate as validate_payload

_log = logging.getLogger("sml.mpss.radio.ims")


class RadioImsAO(ActiveObject):
    """Answers IImsServingSystemManager / IImsSettingsManager RPCs while Ready."""

    def __init__(self, slot: int, mpss_src: str) -> None:
        super().__init__("RadioImsAO")
        self._slot = slot
        self._mpss_src = mpss_src
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None

        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.reg_status = "NOT_REGISTERED"
        self.reg_radio_tech = "RADIO_TECH_UNKNOWN"
        self.reg_error_code = 0
        self.reg_error_string = ""

        # World State -- IImsServingSystemManager::requestServiceInfo
        # (telux::tel::ImsServiceInfo: only sms/voice CellularServiceStatus
        # fields exist on this SDK version -- no video/ut).
        self.sms_service_status = "NO_SERVICE"
        self.voice_service_status = "NO_SERVICE"

        # World State -- IImsServingSystemManager::requestPdpStatus
        # (telux::tel::ImsPdpStatusInfo, in full).
        self.pdp_connected = False
        self.pdp_failure_code = "REASON_NOT_SPECIFIED"
        self.pdp_failure_reason_type = "CE_UNKNOWN"
        self.pdp_failure_reason_code = 0
        self.pdp_apn_name = ""

        # World State -- IImsSettingsManager. taf_radio_SetImsSvcCfg/
        # GetImsSvcCfg's ImsSvcType (IMS_REG/SMS/VOIP/RTT/VONR) each toggle
        # one bool here; VONR is the SDK's dedicated toggleVonr, the other
        # four are ImsServiceConfig's bitset fields.
        self.ims_svc_enabled = {
            "IMS_REG": False, "SMS": False, "VOIP": False, "RTT": False, "VONR": False,
        }
        self.sip_user_agent = ""

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Callable | None = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Posted by ``RadioSubsystem.resubscribe`` after an MQTT reconnect; the AO
        keeps running across the flap. Mirrors the C++ bridge's
        ``Subscribing_St`` -> ``issueAllSubscribes_()``.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived, payload=(topic, payload)))

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
            topics_radio.request_ims_registration_info.req: self._handle_request_registration_info,
            topics_radio.request_ims_service_info.req:      self._handle_request_service_info,
            topics_radio.request_ims_pdp_status.req:        self._handle_request_pdp_status,
            topics_radio.set_ims_service_config.req:        self._handle_set_service_config,
            topics_radio.request_ims_service_config.req:    self._handle_request_service_config,
            topics_radio.set_ims_sip_user_agent.req:         self._handle_set_sip_user_agent,
            topics_radio.request_ims_sip_user_agent.req:     self._handle_request_sip_user_agent,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        _log.info("radio IMS AO subscribed (slot=%d)", self._slot)

    def _do_enter_ready(self) -> None:
        self._publish_subsys_ready(ready=True)
        _log.info("radio IMS AO ready (slot=%d)", self._slot)

    def _do_exit_ready(self) -> None:
        if self._publish_fn is not None:
            try:
                self._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish ims ready=false on stop: %s", exc)

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "radio IMS AO")

    def _do_resubscribe(self) -> None:

        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._do_enter_ready()
        _log.info("radio IMS AO resubscribed (slot=%d)", self._slot)

    def _run_force(self, apply_fn: Callable, data: dict) -> None:
        # Runs on the AO thread, where an uncaught exception kills the thread for good.
        try:
            apply_fn(data)
        except Exception as exc:  # noqa: BLE001
            _log.error("%s failed: %s", apply_fn.__name__, exc)

    # ------------------------------------------------------------------
    # Publish helpers
    # ------------------------------------------------------------------

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _publish_subsys_ready(self, ready: bool) -> None:
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        data = {"ready": ready, "status": status}
        for topic, schema_id in (
            (topics_radio.subsys_ready_ims.ind, "radio.subsys_ready_ims.ind"),
            (topics_radio.subsys_ready_ims_settings.ind, "radio.subsys_ready_ims_settings.ind"),
        ):
            self._pub_ind(topic, schema_id, data, retain=True)
        _log.debug("ims subsystem ready=%s published (slot=%d)", ready, self._slot)

    # ------------------------------------------------------------------
    # RPC handlers -- each mirrors one IImsServingSystemManager/
    # IImsSettingsManager method
    # ------------------------------------------------------------------

    def _handle_request_registration_info(self, msg: dict) -> None:
        data = {
            "imsRegStatus": self.reg_status,
            "rat": self.reg_radio_tech,
            "errorCode": self.reg_error_code,
            "errorString": self.reg_error_string,
        }
        _log.debug("RadioImsAO request_registration_info -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_ims_registration_info.rsp,
                      "radio.request_ims_registration_info.rsp", env)

    def _handle_request_service_info(self, msg: dict) -> None:
        data = {"sms": self.sms_service_status, "voice": self.voice_service_status}
        _log.debug("RadioImsAO request_service_info -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_ims_service_info.rsp,
                      "radio.request_ims_service_info.rsp", env)

    def _handle_request_pdp_status(self, msg: dict) -> None:
        data = {
            "isPdpConnected": self.pdp_connected,
            "failureCode": self.pdp_failure_code,
            "failureReasonType": self.pdp_failure_reason_type,
            "failureReasonCode": self.pdp_failure_reason_code,
            "apnName": self.pdp_apn_name,
        }
        _log.debug("RadioImsAO request_pdp_status -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_ims_pdp_status.rsp,
                      "radio.request_ims_pdp_status.rsp", env)

    def _handle_set_service_config(self, msg: dict) -> None:
        data = msg.get("data") or {}
        service = data.get("service")
        if service in self.ims_svc_enabled:
            self.ims_svc_enabled[service] = bool(data.get("enable", False))
        _log.info("RadioImsAO set_service_config applied: %s=%s",
                  service, self.ims_svc_enabled.get(service))
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_ims_service_config.rsp,
                      "radio.set_ims_service_config.rsp", env)
        self._pub_ind(topics_radio.ims_service_config.ind, "radio.ims_service_config.ind",
                      {"slot": self._slot, "service": service,
                       "enable": self.ims_svc_enabled.get(service, False)})

    def _handle_request_service_config(self, msg: dict) -> None:
        data = msg.get("data") or {}
        service = data.get("service")
        enable = self.ims_svc_enabled.get(service, False)
        _log.debug("RadioImsAO request_service_config(%s) -> %s", service, enable)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {"enable": enable})
        self._pub_rsp(topics_radio.request_ims_service_config.rsp,
                      "radio.request_ims_service_config.rsp", env)

    def _handle_set_sip_user_agent(self, msg: dict) -> None:
        data = msg.get("data") or {}
        self.sip_user_agent = data.get("userAgent", self.sip_user_agent)
        _log.info("RadioImsAO set_sip_user_agent applied: %s", self.sip_user_agent)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_radio.set_ims_sip_user_agent.rsp,
                      "radio.set_ims_sip_user_agent.rsp", env)
        # Mirrors rat_pref's side-effect pattern -- backs
        # IImsSettingsListener::onImsSipUserAgentChange. Added for
        # Events.json parity (tel_ims_settings.imsSipUserAgentUpdate).
        self._pub_ind(topics_radio.ims_sip_user_agent.ind, "radio.ims_sip_user_agent.ind",
                      {"userAgent": self.sip_user_agent})

    def _handle_request_sip_user_agent(self, msg: dict) -> None:
        data = {"userAgent": self.sip_user_agent}
        _log.debug("RadioImsAO request_sip_user_agent -> %s", data)
        env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], data)
        self._pub_rsp(topics_radio.request_ims_sip_user_agent.rsp,
                      "radio.request_ims_sip_user_agent.rsp", env)

    # ------------------------------------------------------------------
    # Force-* actions (test/control-plane hook points; wired to
    # ctrl/cmd/action/radio/force_ims_* via action_radio.yaml -- see
    # RadioActionDispatcher)
    # ------------------------------------------------------------------

    def force_ims_registration_info(self, data: dict) -> None:
        """Patch the IMS registration-info snapshot and re-publish it as an
        indication. Drives IImsServingSystemListener::onImsRegStatusChange."""
        self.post_fifo(Event(signal=signals.ForceImsRegistrationInfo, payload=data))

    def _apply_force_ims_registration_info(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "imsRegStatus" in data:
            self.reg_status = data["imsRegStatus"]
        if "rat" in data:
            self.reg_radio_tech = data["rat"]
        if "errorCode" in data:
            self.reg_error_code = data["errorCode"]
        if "errorString" in data:
            self.reg_error_string = data["errorString"]
        _log.info("RadioImsAO force_ims_registration_info applied: %s", data)
        self._pub_ind(topics_radio.ims_registration_info.ind, "radio.ims_registration_info.ind",
                      {"imsRegStatus": self.reg_status, "rat": self.reg_radio_tech,
                       "errorCode": self.reg_error_code, "errorString": self.reg_error_string})

    def force_ims_service_info(self, data: dict) -> None:
        """Patch the IMS per-service (sms/voice) availability snapshot and
        re-publish it as an indication. Drives
        IImsServingSystemListener::onImsServiceInfoChange."""
        self.post_fifo(Event(signal=signals.ForceImsServiceInfo, payload=data))

    def _apply_force_ims_service_info(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "sms" in data:
            self.sms_service_status = data["sms"]
        if "voice" in data:
            self.voice_service_status = data["voice"]
        _log.info("RadioImsAO force_ims_service_info applied: %s", data)
        self._pub_ind(topics_radio.ims_service_info.ind, "radio.ims_service_info.ind",
                      {"sms": self.sms_service_status, "voice": self.voice_service_status})

    def force_ims_pdp_status(self, data: dict) -> None:
        """Patch the IMS PDP context status snapshot and re-publish it as
        an indication. Drives
        IImsServingSystemListener::onImsPdpStatusInfoChange."""
        self.post_fifo(Event(signal=signals.ForceImsPdpStatus, payload=data))

    def _apply_force_ims_pdp_status(self, data: dict) -> None:
        if self._publish_fn is None:
            return
        if "isPdpConnected" in data:
            self.pdp_connected = data["isPdpConnected"]
        if "failureCode" in data:
            self.pdp_failure_code = data["failureCode"]
        if "failureReasonType" in data:
            self.pdp_failure_reason_type = data["failureReasonType"]
        if "failureReasonCode" in data:
            self.pdp_failure_reason_code = data["failureReasonCode"]
        if "apnName" in data:
            self.pdp_apn_name = data["apnName"]
        _log.info("RadioImsAO force_ims_pdp_status applied: %s", data)
        self._pub_ind(topics_radio.ims_pdp_status.ind, "radio.ims_pdp_status.ind",
                      {"isPdpConnected": self.pdp_connected, "failureCode": self.pdp_failure_code,
                       "failureReasonType": self.pdp_failure_reason_type,
                       "failureReasonCode": self.pdp_failure_reason_code,
                       "apnName": self.pdp_apn_name})


# ---------------------------------------------------------------------------
# HSM state handlers.
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
    elif e.signal in (signals.ForceImsRegistrationInfo, signals.ForceImsServiceInfo,
                       signals.ForceImsPdpStatus):
        _log.debug("radio IMS AO: %s dropped -- not Ready", e.signal_name)
        status = return_status.HANDLED
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway and Ready-entry publishes; a reconnect
        # racing the initial start needs nothing extra.
        _log.debug("radio IMS AO: Resubscribe dropped -- not Ready")
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
        while chart.recall() is not None:  # recall() replays only one deferred event per call
            pass
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        chart._do_exit_ready()
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceImsRegistrationInfo:
        chart._run_force(chart._apply_force_ims_registration_info, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceImsServiceInfo:
        chart._run_force(chart._apply_force_ims_service_info, e.payload)
        status = return_status.HANDLED
    elif e.signal == signals.ForceImsPdpStatus:
        chart._run_force(chart._apply_force_ims_pdp_status, e.payload)
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


__all__ = ["RadioImsAO"]
