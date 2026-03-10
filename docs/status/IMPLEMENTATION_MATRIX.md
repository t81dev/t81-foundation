# Implementation Matrix

Status: Active
Last Updated: 2026-03-08
Owner: @t81dev

Alignment truth. One row per subsystem. No narrative.

Authority remains: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

---

## Matrix

| Subsystem | Spec Reference | Spec Authority | Implementation Maturity | Promotion State | Spec-Impl Alignment | Drift Risk | Last Alignment Review | Owner | Target | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Data Types** | `spec/t81-data-types.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Audit 2026-02-27: `Cell` signed-overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` enforcement hardened. Documentation reorganization completed 2026-03-06. |
| **TISC ISA** | `spec/tisc-spec.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Freeze integrity enforced by `check_tisc_freeze_integrity.py`. Documentation reorganization completed 2026-03-06. |
| **T81VM** | `spec/t81vm-spec.md` | Beta | Beta | Beta | Medium | Low | 2026-03-08 | @t81dev | 2026-04-15 | Non-JIT path DCP-verified. FW-02 closure landed: Axion opcode pre-dispatch moved outside main VM switch. RFC-0026 phase-1 extended: all six opcodes have runtime semantics. TLOADHASH null-canonfs SEGFAULT fixed (2026-03-08): null-guard with hash-format validation; `set_canonfs_root()` API added to `IVirtualMachine`; DecodeFault/BoundsFault taxonomy enforced. 332/332 tests passing. |
| **T81Lang** | `spec/t81lang-spec.md` | Draft | **Beta** | Beta (impl) / Draft (spec) | Medium | Low | 2026-03-08 | @t81dev | 2026-05-15 | BG-06/BG-07/BG-08/BG-09 closed. Frontend refactor complete (2026-03-08): typed AST (`Expr::resolved_type`), unified builtin registry (`kBuiltinTable`, 130 entries), IRGen extracted to `ir_generator.cpp`; SA dispatch ordering fixed (table-driven fallback after custom handlers). 332/332 tests passing. |
| **Axion Kernel** | `spec/axion-kernel.md` | Draft | **Beta** | Beta | Medium | Medium | 2026-03-10 | @t81dev | 2026-04-30 | §1.6/1.9 implemented (bounded). AX-M5..M7 evidence closures landed; pending Beta candidacy review cycle. §2.5 remains deferred. Documentation reorganization completed 2026-03-06. |
| **T81Graph** | Surface inventory (non-normative) | Draft | Draft | Experimental | Medium | Medium | 2026-03-06 | @t81dev | 2026-05-15 | VM opcode lowering and lang-side serialization wiring complete with determinism coverage. Surface remains governed non-DCP until promotion. Documentation reorganization completed 2026-03-06. |
| **Cognitive Tiers** | `spec/cognitive-tiers.md` | Draft | Concept / Experimental | **Experimental** | Low | High | 2026-03-08 | @t81dev | 2026-06-15 | Experimental, non-DCP. RFC-0000 §6 (2026-03-08): `TierId::Tier6` (T6561, 3^8) added; `MeshReflector`/`MonadState` types in `tier6/distributed_monad.hpp`; Θ₇ entropy containment gate; `promotion.cpp` Tier5→Tier6 promotion path. Tiers 0–6 now modelled. |
| **Hanoi VM** | `spec/rfcs/RFC-0000` | Draft | **Alpha** | **Experimental** | Low | Medium | 2026-03-08 | @t81dev | 2026-06-30 | RFC-0000 §4 (2026-03-08): `Kernel::boot()` interface added; `InMemoryKernel` evaluates Θ₁–Θ₉ via `check_ethics()` before first spawn; 81-slot scheduler cap enforced (`Error::SchedulerFull`); `EthicsViolation`/`CapabilityDenied` traps in `vm::Trap` enum. Still experimental/non-DCP; no formal spec beyond RFC-0000 normative clauses. |
| **Governed llama.cpp** | `docs/records/archive/project-reports/llama-governed-repro.md` (guidance) | Non-normative | Experimental | **Governed non-DCP** | Medium | Medium | 2026-02-28 | @t81dev | 2026-04-30 | Classified governed non-DCP (DEC-003). Practical reproducibility only. Promotion requires governed AGI pipeline. |

---

## Governed AGI Surface Taxonomy

| Layer | Paths | Determinism Status | Promotion State | Governance Gate |
| :--- | :--- | :--- | :--- | :--- |
| Deterministic Substrate | `core/types`, `core/isa`, `core/vm`, `include/t81/**` | DCP / registry Verified | Verified | Freeze enforcement + DCP release discipline |
| Governance Kernel | `kernel/axion` | Partially verified, scope-bounded | Alpha | Axion evidence milestones + incident-response |
| AGI Runtime / Research | `runtime/tracing`, `experimental/*`, cognitive tiers | Non-DCP unless promoted | Experimental | Governed AGI promotion pipeline |
| Governed Inference | `third_party/llama.cpp`, `tooling/model/`, CLI `llama-run` | Governed non-DCP | Experimental | Governed AGI pipeline + release boundary classification |

---

## Update Rules

1. `Last Alignment Review` must be updated whenever any implementation work touches that subsystem.
2. Any row with Drift Risk = High and no closure plan within 90 days must be escalated.
3. Registry status changes (Verified ↔ Partial) require a matrix update on the same PR.
4. See `GOVERNANCE_REVIEW_CADENCE.md §3` for the full registry update procedure.

---

## Cross-References

- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/HARDENING_BACKLOG.md`
- `docs/status/EXTENSION_PROFILE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/architecture/adr/`

## Versioning Statement

Descriptive control artifact; does not override `/spec` or freeze policy.
Experimental, non-DCP, non-verified unless promoted through governance
