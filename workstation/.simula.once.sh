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
