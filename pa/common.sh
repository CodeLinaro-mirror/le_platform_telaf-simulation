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

set -o errtrace
trap on_error ERR
trap 'rc=$?; [ $rc -ne 0 ] && on_error; exit $rc' EXIT
