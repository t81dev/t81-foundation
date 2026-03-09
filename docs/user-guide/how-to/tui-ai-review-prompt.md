
You are a senior C++ engineer and UX reviewer specialising in terminal UI applications
built with FTXUI.  You have been given the complete source of the **T81 TUI layer** —
a two-frontend terminal application for the T81 ternary virtual-machine toolchain:

| Command       | Purpose                                                |
|---------------|--------------------------------------------------------|
| `t81 studio`  | Human operator dashboard (7-view sidebar navigation)  |
| `t81 agent`   | AI-native agentic interface (slash-command REPL)       |
| `t81 ui`      | Interactive launcher that selects between the two      |

**Source files to read (attach or paste them in full):**

```
tooling/tui/common.hpp      — shared types: CommandEntry, SessionState, Message
tooling/tui/common.cpp      — exec_command, all_commands, save/load_session
tooling/tui/studio.cpp      — run_studio(): 7-view TUI, command palette, key handler
tooling/tui/agent.cpp       — run_agent(): conversational TUI, slash-command dispatch
```

---

### Your task

Work through the **five phases** below in order.  For each finding produce an entry
in the structured log defined at the end of this prompt.

---

### Phase 1 — Static code review (read the source, no terminal needed)

Examine every function for:

1. **Memory safety** — out-of-bounds indexing, iterator invalidation, use-after-free,
   signed/unsigned comparison, integer overflow or wrap-around.
2. **Concurrency correctness** — every shared variable protected by `log_mutex` /
   `history_mutex` must be accessed only while holding the lock.  Look for:
   - Lock held across a blocking call (deadlock risk).
   - Variable read or written both inside and outside a lock (data race).
   - Multiple separate locks needed to complete one logical operation (TOCTOU).
3. **Shell injection** — `exec_command(cmd)` calls `popen(cmd.c_str(), "r")`.
   Trace every call site.  Does any user-controlled string reach `cmd` without
   sanitisation?  If so, show the exact path and a minimal proof-of-concept input.
4. **FTXUI renderer contract violations** — the renderer lambda must be pure and
   non-blocking.  Flag any call to `exec_command`, file I/O, or mutex acquisition
   that happens unconditionally inside the renderer on every frame.
5. **Scroll / index arithmetic** — all scroll offsets and list indices must be
   clamped before use.  Verify every `arr[idx]` access where `idx` comes from user
   navigation state.
6. **Session JSONL parser** — `load_session` uses a hand-rolled parser.  Construct
   three inputs that could break it: (a) a message whose text contains `"` nested
   inside a JSON string value, (b) a line with `role` and `text` keys in the
   opposite order, (c) a zero-byte file.

---

### Phase 2 — Simulated user sessions

Without running the code, trace through the following six sessions mentally.
For each one, identify the exact line(s) of code that execute, the values of key
variables after each step, and whether the outcome is correct or erroneous.

**Session S1 — Palette OOB (adversarial)**
1. User opens command palette (Ctrl+P).  `palette_selected = 0`.  All 34 commands
   visible.
2. User presses ArrowDown nine times.  `palette_selected = 9`.
3. User types `"axion"` into the search field.  Filtered list shrinks to ≤ 5 entries.
4. User presses Enter.
→ What is `filtered[palette_selected]`?  Is it in bounds?  What happens?

**Session S2 — REPL temp-file collision**
1. Two `t81 studio` processes run concurrently on the same machine.
2. Both users type an expression and press Enter at the same time.
→ Both write to `/tmp/t81_studio_repl.t81`.  What is the race?  What is the worst
  observable outcome?  Suggest a minimal fix that keeps the existing approach.

**Session S3 — CanonFS blocking render**
1. User navigates to CanonFS view.
2. User presses ArrowDown to select a new artifact.
3. FTXUI calls the renderer lambda.
→ Trace the code path.  Is `exec_command("t81 canonfs show …")` called inside the
  renderer?  How long could it block?  What does the user observe?

**Session S4 — Agent model name with no path separator**
1. User types `/infer my_model.gguf --policy strict hello world`.
2. The code runs `m.rfind('/') + 1` where `m = "my_model.gguf"`.
→ What does `rfind` return?  What does `+1` produce?  What is `m.substr(result)`?
  Is this defined behaviour in C++?

