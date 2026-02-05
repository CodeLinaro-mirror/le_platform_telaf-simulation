#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

from .logger import L
import subprocess
import os, sys


class MasterContainer():
    # Only one 'master'
    CURRENT_MASTER = "/root/simulation/.simula.master"

    def __init__(self): pass

    def _get_current_master(self):
        with open(MasterContainer.CURRENT_MASTER) as current_master_fd:
            cname, ipaddr = current_master_fd.readline().strip().split()
            return (cname, ipaddr)

    def login_master(self):
        cname, ipaddr = self._get_current_master()
        os.system("ssh -o LogLevel=quiet root@{}".format(ipaddr))

    def show_info(self):
        print("Master Information")
        cname, ipaddr = self._get_current_master()
        print("  {} @ {} [master]".format(cname, ipaddr))


class SlavexContainer():

    SLAVEX_LIST_FILE = "/root/simulation/.slavex"
    CURRENT_SLAVE = "/root/simulation/.simula.slave"

    def __init__(self): pass

    def remote_do(self, command):
        command = "ssh -o LogLevel=quiet -t root@{slave_addr} \"bash -cil '{slave_cmd}'\"".format(
                  slave_addr = self._get_current_slave()[1], # just get addr
                  slave_cmd = command)
        os.system(command)

    def _get_current_slave(self):
        with open(SlavexContainer.CURRENT_SLAVE) as current_slave_fd:
            cname, ipaddr = current_slave_fd.readline().strip().split()
            return (cname, ipaddr)

    def login_slave(self):
        cname, ipaddr = self._get_current_slave()
        os.system("ssh -o LogLevel=quiet root@{}".format(ipaddr))

    def list_slave(self):
        _, current_slave_ip = self._get_current_slave()
        with open(SlavexContainer.SLAVEX_LIST_FILE) as slavex_fd:
            print("Slave List Information:")
            for line in slavex_fd.readlines():
                cname, ipaddr = line.strip().split()
                if current_slave_ip == ipaddr:
                    print("  {name} @ {ip} <--".format(name = cname, ip = ipaddr))
                else:
                    print("  {name} @ {ip}".format(name = cname, ip = ipaddr))

    def change_def_and_login(self, slave_ip):
        found_it = False
        with open(SlavexContainer.SLAVEX_LIST_FILE) as slavex_fd:
            for line in slavex_fd:
                cname, ipaddr = line.strip().split()
                if slave_ip == ipaddr:
                    with open(SlavexContainer.CURRENT_SLAVE, "w") as current_slave_fd:
                        current_slave_fd.write("{} {}\n".format(cname, ipaddr))
                        found_it = True
        if found_it is False:
            print("Bad slave IP [{}], no match one, please check slave list".format(slave_ip))
            sys.exit(-1)

        self.login_slave()

master = MasterContainer()
slavex = SlavexContainer()
