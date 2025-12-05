# Release & Audit Manual

This manual explains how to capture, archive, and verify Axion/Cli artifacts so auditors or downstream reviewers can replay deterministic traces without rebuilding the codebase. It ties together the CLI, Axion trace, policy, and runtime manuals already published.

## 1. Artifact collection checklist

Each release (CI run, patch release, or proof submission) should publish:

1. **Binary & policy artifacts**
   - `build/t81` and helper targets (`axion_policy_runner`, `axion_policy_trace`) built from `cmake --build build --parallel`.
   - `policy/guards.axion` (or your custom policy) stored alongside the release so policy tests can be rerun.
2. **Axion logs**
   - Run `./scripts/capture-axion-trace.sh` (or `build/axion_policy_runner` directly) to produce `build/artifacts/axion_policy_runner.log`.
   - Archive the log output referencing canonical strings like `stack frame allocated`, `AxRead guard segment=stack addr=5`, `enum=Option variant=Some match=pass`.
3. **CI test snippets**
   - Capture the `ctest --test-dir build -R axion_segment_trace_test --output-on-failure` block (includes `bounds fault ...` strings).
   - Include the `axion_policy_match_guard_test`/`axion_policy_segment_event_test` pass logs so reviewers know the guard/segment requirements were satisfied.
4. **CLI transcripts**
   - Save `t81 compile --verbose match_guard.t81 -P policy/guards.axion` output showing the guard/segment `verdict.reason` lines.
   - Record the `t81 repl :trace` output if the release needs REPL verification.

## 2. Automated capture scripts

- `scripts/capture-axion-trace.sh` builds the policy runner target and redirects stdout into `build/artifacts/axion_policy_runner.log`. Run it via CI after every build, and store the log (see `docs/guides/axion-trace.md` §6).
- CI pipelines should rerun `ctest --test-dir build -R axion_segment_trace_test --output-on-failure` so the log snippet is in the test output (and optionally copy/paste the snippet into release notes).  
- Use `t81 compile --verbose` in automation to ensure guard metadata (enum strings) appear before policies execute; capture those transcripts in an audit directory.

## 3. Release note template

When publishing a release, include:

```
Axion policy trace log: build/artifacts/axion_policy_runner.log
Segment trace CI snippet: [ctest output block]
Guard trace CLI transcript: [CLI log file]
```

Reference the RFCs (RFC-0009, RFC-0013, RFC-0019) and manuals in the release note so auditors can locate the exact deterministic semantics backing the trace strings.

## 4. Audit verification steps

1. Download the release artifacts and rerun `cmake --build build --parallel` (optional).  
2. Reexecute `./scripts/capture-axion-trace.sh` and compare the produced log with the archived `axion_policy_runner.log`. They should match line-for-line in terms of `verdict.reason` strings.  
3. Rerun the `axion_policy_*` tests (`ctest --test-dir build -R axion_policy_match_guard_test`) and ensure they pass; failure indicates policy strings changed.  
4. Confirm the release note references `policy/guards.axion`, RFC-0009/0013/0019, and the manuals so future reviewers understand what each artifact guarantees.

## 5. CanonFS & Axion policy runner

- CanonFS writes occur through `AXSET`, meaning Axion can validate metadata before writing; include the corresponding trace strings (`meta slot axion event`, `loop hint ...`) in your release artifacts.  
- The policy runner and CLI `t81 compile --verbose` outputs should align (use `policy/guards.axion` or your policy file) so auditors can verify `require-match-guard`/`require-segment-event` constraints without delving into the VM code.

## 6. Cross-reference matrix

| Artifact | Purpose | RFC/docs link |
| --- | --- | --- |
| `axion_policy_runner.log` | Deterministic guard + segment strings | RFC-0009, RFC-0013 |
| `axion_segment_trace_test` snippet | Bounds faults / segment coverage | RFC-0013 |
| `t81 compile --verbose` transcript | Guard metadata and match payload strings | RFC-0019 |
| `State::axion_log` from REPL `:trace` | Runtime trace accessible from CLI | docs/guides/runtime-observability-manual.md |
| `policy/guards.axion` | Source policy showing requirements | docs/guides/axion-policy-manual.md |
| `axion_policy_gc_trace_test` | Validates GC trace string `interval stack_frames=...` via `(require-axion-event ...)` | RFC-0009, RFC-0013 |

Include this matrix in release artifacts or append it to release notes to help auditors cross-check each entry quickly.
