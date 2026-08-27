# sml.tools.alias

Short-name shortcuts for common invocations — `sml alias step` instead
of `sml mqttcli pub sml/tools/mqttcli/commands/scenario_step.yaml`.

```
sml alias                        # list available aliases
sml alias <name> [extra_args...] # run the aliased command
```

## Design

Each entry in `aliases.yaml` is **exactly one** of two forms —
`tool`/`args` and `shell` are mutually exclusive on a given entry.

### `tool` + `args` — forward to an `sml/tools/<tool>` package

```yaml
step:
  tool: mqttcli
  args: [pub, mqttcli/commands/scenario_step.yaml]
```

Runs in-process: imports `sml.tools.<tool>.__main__` and calls its
`main(args + extra_args)`. File-path args are resolved relative to
`sml/tools/`, so this works regardless of the caller's cwd. Use this
form for any alias that's really just a shortcut into an existing
`sml`-dispatched tool (`mqttcli`, or any future one).

### `shell` (+ optional `cwd`) — run an arbitrary command

```yaml
power-state:
  shell: "cat /sys/fs/cgroup/telaf/cgroup.events"
```

Use this form for commands that aren't `sml`-dispatched tools at all
(`cat`, `ls`, any executable on `PATH`).

**Argument handling is deliberately strict, to avoid shell-injection
surprises:** the fixed `shell` string is split with `shlex.split` and
run via `subprocess.run` with no `shell=True` anywhere. Any
`extra_args` given on the command line (`sml alias power-state --foo`)
are appended as literal argv elements after the fixed command's argv —
shell metacharacters in them (`;`, `|`, `$(...)`, `&&`, ...) are passed
through as literal text, never re-interpreted, so they can't inject an
extra command or redirect.

Pipes/redirects/`&&` are NOT supported inside the fixed `shell` string
either, since there's no shell to interpret them. If an alias
genuinely needs shell features, wrap it explicitly:
`shell: "bash -c 'cat /a | grep b'"`.

`cwd` (optional, `shell` form only) is resolved relative to
`sml/tools/` if not absolute; omit it to use the caller's cwd.

No data-field overrides at alias-invocation time — if a `tool`-form
alias needs a different payload, add a new command YAML and a new
alias entry rather than templating `data` at runtime.

## Adding a new alias

Pick the form that fits and add an entry to `aliases.yaml`:

```yaml
<name>:
  tool: <tool-dir-name-under-sml/tools/>
  args: [<fixed argv for that tool's main()>]

# or

<name>:
  shell: "<command> [fixed args...]"
  cwd: "<optional working directory>"
```

No code changes needed.
