# F01 BOLQ Contract Migration — Independent Review Plan

**NON-NORMATIVE / PROPOSAL ONLY / NOT ADOPTED / NOT QUALIFIED**  
Prepared: 2026-11-17. No implementation, execution, host acquisition, active gate/schema/lock edits or merge authorized.

## Source and decision pins

- PR #813 decision memorandum: `473770c7a4aaf83327eaaededd1de14bad177e4c`.
- PR #814 candidate amendment: `0e4532e7d9813bd1447244041cbd3d99fca88bed`; inspect `CANDIDATE_AMENDMENT.md` A1–A6, `HOST_RECEIPT_FIELD_DICTIONARY.md`, `PR811_APPLICABILITY.md`, and `TRACEABILITY.md`.
- PR #811 provisioning plan: `d0edeb486993860d733abc04b2ce740b35442874`.
- PR #812 original unsupported guard feasibility: `8ced4f076c3ea18594175a8e0a72885f54bb3c42`.
- Existing operative F01 protocol SHA-256: `d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`.
- Existing `limits_demonstrated:true` semantics and `linux_relock_required=true` remain unchanged.
- Human acknowledged conditional design acceptance and STOP. This does not constitute adoption or acceptance of residual physical-overrun risk.

## Proposed migration sequence — each step requires its own future authorization

**M0: Independent documentation reconciliation (GO for review only).** Check exact source pins, original obligations O01–O10, proposed clauses L01–L08, PR #814 A1–A6, and all existing hard deterministic checks. Identify contradictory or missing terms. Produce an evidence-linked review; do not modify operative artifacts.

**M1: Human model and residual-risk decision (STOP pending explicit human approval).** Require a separately attributable decision that explicitly names F01-HOST-BOLQ-v1, scope, bounded observed rather than universal assurance, permitted exclusions, possible actual wall/CPU/memory overrun, delayed or unknown teardown, lost receipts, and unresolved Linux failure paths. Approval of drafting is not approval of risk.

**M2: Complete prospective protocol version (STOP).** Compose a new complete protocol under a new version/path, preserving original bytes/history. Publish a new digest, exact changed paragraphs, unchanged science and numerical ceilings, and mapping to O01–O10/L01–L08. Original protocol digest remains historical and operative until explicit adoption. No in-place reinterpretation.

**M3: Receipt and parser/gate design (STOP).** Propose distinct `t81.f01.host-qualification.bolq.v1` receipt, schema and explicit version-aware routing. Legacy `limits_demonstrated:true` cannot be reused, aliased, auto-upgraded or satisfied by a BOLQ record. Define required field types, unknown/duplicate rejection, max 8192 UTF-8 core bytes, referenced raw evidence within 25MiB, raw evidence accessibility, human provenance verification, stable identity and uncertainty rules. Design only; no schema/parser/gate code edits.

**M4: Provisioning-specific applicability (STOP).** Keep PR #811 timeline (P0–P4 44s/43s; FINAL 54s/48s, T0 and Ta, 53/47 last-write cutoffs, 55–56s manager expiry, 58s latest fallback) distinct from the A–F envelope. Define prospective fault plan and acceptance criteria; no mechanism configuration proves response latency. Keep 60s/50s, 256MiB, 128MiB helper, 64MiB controller+guard, zero swap, one CPU, 25MiB evidence, and no reset/retry.

**M5: Implementation/host qualification (STOP).** Requires new source review, model/contract adoption, actual host and interpreter identities, rights/containment/reaping/clock/evidence feasibility, admission approvals, implementation authorization and scoped fault qualification. A passing finite scenario cannot claim universal prevention; known violation remains FAILED, missing attempted evidence INCOMPLETE, untested mandatory faults NOT VERIFIED.

**M6: A–F scientific use (STOP).** Separate complete protocol amendment, scientific/admissibility review, source/host/receipt gate migration, and single-run human authorization. A host PASS or successful provisioning never supplies A–F permission.

## Non-negotiable review invariants

1. A stop **request** is not completed stop, exit, reaping or durability.
2. Known physical limit exceedance is sticky **FAILED**, even with missing terminal record.
3. An attempted but unproven exit/reaping is **INCOMPLETE** unless a conclusive violation is observed.
4. Untested mandatory faults are **NOT VERIFIED** and block aggregate PASS.
5. Expected abort response may PASS its own assertion while operation completion remains INCOMPLETE.
6. No new post-kill writer, uncharged observer, privilege, leaf, CPU, evidence allowance or budget reset.
7. Configuration readbacks, admission checks and empirical outcomes must remain separate.
8. Measurement interval upper bound within threshold can support PASS; justified lower bound above threshold FAILED; straddling interval INCOMPLETE.
9. The same scope and fault plan must be committed prospectively, never adjusted to rescue a failure.
10. Legacy qualification and A–F remain isolated from candidate receipt.
11. Model adoption, residual-risk acceptance, source preparation, provisioning, qualification and one-run scientific execution are distinct human decisions.
12. Original PR #812 UNSUPPORTED finding must not be erased by proposing a different assurance claim.

## Independent reviewer questions

- Does the new assurance model silently weaken any deterministic pre-side-effect invariant or authorization check?
- What exact Linux/kernel/manager/cgroup/rights assumptions would be needed for each measured scenario?
- What happens to the proposed receipt when controller and guard both die, a task enters D-state, or evidence persistence is unknown?
- Can any field combination yield aggregate PASS despite a mandatory FAILED/INCOMPLETE/NOT VERIFIED criterion?
- Is the proposed 8192-byte receipt plus referenced manifests actually feasible within 25MiB without discarding required raw evidence? **Not yet demonstrated.**
- Does the proposal avoid retroactively changing legacy `limits_demonstrated:true` meaning?
- Are provisioning and A–F scopes and timelines kept distinct?
- Is human risk acceptance separately explicit, attributable and verifiable, not assumed from PR merge or checksum?

## Reviewer disposition

Return **READY FOR HUMAN MODEL/RISK DECISION**, **CORRECTIONS REQUIRED**, or **NOT SUPPORTED**. Cite exact clauses and source pins. Documentation review alone cannot make M1–M6 GO.

## Present state

**GO:** read-only independent review of this planning document.  
**STOP:** adoption, active protocol/receipt/schema/gate changes, implementation, host work, qualification, provisioning, A–F, merge.  
`linux_relock_required=true`.
