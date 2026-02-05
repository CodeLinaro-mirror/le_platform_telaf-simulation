#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SDK_BIN_HOME=/legato/sdk_rootfs/bin
export PATH=$SDK_BIN_HOME:$SCRIPT_DIR:$PATH

alias simula="python3 $SCRIPT_DIR/main.py"

# Example: slave [list | to <address> | shell (empty)]
function simula_slave ()
{
    case "$1" in
    "help")
        python3 $SCRIPT_DIR/main.py slave --help
        ;;
    "list")
        python3 $SCRIPT_DIR/main.py slave --list
        ;;
    "to")
        python3 $SCRIPT_DIR/main.py slave --to $2
        ;;
    "shell" | "")
        python3 $SCRIPT_DIR/main.py slave --shell
        ;;
    *)
        echo "Unknown sub-command [$1], please check help page"
        ;;
    esac
}

alias slave="simula_slave"

# rslave <remote-commands>
alias rslave="python3 $SCRIPT_DIR/main.py slave --remote -- "

function simula_master ()
{
    case "$1" in
    "help")
        python3 $SCRIPT_DIR/main.py master --help
        ;;
    "info")
        python3 $SCRIPT_DIR/main.py master --info
        ;;
    "shell" | "")
        python3 $SCRIPT_DIR/main.py master --shell
        ;;
    *)
        echo "Unknown sub-command [$1], please check help page"
        ;;
    esac
}

# master [info | shell (empty)]
alias master="simula_master"
