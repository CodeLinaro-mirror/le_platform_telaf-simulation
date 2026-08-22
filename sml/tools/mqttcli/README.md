# sml.tools.mqttcli

MQTT `sub`/`pub` CLI for debugging simulator traffic — replaces ad-hoc
`/tmp` paho scripts with a first-class tool that speaks the simulator's
envelope format and validates against the generated JSON Schemas.

```
sml mqttcli sub [topic ...]
sml mqttcli pub <file.yaml>
```

Equivalent, if the `sml` wrapper isn't on `PATH` (see `sml/tools/README.md`):

```
python3 -m sml.tools.mqttcli sub [topic ...]
python3 -m sml.tools.mqttcli pub <file.yaml>
```

## Design

**Connection.** Broker settings default to `sml.mpss.config.load_config().broker`
— the same config the real `sml.mpss` process reads, so the CLI talks to
whatever broker the simulator is actually using (default: Unix domain
socket `/tmp/simula-mqtt.sock`). Override with `--host`/`--port`/`--socket`/
`--transport` on either subcommand.

**`sub` — traffic monitor.** No topic arguments subscribes to `#` (everything).
Give one or more topic filters to narrow it down. By default output is
plain text (safe for piping/logging); pass `-v`/`--verbose` to render
each message as a streaming rich panel (newest at the bottom) colored by
topic class (`ap/req`, `mp/rsp`, `mp/ind`, `ctrl/cmd`, `mp/sys`) — needs
`rich` installed (pinned to `rich==14.2.0` in
`docker/for_ubuntu_2204/Dockerfile.2204.*`); without it, `-v` silently
falls back to the same plain text as the default.

**Envelope-aware rendering.** WireSchema v1 envelopes
(`{v, corrId, ts, src, dest?, data}`) get a title line (clock, topic,
class) followed by a body header — `corrId:`, `From:` (the sender's raw
`src` string), and `To:` (only present when the envelope has a `dest`,
i.e. `rsp`/error responses; `req`/`ind` messages have no `dest` and so no
`To:` line) — then the pretty-printed inner `data`. Anything else (raw
`ctrl/cmd/*` payloads, non-JSON bytes) is shown as pretty JSON or a safe
placeholder. Example:

```
╭─── [13:46:43.936] mp/rsp/data/request_profile_list (rsp) ────╮
│ corrId: 00000000                                              │
│ From: mpss-master 172.18.0.2 telaf_simulation_runtime_2204_m-190
│ To: tafDataCallSvc-unknown-2dc0                                │
│                                                                 │
│ {                                                              │
│     'profiles': [...]                                          │
│ }                                                                │
╰──────────────────────────────────────────────────────────────────╯
```

**`pub` — one file, one command.** `sml mqttcli pub <file.yaml>`
sends what the YAML file describes and exits — it never waits for a
response. See `commands/*.yaml` for worked examples.

**Envelope wrapping on publish.** A per-file `envelope: true/false` field
wins if present. Otherwise the prefix rule applies: `ap/req/*` topics get
wrapped, `ctrl/cmd/*` topics are sent raw, and anything else is wrapped by
default. When wrapping, the tool fills in `v`/`corrId`/`ts`/`src`
(`src` = `cli-<pid>`) — you only supply `data`.

**Validation before send.** The topic's schema id is derived from the
topic string (`ap/req/data/start_data_call` → `data.start_data_call.req`,
`ctrl/cmd/action/data/force_serv_state` → `action.data.force_serv_state.req`)
and looked up in the generated `validators.py`/`ctrl_validators.py`
schemas. A known topic with an invalid payload is a hard failure — nothing
is sent, including earlier items in a sequence. An unknown topic (no
matching schema) is sent as-is with no validation.

## `pub` YAML format

One file = one command:

```yaml
topic: ctrl/cmd/action/data/force_serv_state   # required
qos: 1                # optional, default 1
retain: false         # optional, default false
envelope: false       # optional; omitted -> prefix rule (see above)
interval: 500ms       # optional; only meaningful when data is a list
data:                 # object OR list of objects (a sequence)
  serviceState: IN_SERVICE
  networkRat: WCDMA
```

`data` as a list publishes each entry to the same topic in order; `interval`
(e.g. `500ms`, `2s`, `1m`) is the delay between sends.

## Examples

- `commands/force_serv_state.yaml` — raw `ctrl/cmd/action/*` payload, validated.
- `commands/start_data_call.yaml` — `ap/req/*` payload, auto-enveloped.
- `commands/serv_state_sequence.yaml` — `data` list with an `interval`.

## Usage

```
sml mqttcli sub                         # monitor everything, plain text
sml mqttcli sub -v                      # monitor everything, rich panels
sml mqttcli sub 'mp/ind/#' 'mp/rsp/#'    # narrow filters
sml mqttcli pub sml/tools/mqttcli/commands/force_serv_state.yaml
sml mqttcli pub my_command.yaml --transport tcp --host 127.0.0.1 --port 1883
```

`python3 -m sml.tools.mqttcli ...` works identically wherever `sml` isn't
set up (see `sml/tools/README.md`).

## Tests

```
python3 -m pytest sml/tools/mqttcli/tests/
```

Unit tests mock paho — no broker required. For a live round-trip, run
`sub` in one terminal and `pub` an example command in another against a
running mosquitto instance.
