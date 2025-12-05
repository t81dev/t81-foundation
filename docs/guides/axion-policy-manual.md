# Axion Policy Authoring Manual

This guide covers how to write, embed, and debug Axion Policy Language (APL) policies so your programs stay compliant with **RFC-0009**, **RFC-0020**, and **RFC-0019**. It explains each `require-*` predicate, shows diagnostics from policy regressions, and links to the provided sample files such as `policy/guards.axion`.

## 1. APL policy anatomy

APL is a deterministic s-expression DSL embedded into `tisc::Program.axion_policy_text`. A minimal policy looks like:

```
(policy
  (tier 1)
  (require-match-guard
    (enum Option)
    (variant Some)
    (payload i32)
    (result pass)))
```

Key fields:

- `tier`: maximum cognitive tier (RFC-0009 §2.2). Higher tiers enable more permissive operations but require stronger metadata/logging.
- `require-match-guard`: enforces that the Axion trace contains an enum guard string matching the given metadata (RFC-0009 §2.5 / RFC-0019). Provide `(enum <name>)`, `(variant <name>)`, optional `(payload <type>)`, and `(result pass|fail)` to match the `verdict.reason`.
- `require-segment-event`: asserts the runtime emitted a segment trace string from RFC-0020 (e.g., stack frame allocation, AxRead guard). Include `(segment <name>)` and `(action "...")`, optionally `(addr <value>)`.
- `require-loop-hint`: ensures loop metadata entries appear before privileged opcodes. The policy engine looks for the string described in RFC-0009 §2.5/§2.6.

Include other clauses like `max-stack`, `allow-opcode`, `deny-shape`, and `require-proof` as needed; the policy parser (`include/t81/axion/policy.hpp`) accepts integers, symbols, and string literals so you can place quotes around the `action` text.

## 2. Sample policy files

Use the shipped `policy/guards.axion` to begin. Its contents mirror the regressions:

```
(policy
  (tier 1)
  (require-match-guard
    (enum Option)
    (variant Some)
    (payload i32)
    (result pass))
  (require-segment-event
    (segment stack)
    (action "stack frame allocated"))
  (require-segment-event
    (segment stack)
    (action "AxRead guard")))
```

It demands the guard string `enum=Option variant=Some match=pass guard=pass payload=i32` and the segment strings recorded by RFC-0020 (`stack frame allocated` and `AxRead guard`). Embed this policy via `-P policy/guards.axion` during `t81 compile` or run `axion_policy_runner` directly; the policy engine will automatically reject the program if the strings are missing.

### 2.1 `require-axion-event` for GC/trace verification

Some policies need to assert the presence of specific Axion events beyond guards and segments—for example, that a GC cycle occurs with the canonical `interval stack_frames=...` trace string. Use:

```
(policy
  (tier 1)
  (require-axion-event
    (reason "interval stack_frames=")))
```

After the GC trace strings landed (see `docs/guides/axion-trace.md` §2), you can also assert the heap compaction and relocation entries before allowing subsequent heap/loop actions:

```
(policy
  (tier 1)
  (require-axion-event
    (reason "heap compaction heap_frames="))
  (require-axion-event
    (reason "heap relocation from=")))
```

Both predicates look for the canonical `verdict.reason` that `scripts/capture-axion-trace.sh` archives in `build/artifacts/axion_heap_compaction_trace.log`, `build/artifacts/axion_policy_runner.log`, and `build/artifacts/canonfs_axion_trace.log`. Auditors can replay the bytes by running `./scripts/capture-axion-trace.sh` (it prints each log and keeps the `heap compaction`, `heap relocation`, and CanonFS meta slot lines as CI artifacts), or by concatenating the Axion log from `t81 repl` via:

```
$ t81 repl
t81> :trace
... reason="heap compaction heap_frames=0 heap_ptr=267"
... reason="heap relocation from=267 to=512 size=32"
```

