# Project Control Center

Purpose: concise operational overview. Evidence date: 2026-10-27.
Audited main: `c930b8c2ee7478782bf3c6d231bdd59984595ef2`.

T81 is an experimental computing and systems research platform with an implemented deterministic, policy-gated runtime. The runtime supports research into alternative representations, governed computation and artifact identity; it is not evidence of general ternary superiority or production readiness.

| Dimension             | Current evidence and limit                                                                                                                                                                                                                                                                                                                                                    |
| --------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Implementation        | Interpreter, language frontend, Axion evaluation, CanonFS storage/interchange, CLI and bounded model probes exist. The [implementation matrix](IMPLEMENTATION_MATRIX.md) names source and test boundaries.                                                                                                                                                                    |
| Research              | Canonical H01–H08 remain intact. Representation facts and scoped negative software results do not settle native devices. Investigation 07 has provisional mapped-area evidence, no timing/energy proof; Investigation 08 comparative integration/assurance advantage is inconclusive. See [research assessment](STATUS_RECONCILIATION_2026-10-27.md#scientific-reassessment). |
| Governance            | DCP / verified interpreter and numeric surfaces retain their recorded status. Axion, CanonFS, toolchain and inference remain governed non-DCP; JIT has no whole-surface equivalence promotion. [Registers control classification](../governance/GOVERNANCE_SURFACE_REGISTER.md), not this dashboard.                                                                          |
| CI                    | Live main CI and CodeQL were queued at inspection; no current GO. Branch protection requires `quality gate / required` and `Analyze (cpp)`. Research regression results are a separate slice. [CI evidence](CI_GATE_STATUS.md).                                                                                                                                               |
| Known blockers        | Architecture-target checker fails on unchanged base; traceability test failure signaling is insufficient; Investigation 08 lacks equivalent model execution and matched replay/timing. [Active risks](ACTIVE_RISKS.md).                                                                                                                                                       |
| Historical milestones | March language/VM/Axion stability reviews and April bounded-family milestones remain historical accomplishments, not current blanket GREEN/GO stamps. [Decision history](DECISION_LOG.md), [system record](SYSTEM_STATUS.md).                                                                                                                                                 |

Immediate priorities:

1. Finish this status reconciliation with reviewable evidence and preserve historical findings.
1. Address validation integrity in separate changes: architecture discovery/name drift, traceability failure signaling, and CI logging. Open PRs are pending work, not fixes in this baseline.
1. Preserve the CLI/runtime and RFC-00D1 seed contracts through narrow negative-path, example and portability maintenance.
1. Continue discriminating research under the [Master Continuity Charter](../research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md): H04 radix/adaptation controls, matched conventional execution, and scoped hardware measurements before more architecture.

Use [README](README.md) for navigation, [ROADMAP](ROADMAP.md) for sequencing,
[TASKS](TASKS.md) for acceptance signals, and the [audit report](STATUS_RECONCILIATION_2026-10-27.md) for provenance and uncertainty. No new dashboard authority or activation boundary is introduced.
