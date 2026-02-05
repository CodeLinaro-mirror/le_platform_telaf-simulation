#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""
For DCS unit test, we need prepare some profiles first, if you see the
implementation of DCS unit test, the index number 5 is used. Usually,
we use telsdk_console_app to create the profiles maunally, now we can
use 'simula utest dcs' to add them automatically.
"""

import sys, tempfile, os
from ..core.logger import L
from .helper import quick_run, check_returncode

def to_run():

    result = quick_run("app status")
    if "[running] tafDataCallUnitTest" in result.stdout:
        L.info("Maybe the tafDataCallUnitTest is running, do nothing.")
        sys.exit(-1)
    elif "tafDataCallUnitTest" not in result.stdout:
        L.error("The tafDataCallUnitTest is not installed, please install it first.")
        sys.exit(-1)

    L.info("Start to create some profiles for testing <{}> [Start]".format(__name__))

    once_lock_file = "/tmp/utest.dcs.once"
    if os.path.isfile(once_lock_file):
        L.info("Maybe the profiles were already added, move on ...")
    else:
        L.info("Add some profiles ...")

        profiles = (
            b'6\n10\n2\n0\nprofile3\napn3\n0\n\n\n0\n4\n0\nq\nq\nq\n',
            b'6\n10\n2\n0\nprofile4\napn4\n0\n\n\n0\n6\n0\nq\nq\nq\n',
            b'6\n10\n2\n0\nprofile5\napn5\n0\n\n\n0\n10\n0\nq\nq\nq\n'
        )

        for profile in profiles:
            tmpfile = tempfile.NamedTemporaryFile(delete=False)
            tmpfile.write(profile)
            tmpfile.close()
            check_returncode(quick_run("telsdk_console_app < {}".format(tmpfile.name)))

        with open(once_lock_file, 'w') as file: pass

    check_returncode(quick_run("app start tafDataCallUnitTest"))

    L.info("Preparation is finished, please check 'logread' for test results [End]")
    sys.exit(0)
