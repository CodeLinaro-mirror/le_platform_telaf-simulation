# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Unit tests for PowerSubsystem (walking skeleton + follow-on behaviors)."""
from __future__ import annotations

import json
import time
from unittest.mock import MagicMock

import pytest

from sml.apss.power import ALL_MACHINES, PowerSubsystem
from generated.python.topics import power as topics_power
from generated.python.topics import wakeup as topics_wakeup


def _wait_for_state(ao, target: str, timeout: float = 1.0) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        state = getattr(ao, "state", None)
        fn = getattr(state, "fun", None) if state is not None else None
        if fn is not None and fn.__name__ == target:
            return
        time.sleep(0.005)
    raise AssertionError(f"AO never reached {target}")


def _wait_until(predicate, timeout: float = 1.0, interval: float = 0.005) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(interval)
    return predicate()


def _env(data: dict, corr="abcd", src="test", dest="apss-dev-1") -> bytes:
    return json.dumps({
        "v": 1, "corrId": corr, "ts": 1, "src": src, "dest": dest, "data": data,
    }).encode()


@pytest.fixture
def ao():
    subsystem = PowerSubsystem(local_machine_name="mdm")
    publish_fn = MagicMock()
    subscribe_fn = MagicMock()
    unsubscribe_fn = MagicMock()
    subsystem.start(publish_fn, subscribe_fn, unsubscribe_fn)
    _wait_for_state(subsystem, "smfn_ready", timeout=1.0)
    return subsystem, publish_fn


def _published(publish_fn, topic):
    for call in publish_fn.call_args_list:
        if call.args[0] == topic:
            yield json.loads(call.args[1])


def test_reaches_ready_and_publishes_subsys_ready(ao):
    subsystem, publish_fn = ao
    tcu = list(_published(publish_fn, topics_power.subsys_ready_tcu.ind))
    wakeup = list(_published(publish_fn, topics_power.subsys_ready_wakeup.ind))
    assert tcu and tcu[-1]["data"] == {"ready": True, "status": "AVAILABLE"}
    assert wakeup and wakeup[-1]["data"] == {"ready": True, "status": "AVAILABLE"}


def test_set_state_resume_happy_path(ao):
    """Walking skeleton (plan.md §8): single-machine RESUME, no ACK wait."""
    subsystem, publish_fn = ao
    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "mdm"}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "RESUME", "machineName": "mdm"}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"] == {"ok": True}
    assert subsystem._machines_state["mdm"] == "RESUME"
    ind = list(_published(publish_fn, topics_power.state_update.ind))[-1]
    assert ind["data"] == {"state": "RESUME", "machineName": "mdm"}


def test_set_state_suspend_no_other_machines_completes_immediately(ao):
    """Single registered machine (itself) already matches -> no wait needed."""
    subsystem, publish_fn = ao
    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "mdm"}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"] == {"ok": True}
    assert subsystem._machines_state["mdm"] == "SUSPEND"
    ind = list(_published(publish_fn, topics_power.state_update.ind))[-1]
    assert ind["data"] == {"state": "SUSPEND", "machineName": "mdm"}


def test_set_state_suspend_with_slave_times_out(ao):
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 50
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "adsp"}),
    )
    assert _wait_until(
        lambda: any(_published(publish_fn, topics_power.set_state.rsp)), timeout=1.0
    )
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"]["ok"] is False

    status = list(_published(publish_fn, topics_power.slave_ack_status.ind))[-1]
    assert status["data"]["status"] == "TIMEOUT"
    assert status["data"]["unresponsive"] == [{"clientName": "adsp", "machineName": "adsp"}]


