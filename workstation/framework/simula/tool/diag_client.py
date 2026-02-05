#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import configparser
import os, sys, re
import importlib.machinery as LibLoad

import doipclient
import udsoncan
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.services import DiagnosticSessionControl
from udsoncan.client import Client

def is_valid_ip(ip_str):
    ip_regex = r'^(\d{1,3}\.){3}\d{1,3}$'
    match = re.match(ip_regex, ip_str)
    if match:
        octets = ip_str.split('.')
        if all(0 <= int(octet) <= 255 for octet in octets):
            return True
    return False

def is_valid_hex(input_str):
    return input_str.startswith("0x") \
           and all(c in "0123456789abcdefABCDEF" for c in input_str[2:])

class DiagClient():

    default_diag_config_file = os.path.join(
                               os.path.dirname(os.path.abspath(__file__)),
                               "diag_client_default.ini")

    def __init__(self, conf_file= None,
                 server_ip_address = None,
                 server_physical_address = None):

        self.conf = configparser.ConfigParser()

        if conf_file is None:
            conf_file = DiagClient.default_diag_config_file

        if not os.path.exists(conf_file):
            print("Not found Diag Client config file (.ini): [{}]".format(conf_file))
            sys.exit(-1)

        self.conf.read(conf_file)

        if server_ip_address is None:
            self.s_ip = self.conf["client.doip"]["server-ip-address"]
        else:
            if not is_valid_ip(server_ip_address):
                print("Bad server ip address [{}]".format(server_ip_address))
                sys.exit(-1)
            else:
                self.s_ip = server_ip_address

        if server_physical_address is None:
            self.s_logaddr = int(self.conf["client.uds"]["server-physical-address"], 16)
        else:
            if not is_valid_hex(server_physical_address):
                print("Bad format for server physical address, should HEX, such as: 0x0E00, not [{}]"
                    .format(server_physical_address))
                sys.exit(-1)
            else:
                self.s_logaddr = int(server_physical_address, 16)

    def do_connect(self):
        self.doip_client = doipclient.DoIPClient(self.s_ip, self.s_logaddr)
        uds_connection = DoIPClientUDSConnector(self.doip_client)
        config = dict(udsoncan.configs.default_client_config)
        self.uds_client = Client(uds_connection, config=config)

    def doip_request_diagnostic_power_mode(self):
        return self.doip_client.request_diagnostic_power_mode()

    def doip_request_alive_check(self):
        return self.doip_client.request_alive_check()

    def _uds_change_session(self, which_session):
        return self.uds_client.change_session(which_session)

    def uds_change_to_extended_session(self):
        return self._uds_change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

    def uds_change_to_default_session(self):
        return self._uds_change_session(DiagnosticSessionControl.Session.defaultSession)

    def uds_change_to_programming_session(self):
        return self._uds_change_session(DiagnosticSessionControl.Session.programmingSession)

def minimum_test():
    diag = DiagClient(cfile, s_ip, s_logaddr)
    diag.do_connect()
    print(diag.doip_request_alive_check())
    print(diag.uds_change_to_default_session())

def to_do(cfile, s_ip, s_logaddr):

    path_to_testcase = os.path.join("/root/simulation/diag_test_38_36_37.py")
    if os.path.exists(path_to_testcase):
        loader = LibLoad.SourceFileLoader("diag_testcase", path_to_testcase)
        module = loader.load_module()
        module.testcases(cfile, s_ip, s_logaddr)
    else:
        minimum_test(cfile, s_ip, s_logaddr)
