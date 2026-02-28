# Drift Decomposition

Last Updated: 2026-02-28
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
| **T81VM (non-JIT path)** | `spec/t81vm-spec.md` (Beta) | Reference interpreter is deterministic for all TISC opcodes | Beta — all DCP-scoped opcodes covered by tests; opcode dispatch concentration in policy bridge partially reduced | Dispatch concentration in policy-trace bridge not fully extracted | Extract remaining `AxCheck`/`AxReport` helpers; Phase 3 conformance matrix expansion | @t81dev | 2026-04-15 | **Open** |
| **T81Lang — Compiler Bytecode** | `spec/t81lang-spec.md` §5 (Draft) | Compilation from source to TISC bytecode is bit-exact | Fixture-bounded: 16 corpus programs produce deterministic output; full spec-section traceability gap was partially closed 2026-02-25 | Bytecode emission is partially traceable — full deterministic compilation-profile language not yet complete | Extend fixture corpus and spec-section traceability linkage | @t81dev | 2026-05-15 | **Open** |
| **T81Lang — Type Frontend** | `spec/t81lang-spec.md` §§2-4 (Draft) | All language-level types are exposed in lexer/parser/semantic analyzer with deterministic lowering | Beta — 36 types tracked in surface inventory; `List`/`Map`/`Set`/`Tree`/`Quaternion`/`Prob`/`Cell` exposed with deterministic lowering stubs | Collection types use `Vector` polyfills (O(N) ops); native type enforcement incomplete for 4 collection types | BG-06: Add determinism tests for List/Map/Set/Tree. Collection type enforcement hardening. | @t81dev | 2026-05-15 | **Open** |
| **T81Lang — Serialization** | `spec/t81lang-spec.md` (Draft) | `serialize_canonical` is called from the language runtime for all types | 10 types have `serialize_canonical` in C++ headers; language runtime does not invoke it for collection types or T81Graph | Runtime serialization gap: lang-side canonical serialization not wired up for collections and graph | BG-09: Wire `serialize_canonical` into language runtime | @t81dev | 2026-05-15 | **Open** |
| **T81Lang — BigInt Precision** | `spec/t81lang-spec.md` (Draft) | T81BigInt supports arbitrary-precision integers | VM aliases BigInt to 64-bit; >64-bit literals are silently truncated | Precision gap: unbounded BigInt behavior unimplemented | BG-07: Implement native arbitrary-precision opcodes or define promotion path | @t81dev | 2026-05-15 | **Open** |
| **T81Graph** | Surface inventory (non-normative) | T81Graph is lowered to VM native opcodes with deterministic serialization | VM native opcode lowering complete (PR #424, 2026-02-28); lang-side serialization not wired; no determinism tests | Two gaps: (1) lang-side `serialize_canonical` not invoked, (2) no determinism tests | BG-09 (serialization) + add determinism test suite | @t81dev | 2026-05-15 | **Open** |
| **Axion — Privileged Instruction Arbitration** | `spec/axion-kernel.md` §1.6 (Draft) | `AXREAD`/`AXSET`/`AXVERIFY` arbitration is governed through the policy engine | Implemented (bounded): policy engine and opcode-level arbitration hooks present | None in this cycle | Maintain | @t81dev | — | **Monitoring** |
| **Axion — Policy Enforcement** | `spec/axion-kernel.md` §1.9 (Draft) | Policy parser, bytecode serialization, and verdict path are deterministic | Implemented (bounded): parser, serialization, and guard/trace paths present | None in this cycle | Maintain | @t81dev | — | **Monitoring** |
| **Axion — Determinism Stewardship** | `spec/axion-kernel.md` §1.1 (Draft) | Canonical trace is steward across all Axion-visible memory transitions | Partial: selected events covered; full post-GC/compaction enforcement mapping incomplete | Canonical-memory enforcement traceability gap across all transitions | M5: close evidence mapping by 2026-03-10 | @t81dev | 2026-03-10 | **Open** |
| **Axion — CanonFS Observability** | `spec/axion-kernel.md` §1.10 (Draft) | CanonFS persistence lifecycle is fully auditable via Axion trace | Partial: hook and segment-event trace present; end-to-end lifecycle audit incomplete | Persistence lifecycle audit closure gap | M6: map evidence for full lifecycle by 2026-03-12 | @t81dev | 2026-03-12 | **Open** |
| **Axion — Complexity Measurement** | `spec/axion-kernel.md` §1.3 (Draft) | Call-graph complexity and branch-path divergence are measurable | Partial: instruction and recursion limits covered; call-graph/path-divergence evidence missing | Call-graph complexity evidence gap | M7: map evidence by 2026-03-14 | @t81dev | 2026-03-14 | **Open** |
| **Axion — Tier Transition** | `spec/axion-kernel.md` §2.5 (Draft) | Full cognitive-tier promotion/demotion orchestration is governed by Axion | Deferred: tier-transition orchestration beyond policy ceilings is out of this cycle | Full transition orchestration is unimplemented | Governed alongside experimental cognitive-tier maturation post-2026-04-30 | @t81dev | 2026-06-15 | **Deferred** |
| **T3K Quantization** | `spec/t3k-quantization-spec.md` (missing) | T3K GGUF quantization is bit-exact | Verified by `scripts/ci/t3k_repro_gate.py`; no formal spec document exists | Spec gap: no `spec/t3k-quantization-spec.md` | Author T3K spec or explicitly classify T3K as governed non-spec surface | @t81dev | 2026-04-30 | **Open** |
| **Complex Binary Serialization** | `spec/t81-data-types.md` (Frozen) | T81Complex supports binary pool serialization | Binary pool serialization absent in `binary_io.cpp` | Persistence gap for T81Complex | BG-08: Implement binary pool round-trip | @t81dev | 2026-05-15 | **Open** |

---

## Summary Counts (2026-02-28)

| State | Count |
| :--- | :--- |
| Closed | 2 |
| Open | 11 |
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
