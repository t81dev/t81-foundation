# Extension Profile

Last Updated: 2026-02-28
Owner: @t81dev

**What is explicitly not frozen. What can break. What has no determinism guarantees.**

This document prevents scope confusion. If a surface is not listed in
`FROZEN_CORE_PROFILE.md` as Verified, assume it belongs here.

---

## The Rule

> If it is not in the Frozen Core and not in the Determinism Surface Registry
> as Verified, it is experimental by default. Treat its output as non-reproducible
> unless promotion evidence has been published.

---

## Experimental Surfaces (Non-DCP)

| Surface | Path | Stability | Can Break? | Determinism | Promotion State |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Cognitive Tiers** | `experimental/tiers/cog/`, `experimental/tiers/` | Experimental | Yes | Non-verified | Experimental — non-DCP unless promoted |
| **Hanoi VM Kernel** | `experimental/hanoi/` | Experimental | Yes | Non-verified | Experimental — non-DCP unless promoted |
| **Distributed Compute** | `experimental/distributed/` | Experimental | Yes | Non-deterministic by design | Experimental — non-DCP unless promoted |
| **Trace-JIT** | `runtime/jit/jit_compiler.cpp` | Alpha / Stub | Yes | Non-verified (equivalence unproven) | Experimental — non-DCP unless promoted |
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
| **Axion Kernel** | `kernel/axion/` | Alpha; policy engine and opcode arbitration present | Scope-bounded partial verification | §1.6 and §1.9 are Implemented (bounded); §1.1/1.3/1.10 are Partial; §2.5 is Deferred. See `DRIFT_DECOMPOSITION.md`. |

---

## Draft / Beta Non-Frozen Surfaces

These surfaces are in active use but are not frozen. They can change across
minor versions without a major version bump. They carry determinism evidence
only where explicitly stated.

| Surface | Path | Maturity | Spec Authority | Determinism Scope |
| :--- | :--- | :--- | :--- | :--- |
| **T81Lang Frontend** | `lang/frontend/` | Beta (implementation) | Draft (`spec/t81lang-spec.md`) | Fixture-bounded only — 16 corpus programs; full spec-section traceability incomplete |
| **T81VM** | `core/vm/` | Beta | Beta (`spec/t81vm-spec.md`) | DCP-scoped opcodes verified; policy-bridge dispatch partially extracted |
| **T81Graph** | `lang/frontend/` (graph types) | Draft | Non-normative (surface inventory) | No determinism tests; no DCP scope without promotion |
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
must pass the governed AGI promotion pipeline:

1. **Planned** → **Experimental**: design intent documented, implementation started.
2. **Experimental** → **Verified Candidate**: ADR published; threat-model updated;
   registry entry drafted; determinism tests pass; incident-response controls operational.
3. **Verified Candidate** → **Verified** (→ DCP eligible): governance approval;
   no open Severity-2/3 incidents; release packet GO; registry entry published.

Rollback: a Verified-surface determinism regression requires immediate registry
downgrade and critical defect classification.

Reference: `docs/status/GOVERNANCE_REVIEW_CADENCE.md` (§ Promotion Gate Protocol)

---

## What This Document Is NOT

- It is not a roadmap.
- It is not a feature wishlist.
- It is not a promise that these surfaces will ever be promoted.

Promotion is governed. Until it happens, assume experimental.

---

## Cross-References

- `docs/status/FROZEN_CORE_PROFILE.md` (what IS frozen)
- `docs/status/DRIFT_DECOMPOSITION.md` (where the gaps are)
- `docs/status/GOVERNANCE_REVIEW_CADENCE.md` (promotion gate protocol)
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
