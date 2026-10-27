#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -e

SCRIPT_PATH=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT_PATH")
. "${ROOT_DIR}/../common.sh"

INSTALL_DIR="${1:-${ROOT_DIR}/staging}"
BUILD_DIR="${ROOT_DIR}/build"
STATE_FILE="${BUILD_DIR}/.simulation-pa-build"

# Source-built 3rd-party deps (openssl/curl/boost/dlt) live here. Put on
# CMAKE_PREFIX_PATH so find_library/find_path resolves them ahead of apt.
# libmosquitto comes from apt (libmosquitto-dev), NOT source -- port==0 UDS
# support is already present in the 2.0.11 stock package.
DEPS_ROOTFS="${SIMULATION_DEPS_ROOTFS:-$(readlink -f "${ROOT_DIR}/../../deps/taf_rootfs")}"
SML_ROOT="${ROOT_DIR}/../../sml"
PA_INPUTS=(
    "$SCRIPT_PATH"
    "${ROOT_DIR}/../common.sh"
    "${ROOT_DIR}/CMakeLists.txt"
    "${ROOT_DIR}/component"
    "${ROOT_DIR}/include"
    "${ROOT_DIR}/sdk-include"
    "${SML_ROOT}/shared"
    "${SML_ROOT}/control"
    "${SML_ROOT}/config"
    "${SML_ROOT}/generated"
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
    -DCMAKE_PREFIX_PATH="${DEPS_ROOTFS}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"

# cmake --build .
make -j$(nproc)

echo ">>> Installing to ${INSTALL_DIR}"
mkdir -p "${INSTALL_DIR}"
# cmake --install .
make install

pa_manifest_outputs_exist "$BUILD_DIR/install_manifest.txt" "$INSTALL_DIR" \
    || die "Install manifest missing, incomplete, or outside ${INSTALL_DIR}"
pa_save_build_state "$STATE_FILE" "$INSTALL_DIR" "${PA_INPUTS[@]}"

echo ">>> Done, output: ${BUILD_DIR}, install: ${INSTALL_DIR}"
