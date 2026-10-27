# Status documentation

Purpose: navigation and evidence vocabulary. Evidence date: 2026-10-27.
Audited main: `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.

Start with the [control center](PROJECT_CONTROL_CENTER.md), then follow the relevant evidence. Status prose is descriptive: [specification authority](../governance/SPEC_AUTHORITY_MODEL.md), the [surface register](../governance/GOVERNANCE_SURFACE_REGISTER.md) and [determinism registry](../governance/DETERMINISM_SURFACE_REGISTRY.md) control contracts and classifications. Legacy paths or conflicting authority claims in retained records do not define a new hierarchy.

## Evidence vocabulary

Implementation labels describe different evidence properties; they are not automatic promotion steps.

| Label                     | Meaning                                                                          |
| ------------------------- | -------------------------------------------------------------------------------- |
| Specified                 | Documented contract with no confirmed implementation.                            |
| Prototype                 | Executable experimental implementation.                                          |
| Implemented               | Confirmed source path providing the behavior.                                    |
| Tested                    | Executed tests with documented scope, commit and environment.                    |
| Measured                  | Reproducible quantitative evidence with protocol and inputs.                     |
| Comparatively established | Valid matched controls support the stated comparative conclusion.                |
| Historical                | Previously accurate or historically asserted; not a current operational verdict. |
| Unsupported               | Insufficient evidence for the claim.                                             |

Research dispositions: **Supported within tested scope**, **Not supported within tested scope**, **Inconclusive**, **Not tested**, **Superseded**. A report's historical verdict is distinguished from this audit's disposition. Canonical [H01–H08](../research/continuity/HYPOTHESIS_REGISTER_v1.json) differ from numbered investigations and H-S1 subclaims.

Governance remains **DCP / verified deterministic**, **governed non-DCP**, **experimental / non-DCP**, with out-of-scope behaviors as recorded by the controlling register. Tests, implementation completeness, production readiness and comparative advantage are separate properties. No classification changes are made here.

## Operational navigation

| Document                                                                | Purpose                                                                            |
| ----------------------------------------------------------------------- | ---------------------------------------------------------------------------------- |
| [PROJECT_CONTROL_CENTER](PROJECT_CONTROL_CENTER.md)                     | Concise operational overview.                                                      |
| [IMPLEMENTATION_MATRIX](IMPLEMENTATION_MATRIX.md)                       | Primary source/test/maturity matrix.                                               |
| [ACTIVE_RISKS](ACTIVE_RISKS.md)                                         | Active risks and bounded mitigations.                                              |
| [DECISION_LOG](DECISION_LOG.md)                                         | Preserved chronological decision records, including known identifier ambiguity.    |
| [ROADMAP](ROADMAP.md)                                                   | Forward plan; historical long-horizon proposals retained separately.               |
| [TASKS](TASKS.md)                                                       | Immediate actions and acceptance evidence.                                         |
| [SYSTEM_STATUS](SYSTEM_STATUS.md)                                       | Current interpretation plus historical subsystem/milestone record.                 |
| [STATUS_RECONCILIATION_2026-10-27](STATUS_RECONCILIATION_2026-10-27.md) | Complete inventory, disposition, evidence, scientific reassessment and validation. |

## Specialized evidence and records

| Area                        | Documents                                                                                                                                                                                                                                                                                                                        |
| --------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| AI implementation           | [CLI matrix](AI_CLI_IMPLEMENTATION_MATRIX.md), [milestone evidence](AI_CLI_MILESTONE_EVIDENCE.md), [RFC backlog](AI_RFC_BACKLOG.md), [bounded family](BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md).                                                                                                                                    |
| Boundaries and review       | [Frozen profile](FROZEN_CORE_PROFILE.md), [extensions](EXTENSION_PROFILE.md), [clarification](GOVERNANCE_BOUNDARIES_CLARIFICATION.md), [cadence](GOVERNANCE_REVIEW_CADENCE.md).                                                                                                                                                  |
| Determinism and remediation | [Audit log](DETERMINISM_AUDIT_LOG.md), [drift](DRIFT_DECOMPOSITION.md), [JIT gap](JIT_EQUIVALENCE_GAP.md), [crosswalk](AUDIT_REMEDIATION_CROSSWALK.md), [hardening](HARDENING_BACKLOG.md).                                                                                                                                       |
| CI and dependencies         | [Gate status](CI_GATE_STATUS.md), [workflow confirmation](CI_WORKFLOW_CONFIRMATION.md), [dependency health](DEPENDENCY_HEALTH.md).                                                                                                                                                                                               |
| Language evidence           | [Traceability](T81LANG_TRACEABILITY_MATRIX.md), [promotion history](T81LANG_PROMOTION_GATE.md), [March snapshot](T81LANG_PROMOTION_GATE_SNAPSHOT.md), [March queue](T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md), [stdlib snapshot](STDLIB_PROMOTION_SNAPSHOT.md), [float-domain decision](RFC_0030_FLOAT_DOMAIN_PROMOTION.md). |
| Architecture context        | [Ternary Fabric note](TERNARY_FABRIC_STRATEGIC_CONVERGENCE_NOTE.md).                                                                                                                                                                                                                                                             |

All 31 original tracked Markdown files remain present. Some are historical snapshots or placeholders despite living under `status/`; the old index incorrectly described several as archived. Retained timestamps refer to original records, while dated evidence-review sections state current qualifications. No snapshot generator was run to overwrite prior evidence.