def test_set_state_suspend_with_slave_ack_completes(ao):
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 2000
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "adsp"}),
    )
    subsystem.handle_message(
        topics_power.slave_ack.req,
        _env({"response": "ACK", "state": "SUSPEND", "machineName": "adsp"}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"] == {"ok": True}
    status = list(_published(publish_fn, topics_power.slave_ack_status.ind))[-1]
    assert status["data"]["status"] == "COMPLETE"
    assert subsystem._machines_state["adsp"] == "SUSPEND"


def test_get_machine_names(ao):
    subsystem, publish_fn = ao
    subsystem.add_machine({"machineName": "adsp"})
    subsystem.handle_message(topics_power.get_machine_names.req, _env({}))
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.get_machine_names.rsp)))
    rsp = list(_published(publish_fn, topics_power.get_machine_names.rsp))[-1]
    assert sorted(rsp["data"]["machineNames"]) == ["adsp", "mdm"]


def test_add_and_remove_machine_publish_machine_update(ao):
    subsystem, publish_fn = ao
    subsystem.add_machine({"machineName": "adsp"})
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.machine_update.ind)))
    added = list(_published(publish_fn, topics_power.machine_update.ind))[-1]
    assert added["data"] == {"machineName": "adsp", "machineEvent": "AVAILABLE"}

    subsystem.remove_machine({"machineName": "adsp"})
    assert _wait_until(
        lambda: list(_published(publish_fn, topics_power.machine_update.ind))[-1]["data"]
        == {"machineName": "adsp", "machineEvent": "UNAVAILABLE"}
    )
    removed = list(_published(publish_fn, topics_power.machine_update.ind))[-1]
    assert removed["data"] == {"machineName": "adsp", "machineEvent": "UNAVAILABLE"}


def test_force_slave_ack_injects_ack(ao):
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 2000
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "adsp"}),
    )
    subsystem.force_slave_ack({"machineName": "adsp", "response": "ACK"})
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"] == {"ok": True}


def test_wakeup_event_relayed_to_power_wakeup_ind(ao):
    subsystem, publish_fn = ao
    payload = {
        "serviceId": 5, "sourceNodeId": 0, "destinationNodeId": 0,
        "isMsgIdValid": True, "msgId": 1, "isPIDValid": False, "pid": 0,
        "isProcessNameValid": False, "processName": "",
    }
    subsystem.handle_message(topics_wakeup.event.ind, _env(payload))
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.wakeup.ind)))
    relayed = list(_published(publish_fn, topics_power.wakeup.ind))[-1]
    assert relayed["data"]["wakeupType"] == "QMI"
    assert relayed["data"]["qmiWakeupInfo"] == payload


def test_set_state_all_machines_suspend_local_only_sends_ind(ao):
    """Scenario 1 (apss_power.md §5): ALL_MACHINES targeting only the local
    machine still gets its own state_update.ind and skips slave_ack_status."""
    subsystem, publish_fn = ao
    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": ALL_MACHINES}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"] == {"ok": True}
    ind = list(_published(publish_fn, topics_power.state_update.ind))[-1]
    assert ind["data"] == {"state": "SUSPEND", "machineName": "mdm"}
    assert not list(_published(publish_fn, topics_power.slave_ack_status.ind))


def test_set_state_resume_after_suspend_sends_ind_to_previously_suspended(ao):
    """Scenario 3: ALL_MACHINES RESUME after a SUSPEND round must ind every
    machine that was actually SUSPENDed (including the local one), with no
    ACK wait."""
    subsystem, publish_fn = ao
    subsystem.add_machine({"machineName": "adsp"})
    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": ALL_MACHINES}),
    )
    subsystem.force_slave_ack({"machineName": "adsp", "response": "ACK"})
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "RESUME", "machineName": ALL_MACHINES}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"] == {"ok": True}
    inds = list(_published(publish_fn, topics_power.state_update.ind))
    machines_told = {i["data"]["machineName"] for i in inds}
    assert machines_told == {"mdm", "adsp"}
    assert all(i["data"]["state"] == "RESUME" for i in inds)
    assert not list(_published(publish_fn, topics_power.slave_ack_status.ind))


