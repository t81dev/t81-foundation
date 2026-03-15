# Implementation Matrix

Status: Active
Last Updated: 2026-03-14
Owner: @t81dev

Alignment truth. One row per subsystem. No narrative.

Authority remains: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

---

## Matrix

| Subsystem | Spec Reference | Spec Authority | Implementation Maturity | Promotion State | Spec-Impl Alignment | Drift Risk | Last Alignment Review | Owner | Target | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Data Types** | `spec/t81-data-types.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Audit 2026-02-27: `Cell` signed-overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` enforcement hardened. Documentation reorganization completed 2026-03-06. |
| **TISC ISA** | `spec/tisc-spec.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Freeze integrity enforced by `check_tisc_freeze_integrity.py`. Documentation reorganization completed 2026-03-06. |
| **T81VM** | `spec/t81vm-spec.md` | Beta | Beta | Beta | Medium | Low | 2026-03-14 | @t81dev | 2026-04-15 | Non-JIT path DCP-verified. FW-02 closure landed: Axion opcode pre-dispatch moved outside main VM switch. RFC-0026 phase-1 extended: all six opcodes have runtime semantics. TLOADHASH null-canonfs SEGFAULT fixed (2026-03-08): null-guard with hash-format validation; `set_canonfs_root()` API added to `IVirtualMachine`; DecodeFault/BoundsFault taxonomy enforced. 338/338 tests passing. |
| **T81Lang** | `spec/t81lang-spec.md` | Draft | **Beta** | **Beta** | Medium | Low | 2026-03-15 | @t81dev | 2026-05-15 | **✅ BETA PROMOTION COMPLETE** - TG-01..TG-06 gate criteria satisfied; deterministic compilation verified (21 fixtures, hash verified); semantic conformance complete (7/7 tests passing); comprehensive test coverage (36/36 tests); spec sections 0-7 fully implemented. Translation staleness identified as low-priority maintenance. Review record: `docs/records/audits/T81LANG_BETA_PROMOTION_EVIDENCE_2026-03-15.md`. |
| **Axion Kernel** | `spec/axion-kernel.md` | Draft | **Stable** | **Stable** | Low | Low | 2026-03-15 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - Beta candidacy review PASSED 2026-03-15; P4 (§1.2 Safety & Ethics) and P5 (§1.6 Privileged Instruction) fully satisfied; AX-M6 verbatim reason-string concatenation implemented; 54/54 tests passing (49/49 axion + 5/5 AX-M6); production-ready governance capabilities verified. §2.5 policy subsystem separation appropriately deferred to post-Stable. Review record: `docs/records/audits/AXION_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`. |
| **T81Graph** | Surface inventory (non-normative) | Draft | **Beta** | **Beta** | Low | Low | 2026-03-15 | @t81dev | 2026-05-15 | **✅ READY FOR BETA PROMOTION** - VM opcode lowering and lang-side serialization complete with determinism coverage; all graph operations work from language level; comprehensive test suite (6/6 passing); DCP verification complete. Experimental status outdated. |
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
