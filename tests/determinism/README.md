# T81 Determinism Tests

This directory contains determinism gate tests for the T81 Foundation.
All tests enforce bit-exact reproducibility across platforms and runs
(RFC-0031 §3, RFC-0032 §5).

## evidence-schema-v1

The `EvidenceCollector` class (promoted from `experiments/ai/determinism/`)
writes evidence reports in a plain key=value text format called
**evidence-schema-v1**.  No external dependencies are required to read or
write this format.

### Format specification

One key=value pair per line, terminated by `\n`.  No quoting, no escaping.
Keys use `.` as a namespace separator.  Reserved keys:

| Key | Type | Description |
|-----|------|-------------|
| `schema_version` | string | Always `evidence-schema-v1` |
| `experiment` | string | Experiment name (free-form identifier) |
| `run_count` | uint | Number of recorded runs |
| `run.<N>.input_hash` | hex64 | FNV-1a 64-bit hex of raw input data |
| `run.<N>.output_hash` | hex64 | FNV-1a 64-bit hex of raw output data |
| `run.<N>.trace_hash` | hex64 | FNV-1a 64-bit hex of trace events joined by `\n` |
| `run.<N>.metric.<key>` | string | Arbitrary metric value for run N |
| `determinism_pass` | bool | `true` iff all runs have identical hashes |

### Hash algorithm

All hashes use **FNV-1a 64-bit** (no external dependencies):

```
offset_basis = 14695981039346656037
prime        = 1099511628211

hash(data):
  h = offset_basis
  for each byte c in data:
    h ^= c
    h *= prime
  return hex(h, width=16)
```

No wall-clock time, no timing fields, no floating-point metrics appear in
the schema.  These are deliberately excluded to prevent non-deterministic
fields from contaminating the evidence record.

### Trace compatibility

`record_trace()` accepts a `std::vector<std::string>` identical in type to
`AIHookEngine::ai_trace()` (RFC-0032 Phase 3).  Events are concatenated
with `\n` before hashing.  Two runs that produce the same Axion AI event
strings will always produce the same `trace_hash`.

### Example output

```
schema_version=evidence-schema-v1
experiment=qmatmul-reproducibility
run_count=2
run.0.input_hash=3b5d2e1a7f8c904d
run.0.output_hash=c4f19e2a83b7051e
run.0.trace_hash=9a2b3c4d5e6f7081
run.0.metric.trit_ops=243
run.0.metric.tensors=2
run.1.input_hash=3b5d2e1a7f8c904d
run.1.output_hash=c4f19e2a83b7051e
run.1.trace_hash=9a2b3c4d5e6f7081
run.1.metric.trit_ops=243
run.1.metric.tensors=2
determinism_pass=true
```

## Test targets

| Target | File | Phase |
|--------|------|-------|
| `t81_determinism_codec_test` | `codec/ternary_codec_test.cpp` | RFC-0032 Phase 1 |
| `t81_determinism_evidence_test` | `evidence_collector.cpp` | RFC-0032 Phase 5 |
| `t81_determinism_primitives_test` | `test_primitives.cpp` | core |
| `t81_determinism_float_test` | `test_float.cpp` | core |
| `t81_determinism_containers_test` | `test_containers.cpp` | core |
| `t81_determinism_math_test` | `test_math.cpp` | core |
