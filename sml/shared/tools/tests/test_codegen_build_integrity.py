# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Build-integrity tests for the contract code-generation pipeline.

These pin the two independent defects behind this build failure:

    component/common/ModemBridge.cpp:18:10: fatal error:
        generated/cpp/validators.h: No such file or directory

1. MISSING CMAKE DEPENDENCY. `telux_common` includes `generated/cpp/
   validators.h` but had no `add_dependencies(telux_common simula_codegen)`,
   so with `make -j$(nproc)` its compile could start before gen_cpp.py wrote
   the header. `telux_data`/`telux_tel` had the dependency; `telux_common` --
   which compiles FIRST and is linked by both -- did not.

2. LOCALE-DEPENDENT DECODE. The generators read registry/schema files with a
   bare `read_text()`, i.e. `locale.getpreferredencoding()`. On a build host
   that is not UTF-8 (cp1252, or a container with LANG unset -> ascii) any
   non-ASCII byte raises UnicodeDecodeError, the generator aborts, and NO
   header is written -- surfacing as the same "file not found" error, which
   points at the wrong layer entirely.

Both are invisible to a warm incremental build (the header already exists from
an earlier run) and to a UTF-8 host respectively -- they only bite on a clean
checkout / different host, which is exactly why they need pinning here.
"""
from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

import pytest

_SML_ROOT = Path(__file__).resolve().parents[3]
_PA_ROOT = _SML_ROOT.parent / "pa" / "telaf-pa-simula"
_CMAKE = _PA_ROOT / "CMakeLists.txt"
_COMPONENT = _PA_ROOT / "component"

_GEN_INCLUDE_RE = re.compile(r'#include\s+"generated/cpp/(\w+)\.h"')

# component dir -> CMake target its sources compile into. component/sim has no
# CMakeLists of its own: its .cpp files are pulled into telux_tel so that
# PhoneFactory has a single definition (see component/tel/CMakeLists.txt).
_DIR_TO_TARGET = {
    "common": "telux_common",
    "data": "telux_data",
    "sim": "telux_tel",
    "tel": "telux_tel",
    "platform": "telux_platform",
}


def _cmake_text() -> str:
    return _CMAKE.read_text(encoding="utf-8")


def _targets_depending_on_codegen() -> set[str]:
    """Targets ordered after `simula_codegen`, however that is expressed.

    Accepts both the explicit `add_dependencies(<t> simula_codegen)` form and
    the list-driven `foreach(target ${CODEGEN_CONSUMERS})` form, so this test
    pins the REQUIRED ORDERING rather than one particular way of spelling it.
    """
    text = _cmake_text()
    targets = set(re.findall(r"add_dependencies\(\s*(\w+)\s+simula_codegen\s*\)", text))

    list_match = re.search(r"set\(CODEGEN_CONSUMERS(.*?)^\)", text, re.S | re.M)
    loops_over_list = re.search(
        r"foreach\(\s*\w+\s+\$\{CODEGEN_CONSUMERS\}(.*?)endforeach\(", text, re.S
    )
    if list_match and loops_over_list and "simula_codegen" in loops_over_list.group(1):
        body = re.sub(r"#[^\n]*", "", list_match.group(1))  # strip trailing comments
        targets |= set(body.split())
    return targets


def _dirs_including_generated_headers() -> dict[str, set[str]]:
    """component subdir -> {generated header stems it includes}."""
    found: dict[str, set[str]] = {}
    for src in _COMPONENT.rglob("*"):
        if src.suffix not in {".cpp", ".hpp"} or "tests" in src.parts:
            continue
        hits = _GEN_INCLUDE_RE.findall(src.read_text(encoding="utf-8", errors="replace"))
        if hits:
            found.setdefault(src.relative_to(_COMPONENT).parts[0], set()).update(hits)
    return found


# ---------------------------------------------------------------------------
# 1. CMake ordering
# ---------------------------------------------------------------------------

def test_generated_header_consumers_are_discoverable():
    """Guard the guard: if this finds nothing, the tests below pass vacuously."""
    dirs = _dirs_including_generated_headers()
    assert "common" in dirs, "expected component/common to include a generated header"
    assert "validators" in dirs["common"], (
        "ModemBridge.cpp should include generated/cpp/validators.h"
    )


def test_every_generated_header_consumer_depends_on_codegen():
    """THE regression test for the reported fatal error.

    Any component that includes generated/cpp/*.h must have its CMake target
    ordered after simula_codegen, or a clean parallel build races the
    generator and fails with "No such file or directory".
    """
    depending = _targets_depending_on_codegen()
    missing = {}
    for comp_dir, headers in sorted(_dirs_including_generated_headers().items()):
        target = _DIR_TO_TARGET.get(comp_dir)
        assert target, (
            f"component/{comp_dir}/ includes generated/cpp/* but is absent from "
            "_DIR_TO_TARGET -- map it to its CMake target"
        )
        if target not in depending:
            missing[comp_dir] = (target, sorted(headers))

    assert not missing, (
        "these components include generated/cpp/*.h but their CMake target is "
        f"NOT ordered after simula_codegen: {missing}. A clean `make -j` build "
        "will intermittently fail with 'generated/cpp/<h>: No such file or "
        "directory'. Add the target to CODEGEN_CONSUMERS in "
        f"{_CMAKE.relative_to(_SML_ROOT.parent)}."
    )


def test_codegen_consumer_list_has_no_stale_entries():
    """A renamed/removed target must not silently drop its dependency."""
    text = _cmake_text()
    if "CODEGEN_CONSUMERS" not in text:
        pytest.skip("CODEGEN_CONSUMERS list not in use")
    assert "if(NOT TARGET" in text, (
        "CODEGEN_CONSUMERS must validate each entry is a real target, else a "
        "typo/rename silently removes the codegen ordering"
    )


# ---------------------------------------------------------------------------
# 2. Locale-independent code generation
# ---------------------------------------------------------------------------

_GENERATORS = [
    "shared/tools/gen_cpp.py",
    "shared/tools/gen_python.py",
    "control/tools/gen_python.py",
    "shared/tools/validate_registry.py",
    "control/tools/validate_registry.py",
]


@pytest.mark.parametrize("script", _GENERATORS)
def test_no_locale_dependent_file_reads(script):
    """Every registry/schema read must pin encoding="utf-8".

    `read_text()`/`open()` without an encoding uses the platform locale. The
    contract files contain non-ASCII (en dashes, arrows) in descriptions, so on
    a non-UTF-8 host the generator dies mid-run and writes no header.
    """
    src = (_SML_ROOT / script).read_text(encoding="utf-8")
    for helper in ("schema_common.py",):
        helper_path = (_SML_ROOT / script).parent / helper
        if helper_path.exists():
            src += helper_path.read_text(encoding="utf-8")

    # Match only actual call sites, not comments or docstrings that mention
    # the pattern. A comment line starts with optional whitespace then '#';
    # a string literal containing the pattern is inside quotes. Strip both
    # before scanning so the test doesn't match its own explanatory text.
    code_only = re.sub(r'#[^\n]*', '', src)          # strip # comments
    code_only = re.sub(r'""".*?"""', '', code_only, flags=re.S)  # strip docstrings
    code_only = re.sub(r"'''.*?'''", '', code_only, flags=re.S)
    bad = re.findall(r'(?<!encoding=)\bread_text\(\s*\)', code_only)
    assert not bad, (
        f"{script} (or its schema_common.py) calls read_text() with no "
        "encoding -- this aborts code generation on a non-UTF-8 build host and "
        "surfaces as 'generated/cpp/validators.h: No such file or directory'"
    )


@pytest.mark.parametrize("script", _GENERATORS)
def test_generators_run_under_non_utf8_locale(script, tmp_path):
    """Actually execute each generator with a non-UTF-8 IO encoding.

    The static check above catches the known spelling; this catches any other
    locale-sensitive read (e.g. a bare open()) by reproducing the real
    condition rather than pattern-matching for it.
    """
    env = {
        "PYTHONIOENCODING": "cp1252",
        "PYTHONUTF8": "0",
        "LC_ALL": "C",
        "LANG": "C",
        "PATH": "",
        "SYSTEMROOT": "C:\\Windows",  # harmless on POSIX, required by Py on Win
    }
    proc = subprocess.run(
        [sys.executable, str(_SML_ROOT / script)],
        capture_output=True, text=True, env=env, cwd=str(_SML_ROOT),
    )
    assert proc.returncode == 0, (
        f"{script} failed under a non-UTF-8 locale (returncode="
        f"{proc.returncode}).\nstderr:\n{proc.stderr[-2000:]}"
    )
    assert "UnicodeDecodeError" not in proc.stderr


def test_gen_cpp_emits_every_header_cmake_expects():
    """`GEN_CPP_HEADERS` is the custom-command OUTPUT list.

    If gen_cpp.py stops emitting one of these, CMake reruns the generator on
    every build and the missing header never appears -- the same fatal error,
    from a third direction.
    """
    declared = set(re.findall(r"\$\{SML_ROOT\}/generated/cpp/(\w+\.h)", _cmake_text()))
    assert declared, "GEN_CPP_HEADERS not found in CMakeLists.txt"

    subprocess.run(
        [sys.executable, str(_SML_ROOT / "shared/tools/gen_cpp.py")],
        capture_output=True, text=True, check=True, cwd=str(_SML_ROOT),
    )
    out_dir = _SML_ROOT / "generated" / "cpp"
    missing = {h for h in declared if not (out_dir / h).is_file()}
    assert not missing, (
        f"CMake's GEN_CPP_HEADERS declares {sorted(declared)} but gen_cpp.py "
        f"did not produce {sorted(missing)}"
    )


def test_sim_domain_schema_ids_resolve_in_generated_validators():
    """Every "sim.*" schema id the PA passes must exist in validators.h.

    ModemBridge looks these up by literal string; an id with no entry means the
    lookup misses and a valid response is discarded as schema-invalid -- which
    the PA treats exactly like a timeout, stalling taf_sim_GetState/GetICCID/
    GetIMSI rather than failing loudly.
    """
    subprocess.run(
        [sys.executable, str(_SML_ROOT / "shared/tools/gen_cpp.py")],
        capture_output=True, text=True, check=True, cwd=str(_SML_ROOT),
    )
    validators = (_SML_ROOT / "generated" / "cpp" / "validators.h").read_text(
        encoding="utf-8"
    )
    available = set(re.findall(r'"(sim\.[a-z_]+\.(?:req|rsp|ind))"', validators))

    # sim C++ passes schema ids as the second argument to send_request /
    # subscribe_event, spelled as a plain string literal immediately after the
    # topic constant, e.g.:
    #   bridge_.send_request(topics::sim::get_state::req, "sim.get_state.rsp", ...)
    # Match that literal form rather than a generic string scan so the test
    # doesn't accidentally match topic strings or comments.
    referenced: set[str] = set()
    for src in (_COMPONENT / "sim").glob("*.cpp"):
        text = src.read_text(encoding="utf-8")
        # send_request / subscribe_event schema-id argument
        referenced |= set(re.findall(
            r'(?:send_request|subscribe_event)\s*\([^,]+,\s*"(sim\.[a-z_]+\.(?:req|rsp|ind))"',
            text))

    assert referenced, (
        "no sim.* schema ids found in component/sim/*.cpp -- "
        "every send_request/subscribe_event call must carry an explicit "
        "schema_id argument (second parameter after the topic)"
    )
    assert not (referenced - available), (
        f"component/sim/*.cpp references schema id(s) absent from generated "
        f"validators.h: {sorted(referenced - available)}"
    )
