# sml cc — Command Card

Powerful command-card library: **one card per file (YAML)** + **fzf panel** + **task-spooler background queue**.
Replace manually typed commands with text cards that stay reproducible even after the container restarts.

---

## Design conventions (frozen)

| Item | Decision |
|---|---|
| Card format | YAML, one card per file, stored in `/root/sml/tools/cc/cards/` |
| Parameterization | No placeholders; different parameters = different cards, composed via `steps` |
| `type` | **Inferred**: has `run` → action; has `steps` → pipeline (no `type` field is written) |
| `id` | **Equals the filename** (without `.yaml`); the engine locates cards directly by filename |
| `steps` | **Pure references** to other card ids, executed in order |
| pipeline failure | **Fail-fast**, stops immediately and reports which step it stopped at; `--from <step>` resumes |
| Failure logs | Background runs are written to `/tmp/sml-cc-logs/runs/<ts>-<id>.log` |
| `queue` | Each value → its own `TS_SOCKET=/tmp/ts_<queue>`, **true parallelism across queues, serial within a queue** |

---

## Commands

| Command | Effect |
|---|---|
| `sml cc` | Open the fzf panel (default action, most frequent) |
| `sml cc run <id>` | Run a card / pipeline in the foreground |
| `sml cc run <id> --from <step>` | Resume a pipeline from a given step |
| `sml cc bg <id>` | Submit to the ts background queue per the card's `queue` |
| `sml cc find <kw>` | Fuzzy search by id/desc/tags |
| `sml cc show <id>` | Print a single card's details (also used as the fzf preview) |
| `sml cc ls` | Machine-readable list (TSV, for scripts/pipes) |

**fzf panel keys**: `Enter` run in foreground · `Alt-Enter` run in background (bg) · `Tab` multi-select an ad-hoc chain · `Esc` quit.

---

## Card examples

**action:**
```yaml
id: boot-emulator          # = filename
desc: Boot the emulator and load the default scenario   # shown in the fzf list
tags: [sim, boot]          # optional, for search/classification
queue: sim                 # optional, bg submits to /tmp/ts_sim
desc_long: |               # optional, shown in preview details
  Boot QUTS and load the default scenario.
run: |                     # required
  quts boot --scene default
```

**pipeline:**
```yaml
id: full-boot
desc: Full boot sequence (boot → flash → validate)
queue: sim
steps:                     # pure references to other card ids, run in order, fail-fast
  - boot-emulator
  - flash-image
  - validate-scenario
```

Only 3 required fields: `id` / `desc` / (`run` or `steps`).

---

## What `queue` does

`queue` decides which **independent socket** ts queue a card enters when run with `bg`:

- Cards sharing the same `queue` run **serially** (tasks contending for the same resource, e.g. all booting the emulator, never overlap)
- Cards in different `queue`s run **in parallel** (unrelated tasks, e.g. `sim` and `build` each run on their own)
- Defaults to `default`

Group cards by "things that contend for the same resource": `sim` / `build` / `flash` / `default`, etc.
Inspect a queue with: `TS_SOCKET=/tmp/ts_sim tsp`

---

## Layout

```
~/.sml/
├── cards/        # cards (recommended as its own git repo, not committed to the project repo)
├── runs/         # background run logs (can be .gitignored)
```

Engine code lives at `sml/tools/cc/` in this repo; invoke it as `sml cc ...` or
`python3 -m sml.tools.cc ...`.
