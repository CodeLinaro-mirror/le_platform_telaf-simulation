#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -e

SCRIPT_PATH=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT_PATH")
. "${ROOT_DIR}/../common.sh"

INSTALL_DIR="${1:-${ROOT_DIR}/staging}"
BUILD_DIR="${ROOT_DIR}/build"
STATE_FILE="${BUILD_DIR}/.simulation-pa-build"

if [[ -n "${TELAF_ROOT:-}" ]]; then
    PA_SOURCE_DIR=$(readlink -f "${TELAF_ROOT}/../telaf-pa")
else
    PA_SOURCE_DIR=$(readlink -f "${ROOT_DIR}/../../../telaf-pa")
fi
DEPS_ROOTFS="${SIMULATION_DEPS_ROOTFS:-$(readlink -f "${ROOT_DIR}/../../deps/taf_rootfs")}"
SIMULA_DIR="${ROOT_DIR}/../telaf-pa-simula"
PA_INPUTS=(
    "$SCRIPT_PATH"
    "${ROOT_DIR}/../common.sh"
    "${ROOT_DIR}/CMakeLists.txt"
    "${PA_SOURCE_DIR}/CMakeLists.txt"
    "${PA_SOURCE_DIR}/component"
    "${PA_SOURCE_DIR}/include"
    "${SIMULA_DIR}/sdk-include"
    "${SIMULA_DIR}/staging/lib"
    "${DEPS_ROOTFS}/include"
    "${DEPS_ROOTFS}/lib"
)

if pa_build_is_current "$STATE_FILE" "$BUILD_DIR" "$INSTALL_DIR" "${PA_INPUTS[@]}"; then
    echo ">>> unchanged: skipping build/install for ${ROOT_DIR}"
    exit 0
fi

echo ">>> updating build: ${BUILD_DIR}"
rm -f "$STATE_FILE" "$BUILD_DIR/install_manifest.txt"
rm -rf "${INSTALL_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo ">>> Running CMake"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

make -j$(nproc)

echo ">>> Installing to ${INSTALL_DIR}"
mkdir -p "${INSTALL_DIR}"
make install

pa_manifest_outputs_exist "$BUILD_DIR/install_manifest.txt" "$INSTALL_DIR" \
    || die "Install manifest missing, incomplete, or outside ${INSTALL_DIR}"
pa_save_build_state "$STATE_FILE" "$INSTALL_DIR" "${PA_INPUTS[@]}"

echo ">>> Done, output: ${BUILD_DIR}, install: ${INSTALL_DIR}"
