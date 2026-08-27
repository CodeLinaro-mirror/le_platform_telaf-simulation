#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Standalone debug build for the telaf-np sublayers only.
#
# This is a convenience for iterating on component/telaf-np/ without rebuilding
# the whole telaf-pa-simula tree. It is deliberately NOT called from
# pa/simulation_build_pa.sh -- the shipping path builds telaf-np as a
# subdirectory of telaf-pa-simula (see that CMakeLists' MODULE_LIST), so this
# script must never become a second source of staged artifacts.
#
# Prerequisite: telaf-pa-simula must already have been built once, because
# taf_prop_pa_pms links against libtelux_common from its staging/lib.

set -e

ROOT_DIR=$(dirname "$(readlink -f "$0")")
SIMULA_DIR=$(readlink -f "${ROOT_DIR}/../..")

INSTALL_DIR="${1:-${ROOT_DIR}/staging}"
BUILD_DIR="${ROOT_DIR}/build"

# Same source-built 3rd-party deps root as build_pa.simula.sh.
DEPS_ROOTFS="${SIMULATION_DEPS_ROOTFS:-$(readlink -f "${SIMULA_DIR}/../../deps/taf_rootfs")}"

SIMULA_STAGING_LIB="${SIMULA_DIR}/staging/lib"
if [ ! -e "${SIMULA_STAGING_LIB}/libtelux_common.so" ] &&
   [ ! -e "${SIMULA_STAGING_LIB}/libtelux_common.so.1" ]; then
    echo "!!! ${SIMULA_STAGING_LIB} has no libtelux_common."
    echo "!!! Build the parent first: bash ${SIMULA_DIR}/build_pa.simula.sh"
    exit 1
fi

echo ">>> cleaning: ${BUILD_DIR}"
rm -rf "${INSTALL_DIR}"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo ">>> Running CMake (telaf-np only)"
cmake "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH="${DEPS_ROOTFS}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DTELAF_NP_STANDALONE=ON \
    -DTELAF_NP_SIMULA_LIB_DIR="${SIMULA_STAGING_LIB}"

make -j"$(nproc)"

echo ">>> Installing to ${INSTALL_DIR}"
make install || true

echo ">>> Done, output: ${BUILD_DIR}, install: ${INSTALL_DIR}"
