# Research documentation index

Status: Active
Last Updated: April 2026

## Purpose

Research and exploratory artifacts that inform roadmap work.

## Scope & Research Track Structure

This README is a navigation aid for this directory. It does not change
specification authority, freeze boundaries, or determinism claim scope.

Research inquiries in T81 are organized into three coordinated tracks governed by the [Master Continuity Charter](continuity/MASTER_CONTINUITY_CHARTER_v1.0.md):

- **Track R (Scientific Computing & Alternative Representations):** Scientific ternary investigation, Base-81 digit grouping, adaptive tiering, and low-bit tensor inference (Canonical Hypotheses H01–H06).
- **Track F (Foundation Systems & Governance Engineering):** Deterministic execution (DCP), content-addressed storage (CanonFS / RFC-00D1), pre-side-effect policy authorization (Axion), and detached evidence verification (RFC-00D4) (Canonical Hypothesis H07).
- **Track M (AI-Assisted Research Methodology):** Auditable prompt-to-code provenance, error rate tracking, and trustworthy research iteration (Canonical Hypothesis H08).

## Key Research Consolidations & Decisions

- **[T81 Human–AI Research Initiative](/docs/research/HUMAN_AI_RESEARCH_INITIATIVE.md)** — Core charter, multi-AI collaboration protocol, prospective 12-month research roadmap, qualitative technical assessment, publication strategy, and 30-day action plan.
- **[Human–AI Initiative Governance Reconciliation Report](/docs/research/HUMAN_AI_INITIATIVE_RECONCILIATION_REPORT_2026-10-27.md)** — Authoritative post-merge reconciliation report of PR #607, restoring canonical H01–H08 definitions, correcting initiative chronology, removing arbitrary maturity scores, scrubbing unsupported model attributions, and aligning claims with primary evidence.

- **[Checked conversion contract proposal](../../experiments/checked_float_contract/README.md)** — Draft numerical semantics, independent bidirectional corpus, compatibility/decision record and bounded plan; no production replacement or new determinism claim.

- **[Native float conversion audit](../../experiments/float_conversion_audit/README.md)** — Exact binary64 reference, preserved numerical failures and explicit exclusion from evidence v2; production conversion remains unresolved.

- **[Deterministic execution and provenance, Phase 1](../../experiments/deterministic_provenance/README.md)** — Bounded production VM/Axion/CanonFS experiment, independent trace/result verifier, adversarial tests and native measurement limits.

- **[Ternary Research Closure and Next-Hypothesis Selection](TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md)** — Final evidence synthesis closing the CPU ternary arithmetic and inference research sequence (PRs #544–#549), marking CPU ternary MatVec optimization as frozen (STOP), and selecting Candidate 3 (Deterministic Execution & Canonical Provenance Verification in Policy-Gated AI Runtimes) as the next 30-day research objective.
- **[Research evidence ledger and architecture decision](consolidation/README.md)** — Consolidation of PRs #537–#541 establishing Option A (Governed Deterministic Runtime) as T81's core architecture foundation.

## Preserved / Frozen Research Reports (PRs #544–#549)

- **[Cross-Architecture Break-Even Analysis](TERNARY_INFERENCE_CROSS_ARCH_BREAK_EVEN_REPORT.md)** — *(Frozen)* Empirical break-even evaluation of 2-bit packed ternary vs INT8 vector inference across x86_64 AVX2 and ARM64 NEON.
- **[Packed Ternary MatVec Performance Report](PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)** — *(Frozen)* Performance analysis of 2-bit packed ternary MatVec and VPSHUFB SIMD lookup-table decoding.
- **[Ternary Arithmetic and Radix Comparison](TERNARY_ARITHMETIC_AND_RADIX_COMPARISON.md)** — *(Frozen)* Audited mathematical research on Base-81 digit grouping ($81 = 3^4$) and Avizienis redundant signed-digit arithmetic.

## Versioning Statement

Operational index only. Authority hierarchy remains:
/spec > docs/architecture/OVERVIEW.md > /docs > /book.

## Research Commons discovery

The [read-only Research Commons](commons/README.md) indexes canonical H01–H08,
negative evidence, reproduction instructions and unresolved discrepancies with
pinned source identities. It preserves the authoritative research record.
