# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

"""Message formatting for `sml.tools.mqttcli sub`.

Envelope-aware: WireSchema v1 envelopes (`{v,corrId,ts,src,dest?,data}`,
see `sml.mpss.data.envelope`) get a title line (clock, topic, class) plus
a body header (`corrId:`/`From:`/`To:`, one per line -- `To:` only when
`dest` is present) and pretty-printed `data`. Everything else (ctrl/raw
payloads, non-JSON bytes) is pretty-printed as-is.

Degrades to plain `json.dumps(indent=2)` if `rich` isn't importable.
"""
from __future__ import annotations

import json
from datetime import datetime, timezone
from typing import Any, Optional

try:
    from rich.console import Console, Group
    from rich.panel import Panel
    from rich.pretty import Pretty
    from rich.text import Text
    _HAVE_RICH = True
except ImportError:  # pragma: no cover - exercised via _HAVE_RICH=False path
    _HAVE_RICH = False


_ENVELOPE_KEYS = {"v", "corrId", "ts", "src"}


def _topic_class(topic: str) -> str:
    if topic.startswith("ap/req/"):
        return "req"
    if topic.startswith("mp/rsp/"):
        return "rsp"
    if topic.startswith("mp/ind/"):
        return "ind"
    if topic.startswith("ctrl/cmd/"):
        return "ctrl"
    if topic.startswith("mp/sys/"):
        return "sys"
    return "other"


_TOPIC_CLASS_COLOR = {
    "req": "cyan",
    "rsp": "green",
    "ind": "yellow",
    "ctrl": "magenta",
    "sys": "blue",
    "other": "white",
}


def _decode_payload(payload: bytes) -> Optional[Any]:
    try:
        return json.loads(payload.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError):
        return None


def _is_envelope(body: Any) -> bool:
    return isinstance(body, dict) and _ENVELOPE_KEYS.issubset(body.keys())


def _fmt_clock(ts_ms: int) -> str:
    return datetime.fromtimestamp(ts_ms / 1000.0, tz=timezone.utc).strftime("%H:%M:%S.%f")[:-3]


def format_message(topic: str, payload: bytes, qos: int) -> str:
    """Render one inbound MQTT message as a display string (rich markup
    stripped out here; callers needing live rich rendering should use
    `render_message` instead)."""
    body = _decode_payload(payload)

    if body is None:
        return f"[{topic}] (qos={qos}) <undecodable: {len(payload)} bytes>"

    cls = _topic_class(topic)
    if _is_envelope(body):
        title = _envelope_title(topic, cls, body)
        header_lines = _envelope_header_lines(body)
        data = body.get("data", body.get("error"))
        pretty = json.dumps(data, indent=2, sort_keys=True)
        return f"{title}\n{header_lines}\n\n{pretty}"

    pretty = json.dumps(body, indent=2, sort_keys=True)
    return f"[{topic}] (qos={qos}, class={cls})\n{pretty}"


def _envelope_title(topic: str, cls: str, body: dict) -> str:
    clock = _fmt_clock(body["ts"]) if isinstance(body.get("ts"), int) else str(body.get("ts"))
    return f"[{clock}] {topic} ({cls})"


def _envelope_header_lines(body: dict) -> str:
    lines = [f"corrId: {body.get('corrId')}", f"From: {body.get('src', '?')}"]
    if "dest" in body:
        lines.append(f"To: {body['dest']}")
    return "\n".join(lines)


def render_message(console: Optional["Console"], topic: str, payload: bytes, qos: int) -> None:
    """Print one message. If `console` is None (rich unavailable, or the
    caller opted out via `-v`/lack thereof), falls back to plain
    `print(format_message(...))`; otherwise renders rich panels on it."""
    if console is None:
        print(format_message(topic, payload, qos))
        return

    body = _decode_payload(payload)
    if body is None:
        console.print(Panel(f"<undecodable: {len(payload)} bytes>",
                            title=f"{topic} (qos={qos})", border_style="red"))
        return

    cls = _topic_class(topic)
    color = _TOPIC_CLASS_COLOR.get(cls, "white")
    if _is_envelope(body):
        title = _envelope_title(topic, cls, body)
        header = Text(_envelope_header_lines(body))
        data = body.get("data", body.get("error"))
        console.print(Panel(Group(header, Text(""), Pretty(data)), title=title, border_style=color))
    else:
        console.print(Panel(Pretty(body), title=f"{topic} (qos={qos}, class={cls})",
                            border_style=color))


__all__ = ["format_message", "render_message"]
