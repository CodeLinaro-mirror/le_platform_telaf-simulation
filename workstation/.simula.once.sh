#!/usr/bin/env bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

MNGD_CONN_SVC_JSON=/legato/taf_rootfs/mngdConnectivity.json
if [ -e "$MNGD_CONN_SVC_JSON"  ]; then
    # Not overwrite the file that already exists.
    cp -n $MNGD_CONN_SVC_JSON /data/ManagedServices/
    # Set read and write permission
    chmod 666 /data/ManagedServices/mngdConnectivity.json
fi

# Try to export all environment variables for Common API
if [ -e /legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini ]; then
    export COMMONAPI_CONFIG=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/commonapi4someip.ini
    export VSOMEIP_CONFIGURATION=/legato/taf_rootfs/etc/vsomeip/E01HelloWorld/vsomeip-local.json
    export LD_LIBRARY_PATH=/legato/taf_rootfs/lib:$LD_LIBRARY_PATH
fi

# Mosquitto persistence directory (must exist before broker starts).
mkdir -p /tmp/mosquitto_persist/

# Simulation PA runtime config (log level / log file / chart::Spy mode).
# The PA reads the absolute path /etc/sml_pa.conf (hardcoded as kConfPath in
# pa/telaf-pa-simula/component/common/Log.cpp), but the editable source of
# truth lives in the volume-mounted /root/sml/ tree -- so install it here
# rather than baking it into the image. Copied (not symlinked) because the
# PA opens it with a plain ifstream very early in process start, before any
# of the mount juggling below.
#
# `cp -n` on purpose: never clobber a copy someone already hand-edited in
# /etc to reproduce an issue. Delete /etc/sml_pa.conf to re-seed from sml/.
if [ -e /root/sml/sml_pa.conf ]; then
    cp -n /root/sml/sml_pa.conf /etc/sml_pa.conf
fi

# Config lives in /root/sml/ (volume-mounted) — edit without rebuilding.
# Use supervisorctl -c /root/sml/supervisord.sml.conf <status|restart|stop>
supervisord -c /root/sml/supervisord.sml.conf

# Adjust DefaultCallbackDelay to resolve missing Dialling state in Ecall flow
ICALL_MANAGER_JSON=/data/telux/json/api/tel/ICallManagerSlot1.json
if [ -e "$ICALL_MANAGER_JSON"  ]; then
	if ! sed -i 's/"DefaultCallbackDelay"[[:space:]]*:[[:space:]]*[0-9]\+/"DefaultCallbackDelay" : 0/'  "$ICALL_MANAGER_JSON"; then
        echo "Warning: Failed to update DefaultCallbackDelay in $ICALL_MANAGER_JSON" >&2
    fi
fi
