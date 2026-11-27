# Exact maintainer decision requested — blank, not an approval

Two distinct decisions must not be collapsed into a chat acknowledgement or PR merge.

## Decision A: route and amendment authority

Choose one, in an actual retained maintainer record:

- **Keep current policy / HOLD:** retain two eligible reviewer approvals; designate
  and verify reviewers without asserting that two CODEOWNER accounts are required.
- **Return proposal for revision:** specify changes; nothing activates.
- **Pursue the scoped amendment:** approve the exact amendment text at the proposal SHA for ratification;
  identify the authority/process under which the two-reviewer rule
  can validly be amended, including its ratifiers and evidence. If that authority
  cannot be established, remain HOLD under current policy. Sole ownership/admin
  ability or this text is not sufficient evidence of that power.

For the third route, actual decision text must explicitly say:

> I approve the proposed ADR-0007-only review-count amendment text at exact
> proposal SHA [fill actual SHA], targeting package
> 781ec9ac8785a8449762f88d83e575fe73171dbd. The amendment authority and valid adoption
> process are [identify, cite and retain actual ratification]. I acknowledge that
> the present two-reviewer requirement is unsatisfied and is not replaced by AI
> technical reviews or by this proposal's publication.

Approval of proposed text still needs valid ratification; it is not automatic activation or future
implementation authorization. Gemini's subsequent technical proposal review is
pending; retain it with reviewed SHA and authorship disclosure, not as a fabricated
human approval. A revised proposal requires a new exact-SHA decision.

## Decision B: later activation/exercise (not requested for execution now)

Only after a valid amendment exists may a separate actual record identify its
ratified policy SHA, approved exception-proposal SHA, exact package target, scope,
expiry, owner and revocation conditions and say that the named scoped substitution
is active. It must preserve the actual unsatisfied historical approval count.
No activation is performed in this task, even if Decision A is later supplied.

Implementation and witness-transition targets are currently null. Any future
exercise must identify each exact SHA, retain adversarial technical review,
Release-active failure evidence, actual target qualification and the approved
witness transition. Unavailable evidence blocks exercise. A package approval never
populates those missing targets or authorizes a compiler switch/merge/release.

The rule that resolves the authority gap must be adopted legitimately, not merely
printed in a new exception record. No special “founder waiver,” eligibility class,
branch-protection bypass or self-approving mechanism is invented here.
