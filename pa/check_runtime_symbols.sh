#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#
# Runtime undefined-symbol check: does every taf_* symbol a deployed library
# needs actually have a definition somewhere in the deployed system?
#
# This covers the half that simulation_install_pa.sh's verify_dt_needed cannot
# see. That one runs at *install* time over the staging tree and matches whole
# libraries (DT_NEEDED sonames). This one runs over a *deployed* tree and
# matches individual symbols -- which is where the ns -> prop prefix rename
# actually bites:
#
#   Failed to load library 'libComponent_tafPMSvc.so'
#     (.../libComponent_taf_ns_common.so: undefined symbol: taf_prop_common_LogSetlevel)
#
# That library was a leftover from a previous install: its *filename* still said
# ns while its symbol references had already moved to prop, and nothing in the
# deployed tree defined them. The staging tree was clean, so no build-time check
# could have known. Only the deployed tree shows it.
#
# Why it is fatal rather than merely wrong: Legato's generated LoadLib() calls
# dlopen(RTLD_LAZY) but then does LE_FATAL_IF(dlerror() != NULL) -- see
# legato-af/framework/tools/mkTools/codeGenerator/exeMainGenerator.cpp:127-132.
# RTLD_LAZY defers *function* symbols, but an undefined *data* symbol in a
# dependency must resolve at load time, so dlerror() is non-NULL and every
# service dies in LoadLib() and crash-loops under the supervisor. There is no
# latent-failure mode here: it either boots or nothing boots.
#
# Usage:
#   ./check_runtime_symbols.sh                     # default: deployed /legato
#   ./check_runtime_symbols.sh <system_root>       # e.g. a staging_combined/systems/current
#
# <system_root> is the directory holding lib/ and appsWriteable/. Both are
# scanned: PA/framework libs live in lib/, service libs in appsWriteable/*/lib/.
#
# Exit 0 = every taf_* reference resolves. Exit 1 = at least one does not.

set -Eeuo pipefail

. "$(dirname "${BASH_SOURCE[0]}")/common.sh"

: "${NM:=nm}"

# Which undefined symbols we hold ourselves responsible for. libc/libstdc++
# symbols resolve from the runtime host's own libraries, which may legitimately
# not be part of the tree being scanned.
SYM_REGEX="${SYM_REGEX:-^taf_}"

SYSTEM_ROOT="${1:-/legato/systems/current}"

check_runtime_symbols()
{
    local root="$1"

    command -v "$NM" >/dev/null || die "cannot find $NM"
    [[ -d "$root" ]] || die "system root not found: $root (on a build host, pass a staging_combined/systems/current instead)"

    local -a libdirs=()
    [[ -d "$root/lib" ]] && libdirs+=("$root/lib")
    # Service libs are per-app; this is where a tafXxxSvc that fails LoadLib()
    # actually lives.
    [[ -d "$root/appsWriteable" ]] && libdirs+=("$root/appsWriteable")
    (( ${#libdirs[@]} )) || die "neither lib/ nor appsWriteable/ under $root"

    local -a sos=()
    local so
    while IFS= read -r -d '' so; do
        # Symlinks point at a real file already in the list.
        [[ -L "$so" ]] || sos+=("$so")
    done < <(find "${libdirs[@]}" \( -type f -o -type l \) -name '*.so*' -print0 2>/dev/null)

    (( ${#sos[@]} )) || die "no shared libraries found under ${libdirs[*]}"

    # Definition index over the whole deployed tree. A symbol counts as defined
    # if ANY library defines it: at runtime these are all loaded into one
    # process with RTLD_GLOBAL, so that is the real resolution scope.
    local -A defined=()
    local sym
    for so in "${sos[@]}"; do
        while read -r sym; do
            [[ -n "$sym" ]] && defined["$sym"]=1
        done < <("$NM" -D --defined-only "$so" 2>/dev/null \
                 | awk '{print $NF}' | grep -E "$SYM_REGEX" || true)
    done

    info "[SYM-CHECK] ${#sos[@]} libs under $root, ${#defined[@]} distinct ${SYM_REGEX} definitions"

    local -a bad=()
    local base
    for so in "${sos[@]}"; do
        base="${so#$root/}"
        while read -r sym; do
            [[ -n "$sym" ]] || continue
            [[ -n "${defined[$sym]:-}" ]] && continue
            bad+=("$base -> $sym")
        done < <("$NM" -D -u "$so" 2>/dev/null \
                 | awk '{print $NF}' | grep -E "$SYM_REGEX" || true)
    done

    if (( ${#bad[@]} )); then
        local e
        for e in "${bad[@]}"; do
            warn "[SYM-CHECK] UNDEFINED: $e"
        done
        # Name the likely cause: a filename whose prefix disagrees with the
        # prefix of the symbols it wants is the ns/prop leftover signature.
        warn "[SYM-CHECK] If a lib's name says taf_ns_* but it references taf_prop_*" \
             "(or vice versa), it is a leftover from an install made before the" \
             "prefix rename -- delete it from the deployed tree and reinstall." \
             "See .claude/md/debug/ns-prop-prefix.md."
        die "[SYM-CHECK] ${#bad[@]} unresolved ${SYM_REGEX} reference(s) in $root"
    fi

    info "[SYM-CHECK] all ${SYM_REGEX} references resolve in $root"
}

check_runtime_symbols "$SYSTEM_ROOT"
