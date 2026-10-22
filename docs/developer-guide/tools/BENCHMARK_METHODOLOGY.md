# Benchmark Methodology & Determinism Protocol

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Benchmark Methodology & Determinism Protocol](#benchmark-methodology-&-determinism-protocol)
  - [1. Philosophy](#1-philosophy)
  - [2. Benchmark Suite Structure](#2-benchmark-suite-structure)
  - [3. Running Benchmarks](#3-running-benchmarks)
  - [4. Deterministic Performance Gating](#4-deterministic-performance-gating)
  - [5. Artifact Retention](#5-artifact-retention)

<!-- T81-TOC:END -->


## 1. Philosophy
T81 benchmarks serve two purposes:
1.  **Performance Regression Testing**: Detecting speed regressions in critical paths (VM dispatch, Tritwise ops).
2.  **Determinism Verification**: Ensuring that performance optimizations do not alter semantic behavior.

## 2. Benchmark Suite Structure
The benchmark suite is located in `/benchmarks` and consists of Google Benchmark binaries.

| Benchmark Harness | Target Subsystem | Key Metric |
| :--- | :--- | :--- |
| `BM_PackedTritVector` | Tritwise Operations (AND, OR, NOT, XOR) | Latency (ns), Throughput (ops/s) |
| `BM_VMSimulation` | VM Dispatch Loop | Instructions per Second |
| `BM_CanonFS` | File System IO | IOPS |
| `BM_Lexer` | Compiler Frontend | Tokens/sec |

## 3. Running Benchmarks
To run the full suite:

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target benchmarks
./benchmarks/runner/run_all.sh
```

## 4. Deterministic Performance Gating
We enforce a "Zero Regression Policy" on specific "Hot Path" benchmarks:
*   **Threshold**: Any regression > 5% on 3 consecutive runs triggers a CI failure.
*   **Environment**: Benchmarks must be run on pinned hardware (Reference Node) for official gating.
*   **Workload-level VM gate**: CI now enforces `BM_VMSimulation_Dispatch` vs `BM_NativeCall_Loop`
    ratio guardrails (`scripts/ci/check_vm_workload_benchmark_regression.py`) for args `32` and `256`.

## 5. Artifact Retention
Raw benchmark outputs (`.json` or `.txt`) are stored in `benchmarks/results/` and committed if they represent a release baseline.
