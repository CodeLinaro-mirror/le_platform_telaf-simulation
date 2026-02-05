#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

current_dir=$(dirname "$0")
simulation_workstation=${current_dir}/../workstation

cd ${simulation_workstation}

telaf_img_name=$(python3 ${current_dir}/config_file_parser.py --master ${simulation_workstation}/simulation_configuration.json)

CONTAINER_NAME="telaf_simulation_runtime_${1}_s" \
CONTAINER_OPTIONS="-p 8022:22 --rm -d" \
IMG_NAME="telaf_simulation_runtime_${1}" IMG_VERSION="1.0.0" \
SIMULATION_TARBALL_NAME=$telaf_img_name \
/bin/bash ./up_simulation.sh slave
