#!/usr/bin/env bash
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# Run with bash pa/tests/test_incremental_pa.sh; no compiler or ELF tools needed.
set -euo pipefail
PA_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
TEST_ROOT=$(mktemp -d)
trap 'rm -rf "$TEST_ROOT"' EXIT

mkdir -p "$TEST_ROOT/pa" "$TEST_ROOT/bin" "$TEST_ROOT/upstream/telaf"
cp "$PA_ROOT/common.sh" "$PA_ROOT/simulation_install_pa.sh" "$TEST_ROOT/pa/"
export PATH="$TEST_ROOT/bin:$PATH"
export TELAF_ROOT="$TEST_ROOT/upstream/telaf" LEGATO_ROOT="$TEST_ROOT/legato"
export SIMULATION_DEPS_ROOTFS="$TEST_ROOT/deps"
export OBJCOPY=objcopy STRIP=strip READELF=readelf

cat > "$TEST_ROOT/bin/cmake" <<'SH'
#!/usr/bin/env bash
for arg in "$@"; do
    case "$arg" in
        -DCMAKE_INSTALL_PREFIX=*) printf '%s' "${arg#*=}" > CMakeCache.txt ;;
    esac
done
SH
cat > "$TEST_ROOT/bin/make" <<'SH'
#!/usr/bin/env bash
set -e
[[ "$1" == install ]] || exit 0
[[ ! -e install_manifest.txt && ! -e .simulation-pa-build ]]
prefix=$(cat CMakeCache.txt)
mkdir -p "$prefix/lib"
printf old > "$prefix/lib/libold.so"
case "${TEST_INSTALL_MODE:-ok}" in
    fail) exit 1 ;;
    outside) output="$prefix-other/libwrong.so"; printf wrong > "$output" ;;
    missing) output="$prefix/lib/libmissing.so" ;;
    ok) output="$prefix/lib/libnew.so"; printf new > "$output" ;;
esac
# CMake manifests can end without a newline: the last entry must be checked.
printf '%s\n%s' "$prefix/lib/libold.so" "$output" > install_manifest.txt
SH
cat > "$TEST_ROOT/bin/objcopy" <<'SH'
#!/usr/bin/env bash
[[ "${TEST_DEBUG_FAIL:-0}" == 0 ]] || exit 1
cp "$2" "$3"
SH
cat > "$TEST_ROOT/bin/strip" <<'SH'
#!/usr/bin/env bash
sed -i '/^DEBUG /d' "$2"
SH
cat > "$TEST_ROOT/bin/readelf" <<'SH'
#!/usr/bin/env bash
exit 0
SH
chmod +x "$TEST_ROOT/bin/"*

expect_failure()
{
    if "$@" > "$TEST_ROOT/run.log" 2>&1; then
        cat "$TEST_ROOT/run.log"
        echo "Expected failure: $*" >&2
        exit 1
    fi
}

for module in telaf-pa-default telaf-pa-simula telaf-pa-target; do
    dir="$TEST_ROOT/pa/$module"
    mkdir -p "$dir/build" "$dir/staging/lib" "$dir/staging-other"
    cp "$PA_ROOT/$module/build_pa.simula.sh" "$dir/"
    printf input > "$dir/CMakeLists.txt"
    printf old > "$dir/staging/lib/libold.so"
    printf '%s' "$dir/staging/lib/libold.so" > "$dir/build/install_manifest.txt"
    printf '%s\nstale\n' "$dir/staging" > "$dir/build/.simulation-pa-build"
    touch "$dir/build/CMakeCache.txt"

    expect_failure env TEST_INSTALL_MODE=fail bash "$dir/build_pa.simula.sh"
    [[ ! -e "$dir/build/install_manifest.txt" && ! -e "$dir/build/.simulation-pa-build" ]]
    for mode in outside missing; do
        expect_failure env TEST_INSTALL_MODE="$mode" bash "$dir/build_pa.simula.sh"
        [[ ! -e "$dir/build/.simulation-pa-build" ]]
    done

    bash "$dir/build_pa.simula.sh" > "$TEST_ROOT/run.log" 2>&1
    [[ -s "$dir/build/.simulation-pa-build" && -e "$dir/staging/lib/libnew.so" ]]
    env TEST_INSTALL_MODE=fail bash "$dir/build_pa.simula.sh" > "$TEST_ROOT/run.log" 2>&1
    [[ "$(cat "$TEST_ROOT/run.log")" == *"skipping build/install"* ]]

    # A matching signature cannot make another prefix's manifest current.
    printf '%s' "$dir/staging-other/libwrong.so" > "$dir/build/install_manifest.txt"
    expect_failure env TEST_INSTALL_MODE=fail bash "$dir/build_pa.simula.sh"
    [[ ! -e "$dir/build/.simulation-pa-build" ]]
    rm -rf "$dir/staging"
done

base="$LEGATO_ROOT/build/simulation/_staging_system.simulation.update_ro"
mkdir -p "$base/systems/current/lib" "$TEST_ROOT/pa/telaf-pa-simula/staging/lib"
source_lib="$TEST_ROOT/pa/telaf-pa-simula/staging/lib/libprobe.so.1"
printf 'DEBUG retained\nCODE retained\n' > "$source_lib"
ln -s libprobe.so.1 "${source_lib%.1}"
mkdir -p "$TEST_ROOT/pa/telaf-pa-target/staging/lib"
printf shadowed > "$TEST_ROOT/pa/telaf-pa-target/staging/lib/libprobe.so.1"

. "$TEST_ROOT/pa/common.sh"
trap 'rm -rf "$TEST_ROOT"' EXIT
before=$(pa_input_signature "$(dirname "$source_lib")")
output="$LEGATO_ROOT/build/simulation"
runtime="$output/staging_combined/systems/current/lib"
for attempt in 1 2; do
    printf '%s' "$attempt" > "$base/trigger"
    bash "$TEST_ROOT/pa/simulation_install_pa.sh" > "$TEST_ROOT/run.log" 2>&1
    [[ "$(cat "$source_lib")" == $'DEBUG retained\nCODE retained' ]]
    [[ "$(cat "$runtime/libprobe.so.1")" == 'CODE retained' ]]
    [[ "$(cat "$output/libprobe.so.1.debug")" == $'DEBUG retained\nCODE retained' ]]
    [[ "$(readlink "$runtime/libprobe.so")" == libprobe.so.1 ]]
    [[ "$(pa_input_signature "$(dirname "$source_lib")")" == "$before" ]]
done

printf failure > "$base/trigger"
expect_failure env TEST_DEBUG_FAIL=1 bash "$TEST_ROOT/pa/simulation_install_pa.sh"
[[ ! -e "$output/.simulation-pa-install" ]]
echo 'PA incremental build and installation checks passed.'
