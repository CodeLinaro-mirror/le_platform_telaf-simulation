#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""
To assistant the SMS service unit test, we need to simulate
the modem to trigger the event of 'receiving an SMS'
"""

import subprocess
import threading
import sys
from ..core.logger import L
from .helper import quick_run, quick_popen, check_returncode

def _monitor_output(process):
    incoming_alphabet_msg_cmd = "telsdk_event_injector -f tel_sms -e incomingsms 1 1 1 1 GSM7 0  0791889653784434040A8190973334790000329070510574234755779A0EA297E7749059FE6E83E86576D8DC8AB94055779A0EA297E7749059FE6E83E86576D8DC92B94055779A0EA297E7749059FE6E83E86576D8DC9AB900 0979334397 'Unit test from telaf-1. Unit test from telaf-2. Unit test from telaf-3.'".encode("utf-8")
    incoming_number_msg_cmd   = "telsdk_event_injector -f tel_sms -e incomingsms 1 1 1 1 GSM7 0  0791889653784434040A81909733347900003290705105052336B0986C46ABD96EB81C081693CD6835DB0D9703C162B219AD66BBE17220584C36A3D56C375C0E048BC966B49AED86CB01 0979334397 '0123456789 0123456789 0123456789 0123456789 0123456789'".encode("utf-8")
    incoming_symbol_msg_cmd   = "telsdk_event_injector -f tel_sms -e incomingsms 1 1 1 1 GSM7 0  0791889653784434040A819097333479000032907051053523169B5E0830126C2826152A15596D509B948EE7FB01 0979334397 '~!@#$^&*()_+{}:<>?'".encode("utf-8")
    incoming_binary_msg_cmd   = "telsdk_event_injector -f tel_sms -e incomingsms 1 1 1 1 GSM8 0  0791889653784434040A8190973334790004329070510565230200FF 0979334397 ÿ".encode("utf-8")
    incoming_ucs2_msg_cmd     = "telsdk_event_injector -f tel_sms -e incomingsms 1 1 1 1 UCS2 0  0791889653784434040A819097333479000832907051150023046E2C8A66 0979334397 測試".encode("utf-8")

    while True:
        try:
            output_line = process.stdout.readline().strip()
        except UnicodeDecodeError as e:
            L.warning("Decoding Error: %s", e)
            continue

        if not output_line:
            break

        L.debug(output_line)

        stop_app = lambda: quick_run("app stop tafSMSUnitTest")

        if "send msg with type [alphabet]" in output_line:
            L.info("Found target string 'alphabet' in output")
            check_returncode(quick_run(incoming_alphabet_msg_cmd), stop_app)

        elif "send msg with type [number]" in output_line:
            L.info("Found target string 'number' in output")
            check_returncode(quick_run(incoming_number_msg_cmd), stop_app)

        elif "send msg with type [symbol]" in output_line:
            L.info("Found target string 'symbol' in output")
            check_returncode(quick_run(incoming_symbol_msg_cmd), stop_app)

        elif "send msg with type [binary]" in output_line:
            L.info("Found target string 'binary' in output")
            check_returncode(quick_run(incoming_binary_msg_cmd), stop_app)

        elif "send msg with type [ucs2]" in output_line:
            L.info("Found target string 'ucs2' in output")
            check_returncode(quick_run(incoming_ucs2_msg_cmd), stop_app)

            # This script is only a helper function
            # and does not participate in the completion of the verification,
            # so just go out and shutdown gracefully.
            break

def to_run():

    result = quick_run("app status")
    if "[running] tafSMSUnitTest" in result.stdout:
        L.info("The tafSMSUnitTest is running, please wait it to finish or stop it")
        sys.exit(-1)
    elif "tafSMSUnitTest" not in result.stdout:
        L.error("The tafSMSUnitTest is not installed, please install it first.")
        sys.exit(-1)
    else:
        # In the case of a system exception,
        # the program may not be in the right state,
        # so you can try to stop it first.
        quick_run("app stop tafSMSUnitTest")

    L.info("Get started for the helper program <{}> [Start]".format(__name__))

    process = quick_popen("logread -f")
    result = quick_run("app start tafSMSUnitTest")
    check_returncode(result)

    monitor_thread = threading.Thread(target=_monitor_output, args=(process,))
    monitor_thread.start()

    monitor_thread.join()
    process.terminate()

    L.info("All done regarding of this helper program, type 'logread' for checking test result [End]")
    sys.exit(0)
