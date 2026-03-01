# Risk Register

Status: Active
Last Updated: 2026-02-28
Owner: Project Management / Governance
Version: 1.0.0

## Purpose

Provide a canonical, tracked inventory of program risks with explicit owners,
severity ratings, mitigations, and resolution targets. Risks are elevated here
from PROJECT_CONTROL_CENTER §6, which no longer duplicates this content.

## Severity Model

| Probability | Impact | Severity |
| :--- | :--- | :--- |
| High | High | **Critical** |
| High | Medium | **High** |
| Medium | High | **High** |
| Medium | Medium | **Medium** |
| Low | Any | **Low** |
| Any | Low | **Low** |

## Risk Registry

| ID | Title | Category | Probability | Impact | Severity | Owner | Status | Date Opened | Target Resolution |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| RISK-001 | Required-context mismatch holds March release packet | Release / Governance | Low | High | **High** | @t81dev | Mitigated — GO on `1ec312e3` | 2026-02-26 | 2026-03-31 (C2 close) |
| RISK-002 | Determinism overclaim if external summaries omit registry boundaries | Determinism / Correctness | Medium | High | **High** | @t81dev | Monitoring | 2026-02-25 | Ongoing |
| RISK-003 | Single-owner concentration for governance and release approvals | Operational | Medium | Medium | **Medium** | @t81dev | Open | 2026-02-25 | 2026-04-30 |
| RISK-004 | AGI-facing surface growth outpacing promotion evidence updates | Governance / AGI | Medium | Medium | **Medium** | @t81dev | Monitoring | 2026-02-25 | Ongoing |
| RISK-005 | Experimental scope creep without promotion or retirement actions | Scope / Governance | Low | Medium | **Low** | @t81dev | Monitoring | 2026-02-25 | Ongoing |
| RISK-006 | Benchmark variability introducing false signal noise in guardrail gate | CI / Determinism | Low | Low | **Low** | @t81dev | Monitoring | 2026-02-26 | Ongoing |
| RISK-007 | Jekyll Pages build failure unresolved for future release candidates | CI / Release | Low | Low | **Low** | @t81dev | Deferred — tracked | 2026-02-28 | Post-C2 |
| RISK-008 | T81Graph lang-side serialization gap (BG-09) delays DCP candidacy | Implementation | Medium | Medium | **Medium** | @t81dev | Open (BG-09 in backlog) | 2026-02-28 | 2026-05-15 |
| RISK-009 | Axion Alpha posture with partial spec coverage delays Beta promotion | Implementation / Governance | Medium | High | **High** | @t81dev | Open | 2026-02-25 | 2026-04-30 |

## Risk Detail

### RISK-001 — Required-context mismatch holds March release packet

**Description:** Branch-protection required contexts (`quality gate / required`,
`Analyze (cpp)`) must be `completed` + `success` on the selected candidate SHA.
A mismatch keeps the packet in HOLD and delays release.

**Current State:** Mitigated. Both required contexts are `completed` + `success`
on `1ec312e3` as of 2026-02-28. GO recommendation stamped; pending final
@t81dev approval.

**Mitigation:** Verification procedure documented in
`docs/status/RELEASE_READINESS_PACKET_2026-03.md`. Re-run before executing C2
runbook on 2026-03-31.

---

### RISK-002 — Determinism overclaim

