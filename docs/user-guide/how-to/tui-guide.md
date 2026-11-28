# T81 TUI Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 TUI Guide](#t81-tui-guide)
  - [t81 studio](#t81-studio)
    - [Layout](#layout)
    - [Keyboard shortcuts](#keyboard-shortcuts)
    - [Views](#views)
  - [t81 agent](#t81-agent)
    - [Flags](#flags)
    - [Layout](#layout)
    - [Slash commands](#slash-commands)
    - [Trit-probability display](#trit-probability-display)
    - [Session persistence](#session-persistence)
  - [Building with TUI support](#building-with-tui-support)
  - [Troubleshooting](#troubleshooting)

<!-- T81-TOC:END -->


The T81 toolchain ships two optional text-based user interfaces built with
[FTXUI](https://github.com/ArthurSonzogni/FTXUI):

| Command | Audience | Purpose |
|---------|----------|---------|
| `t81 studio` | Human operators | Dashboard for compilation, tracing, CanonFS, Axion |
| `t81 agent` | AI agents / automation | Stateful conversational interface with session persistence |
| `t81 ui` | Anyone | Interactive launcher to choose between the two |

The TUI layer is **optional**. Rebuild with `-DT81_BUILD_TUI=ON` if not
present in your binary.

---

## t81 studio

The Human Operator TUI provides a seven-pane navigation sidebar. Press
**Ctrl+P** at any time to open the command palette and fuzzy-search any T81
CLI command.

### Layout

```
+----------------+-------------------------------------------+
| Navigation     | Output / Detail pane                      |
|                |                                           |
| > Workspace    |                                           |
| > CanonFS      |                                           |
| > Axion        |                                           |
| > Trace Diff   |                                           |
| > Trace Summary|                                           |
| > Determinism  |                                           |
| > Weights      |                                           |
+----------------+-------------------------------------------+
| [Tier N] | Axion: <mode> | Trace: <hash>    PgUp/PgDn     |
+------------------------------------------------------------+
```

### Keyboard shortcuts

| Key | Action |
|-----|--------|
| `j` / `k` | Scroll log down / up (1 line) |
| `J` / `K` | Scroll log down / up (5 lines) |
| `PgDn` / `PgUp` | Scroll log by half a page |
| `r` | Refresh the current view |
| `Ctrl+P` | Open command palette |
| `Esc` | Exit |

### Views

**Workspace** — The welcome and status overview. Shows the current Axion
mode, VM tier, and most-recent trace hash.

**CanonFS Browser** — Lists CanonFS artifacts. Select an entry to inspect its
metadata and hash in the detail pane. Press `r` to re-fetch.

**Axion Dashboard** — Runs `t81 axion status` and colour-codes each line:
ALLOW (green), WARN (yellow), DENY (red).

**Trace Diff / Trace Summary** — Runs `t81 trace diff` or `t81 trace summary`
and highlights diverged lines in red.

**Determinism** — Placeholder for `t81 determinism hash` output.

**Weights** — Placeholder for `t81 weights info` output.

---

## t81 agent

The AI-Native TUI exposes a conversational, stateful interface optimised for
multi-step reasoning and scripted agent harnesses.

### Flags

```
t81 agent [--resume <path.jsonl>] [--session <path.jsonl>]
```

* `--resume` — Load a previously saved session from a JSONL file.
* `--session` — Override the default auto-save path
  (`~/.t81/sessions/<timestamp>.jsonl`).

### Layout

```
+----------------------------------------+------------------+
| Interaction History                    | Context View     |
|                                        |                  |
| [You]   /compile src/main.t81          | Model: None      |
| [Agent] Compiled → hash 3f8a12...      | Tier:  1         |
|                                        | Axion: Strict    |
|                                        | Trace: 3f8a12…   |
|                                        |------------------|
|                                        | Trit Probs:      |
|                                        | [/trits to enable]
| >> _                                   |                  |
+----------------------------------------+------------------+
| [Tier 1]  |  Axion: Strict  |  Trace: —    PgUp/PgDn     |
+------------------------------------------------------------+
```

### Slash commands

| Command | Description |
|---------|-------------|
| `/compile <file>` | Compile `.t81` → TISC; updates trace hash |
| `/run <file>` | Compile and execute |
| `/check <file>` | Syntax-check without compiling |
| `/disasm <file>` | Disassemble TISC bytecode |
| `/trace <file>` | Show execution trace |
| `/hash <file>` | Compute determinism hash; updates context panel |
| `/infer <model> --policy <p> <prompt>` | Run governed `llama-run` inference; updates trit-probability display |
| `/axion` | Refresh Axion policy status |
| `/policy list\|validate <f>\|status` | Policy management |
| `/allow <hash>` | Append hash to active Axion whitelist |
| `/write <file> <content>` | Write content to a file on disk |
| `/tier <n>` | Set cognitive tier display (1–5) |
| `/trits` | Toggle trit-probability side-panel |
| `/save` | Manually save session |
| `/clear` | Clear conversation history |
| `/quit` | Exit |

### Trit-probability display

After running `/infer`, toggle `/trits` to reveal a live bar chart:

```
+1 ########--------   (P(+1) derived from token distribution)
 0 ###-------------
-1 ##--------------
n=128 tokens
```

The distribution is derived from `token_id % 3` of the tokens produced by
`t81 internal llama-run`. It provides a proxy signal for the ternary
character of the current inference run.

### Session persistence

Sessions are auto-saved to `~/.t81/sessions/<timestamp>.jsonl` after every
turn. Use `--resume` to reload a previous session.

To share or inspect a session file:

```bash
cat ~/.t81/sessions/2026-03-09T14-30-00.jsonl
```

Each line is a JSON object:
```json
{"role":"user","text":"..."}
{"role":"agent","text":"..."}
```

---

## Building with TUI support

The TUI is disabled by default in most build presets. Enable it:

```bash
cmake -S . -B build -DT81_BUILD_TUI=ON
cmake --build build --target t81
```

FTXUI v5 is fetched automatically via CMake `FetchContent`.

---

## Troubleshooting

**`TUI frontends not available`** — Rebuild with `-DT81_BUILD_TUI=ON`.

**Garbled output** — Ensure your terminal supports UTF-8 and 256-color ANSI.
Use a modern emulator (iTerm2, Alacritty, WezTerm, Windows Terminal).

**`t81 studio` hangs on refresh** — The `r` key invokes subprocess calls via
`popen`. If the `t81` binary is not on `$PATH`, these will timeout silently.
Ensure `build/t81` is on your `$PATH` before launching the TUI.
