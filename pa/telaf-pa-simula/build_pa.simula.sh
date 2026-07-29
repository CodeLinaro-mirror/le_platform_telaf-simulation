#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -e

ROOT_DIR=$(dirname "$(readlink -f "$0")")

INSTALL_DIR="${1:-${ROOT_DIR}/staging}"
BUILD_DIR="${ROOT_DIR}/build"

# Source-built 3rd-party deps (openssl/curl/boost/dlt) live here. Put on
# CMAKE_PREFIX_PATH so find_library/find_path resolves them ahead of apt.
# libmosquitto comes from apt (libmosquitto-dev), NOT source -- port==0 UDS
# support is already present in the 2.0.11 stock package.
DEPS_ROOTFS="${SIMULATION_DEPS_ROOTFS:-$(readlink -f "${ROOT_DIR}/../../deps/taf_rootfs")}"

echo ">>> cleaning: ${BUILD_DIR}"
rm -rf "${INSTALL_DIR}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo ">>> Running CMake"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${DEPS_ROOTFS}" \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}

# cmake --build .
make -j$(nproc)

echo ">>> Installing to ${INSTALL_DIR}"
# cmake --install .
make install || true

echo ">>> Done, output: ${BUILD_DIR}, install: ${INSTALL_DIR}"
