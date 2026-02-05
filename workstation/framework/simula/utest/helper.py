#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import subprocess
import sys
from ..core.logger import L

def check_returncode(result, callback=None, need_exit=True):
    if result.returncode != 0:
        L.error("Error: %s, %s", result.args, result.stderr)
        if callback:
            callback()
        if need_exit == True:
            sys.exit(result.returncode)
    else:
        L.info("The command [%s] is done.", result.args)

def quick_run(command):
    return subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          universal_newlines=True)

def quick_popen(command):
    return subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            universal_newlines=True, bufsize=1, encoding="utf-8")