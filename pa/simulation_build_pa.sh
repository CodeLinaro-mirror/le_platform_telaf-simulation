#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -Eeuo pipefail

. "$(dirname "${BASH_SOURCE[0]}")/common.sh"

TARGET=simulation

umask 022

PA_ROOT_DIR=$(dirname "$(readlink -f "$0")")

cd ${PA_ROOT_DIR}

# Try to compile the valid PA one by one
info "[simulation]: building PA layers"
for PA in telaf-pa-default telaf-pa-simula telaf-pa-target
do
    bash ${PA}/build_pa.simula.sh
done

# Back to the root directory and prompt
cd ${PA_ROOT_DIR} && info "[simulation]: PA compilation [OK]"

# checking symbols
info "[simulation]: checking default & real-target PA ..."
bash ./check_pa_symbols.sh telaf-pa-simula/staging telaf-pa-default/staging

cd ${PA_ROOT_DIR} && info "[simulation]: symbol checking done"
