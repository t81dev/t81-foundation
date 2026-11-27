# Proposed ADR-0007 approval-count exception — documentation only

**DRAFT / INACTIVE. Existing two-reviewer requirement remains unsatisfied.**
This package does not grant a waiver, amend active policy, implement runtime code,
merge anything, change protection, publish a release or advance scientific gates.
Gemini reviews this proposal afterward; no such review is claimed yet.

## Conclusion and decision needed

[Authority review](AUTHORITY_REVIEW.md): the ownership charter explicitly requires
two approvals and the delegation policy preserves them even for freeze exceptions.
No standing unilateral waiver power was found in the inspected sources. A one-
maintainer route with retained technical reviews therefore needs a **scoped policy
amendment**, not an ordinary exception approval or administrative GitHub capability.
The amendment-adoption authority/process is not established in those sources;
this proposal cannot create that power by declaring itself authorized.

The maintainer must choose current-policy HOLD / obtain eligible reviewers,
request revision, or approve the exact amendment text at the proposal SHA for ratification
and identify its valid adoption authority/ratification process. See the exact blank
[decision form](MAINTAINER_DECISION.md). No activation decision is exercised now.

The existing rules do not explicitly require two CODEOWNER accounts or two GitHub
APPROVED review events. GitHub settings and #790's stronger draft association
record contract are separate controls. Known CODEOWNER @t81dev does not establish
another person's eligibility; AI reviews cannot be represented as formal human or
GitHub approvals. This clarifies earlier Codex wording without modifying history.

## Scope and proposed path

- Policy baseline: main 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2.
- Exact decision package: [#790 at 781ec9ac8785a8449762f88d83e575fe73171dbd](https://github.com/t81dev/t81-foundation/tree/781ec9ac8785a8449762f88d83e575fe73171dbd/docs/records/decisions/2026-11-14-artifact-admission).
- Reviewed design: 07968a9a307415bf8a7aa6f04056caa22db01c78 (#789).
- [Dormant amendment text](PROPOSED_AMENDMENT.md): exact-SHA maintainer approval;
  retained scoped technical reviews/authorship disclosures; adversarial final-code
  review; Release-active negative tests; actual cross-platform qualification;
  approved dual-witness transition. Unsupported evidence is blocked, not waived.
- Future implementation/witness-transition SHAs are null and need separate exact
  decisions. No branch/descendant wildcard, automatic scope expansion or activation.
- #790's build-association review requirement needs an explicit narrow addendum if
  changed; never fill actual approval slots with AI reviews. Other constraints stay.
- No change to DM-01–DM-08, ISA/VM/trap semantics, raw13 APIs, numeric encodings,
  caps, historical bytes, witnesses, required checks or unrelated freeze rules.

GitHub read-only evidence reports main strict required checks quality gate /
required and Analyze (cpp), without an exposed required-review rule. That does not
satisfy or waive repository policy. App-access errors and authenticated read-only
fallback responses are both retained. No setting was changed or bypass proposed.

## Evidence and authorship

[Proposed governance ADR](../../../architecture/adr/ADR-PROPOSED-adr0007-review-count-exception.md)
is descriptive and nonactivating. evidence/ contains immutable source hashes,
line-accurate excerpts, settings responses/commands, and documentation verification.
decision-template.json contains only pending/null decision/review/evidence slots.
It is not an approval record. New implementation tests/platform runs have not occurred.

Codex authored this proposal and the earlier ADR-0007/release packages and has
Windows/capture/#787 contributions. It cannot provide independent acceptance of
its own work. Retain Gemini's actual later review at its reviewed SHA, including
shared context and author contributions; a separate role/model is not proof of
independence. Do not manufacture reviewer eligibility or count either AI role as a
formal GitHub approval. This is a concrete decision instrument, not activated policy.
