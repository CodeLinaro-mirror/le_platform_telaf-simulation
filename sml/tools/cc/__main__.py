#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""`sml.tools.cc` -- Command Card engine.

    sml cc [args...]                        # via the `sml` shell wrapper
    python3 -m sml.tools.cc [args...]        # equivalent, no wrapper needed

A command-card library: one card per file (YAML), with an fzf
panel and a task-spooler background queue.

Design conventions (frozen):
  * format       : YAML, one card per file, stored under CARDS_DIR
  * type inferred: has `run` -> action ; has `steps` -> pipeline (no `type`
                   field is written)
  * id           : equals the filename (without `.yaml`); the engine locates
                   cards directly by filename
  * steps        : pure references to other card ids, executed in order
  * pipeline     : fail-fast -- stops immediately on error and reports which
                   step it stopped at; supports `--from` to resume
  * failure logs : background runs are written to RUNS_DIR/<ts>-<id>.log
  * queue        : each queue value maps to its own TS_SOCKET=/tmp/ts_<queue>,
                   giving true parallelism across queues while staying
                   serial within a queue

Uses only the standard library plus PyYAML. fzf / ts are invoked as
subprocesses.
"""
from __future__ import annotations

import argparse
import datetime
import os
import shutil
import subprocess
import sys
import textwrap

try:
    import yaml
except ImportError:
    sys.stderr.write("Missing dependency: python3-yaml (PyYAML). Run `apt-get install -y python3-yaml`\n")
    sys.exit(2)

# ----------------------------------------------------------------------------
# Paths and constants
# ----------------------------------------------------------------------------
HOME = os.path.expanduser("~")
SML_HOME = os.environ.get("SML_HOME", os.path.join(HOME, "sml"))
CARDS_DIR = os.environ.get("SML_CARDS_DIR", os.path.join(SML_HOME, "tools", "cc", "cards"))
RUNS_DIR = os.environ.get("SML_RUNS_DIR", os.path.join("/tmp/sml-cc-logs", "runs"))
DEFAULT_QUEUE = "default"

# Colors (only enabled when stdout is a tty)
def _c(code):
    return code if sys.stdout.isatty() else ""
CLR_G = _c("\033[32m")
CLR_R = _c("\033[31m")
CLR_Y = _c("\033[33m")
CLR_B = _c("\033[34m")
CLR_D = _c("\033[2m")
CLR_0 = _c("\033[0m")


# ----------------------------------------------------------------------------
# Card model
# ----------------------------------------------------------------------------
class CardError(Exception):
    pass


class Card:
    def __init__(self, cid, data, path):
        self.id = cid
        self.path = path
        self.desc = data.get("desc", "")
        self.desc_long = data.get("desc_long", "")
        self.tags = data.get("tags", []) or []
        self.queue = data.get("queue", DEFAULT_QUEUE)
        self.run = data.get("run")
        self.steps = data.get("steps")
        # type inferred: run -> action, steps -> pipeline
        if self.run is not None and self.steps is not None:
            raise CardError(f"card {cid} has both run and steps, cannot determine type")
        if self.run is not None:
            self.type = "action"
        elif self.steps is not None:
            self.type = "pipeline"
        else:
            raise CardError(f"card {cid} has neither run nor steps")

    @property
    def is_pipeline(self):
        return self.type == "pipeline"


def _card_path(cid):
    return os.path.join(CARDS_DIR, cid + ".yaml")


def load_card(cid):
    """id == filename, located directly."""
    path = _card_path(cid)
    if not os.path.isfile(path):
        # also accept .yml
        alt = os.path.join(CARDS_DIR, cid + ".yml")
        if os.path.isfile(alt):
            path = alt
        else:
            raise CardError(f"card not found: {cid} (expected {path})")
    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}
    if not isinstance(data, dict):
        raise CardError(f"card {cid} malformed: top level must be a mapping")
    return Card(cid, data, path)


def all_cards():
    cards = []
    if not os.path.isdir(CARDS_DIR):
        return cards
    for fn in sorted(os.listdir(CARDS_DIR)):
        if not (fn.endswith(".yaml") or fn.endswith(".yml")):
            continue
        cid = fn.rsplit(".", 1)[0]
        try:
            cards.append(load_card(cid))
        except CardError as e:
            sys.stderr.write(f"{CLR_Y}skipping invalid card {fn}: {e}{CLR_0}\n")
    return cards


# ----------------------------------------------------------------------------
# Execution
# ----------------------------------------------------------------------------
def _now_tag():
    return datetime.datetime.now().strftime("%Y%m%d-%H%M")


def _run_shell(cmd, log_fh=None):
    """Run a shell snippet in the foreground, return the exit code. Also
    writes to log_fh when provided. Runs under `bash -x` so each line is
    echoed (with variables expanded) right before it executes."""
    proc = subprocess.Popen(
        ["/bin/bash", "-xc", cmd],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    for line in proc.stdout:
        sys.stdout.write(line)
        if log_fh:
            log_fh.write(line)
    proc.wait()
    return proc.returncode


def _emit(msg, log_fh=None):
    sys.stdout.write(msg + "\n")
    sys.stdout.flush()
    if log_fh:
        # strip color codes before writing to disk
        import re
        log_fh.write(re.sub(r"\033\[[0-9;]*m", "", msg) + "\n")
        log_fh.flush()


def _emit_cmd(cmd, log_fh=None):
    """Print the shell command about to run, one line per line of cmd."""
    for line in cmd.rstrip().splitlines():
        _emit(f"  {CLR_D}$ {line}{CLR_0}", log_fh)


def _resolve_steps(card):
    """Expand a pipeline's steps into a list of Card objects (pure references)."""
    steps = []
    for sid in card.steps:
        if not isinstance(sid, str):
            raise CardError(f"step in pipeline {card.id} must be a card id string, got {sid!r}")
        steps.append(load_card(sid))
    return steps


