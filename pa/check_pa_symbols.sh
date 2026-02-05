#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -Eeuo pipefail

. "$(dirname "${BASH_SOURCE[0]}")/common.sh"

IFS=$'\n\t'

PA_API_PREFIX_REGEX="${PA_API_PREFIX_REGEX:-^taf_pa_}"

extract_pa_api_sets() {
    local so="$1" out_set="$2" out_list="$3"
    [[ -f "$so" || -L "$so" ]] || { warn "[API-CHECK] skip missing so: $so"; : >"$out_set"; : >"$out_list"; return; }

    command -v readelf >/dev/null || die "cannot find readelf"
    if [[ ! -x /usr/bin/c++filt ]]; then
        warn "c++filt not found; C++ API demangle unavailable.."
    fi

    local tmp_syms tmp_dm
    tmp_syms="$(mktemp)" || die "[API-CHECK] mktemp failed"
    tmp_dm="$(mktemp)"   || die "[API-CHECK] mktemp failed"

    readelf -Ws "$so" \
        | awk '$4=="FUNC" && ($5=="GLOBAL" || $5=="WEAK") && $6=="DEFAULT" && $7!="UND" {print $8}' \
        | LC_ALL=C sort -u > "$tmp_syms"

    : > "$tmp_dm"
    while IFS= read -r sym; do
        local dm sig
        if [[ -x /usr/bin/c++filt ]]; then
            dm="$(/usr/bin/c++filt "$sym" 2>/dev/null || echo "$sym")"
        else
            dm="$sym"
        fi

        sig="$(awk '
        {
            s=$0
            while (match(s,/<[^<>]*>/)) {
                s = substr(s,1,RSTART-1) "" substr(s,RSTART+RLENGTH)
            }
            print s
        }
        ' <<< "$dm")"

        local func_name="${sig%%(*}"
        func_name="${func_name##*::}"

        if [[ -n "${PA_API_PREFIX_REGEX:-}" ]]; then
            [[ "$func_name" =~ $PA_API_PREFIX_REGEX ]] || continue
        else
            [[ "$func_name" =~ ^taf_pa_ ]] || continue
        fi

        printf '%s\n' "$sig" >> "$tmp_dm"
    done < "$tmp_syms"

    LC_ALL=C sort -u "$tmp_dm" > "$out_set"
    cp "$out_set" "$out_list"

    rm -f "$tmp_syms" "$tmp_dm"
}

verify_one_pair() {
    local strong="$1" weak="$2" base strong_set weak_set strong_dm weak_dm
    base="$(basename "$strong")"
    strong_set="$(mktemp)" || die "[API-CHECK] mktemp failed"
    weak_set="$(mktemp)"   || die "[API-CHECK] mktemp failed"
    strong_dm="$(mktemp)"  || die "[API-CHECK] mktemp failed"
    weak_dm="$(mktemp)"    || die "[API-CHECK] mktemp failed"

    info "[API-CHECK] Pair: strong=$strong"
    info "[API-CHECK]       weak  =$weak"

    extract_pa_api_sets "$strong" "$strong_set" "$strong_dm"
    extract_pa_api_sets "$weak"   "$weak_set"   "$weak_dm"

    info "[API-CHECK] Strong API list:"
    if [[ -s "$strong_dm" ]]; then
        sed 's/^/    /' "$strong_dm"
    else
        info "    (none)"
    fi

    info "[API-CHECK] Weak API list:"
    if [[ -s "$weak_dm" ]]; then
        sed 's/^/    /' "$weak_dm"
    else
        info "    (none)"
    fi

    local _strong_f _weak_f
    _strong_f="$(mktemp)" || die "[API-CHECK] mktemp failed"
    _weak_f="$(mktemp)"   || die "[API-CHECK] mktemp failed"

    cp "$strong_dm" "$_strong_f"
    cp "$weak_dm" "$_weak_f"

    local miss_in_weak extra_in_weak rc=0
    miss_in_weak="$(LC_ALL=C comm -23 "$_strong_f" "$_weak_f")"
    if [[ -n "$miss_in_weak" ]]; then
        rc=1
        warn "[API-CHECK] Strong-only API:"
        printf '%s\n' "$miss_in_weak" | sed 's/^/    /'
    fi

    extra_in_weak="$(LC_ALL=C comm -13 "$_strong_f" "$_weak_f")"
    if [[ -n "$extra_in_weak" ]]; then
        info "[API-CHECK] Weak-only API:"
        printf '%s\n' "$extra_in_weak" | sed 's/^/    /'
    fi

    rm -f "$_strong_f" "$_weak_f"
    return "$rc"
}

verify_pa_strong_vs_weak() {
    local strong_so weak_pattern weak_so strong_base prefix

    local TARGET_PA_BUILD_DIR DEFAULT_PA_BUILD_DIR
    TARGET_PA_BUILD_DIR=$1
    DEFAULT_PA_BUILD_DIR=$2

    [[ -d "$TARGET_PA_BUILD_DIR" ]] || { info "[API-CHECK] skip: no ${TARGET_PA_BUILD_DIR}"; return 0; }
    [[ -d "$DEFAULT_PA_BUILD_DIR" ]] || { info "[API-CHECK] skip: no ${DEFAULT_PA_BUILD_DIR}"; return 0; }

    local any_pair=0 rc_all=0
    while IFS= read -r -d '' strong_so; do
        any_pair=1
        strong_base="$(basename "$strong_so")"
        prefix="${strong_base%%.so*}"                     # e.g. libComponent_taf_pa_voicecall
        weak_pattern="${prefix}Def.so*"                   # e.g. libComponent_taf_pa_voicecallDef.so*
        weak_so="$(find "$DEFAULT_PA_BUILD_DIR" -type f -name "$weak_pattern" -print -quit)"
        if [[ -z "$weak_so" ]]; then
            die "[API-CHECK] missing weak library for strong: $strong_base (expect pattern: $weak_pattern under DEFAULT_PA_BUILD_DIR)"
        fi
        if ! verify_one_pair "$strong_so" "$weak_so"; then
            rc_all=1
        fi
    done < <(find "$TARGET_PA_BUILD_DIR" -maxdepth 2 -type f -name 'libComponent_taf_pa_*.so*' -print0)

    if (( any_pair == 0 )); then
        warn "[API-CHECK] no strong PA libraries found under TARGET_PA_BUILD_DIR=$TARGET_PA_BUILD_DIR"
    fi

    return "$rc_all"
}

verify_pa_strong_vs_weak $1 $2
