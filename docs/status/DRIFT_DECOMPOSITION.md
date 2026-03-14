# Drift Decomposition

Last Updated: 2026-03-14
Owner: @t81dev

Explicit mapping of: **Spec claim → Implementation reality → Closure plan**

No narrative. Each row is a falsifiable statement.

---

## How to Read This Table

- **Spec Claim** — what the spec says is true
- **Reality** — what the implementation currently does
- **Gap** — the delta between them
- **Closure** — the specific action that closes the gap
- **State** — Closed / Open / Deferred / Monitoring

---

## Drift Matrix

| Surface | Spec Reference | Spec Claim | Implementation Reality | Gap | Closure Plan | Owner | Target | State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **TISC ISA** | `spec/tisc-spec.md` (Frozen) | Opcode semantics are bit-exact and immutable under v1.x | Implemented; freeze integrity check runs in CI | None | Maintain | @t81dev | — | **Closed** |
| **Data Types** | `spec/t81-data-types.md` (Frozen) | Canonical encoding is bit-stable; soft-float uses strict rounding | Implemented; 2026-02-27 audit fixed `Cell` overflow UB, `T81Float` signed-zero, `T81Map`/`T81Set` enforcement | None | Maintain; monitor for UB regressions | @t81dev | — | **Closed** |
| **T81VM (non-JIT path)** | `spec/t81vm-spec.md` (Beta) | Reference interpreter is deterministic for all TISC opcodes | Beta — all DCP-scoped opcodes covered by tests; FW-02 closure landed with Axion opcode pre-dispatch isolation from the main VM switch | None in this cycle | Maintain; monitor regression signals in Axion/VM conformance suites | @t81dev | — | **Closed** |
| **T81Lang — Compiler Bytecode** | `spec/t81lang-spec.md` §5 (Draft) | Compilation from source to TISC bytecode is bit-exact | Fixture-bounded: 16 corpus programs produce deterministic output; full spec-section traceability gap was partially closed 2026-02-25 | Bytecode emission is partially traceable — full deterministic compilation-profile language not yet complete | Extend fixture corpus and spec-section traceability linkage | @t81dev | 2026-05-15 | **Open** |
| **T81Lang — Type Frontend** | `spec/t81lang-spec.md` §§2-4 (Draft) | All language-level types are exposed in lexer/parser/semantic analyzer with deterministic lowering | Beta — collection determinism and enforcement hardening landed (BG-06 closure) with determinism coverage in place | None in this cycle | Maintain; monitor regression signals in determinism suites | @t81dev | — | **Closed** |
| **T81Lang — Serialization** | `spec/t81lang-spec.md` (Draft) | `serialize_canonical` is called from the language runtime for all types | Collection and T81Graph serialization wiring landed (BG-09 closure) and deterministic signatures verified | None in this cycle | Maintain; monitor serialization determinism regressions | @t81dev | — | **Closed** |
| **T81Lang — BigInt Precision** | `spec/t81lang-spec.md` (Draft) | T81BigInt supports arbitrary-precision integers | Oversized literals lower to `BigIntHandle`, serialize deterministically via `bigint_pool`, materialize in VM as canonical `BigInt/1` fraction, and do not silently truncate; JIT deopts `BigIntHandle` to interpreter path | None in this cycle | Maintain fail-closed narrowing behavior (`Frac2I`) and regression coverage | @t81dev | — | **Closed** |
| **T81Graph** | Surface inventory (non-normative) | T81Graph is lowered to VM native opcodes with deterministic serialization | VM lowering plus lang-side serialization wiring are complete; determinism coverage in place | None in this cycle | Maintain as governed non-DCP surface | @t81dev | — | **Closed** |
| **Axion Governance Kernel — Privileged Instruction Arbitration** | `spec/axion-kernel.md` §1.6 (Draft) | `AXREAD`/`AXSET`/`AXVERIFY` arbitration is governed through the policy engine | Implemented (bounded): policy engine and opcode-level arbitration hooks present | None in this cycle | Maintain | @t81dev | — | **Monitoring** |
| **Axion Governance Kernel — Policy Enforcement** | `spec/axion-kernel.md` §1.9 (Draft) | Policy parser, bytecode serialization, and verdict path are deterministic | Implemented (bounded): parser, serialization, and guard/trace paths present | None in this cycle | Maintain | @t81dev | — | **Monitoring** |
| **Axion Governance Kernel — Determinism Stewardship** | `spec/axion-kernel.md` §1.1 (Draft) | Canonical trace is steward across all Axion-visible memory transitions | Evidence mapping closure landed (AX-M5) | None in this cycle | Maintain; re-verify at Beta review cadence | @t81dev | — | **Closed** |
| **Axion Governance Kernel — CanonFS Observability** | `spec/axion-kernel.md` §1.10 (Draft) | CanonFS persistence lifecycle is fully auditable via Axion trace | Lifecycle observability evidence closure landed (AX-M6) | None in this cycle | Maintain; re-verify at Beta review cadence | @t81dev | — | **Closed** |
| **Axion Governance Kernel — Complexity Measurement** | `spec/axion-kernel.md` §1.3 (Draft) | Call-graph complexity and branch-path divergence are measurable | Complexity evidence closure landed (AX-M7) | None in this cycle | Maintain; re-verify at Beta review cadence | @t81dev | — | **Closed** |
| **Axion Governance Kernel — Tier Transition** | `spec/axion-kernel.md` §2.5 (Draft) | Full cognitive-tier promotion/demotion orchestration is governed by Axion | Deferred: tier-transition orchestration beyond policy ceilings is out of this cycle | Full transition orchestration is unimplemented | Governed alongside experimental cognitive-tier maturation post-2026-04-30 | @t81dev | 2026-06-15 | **Deferred** |
| **Axion OS Kernel — Runtime Integration** | `spec/rfcs/RFC-00B3-axion-kernel-architecture.md` | `hal_main` must hand off to a kernel-owned runtime and converge MMU, scheduler, IPC, and device seams into one runtime unit | Initial handoff is implemented in `experimental/ternaryos/`: `hal_main` calls `axion_kernel_main(...)`, which validates and summarizes kernel bootstrap state; full kernel runtime state and fault path are not integrated yet | Kernel entry exists but still only bootstraps summary state; checked MMU faults are not yet consumed by a kernel-facing reporting path | Add kernel fault records on top of `mmu_translate_checked()`, then connect scheduler/IPC/device state to the first persistent kernel runtime object | @t81dev | 2026-05-15 | **Open** |
| **T3K Quantization** | `spec/t3k-quantization-spec.md` | T3K GGUF quantization is bit-exact | Repro gate evidence plus governed spec artifact now present (`spec/t3k-quantization-spec.md`) | None in this cycle | Maintain | @t81dev | — | **Closed** |
| **Complex Binary Serialization** | `spec/t81-data-types.md` (Frozen) | T81Complex supports binary pool serialization | Binary pool serialization implemented and validated (BG-08 closure) | None in this cycle | Maintain | @t81dev | — | **Closed** |

---

## Summary Counts (2026-03-14)

| State | Count |
| :--- | :--- |
| Closed | 12 |
| Open | 2 |
| Deferred | 1 |
| Monitoring | 2 |

---

## Update Protocol

- Each row must be updated when the gap changes state.
- New drift discoveries must be added within the same PR.
- Closed rows stay in the table — they are historical evidence.
- "Deferred" means: scope is explicitly outside current planning cycle with an
  owner and a target date for re-evaluation.

---

## Cross-References

- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/HARDENING_BACKLOG.md`
- `docs/status/AXION_STATUS.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
