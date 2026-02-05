#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import argparse
import subprocess
import sys
from .logger import L
from .. import __version__ as mversion
from ..utest import tafSmsUnitTest_helper as utest_sms_helper
from ..utest import tafSomeipGWTest_helper as utest_someip_helper
from ..utest import tafDataCallUnitTest_helper as utest_dcs_helper
from .container import master, slavex
from ..tool import replace as t_replace
from ..tool import diag_client

def precondition_test():
    result = subprocess.run("telaf status", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    if "TelAF framework is running" not in result.stdout:
        L.error("TelAF framework is NOT running, please start it first")
        sys.exit(-1)

def dispatch_subcmd_jobs(args):

    if args.subcommand == "utest":
        precondition_test()
        if args.which_utest == "someip":
            utest_someip_helper.to_run()
        elif args.which_utest == "sms":
            utest_sms_helper.to_run()
        elif args.which_utest == "dcs":
            utest_dcs_helper.to_run()

    elif args.subcommand == "slave":
        try:
            if args.shell is True:
                slavex.login_slave()
            elif args.list is True:
                slavex.list_slave()
            elif args.to is not None:
                slavex.change_def_and_login(args.to[0])
            elif args.remote is True:
                slavex.remote_do(" ".join(args.remote_todo))
        except FileNotFoundError:
            print("Oops, maybe you just run the master without any slave, right?")

    elif args.subcommand == "master":
        if args.shell is True:
            master.login_master()
        elif args.info is True:
            master.show_info()

    elif args.subcommand == "tool":
        if args.tools == "replace":
            print("FILE: {}\nFROM-PATTERN:{}\nTO-PATTERN:{}".format(
                args.rfile,args.from_pattern, args.to_pattern))
            t_replace.replace(args.rfile, args.from_pattern, args.to_pattern)
        if args.tools == "diag":
            diag_client.to_do(args.cfile, args.s_ip, args.s_logaddr)


def doit():
    parser = argparse.ArgumentParser(prog="simula", description="Simplify some simulation operations.")
    parser.add_argument("-v", "--version", action='version', version="%(prog)s " + mversion)

    subparsers = parser.add_subparsers(dest="subcommand", help="%(prog)s support some actions by sub-commands")

    subparser_slavex = subparsers.add_parser("slave", help="Support some operations for 'slave' nodes")
    subparser_slavex.add_argument("-s", "--shell", dest="shell", action='store_true', default=False,
                                  help="login remote slave container with ssh-client")
    subparser_slavex.add_argument("-l", "--list", dest="list", action='store_true', default=False,
                                  help="list all slave-x nodes and their information")
    subparser_slavex.add_argument("-t", "--to", dest="to", default=None, nargs=1,
                                  help="change current slave pointer to which you want and login")
    subparser_slavex.add_argument("-r", "--remote", dest="remote", action='store_true', default=False,
                                  help="execute commands on remote slave container and back")
    subparser_slavex.add_argument("remote_todo", nargs="*", help="any linux command, such as: telaf")

    subparser_master = subparsers.add_parser("master", help="Support some operation for the 'master' node")
    subparser_master.add_argument("-s", "--shell", dest="shell", action='store_true', default=False,
                                  help="login the master container from slave nodes")
    subparser_master.add_argument("-i", "--info", dest="info", action='store_true', default=False,
                                  help="show the master information for prompt")

    subparser_utest = subparsers.add_parser("utest", help="Run some helper scripts for unit test")
    subparser_utest.add_argument('which_utest', choices=('sms', 'someip', 'dcs'), help="Unit testcase list that supported")

    subparser_tool = subparsers.add_parser("tool", help="Provide some tools to simplify operations")
    subparser_tools = subparser_tool.add_subparsers(dest="tools", help="Collect some tools in here")

    sub_tool_replace = subparser_tools.add_parser("replace", help="Replace some contents in some files with in-place-mode")
    sub_tool_replace.add_argument("rfile", metavar='<file-to-replace>', help="which file do you want replace")
    sub_tool_replace.add_argument("from_pattern", metavar='<from-pattern>', help="original text pattern")
    sub_tool_replace.add_argument("to_pattern", metavar='<to-pattern>', help="new text pattern")

    sub_tool_diag = subparser_tools.add_parser("diag", help="As a diag client tool to do some test")
    sub_tool_diag.add_argument("-c", "--cfile", dest="cfile", metavar='<uds-doip-config-file>', help="a config-file for uds & doip")
    sub_tool_diag.add_argument("-p", "--server-ip", dest="s_ip", metavar='<server-ip-address>', help="doip server IP address")
    sub_tool_diag.add_argument("-a", "--server-logical-address", dest="s_logaddr", metavar='<server-logical-address>', help="uds server logical address")

    args = parser.parse_args()
    dispatch_subcmd_jobs(args)
