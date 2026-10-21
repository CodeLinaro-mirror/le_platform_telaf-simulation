# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -e

echo "CURDIR"
echo "$(pwd)"

echo "TELAF_ROOT=${TELAF_ROOT}"
echo "SIMULATION_HOST_XTOOLS=${SIMULATION_HOST_XTOOLS}"
echo "SIMULATION_DEPS_ROOTFS=${SIMULATION_DEPS_ROOTFS}"
echo "TELAF_SIMULATION_ENABLE_IVSS_DATA=${TELAF_SIMULATION_ENABLE_IVSS_DATA}"
echo "TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY=${TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY}"

COMMONAPI_CORE_GEN="${SIMULATION_HOST_XTOOLS}/bin/commonapi_core_generator/commonapi-core-generator-linux-x86_64"
COMMONAPI_SOMEIP_GEN="${SIMULATION_HOST_XTOOLS}/bin/commonapi_someip_generator/commonapi-someip-generator-linux-x86_64"
FIDL_DIR_MNGDCONN="${TELAF_ROOT}/apps/ivss/fidl/MngdConnSvc"
FIDL_DIR_RADIO="${TELAF_ROOT}/apps/ivss/fidl/RadioSvc"
FIDL_DIR_SIM="${TELAF_ROOT}/apps/ivss/fidl/SimSvc"
FIDL_DIR_INFO="${TELAF_ROOT}/apps/ivss/fidl/InfoSvc"

if [ ! -x "$COMMONAPI_CORE_GEN" ]; then
    echo "ERROR: CommonAPI core generator not found or not executable: $COMMONAPI_CORE_GEN"
    exit 1
fi

if [ ! -x "$COMMONAPI_SOMEIP_GEN" ]; then
    echo "ERROR: CommonAPI SOME/IP generator not found or not executable: $COMMONAPI_SOMEIP_GEN"
    exit 1
fi

if [ ! -f "$FIDL_DIR_MNGDCONN/MngdConnSvc.fidl" ] || [ ! -f "$FIDL_DIR_MNGDCONN/MngdConnSvc.fdepl" ]; then
    echo "ERROR: MngdConnSvc FIDL/FDEPL files not found under: $FIDL_DIR_MNGDCONN"
    exit 1
fi

if [ ! -f "$FIDL_DIR_RADIO/RadioSvc.fidl" ] || [ ! -f "$FIDL_DIR_RADIO/RadioSvc.fdepl" ]; then
    echo "ERROR: RadioSvc FIDL/FDEPL files not found under: $FIDL_DIR_RADIO"
    exit 1
fi

if [ ! -f "$FIDL_DIR_SIM/SimSvc.fidl" ] || [ ! -f "$FIDL_DIR_SIM/SimSvc.fdepl" ]; then
    echo "ERROR: SimSvc FIDL/FDEPL files not found under: $FIDL_DIR_SIM"
    exit 1
fi

if [ ! -f "$FIDL_DIR_INFO/InfoSvc.fidl" ] || [ ! -f "$FIDL_DIR_INFO/InfoSvc.fdepl" ]; then
    echo "ERROR: InfoSvc FIDL/FDEPL files not found under: $FIDL_DIR_INFO"
    exit 1
fi

# MngdConnSvc
"$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_MNGDCONN/MngdConnSvc.fidl"
"$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_MNGDCONN/MngdConnSvc.fdepl"
"$COMMONAPI_SOMEIP_GEN" "$FIDL_DIR_MNGDCONN/MngdConnSvc.fdepl"

# RadioSvc, SimSvc, InfoSvc (telephony-only services)
if [ "$TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY" = "y" ]; then
    # RadioSvc
    "$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_RADIO/RadioSvc.fidl"
    "$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_RADIO/RadioSvc.fdepl"
    "$COMMONAPI_SOMEIP_GEN" "$FIDL_DIR_RADIO/RadioSvc.fdepl"

    # SimSvc
    "$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_SIM/SimSvc.fidl"
    "$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_SIM/SimSvc.fdepl"
    "$COMMONAPI_SOMEIP_GEN" "$FIDL_DIR_SIM/SimSvc.fdepl"

    # InfoSvc
    "$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_INFO/InfoSvc.fidl"
    "$COMMONAPI_CORE_GEN" -sk "$FIDL_DIR_INFO/InfoSvc.fdepl"
    "$COMMONAPI_SOMEIP_GEN" "$FIDL_DIR_INFO/InfoSvc.fdepl"
fi

rm -rf build
mkdir -p build

if [ -z "${SIMULATION_DEPS_ROOTFS}" ]; then
    echo "ERROR: SIMULATION_DEPS_ROOTFS is not set. Please export it before running this script."
    exit 1
fi

cd build
cmake -DCMAKE_INSTALL_PREFIX=../install-soa/ \
      -DSIMULATION_DEPS_ROOTFS="${SIMULATION_DEPS_ROOTFS}" \
      -DTELAF_SIMULATION_ENABLE_IVSS_DATA="${TELAF_SIMULATION_ENABLE_IVSS_DATA}" \
      -DTELAF_SIMULATION_ENABLE_IVSS_TELEPHONY="${TELAF_SIMULATION_ENABLE_IVSS_TELEPHONY}" \
      ..
make
make install

INSTALL_SOA_DIR="../install-soa"
INSTALL_SOA_BIN="${INSTALL_SOA_DIR}/IvssClientTest"
WORKSTATION_INSTALL_SOA_DIR="${SIMULATION_HOME}/workstation/install-soa"

if [ ! -d "${INSTALL_SOA_DIR}" ]; then
    echo "ERROR: Generated install-soa folder not found: ${INSTALL_SOA_DIR}"
    exit 1
fi

if [ ! -f "${INSTALL_SOA_BIN}" ]; then
    echo "ERROR: Generated IvssClientTest binary not found: ${INSTALL_SOA_BIN}"
    exit 1
fi

echo "Setting full permissions on ${INSTALL_SOA_BIN}"
if ! chmod 777 "${INSTALL_SOA_BIN}"; then
    echo "ERROR: Failed to chmod 777 ${INSTALL_SOA_BIN}"
    exit 1
fi

echo "Overriding workstation install-soa folder: ${WORKSTATION_INSTALL_SOA_DIR}"
if ! rm -rf "${WORKSTATION_INSTALL_SOA_DIR}"; then
    echo "ERROR: Failed to remove existing folder: ${WORKSTATION_INSTALL_SOA_DIR}"
    exit 1
fi

if ! cp -a "${INSTALL_SOA_DIR}" "${SIMULATION_HOME}/workstation/"; then
    echo "ERROR: Failed to copy ${INSTALL_SOA_DIR} to ${SIMULATION_HOME}/workstation/"
    exit 1
fi

echo "Copied ${INSTALL_SOA_DIR} to ${WORKSTATION_INSTALL_SOA_DIR}"
