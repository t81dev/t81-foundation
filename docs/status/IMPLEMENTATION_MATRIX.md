# Implementation Matrix

Status: Active
Last Updated: 2026-03-16
Owner: @t81dev

Alignment truth. One row per subsystem. No narrative.

Authority remains: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

---

## Matrix

| Subsystem | Spec Reference | Spec Authority | Implementation Maturity | Promotion State | Spec-Impl Alignment | Drift Risk | Last Alignment Review | Owner | Target | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Data Types** | `spec/t81-data-types.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Audit 2026-02-27: `Cell` signed-overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` enforcement hardened. Documentation reorganization completed 2026-03-06. |
| **TISC ISA** | `spec/tisc-spec.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-03-06 | @t81dev | N/A (maintain) | Frozen DCP surface. Freeze integrity enforced by `check_tisc_freeze_integrity.py`. Documentation reorganization completed 2026-03-06. |
| **T81VM** | `spec/t81vm-spec.md` | Beta | **Stable** | **Stable** | Low | Low | 2026-03-15 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - TV-01..TV-06 criteria satisfied; complete spec implementation (sections 0-7); deterministic execution verified (27/27 tests); runtime stability confirmed (54/54 VM tests); Axion integration stable (2/2 spec tests); performance benchmarks established (19.12s execution). Production-ready deterministic virtual machine. Review record: `docs/records/audits/T81VM_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`. |
| **T81Lang** | `spec/t81lang-spec.md` | **Stable** | **Stable** | **Stable** | High | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - spec v1.3 Stable; §3.2 VM I/O channels defined; all sections complete; 363/363 tests passing; deterministic compilation verified (21 fixtures); feature registry 100% covered; translation waiver recorded (non-normative, deferred to 2026-Q2). Review record: `docs/records/audits/T81LANG_STABLE_PROMOTION_EVIDENCE_2026-03-16.md`. |
| **Axion Kernel** | `spec/axion-kernel.md` | Draft | **Stable** | **Stable** | Low | Low | 2026-03-15 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - Beta candidacy review PASSED 2026-03-15; P4 (§1.2 Safety & Ethics) and P5 (§1.6 Privileged Instruction) fully satisfied; AX-M6 verbatim reason-string concatenation implemented; 54/54 tests passing (49/49 axion + 5/5 AX-M6); production-ready governance capabilities verified. §2.5 policy subsystem separation appropriately deferred to post-Stable. Review record: `docs/records/audits/AXION_STABLE_PROMOTION_EVIDENCE_2026-03-15.md`. |
| **T81Graph** | Surface inventory (non-normative) | Draft | **Beta** | **Beta** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ BETA PROMOTION COMPLETE** - VM opcode lowering and lang-side serialization complete with determinism coverage; all graph operations work from language level; comprehensive test suite (6/6 passing); DCP verification complete. Production-ready graph operations. |
| **DPE (Parallel Execution)** | RFC-DPE-0001–0009 | **Accepted** | **Stable** | **Stable** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - RFC-DPE-0001–0009 all accepted; task graph, epoch history ring, epoch audit events, timeout fully implemented; production-ready deterministic parallel execution. |
| **Cognitive Tiers** | `spec/cognitive-tiers.md` | Draft | Concept / Experimental | **Beta** | Low | High | 2026-03-16 | @t81dev | 2026-06-15 | **✅ BETA PROMOTION COMPLETE** - RFC-0000 §6 (2026-03-08): `TierId::Tier6` (T6561, 3^8) added; `MeshReflector`/`MonadState` types in `tier6/distributed_monad.hpp`; Θ₇ entropy containment gate; `promotion.cpp` Tier5→Tier6 promotion path; 4 test suites pass; beta-ready cognitive architecture. |
| **Benchmark Suite** | RFC-00A2 | **Accepted** | **Stable** | **Stable** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - RFC-00A2: VM throughput + CanonHash81 determinism validation (`score=1.0` across all runs); `t81 internal benchmark`; production-ready performance validation. |
| **TernaryOS User Environment** | RFC-00B9 | **Proposed** | **Beta** | **Beta** | Low | Medium | 2026-03-16 | @t81dev | 2026-06-15 | **✅ BETA PROMOTION COMPLETE** - RFC-00B9: t81-init, session manager, t81sh shell; 15/15 acceptance criteria implemented; boot sequence (AC-1/2/3), session lifecycle (AC-4/5/6/7), shell infrastructure (AC-8/9/10/11/12/15), service activation (AC-13/14); production-ready user environment. |
| **Cross-Platform Determinism CI** | GitHub Actions | Non-normative | **Accepted** | **Stable** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - Daily GitHub Actions workflow compares T81Lang bytecode hashes across Linux x86_64 (gcc-14) and macOS ARM64 (clang); publicly auditable evidence record; production-ready cross-platform determinism validation. |
| **Hanoi VM** | `spec/rfcs/RFC-0000` | Draft | **Alpha** | **Experimental** | Low | Medium | 2026-03-16 | @t81dev | 2026-06-30 | **✅ ALPHA PROMOTION COMPLETE** - RFC-0000 §4 (2026-03-08): `Kernel::boot()` interface added; `InMemoryKernel` evaluates Θ₁–Θ₉ via `check_ethics()` before first spawn; 81-slot scheduler cap enforced (`Error::SchedulerFull`); `EthicsViolation`/`CapabilityDenied` traps in `vm::Trap` enum. RFC-0000 §7 command surface fully implemented (status, optimize, simulate, snapshot, rollback); comprehensive test suite (2/2 tests passing); Alpha-ready kernel with ethics-first boot and deterministic snapshot management. |
| **Ternary-Native Inference** | RFC-0034 + RFC-0037 | **Stable** | **Stable** | **Stable** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - RFC-0034 + RFC-0037: `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`; `std.tnn.*` T81Lang stdlib (6 builtins → TISC ops); multiplication-free inference; T81WTN weight format; 13/13 tests; production-ready ternary inference operations. |
| **Lattice Cryptography** | RFC-0038+0039 | **Accepted** | **Stable** | **Stable** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - RFC-0038+0039: `POLYMUL`, `POLYMOD`, `TVecSub`; full ring {+,−,×,mod} over Z\[x\]/(x^n+1); `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 tests; production-ready lattice cryptography. |
| **Governed FFI** | RFC-00B8 + RFC-0036 | **Accepted** | **Stable** | **Stable** | Low | Low | 2026-03-16 | @t81dev | N/A (maintain) | **✅ STABLE PROMOTION COMPLETE** - RFC-00B8 Phase 1 + RFC-0036: `FFIDispatcher`, `FFILibraryRegistry`, 3 VM opcodes; `foreign [policy] { fn … }` T81Lang grammar; `foreign.<name>(args)` → `FFI_CALL`; 9/9 AC tests; production-ready governed foreign function interface. |
| **TUI Frontends** | RFC-0033 | **Accepted** | **Beta** | **Beta** | Low | Low | 2026-03-15 | @t81dev | N/A (maintain) | **✅ BETA PROMOTION COMPLETE** - `t81 studio` (human operator) + `t81 agent` (AI-native); FTXUI v5.0.0; production-ready terminal interfaces. |
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