Use that snippet to cross-check your policy strings against RFC-0020 without reading source code—`require-axion-event` simply looks for the substring you recorded.

`canonfs_axion_trace_test` now exercises the persistent CanonFS driver
(`make_persistent_driver` in `include/t81/canonfs/canon_driver.hpp`), so CI
artifacts prove that Axion emitted the canonical `meta slot axion event
segment=meta addr=<n>` line *before* bytes were flushed to `objects/<hash>.blk`.
This makes the policy snippet concrete: the log captures exactly what a real
disk-backed CanonFS write recorded, so `(require-axion-event
(reason "... action=Write"))` now verifies persistence from a tangible store
rather than an in-memory stub.

This clause performs a substring match on each `AxionEvent.verdict.reason`; it lets policies verify GC traces, meta slot writes, or any other canonical string recorded by the runtime. Because GC cycles log `interval stack_frames=...` before mutating memory, this predicate guarantees those transitions exist before privileged opcodes proceed.

## 3. Embedding policies in builds

### 3.1 `t81 compile`

```
t81 compile --verbose --axion-policy policy/guards.axion match_guard.t81 -o match_guard.tisc
```

The CLI prints the `verdict.reason` strings before Axion enforces the policy, making it easy to verify the emitted strings match the policy clauses. Use `--axion-policy-text` to inline the s-expression if you prefer not to have a file.

### 3.2 `t81 run` / REPL

When running `.tisc` programs produced by the compiler, Axion uses the embedded policy header even if you execute via `t81 run` or `t81 repl`. Use `:rules` in the REPL to inspect the parsed policy and `:trace` to confirm the required strings already exist in `State::axion_log`.

## 4. Diagnosing policy failures

- **Guard failure**: the policy engine throws `PolicyFault` when the Axion log lacks the required guard string. Inspect `axion_policy_match_guard_test.cpp` to see the failure case: it compiles the same program twice, expecting the second run (which demands `(variant Red)`) to trap with `Trap::SecurityFault`.
- **Segment failure**: `axion_policy_segment_event_test.cpp` deliberately asserts `(addr 9999)` where the Axion log only sees a real stack address, verifying the `PolicyFault` path.
- **Loop hint failure**: if `(require-loop-hint ...)` is present but the Axion log lacks the matching `loop hint file=...` string, the policy rejects the program before executing privileged opcodes.

Always run the regression suite (`ctest --test-dir build -R axion_policy_* --output-on-failure`) when editing policies to ensure the canonical strings stay intact.

## 5. Policy integration tips

1. **Document the required strings** in release notes (see `docs/guides/axion-trace.md` §5 and `docs/guides/cli-user-manual.md` §7). Include the `t81 compile --verbose` transcripts showing exactly the `verdict.reason` strings demanded by your policy.
2. **Archive Axion logs** (`scripts/capture-axion-trace.sh` produces `build/artifacts/axion_policy_runner.log`) so auditors can replay what the policy engine saw.
3. **Use Axion regressions** as examples—the tests named `axion_policy_match_guard_test` and `axion_policy_segment_event_test` show how to embed policies, run them, and expect `Trap::SecurityFault` when requirements are unmet.

## 6. Related RFCs and docs

- `[RFC-0009](../spec/rfcs/RFC-0009-axion-policy-language.md)` – Syntax, predicates, and security semantics for APL.  
- `[RFC-0020](../spec/rfcs/RFC-0020-axion-segment-trace.md)` – Canonical segment trace strings that `require-segment-event` clauses check.  
- `[RFC-0019](../spec/rfcs/RFC-0019-axion-match-logging.md)` – Enum/match metadata that powers `require-match-guard`.  
- `docs/guides/axion-trace.md` / `docs/guides/axion-tracing-manual.md` – CLI samples showing the required strings in action.  
- `include/t81/axion/policy.hpp` and `src/axion/policy_engine.cpp` – Implementation reference for how S-expressions are parsed and enforced.  
