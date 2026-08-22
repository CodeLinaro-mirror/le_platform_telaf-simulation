#!/bin/bash
# Wrap mosquitto: chmod UDS listener socket world-writable right after bind.
#
# Why: mosquitto 2.x creates the UDS socket `srwxrwx--- mosquitto:mosquitto`
# with no config directive to widen the mode, and the Legato supervisor spawns
# PA apps (uid 1001 `telaf`) with a fixed supplementary-groups list from
# system.sdef -- adding telaf to /etc/group's mosquitto group is ignored.
# Simplest workaround inside the sim container: relax the socket mode from
# outside the broker.
#
# Background loop polls until the socket appears (broker binds in ~50ms),
# chmods it once, then exits. `exec` hands the process slot to mosquitto so
# supervisord tracks the correct pid.

SOCK=/tmp/simula-mqtt.sock

(
    for _ in $(seq 20); do
        if [ -S "$SOCK" ]; then
            chmod 0666 "$SOCK"
            exit 0
        fi
        sleep 0.05
    done
) &

exec /usr/sbin/mosquitto -c /root/sml/mosquitto.conf
