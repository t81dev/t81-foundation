# Implementation Matrix

Status: Active
Last Updated: 2026-02-28
Owner: @t81dev

Alignment truth. One row per subsystem. No narrative.

Authority remains: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

---

## Matrix

| Subsystem | Spec Reference | Spec Authority | Implementation Maturity | Promotion State | Spec-Impl Alignment | Drift Risk | Last Alignment Review | Owner | Target | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Data Types** | `spec/t81-data-types.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-02-27 | @t81dev | N/A (maintain) | Frozen DCP surface. Audit 2026-02-27: `Cell` signed-overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` enforcement hardened. |
| **TISC ISA** | `spec/tisc-spec.md` | **Frozen** | Implemented | **Verified** | High | Low | 2026-02-28 | @t81dev | N/A (maintain) | Frozen DCP surface. Freeze integrity enforced by `check_tisc_freeze_integrity.py`. |
| **T81VM** | `spec/t81vm-spec.md` | Beta | Beta | Beta | Medium | Medium | 2026-02-28 | @t81dev | 2026-04-15 | Non-JIT path DCP-verified. Policy-bridge dispatch concentration partially reduced (Phase 3). Open: FW-02. |
| **T81Lang** | `spec/t81lang-spec.md` | Draft | **Beta** | Beta (impl) / Draft (spec) | Medium | Medium | 2026-02-28 | @t81dev | 2026-05-15 | 36 types tracked. BG-06..09 open. Fixture-bounded determinism only. |
| **Axion Kernel** | `spec/axion-kernel.md` | Draft | **Alpha** | Alpha | Medium | Medium | 2026-02-28 | @t81dev | 2026-04-30 | §1.6/1.9 implemented (bounded). §1.1/1.3/1.10 partial. §2.5 deferred. AX-M5..M7 open. |
| **T81Graph** | Surface inventory (non-normative) | Draft | Draft | Experimental | Low | High | 2026-02-28 | @t81dev | 2026-05-15 | VM opcode lowering complete. Lang-side serialization not wired (BG-09). No determinism tests. Non-DCP. |
| **Cognitive Tiers** | `spec/cognitive-tiers.md` | Draft | Concept / Experimental | **Experimental** | Low | High | 2026-02-25 | @t81dev | 2026-06-15 | Experimental, non-DCP, non-verified unless promoted through governance. |
| **Hanoi VM** | — | — | Concept | **Experimental** | — | High | 2026-02-25 | @t81dev | — | Experimental only. No spec. Non-DCP. |
| **Governed llama.cpp** | `docs/how-to/llama-governed-repro.md` (guidance) | Non-normative | Experimental | **Governed non-DCP** | Medium | Medium | 2026-02-28 | @t81dev | 2026-04-30 | Classified governed non-DCP (DEC-003). Practical reproducibility only. Promotion requires governed AGI pipeline. |

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
