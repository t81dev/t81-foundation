---
title: "RFC-00A2 — T81 AI Benchmark Specification and Reporting Format"
status: accepted
version: "0.4"
updated: 2026-03-16
applies_to:
  - T81 Benchmark Suite (benchmarks/)
  - T81 CLI (tooling/cli/main.cpp — `t81 internal benchmark`)
---

## Summary

This RFC defines a standardized benchmark specification and reporting format for AI workloads
in the T81 ecosystem.  It establishes canonical metrics for VM throughput and determinism
validation, wired into the Google Benchmark suite (`benchmark_runner`) and surfaced via
`t81 internal benchmark`.

## Scope Adjustment (2026-03-16)

The original draft referenced `t81 ai benchmark` CLI commands from RFC-00A7, which was
superseded by RFC-0033 (`t81 studio` / `t81 agent`).  The `t81 ai` command hierarchy was
never implemented and is no longer on the roadmap.

The concrete, implemented scope is: **T81 VM-native AI workload benchmarks** — instruction
dispatch throughput, tensor operation throughput, and CanonHash81-based determinism
validation.  These are T81's unique contribution to AI benchmarking and are what
distinguish the platform from MLPerf.

External LLM workloads (LLaMA-7B, GPT-2 via llama.cpp) remain future work gated on
the llama.cpp adapter (RFC-00A5, superseded).

## Acceptance Criteria

| ID | Criterion | Status |
| -- | --------- | ------ |
| [RFC-00A2-01] | Benchmark suite covers VM instruction throughput (ops/s) | met — `BM_VMRun_Arith`, `BM_VMRun_ArithChain` |
| [RFC-00A2-02] | Benchmark suite covers tensor operation throughput | met — existing `BM_TensorMatMul`, `BM_DispatchOverhead` |
| [RFC-00A2-03] | Determinism validation benchmark reports `determinism_score` and `unique_hashes` counters | met — `BM_DeterminismValidation_Arith`, `BM_DeterminismValidation_ArithChain` |
| [RFC-00A2-04] | All determinism benchmarks produce `determinism_score = 1.0` (bit-exact reproducibility proven) | met — verified on all runs |
| [RFC-00A2-05] | Benchmark suite integrated into the build (`T81_BUILD_BENCHMARKS=ON`) and runnable via `t81 internal benchmark` | met — `benchmarks/CMakeLists.txt`, CLI dispatch at `tooling/cli/main.cpp` |
| [RFC-00A2-06] | JSON output available via `--benchmark_format=json` (Google Benchmark native) | met — standard Google Benchmark flag |

6/6 criteria met.

## Benchmark Catalog

### Determinism Benchmarks (RFC-00A2 primary contribution)

| Benchmark | Metric | Expected Value |
| --------- | ------ | -------------- |
| `BM_DeterminismValidation_Arith/10` | `unique_hashes` | 1 |
| `BM_DeterminismValidation_Arith/50` | `determinism_score` | 1.0 |
| `BM_DeterminismValidation_ArithChain/10` | `unique_hashes` | 1 |
| `BM_DeterminismValidation_ArithChain/50` | `determinism_score` | 1.0 |

### Throughput Benchmarks (pre-existing, RFC-00A2 endorsed)

| Benchmark | Measures |
| --------- | -------- |
| `BM_VMRun_Arith` | 6-instruction TISC program dispatch (ops/s) |
| `BM_VMRun_ArithChain` | 82-instruction arithmetic chain dispatch (ops/s) |
| `BM_TensorMatMul` | Ternary matrix multiply throughput |
| `BM_DispatchOverhead` | TISC opcode dispatch overhead |
| `BM_LimbMul` | BigInt limb multiplication throughput |
| `BM_TritwiseWorkloads` | Ternary logic operation throughput |

## Reporting

```bash
# Human-readable table (default):
t81 internal benchmark vm

# JSON output (Google Benchmark native):
t81 internal benchmark -- --benchmark_format=json --benchmark_out=results.json

# Filter to determinism benchmarks:
t81 internal benchmark -- --benchmark_filter=BM_DeterminismValidation
```

JSON output conforms to the Google Benchmark format; each result entry contains `name`,
`iterations`, `real_time`, `cpu_time`, and the custom counters defined in each benchmark
(`unique_hashes`, `determinism_score`, `num_runs`, `ops_per_run`).

## Key Files

| File | Purpose |
| ---- | ------- |
| `benchmarks/BM_DeterminismValidation.cpp` | RFC-00A2 determinism validation benchmarks |
| `benchmarks/BM_VMRun_Arith` (in same file) | VM throughput baselines |
| `benchmarks/CMakeLists.txt` | Build integration |
| `tooling/cli/main.cpp` | `t81 internal benchmark` CLI dispatch |

## Acceptance Note (2026-03-16)

All 6 AC met.  `BM_DeterminismValidation.cpp` adds 4 new benchmarks that prove
bit-exact reproducibility (`determinism_score = 1.0`, `unique_hashes = 1`) via
CanonHash81 hashing of serialized VM state across 10 and 50 repeated runs.

Observed throughput on Apple M-series (arm64):

- `BM_VMRun_Arith`: ~1.42 Mops/s (6 instructions)
- `BM_VMRun_ArithChain`: ~335 Kops/s (82 instructions)
