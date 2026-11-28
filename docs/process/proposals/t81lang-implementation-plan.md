# T81Lang Implementation Plan (Deterministic-First)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Implementation Plan (Deterministic-First)](#t81lang-implementation-plan-deterministic-first)
  - [1. Scope and Constraints](#1-scope-and-constraints)
  - [2. Delivery Sequence](#2-delivery-sequence)
    - [Phase A: Developer Verifiability (Immediate)](#phase-a-developer-verifiability-immediate)
    - [Phase B: Deterministic Compiler Gates](#phase-b-deterministic-compiler-gates)
    - [Phase C: Deterministic JIT MVP](#phase-c-deterministic-jit-mvp)
    - [Phase D: Backend Expansion with Parity Gates](#phase-d-backend-expansion-with-parity-gates)
    - [Phase E: Advisory AI Optimization](#phase-e-advisory-ai-optimization)
  - [3. Non-Goals (Current Cycle)](#3-non-goals-current-cycle)
  - [4. Exit Criteria](#4-exit-criteria)

<!-- T81-TOC:END -->


This plan translates the broader `T81Lang` vision into a deterministic implementation sequence aligned with current repository constraints.

## 1. Scope and Constraints

- Determinism is non-negotiable: identical source and inputs must produce identical `.tisc` and trace outputs.
- Any optimization path (JIT/GPU/AI-assisted tuning) must preserve canonical behavior and Axion observability.
- AI-driven automation is advisory-only in this phase (recommendations, not autonomous code or CI mutation).

## 2. Delivery Sequence

### Phase A: Developer Verifiability (Immediate)

1. `t81 disasm <file.tisc>` command for auditable bytecode inspection.
2. Disassembly regression tests to ensure stable, parseable output shape.
3. Docs for disassembly workflows in debug and CI.

### Phase B: Deterministic Compiler Gates

1. Golden tests for AST/IR canonical stability.
2. Hash-based gate verifying repeated compile emits identical `.tisc`.
3. Cross-arch conformance harness for representative language fixtures.

Current implementation note:

- `tests/cpp/e2e_compile_determinism_test.cpp` enforces compile-twice bytecode
  identity (`tisc::encode`) and SHA3-512 digest equality, and also verifies
  runtime `printed_output` stability across both compile passes.

### Phase C: Deterministic JIT MVP

1. Trace-based JIT for side-effect-free numeric/tensor hot paths only.
2. Explicit Axion entry/exit checks around compiled traces.
3. Equivalence tests: interpreter output and JIT output must match exactly.

### Phase D: Backend Expansion with Parity Gates

1. CPU/GPU parity tests for targeted tensor kernels.
2. Deterministic backend selection (explicit config; no auto-switch by default).
3. Throughput benchmarking integrated with reproducibility artifacts.

### Phase E: Advisory AI Optimization

1. CI artifacts that recommend compiler/runtime tuning.
2. No automatic flag mutation; maintainers apply recommendations explicitly.

## 3. Non-Goals (Current Cycle)

- Autonomous AI merge/rewrite workflows.
- Unbounded JIT for arbitrary opcodes.
- Implicit backend auto-selection that can change outputs or traces.

## 4. Exit Criteria

- `t81 disasm` available and tested.
- Canonical compile output gates in CI for selected fixtures.
- JIT MVP behind explicit flag with deterministic equivalence tests.
- GPU paths gated by deterministic parity tests.
