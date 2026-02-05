#! /usr/bin/env python3

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import sys, re, os
import json
import subprocess
import argparse

def quick_run(command):
    try:
        result = subprocess.run(command,
                                shell=True,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                universal_newlines=True,
                                check=True)
    except subprocess.CalledProcessError as ex:
        if ex.returncode < 0:
            print(f"Failed signal: {ex.returncode}", end="")
        print(f"Err: {ex.stderr}", end="")
        raise
    else:
        print(f"[{result.args}]")
        if result.stdout:
            print(f"{result.stdout}", end="")
        return result

def update(which_git, json_root):
    assert which_git in json_root.keys()

    repo_info = json_root[which_git]

    prefix = f"cd {which_git} && "

    command = prefix + f"git rev-parse HEAD"
    result = quick_run(command)
    if result.stdout.strip() == repo_info['Commit-ID']:
        print(f"Already on {repo_info['Commit-ID']} for [{which_git}]")
        return

    command = prefix + "git fetch --all --tags"
    quick_run(command)

    command = prefix + "git stash"
    quick_run(command)

    command = prefix + f"git checkout base-{repo_info['Commit-ID']} || " + \
                f"git checkout -b base-{repo_info['Commit-ID']} "
    quick_run(command)

    command = prefix + f"git reset --hard {repo_info['Commit-ID']}"
    quick_run(command)

def is_patch_applied(current_patch, patch_record_history):
    if os.path.exists(patch_record_history):
        with open(patch_record_history, 'r') as f:
            if current_patch in f.read().splitlines():
                return True
    return False

def record_patch(patch_cmd, patch_record_history):
    with open(patch_record_history, "a") as f:
        f.write(f"{patch_cmd}\n")

def patch(which_git, json_root):
    assert which_git in json_root.keys()

    patch_list = json_root[which_git]['Patch-Cmd-List']

    if not patch_list:
        print(f"[{which_git}] no patch for this repo")
        return

    print(f"[{which_git}] Patching ...")

    patch_record_history = f"{which_git}/patch_record_history.history"

    for patch_cmd in patch_list:

        if is_patch_applied(patch_cmd, patch_record_history):
            print(f"[{which_git}] patched: {patch_cmd}")
            continue

        print(f"+ [{which_git}]: {patch_cmd}")

        patch_cmd_combo = f"cd {which_git} && {patch_cmd}"
        quick_run(patch_cmd_combo)

        record_patch(patch_cmd, patch_record_history)

def parse_args():
    parser = argparse.ArgumentParser(description="Operations for git repos")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-u", "--update", dest="update", help="update the special git repo")
    group.add_argument("-p", "--patch", dest="patch", help="patch the special git repo")
    args = parser.parse_args()
    return args

def main():
    args = parse_args()

    json_file = 'patch_me.json'
    json_root = None
    with open(json_file) as jf:
        json_root = json.load(jf)

    if args.update:
        update(args.update, json_root)
    elif args.patch:
        patch(args.patch, json_root)

    sys.exit(0)

if __name__ == "__main__": main()
