# TUI Stress-Test & Exploration Prompt

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [TUI Stress-Test & Exploration Prompt](#tui-stress-test-&-exploration-prompt)
  - [System under test](#system-under-test)
  - [How to use this prompt](#how-to-use-this-prompt)
  - [Part A — `t81 studio` systematic walkthrough](#part-a-—-`t81-studio`-systematic-walkthrough)
    - [A1  Navigation and startup](#a1--navigation-and-startup)
    - [A2  REPL view](#a2--repl-view)
    - [A3  Compiler view](#a3--compiler-view)
    - [A4  Determinism view](#a4--determinism-view)
    - [A5  CanonFS view](#a5--canonfs-view)
    - [A6  Axion view](#a6--axion-view)
    - [A7  Trace view](#a7--trace-view)
    - [A8  Command palette (Ctrl+P)](#a8--command-palette-ctrl+p)
    - [A9  Scrolling edge cases](#a9--scrolling-edge-cases)
    - [A10  'q' and Escape exit behavior](#a10--'q'-and-escape-exit-behavior)
  - [Part B — `t81 agent` systematic walkthrough](#part-b-—-`t81-agent`-systematic-walkthrough)
    - [B1  Startup and session management](#b1--startup-and-session-management)
    - [B2  Slash commands](#b2--slash-commands)
    - [B3  Model name parsing from `/infer`](#b3--model-name-parsing-from-`infer`)
    - [B4  Conversation scrolling](#b4--conversation-scrolling)
    - [B5  Session persistence edge cases](#b5--session-persistence-edge-cases)
    - [B6  Exit behavior](#b6--exit-behavior)
  - [Part C — `t81 ui` launcher](#part-c-—-`t81-ui`-launcher)
  - [Part D — Adversarial and boundary scenarios](#part-d-—-adversarial-and-boundary-scenarios)
    - [D1  Shell injection via file paths](#d1--shell-injection-via-file-paths)
    - [D2  Terminal resize](#d2--terminal-resize)
    - [D3  Rapid key mashing](#d3--rapid-key-mashing)
    - [D4  Unicode and multi-byte input](#d4--unicode-and-multi-byte-input)
    - [D5  Extremely long input strings](#d5--extremely-long-input-strings)
  - [Observation log template](#observation-log-template)
  - [Known fixed issues (do not re-report)](#known-fixed-issues-do-not-re-report)

<!-- T81-TOC:END -->


Use this document as a prompt to an AI agent (or a human tester) to systematically
explore, stress-test, and document the behavior of the T81 TUI (`t81 studio` and
`t81 agent`).  The goal is to discover crashes, visual glitches, logic errors, and
UX friction by simulating both realistic and adversarial user sessions.

---

## System under test

```
t81 studio   # Human Operator Interface — 7-panel navigation TUI
t81 agent    # AI-Native Agentic Interface — slash-command conversation TUI
t81 ui       # Interactive launcher (choose between the two above)
```

Both frontends share the `t81::tui` library (`tooling/tui/`).

---

## How to use this prompt

Feed the sections below to an AI agent with shell/file-system access.  Ask it to:

1. Work through every scenario in order.
2. Record each observation (pass / fail / unexpected behavior) in a structured log.
3. Report the root cause of every failure and suggest a fix.

---

## Part A — `t81 studio` systematic walkthrough

### A1  Navigation and startup

```
SCENARIO: Cold start
ACTION:   Run `t81 studio`
EXPECT:   Status bar shows [VM Tier 1], Axion mode, Trace: —
          "Workspace" view is selected in the sidebar
          Version string and Axion status are printed in the output log
          No crash, no garbled rendering
```

```
SCENARIO: Navigate all 7 sidebar entries
ACTION:   Press ArrowDown 6 times and then 6 times ArrowUp
EXPECT:   Each view label (Workspace/Compiler/Determinism/CanonFS/Axion/Trace/REPL)
          activates without error or blank screen
```

```
SCENARIO: Navigate past the end and before the beginning of the sidebar
ACTION:   Hold ArrowDown for 20 presses, then ArrowUp for 20 presses
EXPECT:   Selection stays clamped at the last / first entry; no wrap-around crash
```

---

### A2  REPL view

```
SCENARIO: Basic expression evaluation
ACTION:   Navigate to REPL, type `print(1 + 1)`, press Enter
EXPECT:   "-> print(1 + 1)" appears in the log followed by t81 code output
          Input field is cleared after submission
```

```
SCENARIO: Empty input ignored
ACTION:   Press Enter with an empty REPL field
EXPECT:   No command is dispatched; log is unchanged
```

```
SCENARIO: Multi-line output fits in scrollable log
ACTION:   Type an expression that produces 30+ lines of output (e.g. a loop)
EXPECT:   Log auto-scrolls to the bottom; PgDn/PgUp scroll through all lines
          Scroll indicator shows correct L<start>-<end>/<total>
```

```
SCENARIO: Expression with shell-special characters
ACTION:   Type  `print("hello & world")` and press Enter
EXPECT:   The string is written to a temp file first (not interpolated into a shell
          command); output is correct or a compile error — no shell injection side-effect
```

```
SCENARIO: Rapid-fire expressions (hold Enter)
ACTION:   Type a short expression; hold Enter for 10 rapid submits
EXPECT:   Each is processed sequentially; UI remains responsive; no duplicate log lines
```

---

### A3  Compiler view

```
SCENARIO: Build a valid file
ACTION:   Navigate to Compiler, type the path of a known-good .t81 file, press Enter
EXPECT:   "$ t81 code build <path>" appears in log followed by compiler output
```

```
SCENARIO: Build a nonexistent file
ACTION:   Type `/tmp/does_not_exist.t81`, press Enter
EXPECT:   Error message appears in log; no crash; UI stays usable
```

```
SCENARIO: Run shortcut (Ctrl+R)
ACTION:   Type a valid path, press Ctrl+R
EXPECT:   "$ t81 code run <path>" dispatched (not build)
```

```
SCENARIO: Syntax-check shortcut (Ctrl+K)
ACTION:   Type a path with a syntax error, press Ctrl+K
EXPECT:   "$ t81 code check <path>" dispatched; error report shown
```

```
SCENARIO: Path with spaces
ACTION:   Type `/tmp/my file.t81`, press Enter
EXPECT:   The file path is passed as a single argument; the command does not split
          on the space and execute two separate commands
```

```
SCENARIO: Path containing semicolons / backticks
ACTION:   Type `/tmp/foo.t81; echo INJECTED`, press Enter
EXPECT:   The semicolon is part of the file path argument, not a shell separator;
          "INJECTED" does not appear in the log; exec_command quotes arguments
          (or documents that the caller is responsible)
```

---

### A4  Determinism view

```
SCENARIO: Hash a known file
ACTION:   Navigate to Determinism, type the path of a compiled .t81 artifact, Enter
EXPECT:   "$ t81 determinism hash <path>" runs; hash appears in log
```

```
SCENARIO: Hash the same file twice
ACTION:   Submit the same path twice in a row
EXPECT:   Identical hash output both times; status bar Trace field updates on first hash
```

```
SCENARIO: Nonexistent file
ACTION:   Type a path that does not exist, Enter
EXPECT:   Error message, no crash
```

---

### A5  CanonFS view

```
SCENARIO: List artifacts
ACTION:   Navigate to CanonFS
EXPECT:   Left pane shows artifact list; if empty, shows the placeholder message;
          no crash on empty list
```

```
SCENARIO: Navigate artifact list
ACTION:   Press ArrowDown through all entries; observe detail pane
EXPECT:   Detail pane updates on each selection change (may block briefly on first
          call — note the latency); no crash when at list boundaries
```

```
SCENARIO: Refresh
ACTION:   Press 'r' while in CanonFS view
EXPECT:   `t81 canonfs list` is re-executed; display updates; detail cache is cleared
```

```
SCENARIO: Long hash / label truncation
ACTION:   If any artifact has a very long hash or label, observe the display
EXPECT:   Hash is truncated to 12 chars + "…"; label does not overflow the pane
```

---

### A6  Axion view

```
SCENARIO: Status display
ACTION:   Navigate to Axion view
EXPECT:   Lines from `t81 axion status` appear, color-coded:
          DENY → red, WARN → yellow, ALLOW → green, error → red
```

```
SCENARIO: Scroll through long status output
ACTION:   If output is long, press j / k and PgUp / PgDn
EXPECT:   Scroll indicator updates correctly; content is not duplicated
```

```
SCENARIO: Refresh
ACTION:   Press 'r'
EXPECT:   `t81 axion status` is re-executed; scroll resets to top
```

---

### A7  Trace view

```
SCENARIO: Summary mode (single file)
ACTION:   Navigate to Trace, type a trace file path in Trace A, press Enter; leave B empty
EXPECT:   Summary output from `t81 trace summary <path>` appears; mode label shows
          "[summary mode]"
```

```
SCENARIO: Diff mode (two files)
ACTION:   Type path in Trace A; type a second path in Trace B; press Enter on B
EXPECT:   Mode switches to "[diff mode]"; `t81 trace diff <a> <b>` output shown;
          diverged lines highlighted in red
```

```
SCENARIO: Clear Trace B, refresh
ACTION:   Delete Trace B content; press Enter on Trace B
EXPECT:   Mode reverts to "[summary mode]"; summary for Trace A shown
```

```
SCENARIO: Both fields empty, Enter
ACTION:   Leave both fields empty, press Enter on Trace A
EXPECT:   Placeholder message: "Enter a trace file path above, then press Enter."
```

---

### A8  Command palette (Ctrl+P)

```
SCENARIO: Open and close
ACTION:   Press Ctrl+P; then Escape
EXPECT:   Palette overlay appears, then closes; query cleared; underlying view unchanged
```

```
SCENARIO: Type to filter
ACTION:   Press Ctrl+P; type "axion"
EXPECT:   Only entries whose name contains "axion" are shown (case-insensitive)
```

```
SCENARIO: Navigate results with arrows
ACTION:   Press Ctrl+P; navigate to result index 3 with ArrowDown ×3; press Enter
EXPECT:   Correct (index-3) command is executed; palette closes; log shows "$ <cmd>"
```

```
SCENARIO: Narrow filter after navigating — CRITICAL
ACTION:   Press Ctrl+P; press ArrowDown ×9 (palette_selected=9 with 34 entries visible);
          type "axion" (narrows to ~5 entries); press Enter
EXPECT:   Selected command is one of the ~5 visible axion entries (clamped to list end);
          NO out-of-bounds access; NO crash
```

```
SCENARIO: Filter to no results, press Enter
ACTION:   Press Ctrl+P; type "xyzzy_no_match"; press Enter
EXPECT:   "No results" message shown; Enter does nothing; no crash
```

```
SCENARIO: Execute a command from palette
ACTION:   Press Ctrl+P; type "version"; press Enter on "version" entry
EXPECT:   `t81 version` runs; output appears in Workspace log; view switches to Workspace
```

---

### A9  Scrolling edge cases

```
SCENARIO: Scroll before any log output
ACTION:   Press j / k / PgUp / PgDn immediately after launch
EXPECT:   No crash; scroll stays at 0 (clamped to valid range)
```

```
SCENARIO: Scroll past the end
ACTION:   Press PgDn 100 times
EXPECT:   Scroll stops at last page; indicator shows correct final range
```

```
SCENARIO: j/k inside input fields must NOT scroll
ACTION:   Focus the REPL input; type j and k
EXPECT:   Characters are inserted into the input buffer, NOT interpreted as scroll commands
```

```
SCENARIO: J/K (shift) always scrolls
ACTION:   Focus the REPL input; press Shift+J (capital J)
EXPECT:   Scroll by 10 lines occurs (input_focused check should NOT block J/K)
```

---

### A10  'q' and Escape exit behavior

```
SCENARIO: q exits from non-input view
ACTION:   Navigate to Workspace; press 'q'
EXPECT:   TUI exits cleanly; shell prompt returns
```

```
SCENARIO: q inside input field must NOT exit
ACTION:   Focus REPL input; type 'q'
EXPECT:   'q' is inserted into input, not treated as quit
```

```
SCENARIO: Escape closes palette first
ACTION:   Press Ctrl+P to open palette; press Escape
EXPECT:   Palette closes; TUI does NOT exit
```

```
SCENARIO: Escape exits from normal view
ACTION:   Ensure palette is closed; press Escape
EXPECT:   TUI exits
```

---

## Part B — `t81 agent` systematic walkthrough

### B1  Startup and session management

```
SCENARIO: Fresh start (no flags)
ACTION:   Run `t81 agent`
EXPECT:   Welcome message appears; context panel shows Model: None, Tier: 1, Axion: <live>
          Session path auto-set to ~/.t81/sessions/<timestamp>.jsonl
          File is created on disk after first command
```

```
SCENARIO: Resume a session
ACTION:   Run `t81 agent --resume ~/.t81/sessions/<timestamp>.jsonl`
EXPECT:   Previous conversation history is loaded and displayed
          Welcome message says "Session resumed from …"
          history_scroll set to show most-recent messages
```

```
SCENARIO: Resume nonexistent file
ACTION:   Run `t81 agent --resume /tmp/no_such_session.jsonl`
EXPECT:   Starts fresh (empty history); no crash; load_session returns false silently
```

```
SCENARIO: Custom session path
ACTION:   Run `t81 agent --session /tmp/my_test_session.jsonl`
EXPECT:   On first turn, /tmp/my_test_session.jsonl is created
          /save confirms "Session saved to /tmp/my_test_session.jsonl."
```

---

### B2  Slash commands

For each command below, type the command, press Enter, and verify the response.

```
/help
EXPECT: Formatted help text listing all slash commands; no error

/axion
EXPECT: Output of `t81 axion status`; Context panel Axion field updates

/tier 3
EXPECT: "Cognitive tier set to 3."; Context panel Tier field updates to 3

/tier 0
EXPECT: "[/tier requires an integer 1-5]"; tier unchanged

/tier abc
EXPECT: "[/tier requires an integer 1-5]"; tier unchanged

/compile tests/hello.t81          (use a real .t81 path)
EXPECT: Compiler output; Context panel Trace hash updates if hash in output

/run tests/hello.t81
EXPECT: Execution output; Trace hash may update

/check tests/hello.t81
EXPECT: Syntax check output; no Trace hash change

/disasm tests/hello.t81
EXPECT: Disassembly output

/hash tests/hello.t81
EXPECT: Hash output; Trace context updates

/trace tests/hello.t81
EXPECT: Trace output; Trace context updates

/policy list
EXPECT: Output of `t81 policy list`

/policy status
EXPECT: Axion status output; axion_mode updated

/policy
EXPECT: Same as `/policy list` (default arg)

/write /tmp/test_agent_write.txt hello world
EXPECT: "Written 11 bytes to /tmp/test_agent_write.txt."
        File exists with content "hello world"

/write /tmp/
EXPECT: "[/write requires <file> <content>]" or file-open error

/save
EXPECT: "Session saved to <path>."

/clear
EXPECT: History cleared; welcome back message shown; scroll at top

/trits
EXPECT: "Trit probability display ON." (toggle)
        Context panel now shows the trit bar (or "run /infer to populate")

/trits
EXPECT: "Trit probability display OFF."

/unknown_cmd
EXPECT: "[unknown command /unknown_cmd — try /help]"
```

---

### B3  Model name parsing from `/infer`

```
SCENARIO: Model name with path separator
ACTION:   /infer /models/my_model.gguf --policy strict hello
EXPECT:   Context panel Model shows "my_model.gguf" (basename after last '/')

SCENARIO: Model name with no path separator
ACTION:   /infer my_model.gguf --policy strict hello
EXPECT:   Context panel Model shows "my_model.gguf" (full name, no UB from npos+1)

SCENARIO: Empty model name (edge case)
ACTION:   /infer  --policy strict hello   (extra leading space)
EXPECT:   model name token is "--policy" or similar; no crash
```

---

### B4  Conversation scrolling

```
SCENARIO: History scroll after many messages
ACTION:   Send 40 messages; press PgUp several times; verify history is browseable
EXPECT:   Scroll indicator shows correct range; messages are not duplicated

SCENARIO: Scroll clamping
ACTION:   Press PgUp 100 times from the top
EXPECT:   Stays at message 1; no negative scroll

SCENARIO: Auto-scroll after new message
ACTION:   Scroll up to see old messages; send a new message
EXPECT:   View auto-scrolls to show the new message at the bottom
```

---

### B5  Session persistence edge cases

```
SCENARIO: Message with escaped characters survives roundtrip
ACTION:   Send: /write /tmp/t.txt say "hello" and \path\to\file
          Then quit and reload the session
EXPECT:   The message text is identical after reload (quotes, backslash, any newlines)

SCENARIO: Very long message (>1000 chars)
ACTION:   Paste or generate a 2000-character message
EXPECT:   Saved and reloaded without truncation; no UI overflow

SCENARIO: Empty message not submitted
ACTION:   Press Enter with empty input
EXPECT:   Nothing is added to history; no empty messages saved to JSONL
```

---

### B6  Exit behavior

```
SCENARIO: /quit does not save a duplicate
ACTION:   Send one command; type /quit
EXPECT:   TUI exits; session file has exactly 3 lines (system welcome + user + agent)

SCENARIO: Escape exits without error
ACTION:   Press Escape
EXPECT:   TUI exits; final save is written (quit_requested stays false on Escape)

SCENARIO: /q shortcut
ACTION:   Type /q, Enter
EXPECT:   Identical to /quit behavior
```

---

## Part C — `t81 ui` launcher

```
SCENARIO: Launch and select Studio
ACTION:   Run `t81 ui`; confirm "Studio" is highlighted; press Enter
EXPECT:   run_studio() launches with no args; all studio views available

SCENARIO: Launch and select Agent
ACTION:   Run `t81 ui`; press ArrowDown; select "Agent"; press Enter
EXPECT:   run_agent() launches; session auto-path created

SCENARIO: Press Escape at launcher
ACTION:   Run `t81 ui`; press Escape immediately
EXPECT:   Process exits without launching either TUI
```

---

## Part D — Adversarial and boundary scenarios

### D1  Shell injection via file paths

For each input field that passes user text to `exec_command`, try:

```
Input:  /tmp/foo.t81; touch /tmp/INJECTED
Input:  $(touch /tmp/INJECTED_2)
Input:  /tmp/`touch /tmp/INJECTED_3`.t81
```

**Expected:** None of the `/tmp/INJECTED*` files are created.  The characters are
treated as part of the file path, not as shell metacharacters.

**Note:** `exec_command` uses `popen(cmd.c_str(), "r")` which invokes a shell.
If shell injection is confirmed, document exactly which field is vulnerable and
what the mitigation requires (argument quoting or `execvp`-based spawn).

### D2  Terminal resize

```
ACTION:   Launch studio; resize the terminal from 220×50 to 80×24; then back
EXPECT:   Layout reflows correctly at each size; no garbled output; status bar
          remains on the last line
```

### D3  Rapid key mashing

```
ACTION:   While in Compiler view, hold down Enter for 5 seconds (rapid submits of
          the current file path)
EXPECT:   Each submission is queued and processed; UI does not deadlock; log grows
          monotonically; scroll indicator remains accurate
```

### D4  Unicode and multi-byte input

```
ACTION:   In REPL, type: print("日本語テスト")
EXPECT:   Expression written to temp file and executed; no crash;
          output may be garbled if locale is not set but no crash

ACTION:   In palette search, type "→" (right arrow Unicode)
EXPECT:   No crash; filter either matches nothing or falls through to no-results
```

### D5  Extremely long input strings

```
ACTION:   Paste a 10,000-character string into the REPL input; press Enter
EXPECT:   String is written to /tmp/t81_studio_repl.t81 and submitted;
          log handles the (likely large) output; no crash or OOM
```

---

## Observation log template

Copy this table and fill it in for each scenario:

| ID  | Scenario summary          | Outcome          | Notes / Root cause |
|-----|--------------------------|------------------|--------------------|
| A1  | Cold start               | PASS / FAIL / ?  |                    |
| A2a | REPL basic eval          | PASS / FAIL / ?  |                    |
| …   | …                        | …                |                    |

---

## Known fixed issues (do not re-report)

| Issue | Fixed in | Details |
|-------|----------|---------|
| OOB access on palette Enter after filtering | studio.cpp | `palette_selected` now clamped to `n-1` before `filtered[palette_selected]` |
| Double-lock in `push_log` | studio.cpp | Single `lock_guard` covers both push and scroll-update |
| UB from `rfind('/') + 1` on no-slash model name | agent.cpp | Explicit `npos` check before `substr` |
