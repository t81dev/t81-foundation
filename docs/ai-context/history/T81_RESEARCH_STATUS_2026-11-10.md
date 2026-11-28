# T81 Foundation — PR and Research Continuity Review (2026-11-10)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation — PR and Research Continuity Review (2026-11-10)](#t81-foundation-—-pr-and-research-continuity-review-2026-11-10)
  - [Executive state](#executive-state)
  - [Current pull-request triage](#current-pull-request-triage)
  - [Suggested integration ordering](#suggested-integration-ordering)
  - [Evidence and freshness rules](#evidence-and-freshness-rules)

<!-- T81-TOC:END -->


**Classification:** Non-normative, dated GitHub review; not an independent test execution, approval, benchmark, or merge authorization.
**Repository:** https://github.com/t81dev/t81-foundation
**Observed scope:** GitHub open-PR search, selected PR metadata, merged-PR history, and repository AI-context files, reviewed 2026-11-10. Search inventory is a point-in-time observation; recheck before acting.
**Supersedes:** 2026-11-09 handoff *operational next actions* only. Historical findings and dated snapshots remain intact.

## Executive state

- **SCG-07:** PRs #736–#741 merged. Phase 6 independent arithmetic qualification is **QUALIFIED WITH LIMITATIONS**, not universal correctness or physical advantage. F3 is a 54-trit modular result, and F7 is bounded binary64 approximation rather than universally correctly rounded conversion. The historical 10.6x software scalar ternary addition penalty remains a binding negative result.
- **SCG-08 Phase 0:** Research charter integrated by merged PR #753, merge commit `0bd0817e4b9dc879fffce18c59ea9ff781f7acb3`. Original PR #745 was closed **without merge** as superseded. Phase 0 integration does not authorize harness implementation or experimental execution.
- **SCG-08 Phase 1:** Addendum 03 reconciles canonical signed representation range, stratified cluster delta-method variance, and finite burn-in transients. The signed B-bit domain is a proposed preregistration amendment requiring an authoritative recorded decision. The arithmetic-result semantics (full-precision vs checked overflow vs modular) must be specified consistently for binary and ternary. No Phase 1 harness implementation or Gate 2 campaign is evidenced as authorized by this repository review. A draft Jules instruction in chat is not itself a merged governance decision.
- **Issues:** #717 was closed as completed following merged SCG-07 repairs and consolidated qualification; #723 (H04-R03 correctness/API/retention boundary) and #555 (dual-rail/POPCNT research inquiry) remain open at the latest checked issue audit. Do not infer acceptance from a closed issue alone.

## Current pull-request triage

At the time of this search, **67 open PRs were returned** (search cap 100). This is an inventory, not a claim that every PR has been independently reviewed.

| PR | Subject | Review disposition / dependency |
| --- | --- | --- |
| [#747](https://github.com/t81dev/t81-foundation/pull/747) | CI runner and portability repair | Open; GitHub reports not mergeable at review. Resolve conflicts and verify changed logic and hosted CI before integration. |
| [#748](https://github.com/t81dev/t81-foundation/pull/748) | CanonFS block metadata authorization | Open, stacked on #747; independent policy boundary review and regression proof required. |
| [#750](https://github.com/t81dev/t81-foundation/pull/750) | CLI explicit execution-policy enforcement | Open, stacked on #747; fail-closed policy semantics require review. |
| [#751](https://github.com/t81dev/t81-foundation/pull/751) | Skills policy-validation argv hardening | Open, stacked on #750 and indirectly #747. |
| [#752](https://github.com/t81dev/t81-foundation/pull/752) | H04-R03 retention historical/current status overlay | Open, stacked on #747; preserve Phase C freeze and bounded registered-object retention claims. |
| [#732](https://github.com/t81dev/t81-foundation/pull/732) | SCG-05 generic Result feasibility | Open, not mergeable at review; proposal only, not SDK/API authorization. |
| [#718](https://github.com/t81dev/t81-foundation/pull/718), [#719](https://github.com/t81dev/t81-foundation/pull/719) | ARCH-003 R2 deadline authority proposal and correction | Both open, not mergeable at review; do not activate R2 by implication. |
| [#714](https://github.com/t81dev/t81-foundation/pull/714), [#716](https://github.com/t81dev/t81-foundation/pull/716) | ARCH-002 protocol / ARCH-003 I2 accounting | Open; #716 claims bounded accounting evidence but explicitly withholds independent acceptance and correctness-core pass. |
| [#709](https://github.com/t81dev/t81-foundation/pull/709), [#707](https://github.com/t81dev/t81-foundation/pull/707) | CPA-004 freestanding TISC experiment / opcode conformance tests | Open; do not treat reported PR-local evidence as merged baseline. |
| [#699](https://github.com/t81dev/t81-foundation/pull/699), [#698](https://github.com/t81dev/t81-foundation/pull/698) | BASE-01 budget amendment / physical qualification | Open; earlier physical-host-unavailable finding must not be rewritten as physical qualification. |

## Suggested integration ordering

1. Review #747 independently, reconcile its baseline and failing checks. It is a large cross-platform change (26 files), not a routine green-CI checkbox.
2. Once #747 is accepted, rebase/retarget #748 and #750 individually and recheck their exact changed-file scope, authorization semantics, regression tests, and hosted CI.
3. Then rebase/retarget #751 after #750; handle documentation-only #752 separately against accepted main.
4. Keep older research proposals and stacked implementation branches separate until their individual preregistration, acceptance, and authority gates are satisfied.
5. Finalize SCG-08 Phase 1 signed-domain and arithmetic-result contract in a separate reviewed document/PR; Gate 1 implementation and Gate 2 execution require distinct recorded authorizations.

## Evidence and freshness rules

- This document is a **GitHub metadata and document reconciliation**, not a local build, sanitizer run, physical-host experiment, or independent verification of every open PR.
- GitHub mergeability is a transient snapshot; recheck after any base-branch movement.
- Do not equate theoretical balanced-ternary carry advantages with lower CPU cycles, power, cost, or native semiconductor advantage.
- `CURRENT_STATE.json.selected_view` remains a historical 2026-11-02 partial view; this overlay must not silently rewrite its older evidence.
- Preserve SCG-07 qualification limitations, H04-R03 registered-object/physical-reclamation limits, historical STOP findings, and all existing governance restrictions.
