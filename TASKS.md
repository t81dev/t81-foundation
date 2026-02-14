# T81 Foundation: Actionable Task List

> **Source of Truth:** This document tracks **all pending work**, from near-term tasks to long-term strategic items. It supersedes the deprecated `TODO.md`.

**Last Updated:** February 10, 2026

______________________________________________________________________

## P0 — Keep Determinism Gates Green

- [x] Re-run and document the local ritual on every significant merge window:
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build --parallel`
  - `ctest --test-dir build --output-on-failure`
- [ ] Keep cross-arch T81Lang and T3_K reproducibility gates passing in CI.
- [ ] Keep runtime-contract sync checks green against `t81-vm`.
- [x] Fix T81Float precision loss (negative exponent underflow) impacting agent/float tests.
- [x] Fix BigInt division semantics tests to match Euclidean implementation.

## P1 — Performance Path

- [ ] Profile and optimize hot tensor kernels used by demo/inference paths.
- [ ] Improve CanonFS performance under sustained write/read workloads while preserving deterministic trace strings.

## P2 — Tooling and UX

- [ ] Tighten CLI ergonomics/documentation parity for `disasm`, `debug`, and `trace replay` workflows.
- [ ] Expand deterministic failure diagnostics for compile/run workflows.
- [ ] Keep `examples/` runnable and aligned with docs (including `examples/tisc/` assets).
  - *Partial Fix:* Fixed index off-by-one in `llama32_demo.cpp`.

## P3 — Verification and Hardening

- [ ] Expand property/fuzz coverage for frontend + IR + VM boundary invariants.
- [ ] Add additional parity checks for backend variants against deterministic scalar references.
- [ ] Continue Axion policy/trace regression growth for guard/segment/match paths.

______________________________________________________________________

## Strategic / Long-Horizon (Formerly TODO.md)

### 1. Formal Methods and Proofs

- [ ] Proof-oriented validation for key Axion policy invariants.
- [ ] Expand deterministic replay proofs across compiler + VM boundaries.

### 2. Performance Architecture

- [ ] Expand tensor backend strategy (portable scalar parity + optimized backends).
- [ ] CanonFS scalability for larger persistent workloads with deterministic observability.

### 3. Runtime and Execution

- [ ] Incremental deterministic trace-JIT hardening for numeric/tensor hot paths.
- [ ] Native trace-JIT backend prototype (x86_64/ARM64) with deterministic mmap.
- [ ] Evaluate distributed tensor orchestration patterns that preserve replay guarantees.

### 4. Ecosystem and Interop

- [ ] Expand language bindings and integration surfaces where determinism guarantees can be preserved.
- [ ] Strengthen downstream runtime boundary tooling (`t81-foundation` <-> `t81-vm` <-> examples).

### 5. Hardware and Future Targets

- [ ] Investigate FPGA/hardware-backed ternary execution pathways.
- [ ] Define pragmatic hardware abstraction boundaries without weakening canonical semantics.

______________________________________________________________________

## Completed Highlights (Moved)

Major completed streams (compiler conformance, VM memory model, Axion/CanonFS integration, C++23 default lane) are now tracked in:
- `CHANGELOG.md`
- `ANALYSIS.md`
- `docs/system-status.md`
