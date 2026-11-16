# Authority review — repository policy is not branch protection

**Proposal only. Current two-reviewer requirement remains unsatisfied.**
Policy baseline: main 9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2. Exact source hashes,
line excerpts and read-only GitHub settings responses are retained in evidence/.

## Exact requirements

| Authoritative source | Exact requirement / consequence |
| --- | --- |
| [MODULE_OWNERSHIP_CHARTER lines 50–54](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/MODULE_OWNERSHIP_CHARTER.md#L50-L54) | “Frozen modules require two approvals for” spec, determinism and public API changes |
| [Same charter lines 67–73](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/MODULE_OWNERSHIP_CHARTER.md#L67-L73) | Mandatory controls: ADR entry, registry update, explicit freeze-exception label, “Two reviewer approvals” |
| [APPROVAL_DELEGATION lines 48–51](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/APPROVAL_DELEGATION.md#L48-L51) | Primary/deputy roles do not remove the count: “Freeze exceptions still require two approvals and ADR evidence.” Deputy self-approval of authored GO/HOLD is prohibited |
| [Same policy lines 23–26](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/APPROVAL_DELEGATION.md#L23-L26) | Delegation does not weaken the freeze/authority controls it references |
| [SPEC_AUTHORITY_MODEL lines 78–84](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/SPEC_AUTHORITY_MODEL.md#L78-L84) | Breaking change requires major version, RFC approval, migration guide and changelog |
| [Same model lines 95–113](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/SPEC_AUTHORITY_MODEL.md#L95-L113) | Boundary decisions require an ADR; ADRs are descriptive and do not override normative /spec definitions |
| [FREEZE_ENFORCEMENT section 3](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/FREEZE_ENFORCEMENT.md#L76-L97) | Breaking RFC/spec, implementation matrix, migration, major version, changelog and previous-major disposition are required |
| [Same policy section 4](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/governance/FREEZE_ENFORCEMENT.md#L101-L119) | Verified-surface determinism breaches stop merging; a review-count exception cannot waive divergence |
| [RELEASE_DISCIPLINE lines 26–32](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/product/RELEASE_DISCIPLINE.md#L26-L32) | Release requires required checks and no open freeze exception or verified-surface determinism breach |
| [Same manifest lines 110–113](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/product/RELEASE_DISCIPLINE.md#L110-L113) | Relaxing release preconditions requires explicit governance review and ADR traceability; not a generic waiver power |
| [DECISION_LOG lines 620–623](https://github.com/t81dev/t81-foundation/blob/9256b6ec57a09e07799ae69a6f7b5467fd6ca4d2/docs/status/DECISION_LOG.md#L620-L623) | Operational entries do not override /spec, freeze policy or registry boundaries |

CODEOWNERS identifies @t81dev only. The charter requires at least one primary
owner per module, not two CODEOWNER accounts. It says two approvals/reviewer
approvals; it does **not explicitly prescribe two GitHub APPROVED API objects**.
Eligibility and authentic approval evidence still matter. We do not invent an
eligible second reviewer or an evidence-channel interpretation. AI/shared-effort
technical reviews cannot be represented as human or formal GitHub approvals.

Refinement to earlier Codex wording: #790's draft build-association contract asks
for two actual review records/URLs and no self-approval. That is an additional
proposed contract, not proof that the existing repository policy mandates exactly
two GitHub review events. Its constraints need their own explicit addendum if the
review basis changes; governance relaxation alone does not silently satisfy them.

## Can the maintainer unilaterally authorize this exception now?

**No standing authority to waive the count was found in the inspected rules.**
Being the named primary approver is not an exception to the sentence requiring
two approvals even for freeze exceptions. A one-maintainer route substituting
retained technical reviews therefore requires a **scoped policy amendment**, not
merely a normal freeze-exception label, chat design acceptance, administrative
merge ability or an operational decision-log entry.

The inspected sources do not specify a general sole-founder power or a complete
bootstrap procedure for amending the approval-count rule when a second eligible
reviewer is unavailable. That authority gap is unresolved. An owner must establish
a valid amendment-adoption basis/process, or use the existing two-reviewer route.
This proposal cannot confer its own ratification power. An exact-SHA maintainer
approval is necessary for the proposed route, but insufficient under current rules
without a validly adopted amendment. Undocumented external authority is not guessed.

Historical spec annotations authorized by @t81dev are bounded additive corrections,
not a prospective waiver of review thresholds. DEC-015/C2 concerned a pinned PR775
CI override; it explicitly was exceptional and is not transferable to ADR-0007.
No precedent is treated as general authority to weaken current rules.

## GitHub enforcement observation (read-only, main only)

The app connector could not read protection (403) or route the effective-rules
endpoint; authenticated read-only gh api fallback succeeded. Raw responses and
commands/timestamps are retained, including connector errors.

- main is protected. Classic protection reports strict=true and required contexts
  quality gate / required and Analyze (cpp).
- The classic response has no required_pull_request_reviews field; effective
  rules for main returned []. No required-review rule was exposed by these reads.
- enforce_admins=false; force-push/deletion permissions are false. Administrative
  technical capability does not grant policy authority or permission to bypass.

These are settings observations, not proof that policy is satisfied. Main's
settings do not describe protection of the stacked #790 target branch. No GitHub
settings were changed. Re-read current settings at any future integration decision;
no bypass is requested. If future branch rules require reviews, they remain
independent and must be satisfied without pretending AI or self-reviews qualify.
