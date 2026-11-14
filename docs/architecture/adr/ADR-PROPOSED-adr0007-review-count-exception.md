# Proposed ADR: narrow review-count amendment for ADR-0007

Status: **Proposed / inactive / no policy change**
Date: 2026-11-14. Canonical number not assigned; ADR-0007 belongs to the separate
artifact-admission package at 781ec9ac8785a8449762f88d83e575fe73171dbd.

## Context and decision sought

The ownership charter and delegation policy require two approvals even for freeze
exceptions. Only @t81dev is identified as CODEOWNER; no second eligible reviewer
has been established. That does not make a one-maintainer/AI route valid under
current rules, nor does it prove every eligible reviewer must be a CODEOWNER.

The [proposal package](../../governance/proposals/2026-11-14-adr0007-approval-exception/README.md)
separates repository rules, GitHub enforcement and #790's draft association contract.
It proposes a narrowly scoped amendment with exact-SHA owner acceptance, retained
technical reviews, adversarial implementation review, Release-active negatives,
actual platform qualification and an approved dual-witness transition. It does
not assert amendment authority or activate an exception. The two-reviewer rule
remains unsatisfied until an authorized amendment/exception is actually recorded.

## Alternatives and consequences

Preferred compliant alternative is obtaining the required eligible approvals.
A one-use substitution requires policy amendment and a valid adoption basis; no
standing unilateral waiver was found. Lower-level ADR/log text cannot override
normative sources or bootstrap its own authority. No CI/branch-policy bypass is
proposed. Reviewers and future implementation targets remain unassigned/pending.

All unrelated freeze rules, ISA/VM semantics, raw13, numerical encodings and
historical evidence remain unchanged. Proposal publication is not policy activation,
implementation permission, merge/release approval or scientific-gate advancement.

## Authorship and validation

Codex authored ADR-0007's preceding packages and contributed to earlier Windows/
capture/#787 work. This is a disclosed self-authored governance proposal, not an
independent acceptance. Gemini's subsequent review remains pending, not fabricated.
Source inspection and read-only settings evidence are distinct from unexecuted
implementation/platform tests. Adoption/activation need separate actual records.