def test_set_state_resume_queued_behind_inflight_suspend_does_not_cancel_it(ao):
    """Scenario 4: a RESUME arriving while SUSPEND is in flight queues behind
    it instead of cancelling it -- SUSPEND still runs its full protocol."""
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 2000
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": ALL_MACHINES}, corr="0001"),
    )
    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "RESUME", "machineName": ALL_MACHINES}, corr="0002"),
    )
    subsystem.force_slave_ack({"machineName": "adsp", "response": "ACK"})

    assert _wait_until(
        lambda: len(list(_published(publish_fn, topics_power.set_state.rsp))) >= 2
    )
    rsps = list(_published(publish_fn, topics_power.set_state.rsp))
    assert [r["corrId"] for r in rsps] == ["0001", "0002"]
    assert all(r["data"] == {"ok": True} for r in rsps)
    status = list(_published(publish_fn, topics_power.slave_ack_status.ind))
    assert status and status[-1]["data"]["status"] == "COMPLETE"
    assert subsystem._machines_state["adsp"] == "RESUME"
    assert subsystem._machines_state["mdm"] == "RESUME"


def test_set_state_shutdown_inflight_rejects_new_request(ao):
    """Scenario 5: while SHUTDOWN is the in-flight queue head, any new
    set_state (including RESUME) is rejected at intake, not queued."""
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 2000
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SHUTDOWN", "machineName": ALL_MACHINES}),
    )
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "RESUME", "machineName": "mdm"}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"]["ok"] is False
    assert len(subsystem._request_queue) == 1


def test_set_state_duplicate_queued_request_rejected(ao):
    """Scenario 6: a duplicate SUSPEND for the same target while the first
    one is still in flight (awaiting ack) is rejected at intake."""
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 2000
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "adsp"}, corr="fff1"),
    )
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": "adsp"}, corr="fff2"),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["corrId"] == "fff2"
    assert rsp["data"]["ok"] is False
    assert len(subsystem._request_queue) == 1


def test_set_state_first_nack_finalizes_without_waiting_for_stragglers(ao):
    """Scenario 7: the first NACK finalizes the round immediately -- it does
    not wait for the remaining expected machines to respond."""
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 2000
    subsystem.add_machine({"machineName": "adsp"})
    subsystem.add_machine({"machineName": "cdsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": ALL_MACHINES}),
    )
    subsystem.force_slave_ack({"machineName": "adsp", "response": "NACK"})

    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    rsp = list(_published(publish_fn, topics_power.set_state.rsp))[-1]
    assert rsp["data"]["ok"] is False
    status = list(_published(publish_fn, topics_power.slave_ack_status.ind))[-1]
    assert status["data"]["nack"] == [{"clientName": "adsp", "machineName": "adsp"}]
    assert status["data"]["unresponsive"] == [{"clientName": "cdsp", "machineName": "cdsp"}]


def test_ftimeout_retry_rebroadcasts_local_state(ao):
    """Scenario 8: after a failed round involving the local machine, the
    ftimeout retry re-confirms the local machine's state via a fresh ind,
    without re-querying remotes."""
    subsystem, publish_fn = ao
    subsystem._timeout_ms = 50
    subsystem._ftimeout_ms = 100
    subsystem.add_machine({"machineName": "adsp"})
    publish_fn.reset_mock()

    subsystem.handle_message(
        topics_power.set_state.req,
        _env({"state": "SUSPEND", "machineName": ALL_MACHINES}),
    )
    assert _wait_until(lambda: any(_published(publish_fn, topics_power.set_state.rsp)))
    publish_fn.reset_mock()

    assert _wait_until(
        lambda: any(_published(publish_fn, topics_power.state_update.ind)), timeout=2.0
    )
    ind = list(_published(publish_fn, topics_power.state_update.ind))[-1]
    assert ind["data"] == {"state": "SUSPEND", "machineName": "mdm"}

