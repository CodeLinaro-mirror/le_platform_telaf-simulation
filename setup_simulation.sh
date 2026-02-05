#!/usr/bin/env bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# set -Eeo pipefail

TOP=$(dirname "$(readlink -f "$0")")

echo "Setup simulation project start"

function try_create_dir()
{
    RESULT=0
    if ! [ -e "$1" ]; then
        mkdir -p "$1"
        RESULT=$?
        echo "[Create] $1 dir (new)"
    else
        echo "[Prompt] $1 dir (exists)"
    fi
    return $RESULT
}

function try_clone()
{
    BNAME=${BNAME:="mhead"}

    if [ -e "$3" ] && [ -e "$3/.git" ]; then
        echo "[Prompt] $3 is ready (exists)"
        return $?
    fi

    echo -e "\n[Dwload] ($1) start..."
    git clone --branch $2 $1 $3

    cd $3
    git checkout -b $BNAME $2 > /dev/null 2>&1
    echo "[Switch] branch to track the latest commit-id"
    cd -  > /dev/null 2>&1

    echo -e "[Dwload] $1 done (new)"
}

function try_download_script()
{
    remote_raw_file_base="https://git.codelinaro.org/clo/le/platform/TelAF/-/raw/telaf.lnx.1.1/simulation/"

    if [ -n "$3" ]; then
        remote_raw_file_base="$3"
    fi

    wget_opts="-P $2 -N" # -q

    check_file="simulation_env/$(basename $1)"

    if [ -e "$check_file" ]; then

        if [ -n "$SIMULA_OVERWRITE_SETUP" ]; then
            rm -f $check_file
            echo "[Script] Downloading from $remote_raw_file_base/$1, after removed"
            wget "$remote_raw_file_base/$1" $wget_opts
            if [ $? -ne 0 ]; then
                echo "Err.. wget failed"
                exit 1
            fi
            echo "[Script] $check_file done (re-new)"
            echo
        else
            echo "[Script] $check_file (exists)"
            return
        fi
    else
        echo "[Script] Downloading from $remote_raw_file_base/$1"
        wget "$remote_raw_file_base/$1" $wget_opts
        if [ $? -ne 0 ]; then
            echo "Err.. wget failed"
            exit 1
        fi
        echo "[Script] $check_file done (new)"
        echo
    fi
}

try_create_dir "simulation_env"
try_create_dir "simulation_env/telaf"
try_create_dir "simulation_env/legato/legato-af"
try_create_dir "simulation_env/legato/3rdParty"
try_create_dir "simulation_env/legato/3rdParty/Kconfiglib"
try_create_dir "simulation_env/legato/3rdParty/jansson"
try_create_dir "simulation_env/sdk"
try_create_dir "simulation_env/telaf-pa"
try_create_dir "simulation_env/telaf-pa-default"

remote_server=

branch_mapping_conf="simulation_env/branch_mapping.conf"
echo "external" > simulation_env/FROM

if [ -e "./branch_mapping.x.conf" ]; then

    branch_mapping_conf="./branch_mapping.x.conf"
    echo "internal" > simulation_env/FROM

    if [ -e "./remote.server" ]; then
        if ! [ -s "./remote.server" ]; then
            echo "Err.. Empty [remote.server] configuration"
            exit 3
        fi

        read -r remote_server < "./remote.server"
        echo "[Rmtsvr] ${remote_server}"
        if [ -z "${remote_server}" ]; then
            echo "Err.. Bad [remote.server] configuration"
            exit 2
        fi
    else
        echo "Err.. From: internal, but not provide the [remote.server] for address."
        exit 1
    fi
fi

try_download_script setup/Makefile simulation_env/ $remote_server
try_download_script setup/helpme_setup.py simulation_env/ $remote_server
try_download_script setup/patch_me.json simulation_env/ $remote_server
try_download_script setup/branch_mapping.conf simulation_env/ $remote_server
echo "[Helper] prepare some scripts (override: export SIMULA_OVERWRITE_SETUP=y)"

OLD_IFS=$IFS
IFS="= "

while read -r line; do

    # Drop the empty line and comment
    if [ -z "$line" ] || [[ "$line" == \#* ]]; then
        continue
    fi

    read -r _to_dir _git _branch <<< "$line"

    # parse the configuraiton file and clone gits accordingly
    try_clone $_git $_branch $_to_dir

done < "$branch_mapping_conf"

IFS=$OLD_IFS

echo
echo "Now simulation projects are ready, maybe you need to do as follows:"
echo "   1. Please have a look at 'simulation_env/patch_me.json' and give some changes on demand"
echo "   2. Just run 'cd simulation_env && make help'"
echo

if ! command -v docker &> /dev/null; then
    echo '[docker] is not found, please install it first, refer to: [https://docs.docker.com/engine/install]'
fi

exit 0
