#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -e

ROOT_DIR=$(dirname "$(readlink -f "$0")")

INSTALL_DIR="${1:-${ROOT_DIR}/staging}"
BUILD_DIR="${ROOT_DIR}/build"

echo ">>> cleaning: ${BUILD_DIR}"
rm -rf "${INSTALL_DIR}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo ">>> Running CMake"
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}

# cmake --build .
make -j$(nproc)

echo ">>> Installing to ${INSTALL_DIR}"
# cmake --install .
make install

echo ">>> Done, output: ${BUILD_DIR}, install: ${INSTALL_DIR}"
