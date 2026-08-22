# sml.tools

Container for standalone CLI tools shipped with this repo. Each tool
lives in its own subdirectory and is reachable two ways:

```
sml <tool> [args...]                    # via the `sml` shell wrapper
python3 -m sml.tools.<tool> [args...]   # equivalent, no wrapper needed
```

## `sml` wrapper

`sml/tools/bin/sml` is a one-line shell script (`exec python3 -m
sml.tools "$@"`). `workstation/.simula.always.sh` puts
`sml/tools/bin` on `PATH` so `sml <tool> ...` works in any container
shell without extra setup. `python3 -m sml.tools <tool> ...` is the same
dispatch without relying on `PATH`.

`sml/tools/__main__.py` is the dispatcher: it scans `sml/tools/` for
immediate subdirectories that have a `__main__.py` (that's the whole
"tool" contract — see below), and forwards `sml <tool> [args...]` to
that tool's `main(argv)`.

- `sml` with no arguments lists the tools it found.
- `sml <unknown-tool> ...` prints an error plus the same tool list.
- `sml <tool> ...` imports `sml.tools.<tool>.__main__` and calls its
  `main(argv)`, returning whatever exit code that tool returns.

## Adding a new tool

Drop a package at `sml/tools/<name>/` with a `__main__.py` exposing:

```python
def main(argv: list[str] | None = None) -> int:
    ...

if __name__ == "__main__":
    sys.exit(main())
```

That's the entire contract — no registry, no import list to edit. As
soon as `sml/tools/<name>/__main__.py` exists, `sml <name> ...` and
`python3 -m sml.tools.<name> ...` both work, and `<name>` shows up in
`sml`'s tool list.

See `sml/tools/mqttcli/` for a worked example (and its own
`README.md` for that tool's usage).

## Tools

- [`mqttcli`](mqttcli/README.md) — MQTT `sub`/`pub` CLI for debugging simulator traffic.
