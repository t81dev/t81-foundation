# Axion Trace Reference

This guide explains how to observe the deterministic `AxionEvent.verdict.reason`
strings that document segment transitions and guard evaluations inside the
HanoiVM. These strings are the authority for Axion policies that `require`
segment coverage (see [RFC-0013](../spec/rfcs/RFC-0013-axion-segment-trace.md)
and [RFC-0009](../spec/rfcs/RFC-0009-axion-policy-language.md#segment-trace-predicates)).

## 1. Command-line trace sample

```
cmake --build build --parallel --target axion_policy_trace
./build/axion_policy_trace
```

The `axion_policy_trace` example populates a simple `.tisc` program with tensor,
stack, heap, and Axion guard operations. Each `AxionEvent` is dumped to stdout
with its opcode, tag, and `verdict.reason`. Look for lines such as:

```
opcode=68 tag=2 reason="stack frame allocated stack addr=243 size=16"
opcode=0 tag=5 reason="meta slot axion event addr=1283"
opcode=33 tag=0 reason="tensor slot allocated tensor addr=3"
opcode=22 tag=5 reason="AxRead guard segment=stack addr=5"
opcode=23 tag=4 reason="AxSet guard segment=stack addr=4"
```

Those lines show the exact strings that Axion policies can demand. The policy
suite can replay the trace to confirm each transition occurred before a
privileged opcode or loop.

## 2. Axion segment trace regression

```
ctest --test-dir build -R axion_segment_trace_test --output-on-failure
```

The regression prints its own Axion trace snippet after confirming the presence
of `tensor slot allocated`, `meta slot`, `AxRead guard`, and `AxSet guard`
entries. The `ctest` log becomes your CI artifact; treat the printed snippet as
evidence that the deterministic strings exist in this build.

Out-of-bounds accesses now record a `bounds fault` entry (`reason="bounds fault
segment=<segment> addr=<value> action=…"`), whether the failure comes from a
memory load/store or from stack/heap/tensor operations (e.g.,
`action=stack frame allocate`, `action=heap block allocate`,
`action=tensor handle access`). `tests/cpp/vm_memory_test.cpp` validates those
strings and prints the log snippet for CI artifacts, so auditors can replay
fault scenarios that exercise the HanoiVM memory semantics without inspecting
the source.
The standard `ctest --test-dir build --output-on-failure` run now surfaces that
snippet so observability tooling can capture the `bounds fault ...` strings as
part of the canonical CI artifact.

## 3. CLI & REPL verification

- Run `t81 repl` and type `:trace` after executing a guard-heavy snippet; the CLI
  dumps the same `AxionEvent.verdict.reason` strings recorded by these
  tests/examples.
- Enable `--verbose` on `t81 compile` to watch the CLI print the match/loop/enum
  metadata strings before `axion_policy_trace` or policy runners verify them.

### 3.1 Guard-heavy match sample

Create a simple match to trigger enum guards/variants and emit the Axion metadata
before the policy engine runs:

```
cat <<'EOF' > match_guard.t81
let maybe: Option[i32] = Some(42);

match (maybe) {
  Some(value) if value > 40 => value,
  None => 0,
}
EOF

$ t81 compile --verbose match_guard.t81 -o match_guard.tisc
AxionEvent[EnumIsVariant]: opcode=111 reason="enum=Option variant=Some match=pass guard=pass payload=i32"
AxionEvent[EnumUnwrapPayload]: opcode=112 reason="enum=Option variant=Some payload=i32 value=42"
AxionEvent[AxRead guard]: opcode=22 reason="AxRead guard segment=stack addr=5"
```

Those `AxionEvent` reason strings are the canonical values that the
`axion_policy_match_guard_test` regression and `require-match-guard` clauses
check for; when the CLI prints them you know the policy engine can replay the
guard metadata without touching the Axion source.

### 3.2 Axion policy CLI sample

Use `t81 compile --verbose` with a policy that includes `require-segment-event`
and `require-match-guard` to verify the runtime logs the audited verdict string
before the program halts. `axion_policy_runner` and
`scripts/capture-axion-trace.sh` already exercise these predicates, but you can
reuse the same commands locally:

```
$ t81 compile --verbose match_guard.t81 -P policy/guards.axion
verdict.reason="enum=Option variant=Some match=pass guard=pass payload=i32"
verdict.reason="stack frame allocated stack addr=243 size=16"
```

The `policy/guards.axion` snippet should include the `(require-match-guard ...)`
and `(require-segment-event ...)` s-expressions from `[RFC-0009](../spec/rfcs/RFC-0009-axion-policy-language.md)` so the CLI output matches the strings recorded by the regressions and the CanonFS audit logs.

### 3.3 Reference policy file

Use the bundled `policy/guards.axion` to reproduce the CLI sample without
manually typing policy clauses. The file mirrors the regression requirements:

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
    (action "AxRead guard"))
)
```

Running `t81 compile --verbose match_guard.t81 -P policy/guards.axion` or
`axion_policy_runner` produces the same `enum guard` / `segment trace` strings
that RFC-0009 and RFC-0013 mandate. Auditors can inspect this file to confirm
the CLI output matches the trace snippets collected by the regressions.

## 4. Policy integration checklist

1. Embed `loop`/`match` metadata as before (`program.axion_policy_text`,
   `match_metadata_text`, `enum_metadata`).
2. Use `require-match-guard`, `require-loop-hint`, or the new
   `require-segment-event` predicates in your APL policy when you depend on the
   trace strings.
3. Include the output from `axion_policy_trace`, `axion_policy_runner`, or
   `axion_segment_trace_test` in release notes so auditors can see the actual
   `verdict.reason` strings that guarantee deterministic behavior.

Run the new `axion_policy_match_guard_test` and
`axion_policy_segment_event_test` regressions whenever guard or segment
predicates change. They exercise the `(require-match-guard ...)` and
`(require-segment-event ...)` clauses against the canonical `enum guard` /
`stack frame allocated` strings so you can prove the policy engine sees the
exact same Axion trace that the CLI, CanonFS, and auditors depend on.

For more context, see `docs/guides/cli-toolkit.md` and `spec/axion-kernel.md`.

## 5. Policy runner trace

The new `examples/axion_policy_runner.cpp` implements a minimal Axion policy
runner that mirrors the requirements described in [RFC-0013](../spec/rfcs/0013-axion-segment-trace.md)
and the `require-segment-event` predicates of
[RFC-0009](../spec/rfcs/RFC-0009-axion-policy-language.md#segment-trace-predicates).
Build it with

```
cmake --build build --parallel --target axion_policy_runner
./build/axion_policy_runner
```

The helper runs a program that touches stack, heap, tensor, and meta segments,
invokes both `AxRead` and `AxSet`, and then prints every `AxionEvent.verdict.reason`
string so you can replay the canonical `interpretation` of those guards without
opening the VM sources. Look for lines such as

```
opcode=23 tag=4 reason="AxSet guard segment=stack addr=251 Instruction count within limit"
opcode=22 tag=5 reason="AxRead guard segment=heap addr=267 Instruction count within limit"
```

(The excerpt above shows the `AxRead/AxSet` guard verdicts that RFC-0009 references
when policies demand segment coverage.)

Pair the new runner with `axion_policy_trace` and `axion_segment_trace_test` to
capture a complete policy log in your CI artifacts, giving auditors the exact
strings to match against the `require-segment-event` predicates.

## 6. Publishing the Axion policy trace

CI pipelines should run `scripts/capture-axion-trace.sh` after building to
ensure `build/artifacts/axion_policy_runner.log` contains the canonical Axion
trace covering both `require-match-guard` and `require-segment-event`
strings. Because `axion_policy_runner` runs with `policy/guards.axion`, the log
now records the same guard-heavy `verdict.reason` strings (`enum=Option ...`,
`stack frame allocated ...`, `AxRead guard ...`) that RFC-0009/RFC-0013 expect.
Archive that log alongside the `ctest` output so auditors can compare the
recorded `AxionEvent.verdict.reason` entries without rerunning the example.

```
./scripts/capture-axion-trace.sh
```

Include the produced log in your CI artifacts or release notes whenever the
Axion trace instrumentation changes; the file then serves as the deterministic
artifact linking RFC-0009 policy predicates to the runtime trace strings.

## 7. Release artifact checklist

When cutting a release, archive the outputs described in this guide so auditors
can replay the canonical `verdict.reason` strings without reading the VM code:

1. Copy the `./scripts/capture-axion-trace.sh` output (`build/artifacts/axion_policy_runner.log`)
   into the release bundle or CI artifact store alongside the `ctest` logs.
2. Store the `ctest --test-dir build -R axion_segment_trace_test --output-on-failure`
   block (prints the bounds/segment `verdict.reason` snippet) in the same artifact so segment coverage can be audited.
3. Include the CLI sample (`t81 compile --verbose ... -P policy/guards.axion`) transcript and the guard-heavy `match_guard.t81` source so reviewers can reproduce the `enum=Option ...` strings without rebuilding the VM.

Together these extracts document every guard, segment, and policy trace string that RFC-0009/0013/0019 cite, delivering a deterministic artifact that auditors can inspect before diving into the codebase.
