# Research documentation index

Status: Active
Last Updated: April 2026

## Purpose

Research and exploratory artifacts that inform roadmap work.

## Scope

This README is a navigation aid for this directory. It does not change
specification authority, freeze boundaries, or determinism claim scope.

## Key Research Consolidations & Decisions

- **[Ternary Research Closure and Next-Hypothesis Selection](TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md)** — Final evidence synthesis closing the CPU ternary arithmetic and inference research sequence (PRs #544–#549), marking CPU ternary MatVec optimization as frozen (STOP), and selecting Candidate 3 (Deterministic Execution & Canonical Provenance Verification in Policy-Gated AI Runtimes) as the next 30-day research objective.
- **[Research evidence ledger and architecture decision](consolidation/README.md)** — Consolidation of PRs #537–#541 establishing Option A (Governed Deterministic Runtime) as T81's core architecture foundation.

## Preserved / Frozen Research Reports (PRs #544–#549)

- **[Cross-Architecture Break-Even Analysis](TERNARY_INFERENCE_CROSS_ARCH_BREAK_EVEN_REPORT.md)** — *(Frozen)* Empirical break-even evaluation of 2-bit packed ternary vs INT8 vector inference across x86_64 AVX2 and ARM64 NEON.
- **[Packed Ternary MatVec Performance Report](PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)** — *(Frozen)* Performance analysis of 2-bit packed ternary MatVec and VPSHUFB SIMD lookup-table decoding.
- **[Ternary Arithmetic and Radix Comparison](TERNARY_ARITHMETIC_AND_RADIX_COMPARISON.md)** — *(Frozen)* Audited mathematical research on Base-81 digit grouping ($81 = 3^4$) and Avizienis redundant signed-digit arithmetic.

## Versioning Statement

Operational index only. Authority hierarchy remains:
/spec > docs/architecture/OVERVIEW.md > /docs > /book.
