# Extension Profile

Last Updated: 2026-03-19
Owner: @t81dev

**What is explicitly not frozen. What can break. What has no determinism guarantees.**

This document prevents scope confusion. If a surface is not explicitly inside
the DCP / verified deterministic boundary, assume it belongs here as either
governed non-DCP or experimental / non-DCP.

---

## The Rule

> If it is not in the Frozen Core and not in the Determinism Surface Registry
> as Verified, it is outside DCP. Treat it as governed non-DCP or experimental,
> and do not claim bit-exact reproducibility unless promotion evidence and
> registry status explicitly say so.

---

## Experimental Surfaces (Non-DCP)

| Surface | Path | Stability | Can Break? | Determinism | Promotion State |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Cognitive Tiers** | `experimental/tiers/cog/`, `experimental/tiers/` | Experimental | Yes | Non-verified | Experimental — non-DCP unless promoted |
| **Hanoi VM Kernel** | `experimental/hanoi/` | Experimental | Yes | Non-verified | Experimental — non-DCP unless promoted |
| **Distributed Compute** | `experimental/distributed/` | Experimental | Yes | Non-deterministic by design | Experimental — non-DCP unless promoted |
| **Trace-JIT** | `runtime/jit/jit_compiler.cpp` | Experimental / stub | Yes | Non-verified (equivalence unproven) | Experimental — non-DCP unless promoted |
| **Experimental Headers** | `include/t81/experimental/` | Experimental | Yes | Non-verified | Experimental — non-DCP |
| **Notebooks** | `notebooks/` | Example only | Yes | Non-verified | Not applicable |
| **Examples** | `examples/` | Example only | Yes | Non-verified | Not applicable |

---

## Governed Non-DCP Surfaces

These surfaces have practical governance (controlled build path, reproducibility
evidence, policy classification) but are explicitly **not** in the DCP and carry
**no bit-exact reproducibility guarantee**.

| Surface | Path | Governance | Determinism | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **llama.cpp adapter** | `third_party/llama.cpp`, `tooling/model/llama_cpp_adapter.cpp`, CLI `llama-run` | `T81_ENABLE_LLAMA_CPP=OFF` default; `T81_EXPORT_LLAMA_ADAPTER` guard prevents package export | Practical reproducibility only (controlled model + policy fixture) | Classified governed non-DCP (DEC-003). Promotion requires governed AGI pipeline. |
| **Axion Governance Kernel** | `kernel/axion/` | Governed implementation surface; policy engine and opcode arbitration present | Scope-bounded partial verification with explicit epoch scheduler/audit parity evidence in the experimental kernel lane | Governed non-DCP overall. Some bounded evidence exists, including the `axion-epoch-determinism` CI gate for pooled-vs-unbounded kernel parity, but the broader kernel surface is not automatically a verified deterministic surface. |
| **Axion OS** | `userland/experimental/`, `spec/rfcs/RFC-00B3-axion-kernel-architecture.md` | Governed experimental kernel track; first kernel-owned handoff implemented | Experimental developer-lane verification with CI-enforced epoch execution/audit parity | Runtime handoff from `hal_main` to `axion_kernel_main(...)` is implemented and tested, and scheduler/audit parity is now hard-failed in `axion-epoch-determinism`; promotion still remains non-DCP and broader kernel/runtime claims stay evidence-gated. |
| **DPE (Parallel Execution)** | `include/t81/dpe/`, `src/dpe/` | Governed non-DCP implementation surface | RFC-DPE acceptance + deterministic epoch evidence | Stable implementation surface moved out of `experimental/` for layout clarity. Remains outside DCP / Verified scope. |

---

## Governed Active Surfaces Outside DCP

These surfaces are in active use and may be stable in the product sense, but
they are not automatically DCP / verified deterministic surfaces. They carry
determinism evidence only where explicitly stated.

| Surface | Path | Governance Status | Spec Authority | Determinism Scope |
| :--- | :--- | :--- | :--- | :--- |
| **T81Lang Frontend** | `lang/frontend/` | Governed non-DCP | Draft (`spec/t81lang-spec.md`) | Fixture-bounded only; full compiler/toolchain deterministic promotion remains incomplete |
| **T81VM Extensions Beyond Interpreter DCP Scope** | `core/vm/` | Governed non-DCP | Beta (`spec/t81vm-spec.md`) | Interpreter replay parity is DCP-scoped; broader VM-adjacent acceleration and extension surfaces require explicit promotion |
| **T81Graph** | `include/t81/types/T81Graph.hpp`, `include/t81/cog/v1/`, `core/vm/`, `lang/frontend/` | Governed non-DCP | Non-normative (surface inventory) | Determinism evidence is bounded; the symbolic graph runtime has been moved out of `include/t81/experimental/`, but whole-surface DCP promotion has not been granted |
| **stdlib: std.io, std.sys, std.async, std.agent** | `lang/stdlib/std/` | Experimental | `spec/t81lang-spec.md` (Draft) | No fixture conformance suites yet; experimental classification |
| **T81Lang Experimental Types: T81Time, T81Entropy, T81Promise, T81Agent** | `core/types/` (headers) | Experimental | `spec/t81lang-spec.md` (Draft) | Non-DCP; `serialize_canonical` present but runtime wiring incomplete |
| **Cognitive Tier 1–5 VM opcodes** | `core/vm/` (tier opcode paths) | Experimental | `spec/cognitive-tiers.md` (Draft) | Non-DCP; VM dispatch paths present, determinism not registry-verified |

---

## Explicit Determinism Exclusions

These things are **never deterministic** in this system:

| Item | Why |
| :--- | :--- |
| Wall-clock execution timing | Platform-dependent scheduler |
| Network IO | Latency, ordering, packet loss |
| Hardware FPU | Use `T81Float` soft-float instead |
| External hardware accelerators | Undefined behavior outside CPU |
| Real-time scheduling | OS-dependent |
| JIT-compiled output | Equivalence unproven; disabled by default |

---

## Promotion Path

For any surface to move from this document into `FROZEN_CORE_PROFILE.md`, it
must pass the relevant governance promotion path and earn registry-backed
verified status:

1. **Planned** → **Experimental**: design intent documented, implementation started.
2. **Experimental / Governed non-DCP** → **Verified Candidate**: ADR/RFC path published; threat model updated; registry entry drafted; determinism tests pass; incident-response controls operational.
3. **Verified Candidate** → **Verified** (→ DCP eligible): governance approval; no open Severity-2/3 incidents; release packet GO; registry entry published.

Rollback: a Verified-surface determinism regression requires immediate registry
downgrade and critical defect classification.

Reference: `docs/status/GOVERNANCE_REVIEW_CADENCE.md` (§ Promotion Gate Protocol)

---

## What This Document Is NOT

- It is not a roadmap.
- It is not a feature wishlist.
- It is not a promise that these surfaces will ever be promoted.

Promotion is governed. Until it happens, assume outside DCP.

---

## Cross-References

- `docs/status/FROZEN_CORE_PROFILE.md` (what IS frozen)
- `docs/status/DRIFT_DECOMPOSITION.md` (where the gaps are)
- `docs/status/GOVERNANCE_REVIEW_CADENCE.md` (promotion gate protocol)
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md`
