# Project Roadmap

> **Source of Truth:** This document defines the **strategic forward-looking milestones** for the project. For current operational status, see [../reference/STATUS.md](../reference/STATUS.md). For past changes, see [../reference/CHANGELOG.md](../reference/CHANGELOG.md).

**Last Updated:** February 17, 2026

## 1. Current Phase

T81 is in a **Post-v1.0 Scaling & Cognitive Implementation phase**:
- Core determinism pipeline is implemented (T81Lang -> TISC -> HanoiVM -> Axion traces).
- CanonFS/Axion runtime surfaces are implemented and covered by regression tests.
- Cognitive Tier foundations (headers, basic opcodes) are in place; logic implementation is the primary active workstream.

## 2. 2026 Objectives

### P0: Cognitive Tier Logic
- **Symbolic (T243):** Implement graph rewriting, confluence checking, and canonicalization.
- **Reflective (T729):** Implement justification chains, trace capture, and meta-circular evaluation.
- **Recursive (T2187):** Implement recursion depth proofs and contraction mapping enforcement.
- **Distributed (T6561):** Implement consensus (Gossip/Merge) and coherence protocols.
- **Infinite (T19683):** Implement lazy expansion and signature verification for infinite structures.

### P1: Production Reproducibility Surface
- Keep deterministic build/test/repro rituals green by default.
- Maintain cross-arch bit-identity gates for T81Lang and T3_K.
- Keep runtime contract pinning aligned with `t81-vm`.

### P2: Throughput and Latency
- Optimize multi-limb `T81BigInt` arithmetic (SIMD + algorithmic improvements).
- Improve tensor kernel throughput on hot paths.
- Improve CanonFS sustained throughput while preserving deterministic traces.
- Harden JIT compiler for production use.

### P3: Ecosystem Adoption
- Stabilize and expand CLI workflows (`compile`, `run`, `disasm`, `debug`, `trace replay`, `weights`).
- Keep docs/examples synchronized with real command surfaces.
- Expand integration pathways for downstream consumers (`t81-examples`, runtime boundary).

### P4: Verification Expansion
- Extend formal/proof-oriented coverage of arithmetic and policy invariants.
- Expand deterministic replay and trace-equivalence checks.

## 3. Release Discipline

A release candidate is valid only when:
1. `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` succeeds.
2. `cmake --build build --parallel` succeeds.
3. `ctest --test-dir build --output-on-failure` passes fully.
4. Reproducibility and runtime-contract CI gates remain green.

## 4. Living Backlogs

- Near-term actionable work: `TASKS.md`
- Implementation/spec conformance notes: `../explanation/ANALYSIS.md`
