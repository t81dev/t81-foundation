# Implementation vs. Specification Matrix

Status: Active
Last Updated: 2026-02-28
Owner: Status / Engineering

## Purpose

Track implementation maturity relative to specification surfaces and make drift
risk explicit for planning and governance review.

## Matrix

| Subsystem | Specification Surface | Implementation Maturity | Alignment | Drift Risk | Owner | Target Date | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Data Types** | `spec/t81-data-types.md` (Frozen) | Implemented | High | Low | @t81dev | N/A (maintain) | Frozen deterministic core surface. Determinism audit completed 2026-02-27 (PRs #414, #415): `Cell` signed-overflow UB fixed (`Cell::safe_add`); `T81Float` signed-zero canonicalization enforced; `T81Map`/`T81Set` frontend type-enforcement hardened. See `docs/reports/determinism_types_audit.md`. |
| **TISC ISA** | `spec/tisc-spec.md` (Frozen) | Implemented | High | Low | @t81dev | N/A (maintain) | Frozen deterministic core surface. |
| **T81VM** | `spec/t81vm-spec.md` (Beta) | Beta | Medium | Medium | @t81dev | 2026-04-15 | Beta surface under active verification. |
| **T81Lang** | `spec/t81lang-spec.md` (Draft) | Beta | Medium | Medium | @t81dev | 2026-05-15 | Implementation maturity promotion to Beta decisioned on 2026-02-26 in `docs/status/T81LANG_PROMOTION_GATE.md` (spec authority remains Draft). Drift decomposition tracked in `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md`; executable closure queue tracked in `docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md`; promotion gate tracked in `docs/status/T81LANG_PROMOTION_GATE.md`; ranked engineering backlog tracked in `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md`; A1..A1G closure recorded 2026-02-25; BG-01..BG-05 closed 2026-02-25. 2026-02-26..28 additions: `List`/`Map`/`Set`/`Tree` exposed as first-class frontend types (PR #401, #419); `T81Quaternion`/`T81Prob`/`Cell` exposed in Lexer/Parser/Semantic Analyzer with deterministic lowering (PR #420); `serialize_canonical` added to `T81List`, `T81Set`, `T81Tree`, `T81Complex`, `T81Symbolic`, `T81Polynomial`, `T81Time`, `T81Entropy`, `T81Promise`, `T81Agent`; `t81lang_surface_gate_test` added and AST/IR repro hash baseline refreshed; stress test suite added (`tests/stress/`, 7 modules, PR #404); surface gap inventory published in `docs/status/T81LANG_SURFACE_INVENTORY.md`; new engineering backlog items BG-06..BG-10 opened from gap inventory. |
| **Axion Kernel** | `spec/axion-kernel.md` (Draft) | Alpha | Medium | Medium | @t81dev | 2026-04-30 | Alpha posture with partial-coverage alignment tracked in `docs/status/AXION_PARTIAL_COVERAGE_ALIGNMENT_2026-03.md`; planning milestones M1-M4 synchronized/closed on 2026-02-25 with explicit open-scope boundaries retained. |
| **Cognitive Tiers** | `spec/cognitive-tiers.md` (Draft) | Concept / Experimental | Low | High | @t81dev | 2026-06-15 | Experimental, non-DCP, non-verified unless promoted through governance and determinism registry update. |
| **T81Graph** | `docs/status/T81LANG_SURFACE_INVENTORY.md` (Draft / non-normative) | Draft | Low | High | @t81dev | 2026-05-15 | VM native opcode lowering completed 2026-02-28 (PR #424). Lang-side canonical serialization gap remains open (BG-09): native C++ `serialize_canonical()` is not invoked from the language runtime. Determinism tests: NO. No DCP scope change without registry update. |
| **Governed llama.cpp Path** | `docs/how-to/llama-governed-repro.md` (Guidance, non-normative) | Experimental | Medium | Medium | @t81dev | 2026-04-30 | Governed inference surface (`third_party/llama.cpp`, `tooling/model/llama_cpp_adapter.cpp`, `t81 llama-run`) is explicitly governed non-DCP; practical reproducibility only; promotion requires governed AGI pipeline and registry alignment. |

## Governed AGI Surface Taxonomy

| Layer | Representative Paths | Determinism Status | Promotion State | Governance Gate |
| :--- | :--- | :--- | :--- | :--- |
| Deterministic Substrate | `core/types`, `core/isa`, `core/vm`, `include/t81/**` | DCP/registry bounded | Verified (where registry says Verified) | Freeze enforcement + DCP release discipline |
| Governance Kernel | `kernel/axion` | Partially verified, scope-bounded | Experimental/Verified Candidate by surface | Incident response + threat-model linkage |
| AGI-Oriented Runtime/Research | `runtime/tracing`, `experimental/*`, cognitive tiers | Non-DCP unless promoted | Experimental by default | Governed AGI promotion pipeline |
| Governed Inference Adapter | `third_party/llama.cpp`, `tooling/model/llama_cpp_adapter.cpp`, CLI `llama-run` | Governed non-DCP | Experimental by default | Governed AGI promotion pipeline + release boundary classification |

Promotion policy reference:

- `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`

## Planning Actions

1. Prioritize high-drift subsystem decomposition into measurable milestones.
2. Keep deterministic-core surfaces stable under freeze and DCP discipline.
3. Tie matrix refresh to monthly governance review cadence.
4. Record boundary-impacting alignment decisions in ADRs.
5. Maintain AGI-surface promotion state and determinism-status mapping per the
   governed AGI promotion pipeline.

## Cross-References

- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/SYSTEM_STATUS.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/architecture/adr/`

## Versioning Statement

This matrix is a descriptive control artifact; it does not override `/spec` or
freeze policy.