def run_card(cid, from_step=None, log_fh=None):
    """
    Run in the foreground. An action just runs `run`; a pipeline runs its
    steps in order, fail-fast.
    from_step: only valid for pipelines; resumes starting at that step id.
    Returns the exit code.
    """
    card = load_card(cid)

    if card.type == "action":
        if from_step:
            _emit(f"{CLR_Y}note: {cid} is an action, --from is ignored{CLR_0}", log_fh)
        _emit(f"{CLR_B}▶ {cid}{CLR_0}  {CLR_D}{card.desc}{CLR_0}", log_fh)
        _emit_cmd(card.run, log_fh)
        rc = _run_shell(card.run, log_fh)
        if rc == 0:
            _emit(f"{CLR_G}✓ {cid} done{CLR_0}", log_fh)
        else:
            _emit(f"{CLR_R}✗ {cid} failed (exit={rc}){CLR_0}", log_fh)
        return rc

    # pipeline
    steps = _resolve_steps(card)
    total = len(steps)

    start_idx = 0
    if from_step:
        ids = [s.id for s in steps]
        if from_step not in ids:
            _emit(f"{CLR_R}✗ --from {from_step} is not in {cid}'s steps: {ids}{CLR_0}", log_fh)
            return 2
        start_idx = ids.index(from_step)
        _emit(f"{CLR_D}↳ resuming from step {start_idx+1}/{total} ({from_step}){CLR_0}", log_fh)

    done = []
    for i in range(start_idx, total):
        step = steps[i]
        _emit(f"{CLR_B}▶ step {i+1}/{total} {step.id}{CLR_0}  {CLR_D}{step.desc}{CLR_0}", log_fh)
        if step.is_pipeline:
            _emit(f"{CLR_Y}note: nested pipeline {step.id}, expanding step by step{CLR_0}", log_fh)
            rc = run_card(step.id, log_fh=log_fh)
        else:
            _emit_cmd(step.run, log_fh)
            rc = _run_shell(step.run, log_fh)
        if rc != 0:
            remaining = [s.id for s in steps[i + 1:]]
            _emit("", log_fh)
            _emit(f"{CLR_R}✗ pipeline {cid} failed at step {i+1}/{total}: {step.id} (exit={rc}){CLR_0}", log_fh)
            if done:
                _emit(f"  {CLR_D}↳ completed: {', '.join(done)}{CLR_0}", log_fh)
            if remaining:
                _emit(f"  {CLR_D}↳ not run: {', '.join(remaining)}{CLR_0}", log_fh)
            _emit(f"  {CLR_D}↳ rerun: sml cc run {cid} --from {step.id}{CLR_0}", log_fh)
            return rc
        done.append(step.id)

    _emit(f"{CLR_G}✓ pipeline {cid} done ({total}/{total}){CLR_0}", log_fh)
    return 0


