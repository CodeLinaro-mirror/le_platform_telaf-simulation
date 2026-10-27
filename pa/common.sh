#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

log()
{
    local file="${BASH_SOURCE[2]##*/}"
    local line="${BASH_LINENO[1]}"
    printf '[%s:%s] %s\n' "$file" "$line" "$*" >&2
}

info() { log "INFO: $*"; }
warn() { log "WARN: $*"; }
die()  { log "ERROR: $*"; exit 1; }

on_error()
{
    local rc=$?
    local cmd="${BASH_COMMAND}"
    local src="${BASH_SOURCE[1]:-unknown}"
    local line="${BASH_LINENO[0]:-0}"

    if [ "$rc" -ne 0 ]; then
        printf '[%(%F %T)T] ERROR: rc=%d, cmd=%s, at %s:%s\n' -1 "$rc" "$cmd" "$src" "$line" >&2
    fi
    return "$rc"
}

run()
{
    local _out _err rc
    _out="$(mktemp)" || { echo "[run] mktemp failed" >&2; return 1; }
    _err="$(mktemp)" || { echo "[run] mktemp failed" >&2; rm -f "$_out"; return 1; }

    set +e
    "$@" 1>"$_out" 2>"$_err"
    rc=$?
    set -e

    if [ -s "$_out" ]; then cat "$_out"; fi
    if [ -s "$_err" ]; then cat "$_err" >&2; fi

    rm -f "$_out" "$_err"

    if (( rc != 0 )); then
        echo "[FAIL:$rc] $*" >&2
    fi
    return $rc
}

ensure_dir()
{
    run install -d -m 0755 "$1";
}

# Keep CMake's incremental build tree, but invalidate it when an input is
# added, removed, or modified.  Timestamps are enough here: CMake uses the
# same signal for its dependency checks and hashing the whole source tree
# would erase the time saved by skipping the build.
pa_input_signature()
{
    local path normalized
    for path in "$@"; do
        if normalized=$(readlink -f "$path" 2>/dev/null); then
            :
        else
            normalized="$path"
        fi
        if [[ -d "$normalized" ]]; then
            find "$normalized" -xdev \( -type f -o -type l \) -printf '%T@ %s %p\n'
        elif [[ -e "$normalized" || -L "$normalized" ]]; then
            stat --format='%y %s %n' "$normalized"
        else
            printf 'missing %s\n' "$normalized"
        fi
    done | LC_ALL=C sort | sha256sum | awk '{print $1}'
}

pa_manifest_outputs_exist()
{
    local manifest="$1" install_dir="${2:-}" output
    [[ -s "$manifest" ]] || return 1
    if [[ -n "$install_dir" ]]; then
        install_dir=$(readlink -m "$install_dir") || return 1
    fi
    while IFS= read -r output || [[ -n "$output" ]]; do
        [[ -e "$output" || -L "$output" ]] || return 1
        if [[ -n "$install_dir" ]]; then
            output=$(readlink -m "$output") || return 1
            [[ "$output" == "${install_dir%/}/"* ]] || return 1
        fi
    done < "$manifest"
}

pa_build_is_current()
{
    local stamp="$1" build_dir="$2" install_dir="$3"
    local saved_install saved_signature
    shift 3

    [[ -s "$stamp" ]] || return 1
    [[ -f "$build_dir/CMakeCache.txt" ]] || return 1
    pa_manifest_outputs_exist "$build_dir/install_manifest.txt" "$install_dir" || return 1

    saved_install="$(sed -n '1p' "$stamp")"
    saved_signature="$(sed -n '2p' "$stamp")"
    [[ "$saved_install" == "$install_dir" ]] || return 1
    [[ "$saved_signature" == "$(pa_input_signature "$@")" ]]
}

pa_save_build_state()
{
    local stamp="$1" install_dir="$2" tmp
    shift 2
    tmp="${stamp}.$$"
    printf '%s\n%s\n' "$install_dir" "$(pa_input_signature "$@")" > "$tmp"
    mv -f "$tmp" "$stamp"
}

pa_signature_is_current()
{
    local stamp="$1" manifest="$2" saved_signature
    shift 2

    [[ -s "$stamp" ]] || return 1
    pa_manifest_outputs_exist "$manifest" || return 1
    saved_signature="$(sed -n '1p' "$stamp")"
    [[ "$saved_signature" == "$(pa_input_signature "$@")" ]]
}

pa_save_signature()
{
    local stamp="$1" tmp
    shift
    tmp="${stamp}.$$"
    pa_input_signature "$@" > "$tmp"
    mv -f "$tmp" "$stamp"
}

set -o errtrace
trap on_error ERR
trap 'rc=$?; [ $rc -ne 0 ] && on_error; exit $rc' EXIT
