# F01 BOLQ Contract Migration — Independent Review Plan

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [F01 BOLQ Contract Migration — Independent Review Plan](#f01-bolq-contract-migration-—-independent-review-plan)
  - [Source and decision pins](#source-and-decision-pins)
  - [Proposed migration sequence — each step requires its own future authorization](#proposed-migration-sequence-—-each-step-requires-its-own-future-authorization)
  - [Non-negotiable review invariants](#non-negotiable-review-invariants)
  - [Independent reviewer questions](#independent-reviewer-questions)
  - [Reviewer disposition](#reviewer-disposition)
  - [Present state](#present-state)

<!-- T81-TOC:END -->


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

**M4: Provisioning-specific applicability (STOP).** Keep PR #811 timeline (P0–P4 44s/43s; FINAL reserve 10s/5s with cumulative quiescence 54s/48s, T0 and Ta, 53/47 last-write cutoffs, 55–56s manager expiry, 58s latest fallback) distinct from the A–F envelope. Define prospective fault plan and acceptance criteria; no mechanism configuration proves response latency. Keep 60s/50s, 256MiB, 128MiB helper, 64MiB controller+guard, zero swap, one CPU, 25MiB evidence, and no reset/retry.

Preserve the **49,408-state suite ceiling**, including controls and rotations. Area F retains its **254 planned states within the 256-state cap**; the plan is not a claim of executed states or an increased allowance. Separately preserve the proposed **1 GiB provisioning volume** and **921 MiB allocation**: 128 MiB read-only inputs + 512 MiB installed payload + 256 MiB staging + 25 MiB evidence. Provisioning storage allowances do not transfer to A–F; its 25 MiB uncompressed artifact ceiling remains unchanged.

**M2–M4 reconciliation prerequisite (STOP).** Before any M5 implementation begins, the complete prospective protocol (M2), versioned receipt/schema/routing design (M3), and provisioning applicability (M4) must be reconciled, reviewed together and co-pinned as one exact bundle of versions, source revisions and content digests. Resolve incompatible meanings, thresholds, field requirements and scope before implementation authorization. Changing any component requires renewed reconciliation/co-pinning; completing one stage alone is not sufficient.

**M5: Separately authorized implementation and distinct host gates (STOP).** After the M1 decision, applicable model/contract adoption and the M2–M4 prerequisite, source implementation still requires its own explicit authorization, exact scope and new source review. That permission does not supply host/provider selection, installation, lock activation or qualification rights. Decompose the host decisions using the exact gate names in [pinned PR #811, Approvals and remaining blockers](https://github.com/t81dev/t81-foundation/blob/d0edeb486993860d733abc04b2ce740b35442874/docs/research/proposals/RFC-00D7-F01-HOST-PROVISIONING/PLAN.md#approvals-and-remaining-blockers):

| Distinct PR #811 gate | Required scope; separation retained |
|---|---|
| H0 host selection/acquisition/admin setup | Exact host/account/image/boot and administrative scope; does not authorize building, installation, qualification or A–F |
| H1 provider/build artifact acceptance | Accountable builder, source/patch suitability, separately bounded build if needed, toolchain/recipe and immutable artifact identities; does not authorize target provisioning or qualification |
| H2 one bounded offline provisioning attempt | Exact plan/host/inputs/installer, limits, permitted interpreter/diagnostic execution and cleanup; does not authorize native imports, stress probes, lock activation, qualification or A–F |
| H3 independent provisioning receipt review | Review raw identities, observations, limits and retained negative/incomplete evidence; does not clear the blocker or establish host qualification |
| H4 active re-lock change | Necessary target evidence, exact derived source, exhaustive installed lock and regenerated identities; requires explicit separate permission to change the blocker, and does not authorize probes or A–F |
| H5 Linux qualification | Separate exact-source/host authorization and evidence for bounded enforcement/native-compatibility probes; does not authorize contract experiments |

These are distinct decision scopes, not an assumed strictly linear schedule. If preliminary native-compatibility evidence is needed before H4, H5 needs its own separately approved probe source and budget; never clear the blocker merely to run a probe. Final H5 acceptance must name the actual post-integration source/host identities. None of these approvals exists by virtue of this document. Actual interpreter/host identities, rights, containment, reaping, clocks and evidence feasibility remain prerequisites. A passing finite scenario cannot claim universal prevention; known violation remains FAILED, missing attempted evidence INCOMPLETE, untested mandatory faults NOT VERIFIED.

**M6: A–F scientific use (STOP).** Separate complete protocol amendment, scientific/admissibility review, source/host/receipt gate migration, and single-run human authorization. A host PASS or successful provisioning never supplies A–F permission. PR #811's H6 one A–F run remains a separate decision outside M5/H0–H5.

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
