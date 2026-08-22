#!/usr/bin/env python3
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""`sml.tools.mqttcli` -- MQTT sub/pub CLI for debugging simulator traffic.

    python3 -m sml.tools.mqttcli sub [topic ...]      # no topics -> subscribe '#'
    python3 -m sml.tools.mqttcli pub <file.yaml>       # one file = one command

See sml/tools/mqttcli/README.md and commands/*.yaml for the pub file format.
"""
from __future__ import annotations

import argparse
import json
import sys
import time

from sml.tools.mqttcli.connection import add_connection_args, connect_client, make_client_id, resolve_broker_config
from sml.tools.mqttcli.format import render_message
from sml.tools.mqttcli.pubfile import PubFileError, load_pubfile

try:
    from rich.console import Console
    _HAVE_RICH = True
except ImportError:
    _HAVE_RICH = False


def _cmd_sub(args: argparse.Namespace) -> int:
    broker = resolve_broker_config(args)
    client = connect_client(broker, make_client_id("sml-mqttcli-sub"))
    console = Console() if (args.verbose and _HAVE_RICH) else None
    topics = args.topics or ["#"]

    def _on_message(_client, _userdata, msg):
        render_message(console, msg.topic, bytes(msg.payload), msg.qos)

    client.on_message = _on_message
    for topic in topics:
        client.subscribe(topic, qos=1)

    client.loop_start()
    try:
        while True:
            time.sleep(0.2)
    except KeyboardInterrupt:
        pass
    finally:
        client.loop_stop()
        client.disconnect()
    return 0


def _cmd_pub(args: argparse.Namespace) -> int:
    broker = resolve_broker_config(args)
    client_id = make_client_id("sml-mqttcli-pub")
    try:
        pubfile = load_pubfile(args.file, src=client_id)
    except (PubFileError, OSError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    client = connect_client(broker, client_id)
    client.loop_start()
    try:
        for i, spec in enumerate(pubfile.specs):
            if i > 0 and pubfile.interval_ms:
                time.sleep(pubfile.interval_ms / 1000.0)
            client.publish(spec.topic, json.dumps(spec.payload).encode(),
                           qos=spec.qos, retain=spec.retain)
        time.sleep(0.2)  # let paho's loop thread flush the send(s) before disconnect
    finally:
        client.loop_stop()
        client.disconnect()
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="python3 -m sml.tools.mqttcli")
    subparsers = parser.add_subparsers(dest="command", required=True)

    sub_parser = subparsers.add_parser("sub", help="watch broker traffic")
    sub_parser.add_argument("topics", nargs="*", help="topic filters (default: '#')")
    sub_parser.add_argument("-v", "--verbose", action="store_true",
                            help="render messages with rich panels (default: plain text)")
    add_connection_args(sub_parser)
    sub_parser.set_defaults(func=_cmd_sub)

    pub_parser = subparsers.add_parser("pub", help="publish a declarative YAML command")
    pub_parser.add_argument("file", help="path to a pub command YAML file")
    add_connection_args(pub_parser)
    pub_parser.set_defaults(func=_cmd_pub)

    return parser


def main(argv: list[str] | None = None) -> int:
    # `sub` streams messages as they arrive; without this, stdout is
    # fully buffered (not line-buffered) whenever it's not a tty --
    # e.g. piped into `tee` -- so output arrives in stalled bursts
    # instead of in real time.
    sys.stdout.reconfigure(line_buffering=True)
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
