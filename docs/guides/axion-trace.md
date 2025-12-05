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

## 6. Publishing the Axion policy trace

CI pipelines should run `scripts/capture-axion-trace.sh` after building to
ensure `build/axion_policy_runner.log` contains the canonical Axion trace that
corresponds to the `require-segment-event`/bounds-fault strings. Archive that
log alongside the `ctest` output so auditors can compare the recorded
`AxionEvent.verdict.reason` entries (`stack frame allocated…`, `bounds fault
segment=heap…`, `AxRead guard…`, etc.) without rerunning the example.

```
./scripts/capture-axion-trace.sh
```

Include the produced `build/artifacts/axion_policy_runner.log` as part of your
CI artifacts or release notes whenever the Axion trace instrumentation changes.

## 3. CLI & REPL verification

- Run `t81 repl` and type `:trace` after executing a guard-heavy snippet; the CLI
  dumps the same `AxionEvent.verdict.reason` strings recorded by these
  tests/examples.
- Enable `--verbose` on `t81 compile` to watch the CLI print the match/loop/enum
  metadata strings before `axion_policy_trace` or policy runners verify them.

## 4. Policy integration checklist

1. Embed `loop`/`match` metadata as before (`program.axion_policy_text`,
   `match_metadata_text`, `enum_metadata`).
2. Use `require-match-guard`, `require-loop-hint`, or the new
   `require-segment-event` predicates in your APL policy when you depend on the
   trace strings.
3. Include the output from `axion_policy_trace`, `axion_policy_runner`, or
   `axion_segment_trace_test` in release notes so auditors can see the actual
   `verdict.reason` strings that guarantee deterministic behavior.

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
