#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""
SOME/IP Gateway Service doest not need the Notifications from 'modem'.
But for different targets, we should change the json configuration to
meet different environments, ex. the simulation docker container.
"""

import sys, re
import fileinput
import socket, fcntl, struct
from ..core.logger import L
from .helper import quick_run, check_returncode

def get_ip_address(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ip = fcntl.ioctl(
        s.fileno(),
        0x8915,  # SIOCGIFADDR
        struct.pack('256s', bytes(ifname[:15], 'utf-8'))
    )[20:24]
    return socket.inet_ntoa(ip)

def to_run():

    result = quick_run("app status")
    if "[running] tafSomeipClntUnitTest" in result.stdout:
        L.info("Maybe the tafSomeipGWSvc unit test is running, do nothing.")
        sys.exit(-1)
    elif "tafSomeipGWSvc" not in result.stdout:
        L.error("The tafSomeipGWSvc is not installed, please install it first.")
        sys.exit(-1)
    elif "[running] tafSomeipGWSvc" in result.stdout:
        L.info("Let's stop tafSomeipGWSvc for re-configuring the service.")
        result = quick_run("app stop tafSomeipGWSvc")
        if result.returncode != 0:
            L.error("Can't stop the tafSomeipGWSvc, anyway, keep on...")

    L.info("Start to re-configure the json file for <{}> [Start]".format(__name__))

    find_me = re.compile(r'"unicast"\s*:\s*"(\d+\.\d+\.\d+\.\d+)"')
    try:
        conf_location = "/legato/systems/current/appsWriteable/tafSomeipGWSvc/tafSomeipGWSvc.json"
        local_ip = get_ip_address('eth0') # In container, we fix the ifname to 'eth0'
        with fileinput.input(conf_location, inplace=True) as jfile:
            for line in jfile:
                match = find_me.search(line)
                if match:
                    original_ip = match.group(1)
                    mline = line.replace(original_ip, local_ip)
                    L.info("Change IP [{}] -> [{}]".format(original_ip, local_ip))
                else:
                    mline = line
                print(mline, end="")
    except FileNotFoundError:
        L.error("Not found: {}".format(conf_location))
        sys.exit(-1)

    check_returncode(quick_run("app start tafSomeipGWSvc"))
    check_returncode(quick_run("app start tafSomeipSvrUnitTest"))
    check_returncode(quick_run("app start tafSomeipClntUnitTest"))

    L.info("Configuration is finished, please check 'logread' for test results [End]")
    sys.exit(0)
