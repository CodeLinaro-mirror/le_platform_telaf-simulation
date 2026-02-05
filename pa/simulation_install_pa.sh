#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -Eeuo pipefail

. "$(dirname "${BASH_SOURCE[0]}")/common.sh"

TARGET=simulation

umask 022
: "${OBJCOPY:=objcopy}"
: "${STRIP:=strip}"
CP_FLAGS=( -a --no-preserve=ownership --force )
NO_STRIP=0

PA_ROOT_DIR=$(dirname "$(readlink -f "$0")")

cd ${PA_ROOT_DIR}

OUTPUT="${LEGATO_ROOT}/build/${TARGET}"
STAGE_DIR_COMBINED="${LEGATO_ROOT}/build/${TARGET}/staging_combined"
RUNTIME_LOC="$STAGE_DIR_COMBINED/systems/current/lib"

info "[simulation]: reseting installation dir ${STAGE_DIR_COMBINED}"
rm -rf ${STAGE_DIR_COMBINED}
mkdir -p ${STAGE_DIR_COMBINED}

TARGET_BASIC_DIR=${LEGATO_ROOT}/build/${TARGET}/_staging_system.${TARGET}.update_ro

if [[ -d "$TARGET_BASIC_DIR" ]]; then
    info "Syncing base filesystem from $TARGET_BASIC_DIR"
    run cp -a "$TARGET_BASIC_DIR/." "$STAGE_DIR_COMBINED/"
else
    die "No base ${TARGET_BASIC_DIR} found, starting from empty?"
fi

install_libs_to_runtime()
{
    local module="$1" dir="$2" dst="$STAGE_DIR_COMBINED/systems/current/lib"

    if [[ ! -d "$dir" ]]; then
        info "[$module] skip: source dir not found -> $dir"
        return
    fi

    ensure_dir "$dst"
    info "[$module] install: $dir -> $dst"

    local count=0
    while IFS= read -r -d '' so; do
        local base="$(basename "$so")"
        local target="$dst/$base"

        if [[ -e "$target" || -L "$target" ]]; then
            info "[$module] skip: already exists -> $target"
            continue
        fi

        if [[ ! -L "$so" ]]; then
            if [[ -n "$OBJCOPY" && -n "$OUTPUT" ]]; then
                ensure_dir "$OUTPUT"
                local dbgfile="$OUTPUT/${base}.debug"
                info "[$module] keep debug: $dbgfile"
                run "$OBJCOPY" --only-keep-debug "$so" "$dbgfile"
                (( ! NO_STRIP )) && run "$STRIP" --strip-unneeded "$so"
            else
                (( ! NO_STRIP )) && run "$STRIP" --strip-unneeded "$so"
            fi
        fi

        run cp "${CP_FLAGS[@]}" "$so" "$dst/"
        ((count++))
    done < <(find "$dir" -maxdepth 2 \( -type f -o -type l \) -name '*.so*' -print0) || true

    if (( count == 0 )); then
        info "[$module] no files: no matching files in $dir"
    else
        info "[$module] done: installed $count file(s)"
    fi
}

# Install the generated to the combined directory
install_libs_to_runtime "SIMULA_PA"   telaf-pa-simula/staging
install_libs_to_runtime "DEFAULT_PA"  telaf-pa-default/staging