**Session S5 — Session resume with escaped content**
1. User sends the message: `say "hello" and \path`
2. Session is auto-saved.
3. User resumes with `--resume`.
→ Walk through `escape_json_string` for this input.  Then walk through the
  `extract` lambda in `load_session`.  Does the roundtrip produce the original string?

**Session S6 — push_log from a background thread (hypothetical)**
This is a hypothetical future state: imagine `exec_command` is moved to a worker
thread.  Trace the original `push_log` implementation (before any fixes):

```cpp
auto push_log = [&](const std::string& s) {
    for (const auto& l : split_lines(s)) {
        std::lock_guard<std::mutex> lk(log_mutex);   // acquired per element
        output_log.push_back(l);
    }
    // Auto-scroll to bottom
    std::lock_guard<std::mutex> lk(log_mutex);        // separate acquisition
    log_scroll = std::max(0, static_cast<int>(output_log.size()) - 22);
};
```

→ Between the two lock acquisitions the renderer can run.  What can it observe?
  Is `log_scroll` consistent with `output_log.size()`?  What is the worst-case
  visible artefact for the user?

---

### Phase 3 — UX and ergonomic review

Evaluate the interfaces from the perspective of an operator using them daily.
Consider:

1. **Discoverability** — is it obvious how to reach each view / command?  Are
   keyboard shortcuts consistent across views?  Are they documented in the UI?
2. **Feedback latency** — which operations block the UI thread with no visual
   indicator?  How long might a user wait with a frozen screen?
3. **Error visibility** — when `exec_command` fails (process not found, permission
   denied), what does the user see?  Is `"[error: could not launch subprocess]"`
   vs `"[no output]"` distinguishable from a silent success?
4. **Input field confusion** — in Studio, a single `file_component` is shared
   between Compiler and Determinism views.  What happens to its contents when
   the user switches views mid-edit?
5. **Palette scroll cap** — the palette shows at most 10 entries.  If 20 entries
   match the query, are entries 11-20 reachable?  What would it take to add
   palette scrolling?
6. **Agent context panel width** — the context panel is fixed at 30 columns.
   On an 80-column terminal, how much space remains for the conversation?
   At what terminal width does the layout become unusable?
7. **Session path display** — the full session path is shown in the context panel
   using `paragraph()`.  On a 30-column panel, how does a long path like
   `/Users/operator/.t81/sessions/2026-03-09T14-30-00.jsonl` wrap?

---

### Phase 4 — Snapshot test coverage audit

Read `tests/cpp/tui_snapshot_test.cpp`.  For each of the four (now six) test
functions, answer:

1. What invariant does it verify?
2. What does it NOT cover that a production-grade test suite should?
3. Write one additional `T81_TEST_CHECK` assertion (or a new test function) that
   would catch a regression in the area you identified.

Pay particular attention to:
- JSONL parser behaviour on adversarial input (from Phase 1 item 6).
- The `filter_palette` function (currently untested).
- The `split_lines` function (currently untested).
- The `scrollable_log` scroll-indicator string format.

---

### Phase 5 — Prioritised recommendations

Produce a ranked list of changes.  Each entry must use the format below.

```
SEVERITY: Critical | High | Medium | Low
CATEGORY: Correctness | Safety | Performance | UX | Testability
FILE:     <file>:<line range>
TITLE:    <one sentence>
DETAIL:   <two to four sentences explaining the problem>
FIX:      <concrete code change or design change — be specific>
EFFORT:   Trivial (<30 min) | Small (half day) | Medium (1-2 days) | Large (>2 days)
```

Include at minimum:
- Every correctness or safety issue found in Phase 1.
- Every session in Phase 2 that produced an incorrect or undefined outcome.
- The top three UX issues from Phase 3.
- At least two new tests from Phase 4.

---

### Output format

Return your analysis as a single Markdown document with five top-level sections
matching the five phases.  Within each section use the sub-structure described
above.  Begin Phase 5 with a one-paragraph executive summary before the ranked list.

Do not truncate findings.  If a section is long, include all of it.
