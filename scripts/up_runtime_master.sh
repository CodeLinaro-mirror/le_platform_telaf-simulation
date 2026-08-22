#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

current_dir=$(dirname "$0")
simulation_workstation=${current_dir}/../workstation

cd ${simulation_workstation}

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found on host — required by config_file_parser.py"
    exit 1
fi

telaf_img_name=$(python3 ${current_dir}/config_file_parser.py --master ${simulation_workstation}/simulation_configuration.json)
if [ -z "$telaf_img_name" ]; then
    echo "config_file_parser.py returned empty tarball name, aborting."
    exit 1
fi

# INSTANCE tags a runtime container for multi-instance debugging (empty = legacy name).
INSTANCE_SUFFIX=${INSTANCE:+_$INSTANCE}
SSH_PORT=${SSH_PORT:-9022}

CONTAINER_NAME="telaf_simulation_runtime_${1}_m${INSTANCE_SUFFIX}" \
SSH_PORT="${SSH_PORT}" \
EX_DOCKER_OPTS="${EX_DOCKER_OPTS}" \
SIMULA_MODE="${SIMULA_MODE:-interactive}" \
INSTANCE="${INSTANCE}" \
IMG_NAME="telaf_simulation_runtime_${1}" IMG_VERSION="1.0.0" \
SIMULATION_TARBALL_NAME=$telaf_img_name \
/bin/bash ./up_simulation.sh master
