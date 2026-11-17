#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""
start the script using the command 'simula utest ivss'
"""

import os
import re
import sys
import time
import signal
import tempfile
from ..core.logger import L
from .helper import quick_run, check_returncode
from .tafSomeipGWTest_helper import get_ip_address
from ..core.container import slavex
from ..tool import replace as t_replace

_SIMULATION_WORKDIR = "/root/simulation"

_IVSS_DATA_SVC_VSOMEIP = "/legato/systems/current/appsWriteable/tafIvssDataSvc/vsomeipRef.json"
_SOMEIP_GW_SVC_JSON = "/legato/systems/current/appsWriteable/tafSomeipGWSvc/tafSomeipGWSvc.json"
_IVSS_TELEPHONY_SVC_VSOMEIP = "/legato/systems/current/appsWriteable/tafIvssTelephonySvc/vsomeipRef.json"

_MNGD_CONNECTIVITY_JSON = "/data/ManagedServices/mngdConnectivity.json"

_REQUIRED_APPS = ("tafSomeipGWSvc", "tafMngdPMSvc", "tafMngdConnSvc", "tafIvssDataSvc")

_PM_VHAL_DRV_TEST_SO = "pmVHalDrvTest.so"
_PM_VHAL_DRV_TEST_MODULE = "TafHalPowerManagement"

_SLAVE_INFO_FILE = "/root/simulation/.simula.slave"
_SLAVE_CLIENT_TEST_DIR = "/root/simulation/install-soa"
_SLAVE_CLIENT_TEST_VSOMEIP = "{}/conf/vsomeip-test.json".format(_SLAVE_CLIENT_TEST_DIR)
_SLAVE_CLIENT_TEST_COMMONAPI = "{}/conf/commonapi-test.ini".format(_SLAVE_CLIENT_TEST_DIR)
_SLAVE_CLIENT_TEST_BIN = "{}/IvssClientTest".format(_SLAVE_CLIENT_TEST_DIR)

# Fail fast instead of hanging if the slave is unreachable or its host key changed.
_SLAVE_SSH_OPTS = "-o ConnectTimeout=5 -o BatchMode=yes"

_MASK_PROMPT = """
Select an IVSS test service:
    1) IVSS_TEST_SVC_RADIO_MASK       0x1
    2) IVSS_TEST_SVC_SIM_MASK         0x10
    3) IVSS_TEST_SVC_INFO_MASK        0x100
    4) IVSS_TEST_SVC_MNGDCONN_MASK    0x1000
    5) IVSS_TEST_SVC_MASK_ALL         0xFFFFFFFF
    6) Exit
