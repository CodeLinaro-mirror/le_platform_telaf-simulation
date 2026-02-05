#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import json, argparse
import sys, os

def doit(args):
    if not os.path.exists(args.conf_file):
        print("Bad configuration file you gave, check it.")
        return -1

    with open(args.conf_file) as fd:
        root = json.load(fd)
        if args.master_opt is True:
            print("{}".format(root["master_tarball"]), end="")
        elif args.slavex_opt is True:
            print("{}".format(root["slavex_tarball"]), end="")
        else:
            return -2
        return 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="A tool to parse the simulation configuration file")
    parser.add_argument("conf_file", help="location of the simulation configuration file")
    parser.add_argument("--master", dest="master_opt", default=False, action="store_true", help="get the master tarbal name of the telaf image")
    parser.add_argument("--slavex", dest="slavex_opt", default=False, action="store_true", help="get the slave-x tarball name of the telaf image")
    args = parser.parse_args()

    sys.exit(doit(args))
