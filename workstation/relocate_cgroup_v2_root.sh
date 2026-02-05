#!/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -eu

cd /sys/fs/cgroup

# Check the cgroup.type
if [ ! -f cgroup.type ]; then
    echo "cgroup.type file not found, cannot determine cgroup type"
    exit 1
fi

CG_TYPE=$(cat cgroup.type | tr -d '\n')

if [ "${CG_TYPE}" == "threaded" ]; then
    echo "Now just support [domain, domain threaded], except [threaded]"
    exit 1
fi

CGROOT=__cgroot__

mkdir -p ${CGROOT} || { echo "Failed to create ${CGROOT} directory"; exit 1; }

# Make the processes snapshot
mapfile -t PIDS < cgroup.procs

# Record the PID of the current script process
SELF=$$

# Move except myself
for pid in "${PIDS[@]}"; do
    # Skip empty line and prevent to write twice
    [[ -n "${pid:-}" ]] || continue

    # Only filter PID (number)
    [[ "$pid" =~ ^[0-9]+$ ]] || continue

    # Skip itself
    if [[ "$pid" -eq "$SELF" ]]; then
        continue
    fi

    # Already exit ? check once
    [[ -e "/proc/$pid" ]] || continue

    # Final action... (even FAILURE, true for continue... )
    echo "$pid" > ${CGROOT}/cgroup.procs 2>/dev/null || true
done

# Move myself
if grep -qw "$SELF" cgroup.procs 2>/dev/null; then
    echo "$SELF" > ${CGROOT}/cgroup.procs 2>/dev/null || true
fi

# Check round 5 to clean all
round=0
while [[ -s cgroup.procs && $round -lt 5 ]]; do
    mapfile -t PIDS < cgroup.procs
    for pid in "${PIDS[@]}"; do
        [[ -e "/proc/$pid" ]] || continue
        echo "$pid" > ${CGROOT}/cgroup.procs 2>/dev/null || true
    done
    round=$((round+1))
done

if [[ -s cgroup.procs ]]; then
    echo "[CGROUP V2]: Relocate cgroup-root [NOK], remains:"
    cat cgroup.procs
    echo

    exit 1
else
    # Empty
    echo -e "[CGROUP V2]: Relocate cgroup-root [OK]\n"
fi

# Export the control
for c in $(cat cgroup.controllers); do
  echo +$c > cgroup.subtree_control || { echo "Failed to enable controller $c"; exit 1; }
done

# Inherit the parent's control
mkdir telaf || { echo "Failed to create telaf directory"; exit 1; }

cd telaf/

for c in $(cat cgroup.controllers); do
  echo +$c > cgroup.subtree_control
done

exit 0
