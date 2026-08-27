#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -Eeuo pipefail

. "$(dirname "${BASH_SOURCE[0]}")/common.sh"

TARGET=simulation

umask 022
: "${OBJCOPY:=objcopy}"
: "${STRIP:=strip}"
: "${READELF:=readelf}"
CP_FLAGS=( -a --no-preserve=ownership --force )
NO_STRIP=0

# soname -> which module actually staged it / which ones were shadowed by it.
# Filled by install_libs_to_runtime, reported by verify_dt_needed.
declare -A SONAME_WINNER=()
declare -A SONAME_LOSERS=()

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
            # Record the loser so verify_dt_needed can name who was shadowed.
            # Two trees shipping one soname is legal (that is what the
            # SIMULA-first order is for), but it must never be a surprise.
            SONAME_LOSERS["$base"]+="$module "
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
        SONAME_WINNER["$base"]="$module"
        ((count++))
    done < <(find "$dir" -maxdepth 2 \( -type f -o -type l \) -name '*.so*' -print0) || true

    if (( count == 0 )); then
        info "[$module] no files: no matching files in $dir"
    else
        info "[$module] done: installed $count file(s)"
    fi
}

# ---------------------------------------------------------------------------
# DT_NEEDED resolution check.
#
# Exists because of a failure that produced no error at all. telaf-pa 8bfff04
# renamed the sublayer ABI taf_ns_ -> taf_prop_, which re-pointed the strong
# libComponent_taf_pa_pms.so's DT_NEEDED at a *different provider* -- and the
# build stayed silent, because the new soname happened to resolve too. Nothing
# was unsatisfied, so nothing complained; which sublayer the PA actually loads
# had silently become a function of staging order. check_pa_symbols.sh cannot
# see this: its PA_API_PREFIX_REGEX=^taf_pa_ only diffs the strong/weak facade
# APIs, and no step validated where a NEEDED entry landed.
#
# So: every NEEDED of every staged library must have a provider. A missing
# PA-owned soname (libComponent_*/libtelux_*) is fatal -- we ship those, so an
# unresolved one is our bug and would be a dlopen/exec failure at runtime.
# Everything else (libc, libstdc++, libdlt, libmosquitto...) only warns: those
# come from the rootfs/ldconfig of the *runtime* host, which is not necessarily
# this build host.
verify_dt_needed()
{
    local dst="$RUNTIME_LOC"

    command -v "$READELF" >/dev/null || die "verify_dt_needed: cannot find $READELF"

    local -A provided=()
    local f
    while IFS= read -r -d '' f; do
        provided["$(basename "$f")"]=1
    done < <(find "$dst" -maxdepth 1 \( -type f -o -type l \) -name '*.so*' -print0)

    info "[verify] DT_NEEDED check over ${#provided[@]} staged libs in $dst"

    local -a fatal=()
    local -A ext=()
    local so base need
    while IFS= read -r -d '' so; do
        # Symlinks resolve to a real file already covered by this walk.
        [[ -L "$so" ]] && continue
        base="$(basename "$so")"
        while read -r need; do
            [[ -n "$need" ]] || continue
            [[ -n "${provided[$need]:-}" ]] && continue
            if [[ "$need" == libComponent_* || "$need" == libtelux_* ]]; then
                fatal+=("$base -> $need")
            else
                # Deduped by soname: every lib NEEDs libc, and one warning per
                # edge would bury the fatal lines under ~100 lines of noise.
                ext["$need"]=1
            fi
        done < <("$READELF" -d "$so" 2>/dev/null \
                 | awk '/\(NEEDED\)/{gsub(/[][]/,"",$5); print $5}')
    done < <(find "$dst" -maxdepth 1 \( -type f -o -type l \) -name '*.so*' -print0)

    # Same soname from more than one tree: say who won, so a provider swap is
    # visible in the log instead of being inferred from a crash later.
    local b
    for b in "${!SONAME_LOSERS[@]}"; do
        info "[verify] soname '$b' provided by ${SONAME_WINNER[$b]:-<base rootfs>}" \
             "(shadowed: ${SONAME_LOSERS[$b]% })"
    done

    if (( ${#ext[@]} )); then
        # Not staged by us on purpose -- these come from the runtime host's
        # rootfs/ldconfig, so their absence here says nothing about the runtime.
        info "[verify] external sonames (from runtime rootfs, not staged):" \
             "$(printf '%s ' "${!ext[@]}")"
    fi

    if (( ${#fatal[@]} )); then
        local e
        for e in "${fatal[@]}"; do
            warn "[verify] UNRESOLVED: $e"
        done
        die "verify_dt_needed: ${#fatal[@]} PA-owned NEEDED entry(ies) have no provider in $dst"
    fi

    info "[verify] all PA-owned DT_NEEDED entries resolve"
}

# ---------------------------------------------------------------------------
# Sublayer wiring check: is the edge the *intended* edge?
#
# verify_dt_needed above only proves no edge dangles. That is not enough, as
# reverting telaf-pa past 8bfff04 showed: the strong libComponent_taf_pa_pms.so
# goes back to NEEDing libComponent_taf_ns_pa_pms.so, TARGET's own stub provides
# that soname, so every edge still resolves and the check passes -- while our
# MQTT sublayer silently becomes an orphan nobody loads. The runtime symptom is
# worse than a build break: the ns stub answers NOT_IMPLEMENTED (err 6) from
# every entry point including Init -> PA_FAULT crash-loop, with a clean install
# log. The rename and the revert are the same failure mirrored; neither moves a
# soname, both move which provider wins.
#
# Two assertions, because neither alone catches both directions:
#   1. EXPECTED_NEEDED -- pin the sublayer a strong PA lib must link. Catches a
#      revert or a re-rename that re-points the edge at another live provider.
#   2. orphan scan -- a SIMULA-staged PA lib that nothing NEEDs is a lib we
#      built for nothing. Catches the same thing from the other side, and needs
#      no maintained list.
#
# Both are matched on the unversioned stem (libfoo.so.1.0.0 -> libfoo.so), so
# adding a VERSION/SOVERSION to a target does not quietly disarm them.

# Strong PA lib stem -> sublayer stem it must NEED. Deliberately short: only
# edges where the wrong provider is a runtime crash rather than a mere waste.
declare -A EXPECTED_NEEDED=(
    ["libComponent_taf_pa_pms.so"]="libComponent_taf_prop_pa_pms.so"
)

# SIMULA-staged libs that legitimately have no DT_NEEDED referrer. Being on
# this list is how "staged but not yet wired" stays a *choice someone made*
# instead of an accident -- which is the whole lesson of the ns -> prop rename.
#   - taf_prop_common: the strong PA reaches it via dlopen() by name
#     (tafCommonPa.cpp) + dlsym, so no NEEDED edge points at it.
declare -A ORPHAN_OK=(
    ["libComponent_taf_prop_common.so"]=1
)

# Strip the version tail: libfoo.so.1.0.0 / libfoo.so.1 -> libfoo.so
soname_stem() { printf '%s.so\n' "${1%%.so*}"; }

verify_sublayer_wiring()
{
    local dst="$RUNTIME_LOC"
    local so base stem need

    # stem -> space-separated stems it NEEDs, plus the reverse index.
    local -A needs=() referenced=()
    while IFS= read -r -d '' so; do
        [[ -L "$so" ]] && continue
        base="$(basename "$so")"
        stem="$(soname_stem "$base")"
        while read -r need; do
            [[ -n "$need" ]] || continue
            needs["$stem"]+="$(soname_stem "$need") "
            referenced["$(soname_stem "$need")"]=1
        done < <("$READELF" -d "$so" 2>/dev/null \
                 | awk '/\(NEEDED\)/{gsub(/[][]/,"",$5); print $5}')
    done < <(find "$dst" -maxdepth 1 \( -type f -o -type l \) -name '*.so*' -print0)

    local -a bad=()

    # 1. Pinned edges.
    local want
    for stem in "${!EXPECTED_NEEDED[@]}"; do
        want="${EXPECTED_NEEDED[$stem]}"
        if [[ -z "${needs[$stem]:-}" ]]; then
            info "[verify] pinned edge skipped: $stem is not staged"
            continue
        fi
        if [[ " ${needs[$stem]} " == *" $want "* ]]; then
            info "[verify] pinned edge ok: $stem -> $want"
        else
            warn "[verify] WRONG PROVIDER: $stem should NEED $want, but NEEDs:" \
                 "${needs[$stem]% }"
            bad+=("$stem !-> $want")
        fi
    done

    # 2. Orphans among the libs SIMULA actually won.
    local b
    local -A seen=()
    for b in "${!SONAME_WINNER[@]}"; do
        [[ "${SONAME_WINNER[$b]}" == "SIMULA_PA" ]] || continue
        [[ "$b" == libComponent_* || "$b" == libtelux_* ]] || continue
        stem="$(soname_stem "$b")"
        [[ -n "${seen[$stem]:-}" ]] && continue
        seen["$stem"]=1
        [[ -n "${referenced[$stem]:-}" ]] && continue
        if [[ -n "${ORPHAN_OK[$stem]:-}" ]]; then
            info "[verify] orphan allowed (dlopen-only or intentionally unwired): $stem"
            continue
        fi
        warn "[verify] ORPHAN: SIMULA staged $stem but no staged lib NEEDs it"
        bad+=("$stem (orphan)")
    done

    if (( ${#bad[@]} )); then
        die "verify_sublayer_wiring: ${#bad[@]} wiring problem(s):" \
            "${bad[*]}. Either telaf-pa moved to a different sublayer ABI" \
            "(update EXPECTED_NEEDED), or the lib is deliberately not wired yet" \
            "(add it to ORPHAN_OK). Do not leave it ambiguous."
    fi

    info "[verify] sublayer wiring matches intent"
}

# Install the generated to the combined directory
install_libs_to_runtime "SIMULA_PA"   telaf-pa-simula/staging
install_libs_to_runtime "TARGET_PA"   telaf-pa-target/staging
install_libs_to_runtime "DEFAULT_PA"  telaf-pa-default/staging

verify_dt_needed
verify_sublayer_wiring
