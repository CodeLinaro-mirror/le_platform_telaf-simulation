# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""MPSS-side profile manager — ProfileStore and DataProfileAO.

``ProfileStore`` maintains an in-memory profile database seeded from config.
``DataProfileAO`` is Off → Starting → Ready → Stopping, driven synchronously
through :class:`~miros.ActiveObject`'s HSM machinery -- no dedicated AO thread
(see `serving_system.py`'s module docstring for the shared rationale).
`Starting`'s entry seeds `ProfileStore` and subscribes; `Ready`'s entry
publishes the readiness event and starts answering profile RPCs, emitting
``mp/ind/data/profile_changed`` on each mutating write.

Field names on the wire mirror telux::data::ProfileParams exactly
(profileName/apn/userName/password/techPref/authType/ipFamilyType/apnTypes/
emergencyAllowed/clatEnabled) and the profile object's id field is `id`
(not `profileId`) -- see DataProfileManager.cpp's profileParamsToWire /
wireToDataProfile for the PA-side counterpart these must match byte for
byte.
"""
from __future__ import annotations

import json
import logging
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Callable, Dict, List, Optional, Tuple

from miros import ActiveObject, Event, return_status, signals, spy_on

from sml.mpss import instrumentation as _instr
from sml.mpss.envelope import (
    build_error_envelope,
    build_event_envelope,
    build_success_envelope,
    dispatch_inbound,
)
from sml.config.models import SeedProfile
from sml.runtime.persist import atomic_write_json, read_json
from generated.python.topics import data as topics_data
from generated.python.validators import validate as validate_payload


_MAX_PROFILES = 32
_log = logging.getLogger("sml.mpss.data.profile")

# --- Seed-config -> wire enum string mappings --------------------------------
# SeedProfile (sml/config/models.py) uses short config-friendly enum
# spellings; the wire uses the same strings telux::data::ProfileParams's
# wire tables use (see DataProfileManager.cpp's authTypeToWire/
# techPrefToWire). Translate once at seed time so ProfileStore only ever
# holds wire-shaped values.

_SEED_TECH_TYPE_TO_WIRE = {"3GPP": "TP_3GPP", "3GPP2": "TP_3GPP2", "ANY": "TP_ANY"}
_SEED_AUTH_TYPE_TO_WIRE = {
    "NONE": "AUTH_NONE", "PAP": "AUTH_PAP", "CHAP": "AUTH_CHAP", "PAP_CHAP": "AUTH_PAP_CHAP",
}


def _seed_tech_pref_to_wire(tech_type: str) -> str:
    return _SEED_TECH_TYPE_TO_WIRE.get(tech_type, "TP_3GPP")


def _seed_auth_type_to_wire(auth_type: str) -> str:
    return _SEED_AUTH_TYPE_TO_WIRE.get(auth_type, "AUTH_NONE")


# ---------------------------------------------------------------------------
# ProfileStore
# ---------------------------------------------------------------------------

@dataclass
class Profile:
    id: int
    profileName: str = ""
    apn: str = ""
    userName: str = ""
    password: str = ""
    techPref: str = "TP_3GPP"
    authType: str = "AUTH_NONE"
    ipFamilyType: str = "IPV4V6"
    apnTypes: int = 0
    emergencyAllowed: str = "UNSPECIFIED"
    clatEnabled: bool = False
    # Store-internal bookkeeping only -- real SDK has no per-profile
    # isDefault field; default-ness is queried separately via get/setDefault
    # RPCs, which PA's DataConnectionManager currently stubs NOTSUPPORTED
    # (no wire traffic exists for them yet, out of P0 scope).
    isDefault: bool = False

    def to_wire(self) -> dict:
        """Profile object shape as it appears in query/list RPC responses."""
        d = asdict(self)
        d.pop("isDefault", None)
        return d


class ProfileStore:
    """Thread-safe in-memory profile database.

    `persist_path`, when given, makes `seed`/`create`/`modify`/`delete`
    write a full snapshot to disk on every successful mutation (see
    `sml.runtime.persist`), and enables `load()` to restore a snapshot
    from a prior process's run. `persist_path=None` (the default) is a
    pure in-memory store with no disk I/O at all -- existing zero-arg
    callers are unaffected.
    """

    def __init__(self, persist_path: Optional[Path] = None) -> None:
        self._profiles: Dict[int, Profile] = {}
        self._defaults: Dict[str, int] = {}  # opType → profileId
        self._persist_path = persist_path

    def seed(self, seed_profiles: List[SeedProfile]) -> None:
        for sp in seed_profiles:
            p = Profile(
                id=sp.profileId,
                apn=sp.apn,
                ipFamilyType=sp.ipFamily,
                authType=_seed_auth_type_to_wire(sp.authType),
                userName=sp.username,
                password=sp.password,
                techPref=_seed_tech_pref_to_wire(sp.techType),
                isDefault=sp.isDefault,
            )
            self._profiles[p.id] = p
            if sp.isDefault:
                self._defaults["DATA_LOCAL"] = sp.profileId
        self._maybe_persist()

    def query_all(self) -> List[dict]:
        return [p.to_wire() for p in self._profiles.values()]

    def query_filtered(self, filt: dict) -> List[dict]:
        """Filter on the subset of ProfileParams fields whose "unset"
        sentinel is unambiguous (empty string / UNKNOWN enum). authType,
        emergencyAllowed, apnTypes, clatEnabled, userName, and password are
        not filterable here -- their default value is also a valid real
        value, and the wire format carries no separate "field present" bit
        to disambiguate (known simplification, no schema layer yet).
        """
        out = []
        for p in self._profiles.values():
            if filt.get("apn") and p.apn != filt["apn"]:
                continue
            if filt.get("profileName") and p.profileName != filt["profileName"]:
                continue
            if filt.get("techPref") and filt["techPref"] != "UNKNOWN" and p.techPref != filt["techPref"]:
                continue
            if filt.get("ipFamilyType") and filt["ipFamilyType"] != "UNKNOWN" and p.ipFamilyType != filt["ipFamilyType"]:
                continue
            out.append(p.to_wire())
        return out

    def get_by_id(self, profile_id: int) -> Optional[dict]:
        p = self._profiles.get(profile_id)
        return p.to_wire() if p else None

    def get_default(self, op_type: str) -> Optional[int]:
        return self._defaults.get(op_type)

    def set_default(self, op_type: str, profile_id: int) -> Optional[str]:
        """Set the default profile id for `op_type`; persists like create/
        modify/delete. Returns an error code, or None on success."""
        if profile_id not in self._profiles:
            return "INVALID_ARGUMENTS"
        self._defaults[op_type] = profile_id
        self._maybe_persist()
        return None

    def create(self, data: dict) -> Tuple[Optional[int], Optional[str]]:
        if len(self._profiles) >= _MAX_PROFILES:
            return None, "NO_RESOURCES"
        next_id = next(
            (i for i in range(1, _MAX_PROFILES + 1) if i not in self._profiles),
            None,
        )
        if next_id is None:
            return None, "NO_RESOURCES"
        p = Profile(
            id=next_id,
            profileName=data.get("profileName", ""),
            apn=data.get("apn", ""),
            userName=data.get("userName", ""),
            password=data.get("password", ""),
            techPref=data.get("techPref", "UNKNOWN"),
            authType=data.get("authType", "AUTH_NONE"),
            ipFamilyType=data.get("ipFamilyType", "UNKNOWN"),
            apnTypes=data.get("apnTypes", 0),
            emergencyAllowed=data.get("emergencyAllowed", "UNSPECIFIED"),
            clatEnabled=data.get("clatEnabled", False),
        )
        self._profiles[next_id] = p
        self._maybe_persist()
        return next_id, None

    def modify(self, profile_id: int, data: dict) -> Tuple[Optional[dict], Optional[str]]:
        p = self._profiles.get(profile_id)
        if p is None:
            return None, "INVALID_ARGUMENTS"
        updated = Profile(
            id=profile_id,
            profileName=data.get("profileName", p.profileName),
            apn=data.get("apn", p.apn),
            userName=data.get("userName", p.userName),
            password=data.get("password", p.password),
            techPref=data.get("techPref", p.techPref),
            authType=data.get("authType", p.authType),
            ipFamilyType=data.get("ipFamilyType", p.ipFamilyType),
            apnTypes=data.get("apnTypes", p.apnTypes),
            emergencyAllowed=data.get("emergencyAllowed", p.emergencyAllowed),
            clatEnabled=data.get("clatEnabled", p.clatEnabled),
            isDefault=p.isDefault,
        )
        self._profiles[profile_id] = updated
        self._maybe_persist()
        return updated.to_wire(), None

    def delete(self, profile_id: int) -> Optional[str]:
        p = self._profiles.pop(profile_id, None)
        if p is None:
            return "INVALID_ARGUMENTS"
        for op_type, pid in list(self._defaults.items()):
            if pid == profile_id:
                del self._defaults[op_type]
        self._maybe_persist()
        return None

    def _maybe_persist(self) -> None:
        """No-op when `persist_path` is `None`. Write failures are logged
        and swallowed -- the in-memory mutation that triggered this call
        has already succeeded and must not be rolled back for a disk I/O
        fault; the only consequence is that this change won't survive the
        next restart."""
        if self._persist_path is None:
            return
        snapshot = {
            "profiles": [asdict(p) for p in self._profiles.values()],
            "defaults": dict(self._defaults),
        }
        try:
            atomic_write_json(self._persist_path, snapshot)
        except OSError as exc:
            _log.warning("failed to persist profile store to %s: %s", self._persist_path, exc)

    def load(self, path: Path) -> bool:
        """Restore a snapshot written by `_maybe_persist()` from `path`.

        Returns `True` and replaces `self._profiles`/`self._defaults` on
        success. Returns `False` (leaving state untouched) if `path`
        doesn't exist or fails to parse -- caller falls back to `seed()`.
        """
        snapshot = read_json(path)
        if snapshot is None:
            return False
        self._profiles = {
            p["id"]: Profile(**p) for p in snapshot.get("profiles", [])
        }
        self._defaults = dict(snapshot.get("defaults", {}))
        return True


# ---------------------------------------------------------------------------
# DataProfileAO
# ---------------------------------------------------------------------------

class DataProfileAO(ActiveObject):
    """MPSS-side profile manager Active Object.

    Off → Starting (seeds `ProfileStore`, subscribes) → Ready (answers
    RPCs) → Stopping (unsubscribes). All business logic runs synchronously
    on the caller's thread (the MqttClient AO thread) -- see module
    docstring.
    """

    def __init__(
        self,
        slot: int,
        seed_profiles: List[SeedProfile],
        mpss_src: str,
        persist_path: Optional[Path] = None,
    ) -> None:
        super().__init__("DataProfileAO")
        self._slot = slot
        self._seed_profiles = seed_profiles
        self._mpss_src = mpss_src
        self._persist_path = persist_path
        self._store = ProfileStore(persist_path=persist_path)
        self._publish_fn: Optional[Callable] = None
        self._subscribe_fn: Optional[Callable] = None
        self._unsubscribe_fn: Optional[Callable] = None
        self._pending_start_args: Optional[tuple] = None

        self._owned_topics: frozenset = frozenset()
        self._handlers: dict = {}

        self.start_at(smfn_off)
        _instr.apply_mode(self, _instr.current_mode())

    # ------------------------------------------------------------------
    # Public interface (called from DataSubsystem)
    # ------------------------------------------------------------------

    def start(self, publish_fn: Callable, subscribe_fn: Callable,
              unsubscribe_fn: Optional[Callable] = None) -> None:
        self._pending_start_args = (publish_fn, subscribe_fn, unsubscribe_fn)
        self.post_fifo(Event(signal=signals.Start))

    def stop(self) -> None:
        self.post_fifo(Event(signal=signals.Stop))

    def resubscribe(self) -> None:
        """Re-establish broker subscriptions and retained state.

        Posted by ``DataSubsystem.resubscribe`` after an MQTT reconnect. The AO
        keeps running across the flap -- only what the broker forgot is rebuilt.
        Mirrors the C++ bridge's ``Subscribing_St`` -> ``issueAllSubscribes_()``.
        """
        self.post_fifo(Event(signal=signals.Resubscribe))

    def owns_topic(self, topic: str) -> bool:
        return topic in self._owned_topics

    def handle_message(self, topic: str, payload: bytes) -> None:
        self.post_fifo(Event(signal=signals.MessageReceived,
                             payload=(topic, payload)))

    # ------------------------------------------------------------------
    # Helpers invoked from state handlers
    # ------------------------------------------------------------------

    def _do_start(self) -> None:
        publish_fn, subscribe_fn, unsubscribe_fn = self._pending_start_args
        self._pending_start_args = None
        self._publish_fn = publish_fn
        self._subscribe_fn = subscribe_fn
        self._unsubscribe_fn = unsubscribe_fn

        loaded_from_persist = self._persist_path is not None and self._store.load(self._persist_path)
        if not loaded_from_persist:
            self._store.seed(self._seed_profiles)

        mapping = {
            topics_data.query_profile.req:         self._handle_query,
            topics_data.request_profile_list.req:  self._handle_request_profile_list,
            topics_data.create_profile.req:        self._handle_create,
            topics_data.modify_profile.req:        self._handle_modify,
            topics_data.delete_profile.req:        self._handle_delete,
            topics_data.get_default_profile.req:   self._handle_get_default,
            topics_data.set_default_profile.req:   self._handle_set_default,
        }
        self._handlers = mapping
        self._owned_topics = frozenset(mapping.keys())
        for topic in self._owned_topics:
            subscribe_fn(topic)
        if loaded_from_persist:
            _log.info("profile AO subscribed (slot=%d, restored from persist %s)",
                      self._slot, self._persist_path)
        else:
            _log.info("profile AO subscribed (slot=%d, %d seed profiles)",
                      self._slot, len(self._seed_profiles))

    def _do_stop(self) -> None:
        if self._unsubscribe_fn:
            for topic in self._owned_topics:
                try:
                    self._unsubscribe_fn(topic)
                except Exception as exc:  # noqa: BLE001
                    _log.warning("unsubscribe %s failed: %s", topic, exc)

    def _do_resubscribe(self) -> None:
        """Re-issue every owned subscription, then re-publish retained ready.

        The retained ``ready=true`` has to be re-sent in case the broker
        restarted rather than merely dropping our session.
        """
        if self._subscribe_fn is None:
            return
        for topic in self._owned_topics:
            try:
                self._subscribe_fn(topic)
            except Exception as exc:  # noqa: BLE001
                _log.warning("resubscribe %s failed: %s", topic, exc)
        self._publish_subsys_ready(ready=True)
        _log.info("profile AO resubscribed (slot=%d)", self._slot)

    def _dispatch_message(self, topic: str, payload: bytes) -> None:
        dispatch_inbound(topic, payload, self._handlers, _log, "profile AO")

    def _pub_rsp(self, topic: str, schema_id: str, env: dict) -> None:
        if "data" in env:
            validate_payload(schema_id, env["data"])
        self._publish_fn(topic, json.dumps(env).encode(), 1, False)

    def _pub_ind(self, topic: str, schema_id: str, data: dict, retain: bool = False) -> None:
        validate_payload(schema_id, data)
        env = build_event_envelope(self._mpss_src, data)
        self._publish_fn(topic, json.dumps(env).encode(), 1, retain)

    def _pub_changed(self, event: str, profile_id: int, tech_pref: str) -> None:
        self._pub_ind(topics_data.profile_changed.ind, "data.profile_changed.ind",
                      {"profileId": profile_id, "techPref": tech_pref, "event": event})

    def _publish_subsys_ready(self, ready: bool) -> None:
        # Per-subsystem readiness indication PA's DataProfileManager waits on
        # (retained). Replaces the retired SubsysReadyOrchestrator's
        # notify_ready("profile") -- folded into the Ready state entry/exit.
        status = "AVAILABLE" if ready else "UNAVAILABLE"
        self._pub_ind(topics_data.subsys_ready_profile.ind, "data.subsys_ready_profile.ind",
                     {"ready": ready, "status": status}, retain=True)
        _log.debug("profile subsystem ready=%s published (slot=%d)", ready, self._slot)

    # ------------------------------------------------------------------
    # RPC handlers
    # ------------------------------------------------------------------

    def _handle_query(self, msg: dict) -> None:
        # This topic is shared by two real-SDK methods that both wire onto
        # it (see DataProfileManager.cpp's QueryProfile_Signal and
        # RequestProfile_Signal, both posting to kReqQueryProfile):
        #   - requestProfile(profileId, techPref) -> single lookup by id
        #   - queryProfile(ProfileParams filter)   -> filtered list
        # Disambiguate on presence of `profileId` in the payload.
        data = msg.get("data") or {}
        if "profileId" in data:
            pid = data.get("profileId")
            p = self._store.get_by_id(pid)
            profiles = [p] if p is not None else []
        else:
            profiles = self._store.query_filtered(data)
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"], {"profiles": profiles}
        )
        self._pub_rsp(topics_data.query_profile.rsp, "data.query_profile.rsp", env)

    def _handle_request_profile_list(self, msg: dict) -> None:
        env = build_success_envelope(
            self._mpss_src, msg["corrId"], msg["src"], {"profiles": self._store.query_all()}
        )
        self._pub_rsp(topics_data.request_profile_list.rsp, "data.request_profile_list.rsp", env)

    def _handle_create(self, msg: dict) -> None:
        data = msg.get("data") or {}
        pid, err = self._store.create(data)
        if err:
            env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], err)
        else:
            env = build_success_envelope(
                self._mpss_src, msg["corrId"], msg["src"], {"profileId": pid}
            )
            created = self._store.get_by_id(pid)
            self._pub_changed("CREATE", pid, created["techPref"])
        self._pub_rsp(topics_data.create_profile.rsp, "data.create_profile.rsp", env)

    def _handle_modify(self, msg: dict) -> None:
        data = msg.get("data") or {}
        pid = data.get("profileId")
        updated, err = self._store.modify(pid, data)
        if err:
            env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], err)
        else:
            env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
            self._pub_changed("MODIFY", pid, updated["techPref"])
        self._pub_rsp(topics_data.modify_profile.rsp, "data.modify_profile.rsp", env)

    def _handle_delete(self, msg: dict) -> None:
        data = msg.get("data") or {}
        pid = data.get("profileId")
        tech_pref = data.get("techPref", "UNKNOWN")
        err = self._store.delete(pid)
        if err:
            env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], err)
        else:
            env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
            self._pub_changed("DELETE", pid, tech_pref)
        self._pub_rsp(topics_data.delete_profile.rsp, "data.delete_profile.rsp", env)

    def _handle_get_default(self, msg: dict) -> None:
        data = msg.get("data") or {}
        op_type = data.get("opType", "DATA_LOCAL")
        pid = self._store.get_default(op_type)
        if pid is None:
            env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                       "INVALID_ARGUMENTS", f"no default profile for opType={op_type}")
        else:
            env = build_success_envelope(
                self._mpss_src, msg["corrId"], msg["src"], {"profileId": pid, "slot": self._slot}
            )
        self._pub_rsp(topics_data.get_default_profile.rsp, "data.get_default_profile.rsp", env)

    def _handle_set_default(self, msg: dict) -> None:
        data = msg.get("data") or {}
        slot = data.get("slot")
        op_type = data.get("opType", "DATA_LOCAL")
        pid = data.get("profileId")
        if slot != self._slot:
            env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"],
                                       "INVALID_ARGUMENTS", f"no such slot={slot}")
            self._pub_rsp(topics_data.set_default_profile.rsp, "data.set_default_profile.rsp", env)
            return
        err = self._store.set_default(op_type, pid)
        if err:
            env = build_error_envelope(self._mpss_src, msg["corrId"], msg["src"], err)
        else:
            env = build_success_envelope(self._mpss_src, msg["corrId"], msg["src"], {})
        self._pub_rsp(topics_data.set_default_profile.rsp, "data.set_default_profile.rsp", env)


# ---------------------------------------------------------------------------
# HSM state handlers (see serving_system.py module docstring for the shared pattern).
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
    """Composite parent -- one Stop handler, defers MessageReceived."""
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
    elif e.signal == signals.Resubscribe:
        # Starting subscribes anyway and Ready-entry publishes retained ready;
        # a reconnect racing the initial start needs nothing extra.
        _log.debug("profile AO: Resubscribe dropped -- not Ready")
        status = return_status.HANDLED
    else:
        chart.temp.fun = chart.top
        status = return_status.SUPER
    return status


@spy_on
def smfn_starting(chart, e):
    """Seeds ProfileStore and subscribes on entry; posts StartingDone so
    the next RTC drives the sibling trans() to Ready."""
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
        chart._publish_subsys_ready(ready=True)
        chart.recall()
        status = return_status.HANDLED
    elif e.signal == signals.EXIT_SIGNAL:
        if chart._publish_fn is not None:
            try:
                chart._publish_subsys_ready(ready=False)
            except Exception as exc:  # noqa: BLE001
                _log.warning("failed to publish profile ready=false on stop: %s", exc)
        status = return_status.HANDLED
    elif e.signal == signals.MessageReceived:
        topic, payload = e.payload
        chart._dispatch_message(topic, payload)
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
    """Terminal: unsubscribes on entry. No path back to Ready."""
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


__all__ = ["DataProfileAO", "Profile", "ProfileStore"]