def run_chain(ids, log_fh=None):
    """Run an ad-hoc chain: cards multi-selected in fzf, run in order, fail-fast."""
    total = len(ids)
    _emit(f"{CLR_B}▶ ad-hoc chain ({total} steps): {' → '.join(ids)}{CLR_0}", log_fh)
    done = []
    for i, cid in enumerate(ids):
        _emit(f"{CLR_B}▶ {i+1}/{total} {cid}{CLR_0}", log_fh)
        rc = run_card(cid, log_fh=log_fh)
        if rc != 0:
            _emit(f"{CLR_R}✗ ad-hoc chain failed at {i+1}/{total}: {cid}{CLR_0}", log_fh)
            return rc
        done.append(cid)
    _emit(f"{CLR_G}✓ ad-hoc chain done ({total}/{total}){CLR_0}", log_fh)
    return 0


# ----------------------------------------------------------------------------
# Background (task-spooler) -- queue -> independent socket, true parallelism
# ----------------------------------------------------------------------------
def _ts_bin():
    for name in ("tsp", "ts"):
        if shutil.which(name):
            return name
    raise CardError("task-spooler (ts/tsp) not found. Run `apt-get install -y task-spooler`")


def bg_card(cid, from_step=None):
    """Submit to the ts queue on the card's own socket, running in the background."""
    card = load_card(cid)
    queue = card.queue or DEFAULT_QUEUE
    ts = _ts_bin()

    os.makedirs(RUNS_DIR, exist_ok=True)
    log_path = os.path.join(RUNS_DIR, f"{_now_tag()}-{cid}.log")

    # Re-invoke this engine's `run` in the background (reuses fail-fast,
    # resume support, and log writing)
    self_py = os.path.abspath(__file__)
    inner = f"python3 {self_py} run {cid}"
    if from_step:
        inner += f" --from {from_step}"
    # redirect background output to the runs log
    wrapped = f"{inner} > {log_path} 2>&1"

    env = dict(os.environ)
    env["TS_SOCKET"] = f"/tmp/ts_{queue}"          # key: each queue gets its own socket
    env["SML_HOME"] = SML_HOME

    subprocess.run([ts, "bash", "-c", wrapped], env=env, check=False)
    print(f"{CLR_G}✓ {cid} submitted → queue {queue}{CLR_0}  (socket=/tmp/ts_{queue})")
    print(f"  {CLR_D}log: {log_path}{CLR_0}")
    print(f"  {CLR_D}view queue: TS_SOCKET=/tmp/ts_{queue} {ts}{CLR_0}")


# ----------------------------------------------------------------------------
# Query / display
# ----------------------------------------------------------------------------
def cmd_ls(_args):
    """Machine-readable list: id\tdesc\ttype\tqueue\ttags -- for scripts/fzf."""
    for c in all_cards():
        tags = ",".join(c.tags)
        print(f"{c.id}\t{c.desc}\t{c.type}\t{c.queue}\t{tags}")


def cmd_find(args):
    kw = args.keyword.lower()
    hits = []
    for c in all_cards():
        hay = " ".join([c.id, c.desc, c.desc_long, " ".join(c.tags)]).lower()
        if kw in hay:
            hits.append(c)
    if not hits:
        print(f"{CLR_Y}no match: {args.keyword}{CLR_0}")
        return 1
    for c in hits:
        tags = f"[{','.join(c.tags)}]" if c.tags else ""
        print(f"{CLR_B}{c.id:<20}{CLR_0} {c.desc}  {CLR_D}{tags}{CLR_0}")
    return 0


