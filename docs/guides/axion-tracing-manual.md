# Axion Tracing Manual

This manual dives deep into the Axion trace surface: the runtime events, their canonical `verdict.reason` strings, the CLI/testing hooks that emit them, and the policy obligations they satisfy. It expands on `docs/guides/axion-trace.md` by grouping the instrumentation into an ordered workflow so auditors and Axion stewards can reproduce the deterministic trace without referring directly to source code.

## 1. Trace architecture at a glance

Axion receives deterministic visibility from three main streams:

1. **Segment transitions** (stack, heap, tensor, meta) recorded whenever the VM allocates, frees, or touches a segment.
2. **Guard metadata** emitted for enum/sum pattern checks (`ENUM_IS_VARIANT`, `ENUM_UNWRAP_PAYLOAD`) and option/result guards.
3. **Loop hints** emitted by `format_loop_metadata` so Axion knows when annotated loops execute.

Each stream produces `AxionEvent` entries whose `verdict.reason` strings must match the canonical patterns in [RFC-0013](../spec/rfcs/0013-axion-segment-trace.md) and [RFC-0019](../spec/rfcs/RFC-0019-axion-match-logging.md). The CLI, docs, and policy engine all read those strings, so you can audit determinism purely through them.

## 2. Canonical event strings

### 2.1 Segment transitions

Segment events include the segment name, address, and action:

```
stack frame allocated stack addr=243 size=16
heap block allocated heap addr=512 size=32
tensor slot allocated tensor addr=3
meta slot axion event segment=meta addr=1283
AxRead guard segment=stack addr=5
AxSet guard segment=heap addr=42
bounds fault segment=stack addr=999 action=stack frame allocate
```

The policy engine (`Policy::SegmentEventRequirement`) looks for these patterns exactly; the `action` string is quoted in policies so include the quotes when writing `(action "stack frame allocated")`.

### 2.2 Guard metadata

Guard events include enum/variant names, optional payload type, and the match result:

```
enum guard enum=Option variant=Some payload=i32 match=pass
enum guard enum=Color variant=Blue match=fail
enum payload enum=Option variant=Some payload=i32
```

`Policy::MatchGuardRequirement` matches these strings to satisfy `(require-match-guard ...)`. Always emit the entire string as shown (the CLI/VM already include `enum=...` and `variant=...` when metadata is available).

## 3. Observing traces via CLI

### 3.1 Compile-time tracing

```
t81 compile --verbose match_guard.t81 -P policy/guards.axion
```

The CLI prints each trace string before running the policy, so you can verify the emitted `enum=Option variant=Some match=pass guard=pass payload=i32` line matches the canonical string in RFC-0019.

### 3.2 REPL tracing

Run `t81 repl` and type `:trace` after executing guard-heavy code. The REPL dumps the contents of `State::axion_log`; each entry includes:

- `opcode`: TISC opcode that generated the event.
- `tag`: metadata tag (e.g., 5 for Axion events).
- `verdict.reason`: the canonical string.

Use `:trace chart` to visualize segment time ordering and `:rules` to inspect loaded policy requirements.

Axion policies can now assert any canonical tracing string via
`(require-axion-event (reason "..."))`. For example, requiring
`reason "interval stack_frames="` guarantees that a GC cycle trace exists
before privileged opcodes execute.

## 4. Trace regressions and helpers

- `ctest --test-dir build -R axion_segment_trace_test --output-on-failure` prints canonical segment/guard strings and includes them in CI logs.
- `tests/cpp/axion_policy_match_guard_test.cpp` validates `(require-match-guard  (enum Color) (variant Blue) (payload i32) (result pass))`.
- `tests/cpp/axion_policy_segment_event_test.cpp` ensures `(require-segment-event (segment stack) (action "stack frame allocated"))`.
- `scripts/capture-axion-trace.sh` reruns `examples/axion_policy_runner.cpp` (policy guaranteed to include guards/segments) and saves `build/artifacts/axion_policy_runner.log`. Include that log with your release artifacts.

## 5. Matching policies to traces

For any Axion policy clause:

1. Identify the `verdict.reason` string it requires (`enum=…`, `segment=…`, `bounds fault …`).
2. Run `t81 compile --verbose` or `axion_policy_runner` to capture the string.
3. Store the output (CLI transcript, log file) as a release artifact so auditors can verify the string even if they don't run the code.

If the policy fails because a string is missing, inspect `State::axion_log` or rerun `:trace` in the REPL; the policy engine checks that the canonical string already exists before allowing the privileged opcode.

## 6. Related references

- `docs/guides/axion-trace.md` for quick reference and CLI/REPL samples.
- `docs/guides/cli-user-manual.md` for the CLI commands we just covered.
- `spec/axion-kernel.md` §1.5‑1.10 for Axion responsibilities tied to these trace inputs.
- RFC-0009, RFC-0013, RFC-0019 for the linguistic contract between policies and the runtime logs.