**Description:** Governance or release artifacts may be summarized externally
without registry-bounded language, creating overclaim exposure. Determinism
claims are valid only for surfaces listed as `Verified` in
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md`.

**Current State:** Monitoring. Registry-bounded language is enforced in all
current status artifacts. Governance checks (`governance-metrics` CI gate) are
green on `1ec312e3`.

**Mitigation:** Maintain `governance-metrics` CI gate; enforce registry-bounded
claim language in all external-facing documents.

---

### RISK-003 — Single-owner concentration

**Description:** All governance, release, and approval actions are gated on a
single approver (@t81dev). An unavailability window blocks all decision stamps.

**Current State:** Open. No deputy owner or approval delegation policy exists.

**Mitigation (target):** Define a deputy-approval policy and document it in
`docs/governance/APPROVAL_DELEGATION.md` before 2026-04-30.

---

### RISK-004 — AGI surface growth outpacing promotion evidence

**Description:** As the AGI-facing surface grows (cognitive tiers, llama.cpp
adapter, governed inference), promotion evidence in the governed AGI promotion
pipeline may lag behind, creating coverage gaps.

**Current State:** Monitoring. Pipeline defined at
`docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`; current experimental surfaces
are classified as non-DCP by default.

**Mitigation:** Tie surface inventory refresh (`docs/status/T81LANG_SURFACE_INVENTORY.md`)
to each monthly governance cadence.

---

### RISK-005 — Experimental scope creep

**Description:** Experimental subsystems grow without explicit promotion or
retirement decisions, blurring the boundary between normative and non-normative
surfaces.

**Current State:** Monitoring. Inventory maintained at
`docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md`.

**Mitigation:** Any new experimental component must be registered in the
inventory on the same commit it is introduced.

---

### RISK-006 — Benchmark variability

**Description:** Environment-sensitive benchmark variability may cause the
`benchmark / vm workload gate` CI guardrail to produce false pass/fail signals.

**Current State:** Monitoring. Gate is informational; variability bounded by
fixed reference hardware profiles in CI.

**Mitigation:** Review guardrail thresholds when benchmark results diverge >5%
across consecutive runs on identical environments.

---

### RISK-007 — Jekyll Pages build failure

**Description:** `build` (GitHub Pages / Jekyll) workflow fails due to Liquid
`{{ }}` syntax in `third_party/llama.cpp` modelcard templates. Non-required for
release but persistent noise.

**Current State:** Deferred. Classified non-blocking for C++ release artifact.
Resolution: exclude `third_party/` from Jekyll rendering scope (file tracking
issue).

**Mitigation:** File GitHub issue to scope `_config.yml` exclusions. No action
before C2 close.

---

### RISK-008 — T81Graph serialization gap (BG-09)

**Description:** T81Graph has VM-level opcode lowering but no lang-side
`serialize_canonical()` invocation from the language runtime. This prevents
DCP candidacy and determinism test coverage.

**Current State:** Open. Tracked as BG-09 in
`docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md`.

**Mitigation:** BG-09 resolution is a prerequisite for T81Graph DCP scope
change. Target: 2026-05-15 alongside T81Lang Beta target.

---

### RISK-009 — Axion Alpha posture delays Beta promotion

**Description:** Axion Kernel is Alpha with partial spec coverage. Gaps in
clause-coverage and opcode dispatch concentration affect governance confidence
for Beta promotion candidacy.

**Current State:** Open. Coverage alignment tracked in
`docs/status/AXION_STATUS.md`. Opcode dispatch concentration reduced in recent
sprint (Phase 3 conformance expansion).

**Mitigation:** Track open coverage gaps via `docs/status/AXION_STATUS.md`.
Target Beta candidacy review at 2026-04-30 milestone.

---

## Cadence

- Reviewed and refreshed at each monthly governance cadence (C2 close window).
- New risks must be logged within the same PR that identifies them.
- Resolved risks are moved to the **Closed** table below.

## Closed Risks

| ID | Title | Resolution | Closed Date |
| :--- | :--- | :--- | :--- |
| RISK-010 | CodeQL push trigger missing on `main` | `ad6c2777` added push trigger to `codeql.yml` | 2026-02-26 |

## Cross-References

- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/RELEASE_READINESS_PACKET_2026-03.md`
- `docs/status/AXION_STATUS.md`
- `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md`
- `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`

## Versioning Statement

This register is an operational governance artifact. It does not override
`/spec`, freeze policy, or determinism registry boundaries.