"""

_MASK_CHOICES = {
    "1": "0x1",
    "2": "0x10",
    "3": "0x100",
    "4": "0x1000",
    "5": "0xFFFFFFFF",
}
_EXIT_CHOICE = "6"
_SLAVE_SUPPORTED_MASK = "0x1000"  # IVSS_TEST_SVC_MNGDCONN_MASK is the only mask tafIvssTelephonySvc supports on slave

class _StopTestLoop(Exception):
    """Raised from the SIGTSTP handler to break out of the test loop."""
    pass

def _redirect_to_eth0(conf_location, local_ip):
    L.info("Re-point vsomeip config [{}] -> unicast [{}], device [eth0]".format(conf_location, local_ip))
    t_replace.replace(conf_location, r'192\.168\.225\.1', local_ip)
    t_replace.replace(conf_location, r'172\.18\.0\.3', local_ip)
    t_replace.replace(conf_location, r'bridge0', 'eth0')

def _start_app_if_needed(app_name, status_output):
    if "[running] {}".format(app_name) in status_output:
        L.info("{} is already running, restarting it.".format(app_name))
        check_returncode(quick_run("app restart {}".format(app_name)))
    else:
        check_returncode(quick_run("app start {}".format(app_name)))

    status_after = quick_run("app status")
    if "[running] {}".format(app_name) not in status_after.stdout:
        L.error("Error: {} is not running after start/restart.".format(app_name))
        sys.exit(-1)

def _find_file(root_dir, filename):
    for dirpath, _, filenames in os.walk(root_dir):
        if filename in filenames:
            return os.path.join(dirpath, filename)
    return None

def _enable_app_managed_connectivity_recovery(json_path):
    with open(json_path) as f:
        content = f.read()
    patched = re.sub(
        r'("AppManagedConnectivityRecovery"\s*:\s*\{\s*"Enable"\s*:\s*")No(")',
        r'\1Yes\2',
        content)
    with open(json_path, "w") as f:
        f.write(patched)

def _ensure_pm_resumed():
    pm_state = quick_run("pm state")
    check_returncode(pm_state)
    if "resume" not in pm_state.stdout.lower():
        L.info("Power state is [{}], resuming.".format(pm_state.stdout.strip()))
        check_returncode(quick_run("pm resume"))
    time.sleep(3)

def _slave_ssh(slave_ip, remote_cmd):
    return quick_run("ssh -p 22 {} root@{} {}".format(_SLAVE_SSH_OPTS, slave_ip, remote_cmd))

def _ensure_remote_file_exists(slave_ip, path):
    result = _slave_ssh(slave_ip, "test -f {}".format(path))
    if result.returncode != 0:
        L.error("Error: {} not found on slave [{}]".format(path, slave_ip))
        sys.exit(-1)

def _ensure_remote_executable(slave_ip, path):
    _ensure_remote_file_exists(slave_ip, path)
    exec_result = _slave_ssh(slave_ip, "test -x {}".format(path))
    if exec_result.returncode != 0:
        L.info("{} is not executable on slave [{}], adding execute permission.".format(path, slave_ip))
        check_returncode(_slave_ssh(slave_ip, "chmod +x {}".format(path)))

def _ensure_slave_test_dir_exists(slave_ip):
    result = _slave_ssh(slave_ip, "test -d {}".format(_SLAVE_CLIENT_TEST_DIR))
    if result.returncode != 0:
        L.error("Error: Test files not there, rebuild using TELAF_SIMULATION_IVSS_TEST option enabled.")
        sys.exit(-1)

def _run_client_test_on_slave(mask, telephony_available):
    if not os.path.exists(_SLAVE_INFO_FILE):
        L.error("Error: No slave container available ({} not found), cannot run IvssClientTest on slave.".format(_SLAVE_INFO_FILE))
        sys.exit(-1)

    _, slave_ip = slavex.current_slave()

    _ensure_slave_test_dir_exists(slave_ip)
    _ensure_remote_file_exists(slave_ip, _SLAVE_CLIENT_TEST_VSOMEIP)
    _ensure_remote_file_exists(slave_ip, _SLAVE_CLIENT_TEST_COMMONAPI)
    _ensure_remote_executable(slave_ip, _SLAVE_CLIENT_TEST_BIN)

    L.info("Re-point IvssClientTest's vsomeip config on slave [{}] -> unicast [{}]".format(slave_ip, slave_ip))
    tmp_fd, tmp_path = tempfile.mkstemp(suffix=".json")
    os.close(tmp_fd)
    try:
        check_returncode(quick_run("scp -P 22 {} root@{}:{} {}".format(_SLAVE_SSH_OPTS, slave_ip, _SLAVE_CLIENT_TEST_VSOMEIP, tmp_path)))
        t_replace.replace(tmp_path, r'"unicast"\s*:\s*"(.*?)"', '"unicast" : "{}"'.format(slave_ip))
        check_returncode(quick_run("scp -P 22 {} {} root@{}:{}".format(_SLAVE_SSH_OPTS, tmp_path, slave_ip, _SLAVE_CLIENT_TEST_VSOMEIP)))
    finally:
        os.remove(tmp_path)

    def _on_sigtstp(signum, frame):
        raise _StopTestLoop()

    previous_tstp = None
    if hasattr(signal, "SIGTSTP"):
        previous_tstp = signal.signal(signal.SIGTSTP, _on_sigtstp)

    try:
        while True:
            if mask is None:
                print(_MASK_PROMPT)
                try:
                    choice = input("Enter your choice [1-6]: ").strip()
                except EOFError:
                    break
                if choice == _EXIT_CHOICE:
                    break
                if choice not in _MASK_CHOICES:
                    print("Invalid choice [{}], please enter a number between 1 and 6.".format(choice))
                    continue
                mask = _MASK_CHOICES[choice]

            if not telephony_available and mask != _SLAVE_SUPPORTED_MASK:
                L.error("Error: tafIvssTelephonySvc is not available on slave, only IVSS_TEST_SVC_MNGDCONN_MASK {} can be run.".format(_SLAVE_SUPPORTED_MASK))
                mask = None
                continue

            run_client_cmd = "export VSOMEIP_CONFIGURATION={} && export COMMONAPI_CONFIG={} && {} {}".format(
                _SLAVE_CLIENT_TEST_VSOMEIP, _SLAVE_CLIENT_TEST_COMMONAPI, _SLAVE_CLIENT_TEST_BIN, mask)

            L.info("Running IvssClientTest {} on slave [{}] ...".format(mask, slave_ip))
            slavex.remote_do(run_client_cmd)
            mask = None
    except (_StopTestLoop, KeyboardInterrupt):
        print("")
    finally:
        if previous_tstp is not None:
            signal.signal(signal.SIGTSTP, previous_tstp)

    L.info("IvssClientTest loop on slave is finished.")

def to_run(mask=None):

    L.info("Get started for the helper program <{}> [Start]".format(__name__))

    local_ip = get_ip_address('eth0')  # In container, we fix the ifname to 'eth0'

    try:
        _redirect_to_eth0(_IVSS_DATA_SVC_VSOMEIP, local_ip)
        _redirect_to_eth0(_SOMEIP_GW_SVC_JSON, local_ip)
    except FileNotFoundError as e:
        L.error("Error: Not found: {}".format(e.filename))
        sys.exit(-1)

    os.environ["COMMONAPI_CONFIG"] = "/legato/systems/current/appsWriteable/tafIvssDataSvc/commonapiRef.ini"
    os.environ["VSOMEIP_CONFIGURATION"] = _IVSS_DATA_SVC_VSOMEIP

    result = quick_run("telaf status")
    if "TelAF framework is running" in result.stdout:
        L.info("TelAF framework is already running, skip 'telaf start'.")
    else:
        check_returncode(quick_run("telaf start"))
        time.sleep(5)

    result = quick_run("app status")
    for app in _REQUIRED_APPS:
        if app not in result.stdout:
            L.error("Error: The {} is not installed, please install it first.".format(app))
            sys.exit(-1)

    module_list = quick_run("tafmodule list")
    if _PM_VHAL_DRV_TEST_MODULE in module_list.stdout:
        L.info("{} module is already installed, skip installing {}.".format(_PM_VHAL_DRV_TEST_MODULE, _PM_VHAL_DRV_TEST_SO))
    else:
        drv_path = _find_file(_SIMULATION_WORKDIR, _PM_VHAL_DRV_TEST_SO)
        if drv_path is None:
            L.error("Error: {} not found in {}, install {} driver and retry".format(
                _PM_VHAL_DRV_TEST_SO, _SIMULATION_WORKDIR, _PM_VHAL_DRV_TEST_MODULE))
            sys.exit(-1)
        check_returncode(quick_run("cp {} /tmp/legato/devManager/drivers/".format(drv_path)))
        check_returncode(quick_run("tafmodule install /tmp/legato/devManager/drivers/{}".format(_PM_VHAL_DRV_TEST_SO)))
    _start_app_if_needed("tafMngdPMSvc", result.stdout)
    time.sleep(12)

    if not os.path.exists(_MNGD_CONNECTIVITY_JSON):
        L.error("Error: {} not found".format(_MNGD_CONNECTIVITY_JSON))
        sys.exit(-1)
    t_replace.replace(_MNGD_CONNECTIVITY_JSON, r'"AutoStart"\s*:\s*"Yes"', '"AutoStart" : "No"')
    _enable_app_managed_connectivity_recovery(_MNGD_CONNECTIVITY_JSON)
    check_returncode(quick_run("cp {} /data/persist/telaf/".format(_MNGD_CONNECTIVITY_JSON)))
    _ensure_pm_resumed()
    if "[running] tafDataCallSvc" not in quick_run("app status").stdout:
        L.error("Error: tafDataCallSvc is not running, start it before running the IVSS test.")
        sys.exit(-1)
    _start_app_if_needed("tafMngdConnSvc", result.stdout)
    time.sleep(2)

    time.sleep(2)

    _ensure_pm_resumed()
    _start_app_if_needed("tafIvssDataSvc", result.stdout)

    telephony_vsomeip_exists = os.path.exists(_IVSS_TELEPHONY_SVC_VSOMEIP)
    if "tafIvssTelephonySvc" not in result.stdout:
        L.info("tafIvssTelephonySvc is not installed, skip starting it.")
    elif not telephony_vsomeip_exists:
        L.info("{} not found, skip starting tafIvssTelephonySvc.".format(_IVSS_TELEPHONY_SVC_VSOMEIP))
    else:
        status = quick_run("app status")
        for app in ("tafDevInfoSvc", "tafRadioSvc", "tafSimCardSvc"):
            if "[running] {}".format(app) not in status.stdout:
                L.error("Error: {} is not running, start it before starting tafIvssTelephonySvc.".format(app))
                sys.exit(-1)
        _redirect_to_eth0(_IVSS_TELEPHONY_SVC_VSOMEIP, local_ip)
        _start_app_if_needed("tafIvssTelephonySvc", result.stdout)

    _run_client_test_on_slave(mask, telephony_vsomeip_exists)

    L.info("Configuration is finished, please check 'logread' for test results [End]")
    sys.exit(0)