def cmd_show(args):
    """Print a single card's details -- also used as the fzf --preview command."""
    try:
        c = load_card(args.id)
    except CardError as e:
        print(f"{CLR_R}{e}{CLR_0}")
        return 1
    print(f"{CLR_B}id:{CLR_0}    {c.id}")
    print(f"{CLR_B}desc:{CLR_0}  {c.desc}")
    print(f"{CLR_B}type:{CLR_0}  {c.type}")
    print(f"{CLR_B}queue:{CLR_0} {c.queue}")
    if c.tags:
        print(f"{CLR_B}tags:{CLR_0}  {', '.join(c.tags)}")
    if c.desc_long:
        print(f"{CLR_B}note:{CLR_0}")
        print(textwrap.indent(c.desc_long.rstrip(), "  "))
    if c.type == "action":
        print(f"{CLR_B}run:{CLR_0}")
        print(textwrap.indent(c.run.rstrip(), "  "))
    else:
        print(f"{CLR_B}steps:{CLR_0}")
        for i, sid in enumerate(c.steps, 1):
            print(f"  {i}. {sid}")
    return 0


# ----------------------------------------------------------------------------
# fzf panel (default action)
# ----------------------------------------------------------------------------
def cmd_panel(_args):
    if not shutil.which("fzf"):
        raise CardError("fzf not found. Run `apt-get install -y fzf`")
    cards = all_cards()
    if not cards:
        print(f"{CLR_Y}card directory is empty: {CARDS_DIR}{CLR_0}")
        return 1

    # each line: id TAB desc (fzf displays this; Enter returns column 1)
    lines = [f"{c.id}\t{c.desc}" for c in cards]
    self_py = os.path.abspath(__file__)
    preview = f"python3 {self_py} show {{1}}"

    fzf_args = [
        "fzf",
        "--delimiter", "\t",
        "--with-nth", "1,2",
        "--multi",                              # Tab multi-select -> ad-hoc chain
        "--height", "30%",
        "--border",
        "--preview", preview,
        "--preview-window", "right:55%:wrap",
        "--header", "Enter run | Alt-Enter bg | Tab multi-select chain | Esc quit",
        "--expect", "enter,alt-enter",
    ]
    proc = subprocess.run(
        fzf_args, input="\n".join(lines), text=True, stdout=subprocess.PIPE
    )
    if proc.returncode not in (0,):
        return 0  # user pressed Esc

    out = proc.stdout.splitlines()
    if not out:
        return 0
    key = out[0]                                # key returned via --expect
    selected = [ln.split("\t", 1)[0] for ln in out[1:] if ln.strip()]
    if not selected:
        return 0

    if key == "alt-enter":
        # background: submit each to its own queue
        for cid in selected:
            bg_card(cid)
        return 0

    # foreground (enter)
    if len(selected) == 1:
        return run_card(selected[0])
    return run_chain(selected)                  # multi-select -> ad-hoc chain


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------
def build_parser():
    p = argparse.ArgumentParser(
        prog="sml cc",
        description="Command Card -- personal command-card library (fzf panel + ts background queue)",
    )
    sub = p.add_subparsers(dest="cmd")

    # default action = panel (triggered with no subcommand)
    sub.add_parser("panel", help="open the fzf panel (= bare command default)")

    pr = sub.add_parser("run", help="run a card / pipeline in the foreground")
    pr.add_argument("id")
    pr.add_argument("--from", dest="from_step", default=None,
                    help="pipeline resume: start from this step id")

    pb = sub.add_parser("bg", help="submit to the ts background queue per the card's queue")
    pb.add_argument("id")
    pb.add_argument("--from", dest="from_step", default=None)

    pf = sub.add_parser("find", help="fuzzy search by id/desc/tags")
    pf.add_argument("keyword")

    ps = sub.add_parser("show", help="print a single card's details (also used as fzf preview)")
    ps.add_argument("id")

    sub.add_parser("ls", help="machine-readable list (TSV, for scripts/pipes)")

    return p


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        if args.cmd in (None, "panel"):
            return cmd_panel(args) or 0
        if args.cmd == "run":
            return run_card(args.id, from_step=args.from_step) or 0
        if args.cmd == "bg":
            bg_card(args.id, from_step=args.from_step)
            return 0
        if args.cmd == "find":
            return cmd_find(args) or 0
        if args.cmd == "show":
            return cmd_show(args) or 0
        if args.cmd == "ls":
            return cmd_ls(args) or 0
    except CardError as e:
        sys.stderr.write(f"{CLR_R}error: {e}{CLR_0}\n")
        return 1
    except KeyboardInterrupt:
        sys.stderr.write("\ninterrupted\n")
        return 130
    return 0


if __name__ == "__main__":
    sys.exit(main())
