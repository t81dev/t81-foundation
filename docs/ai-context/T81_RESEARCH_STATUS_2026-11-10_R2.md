# T81 operational follow-up after SCG-08 PR #765 — November 10, 2026 (R2)

**Observed at:** 2026-11-11T01:02:53Z / November 10, 2026, 20:02:53 EST. Sequential collection checkpoint, not atomic or live state. **READY FOR INDEPENDENT REVIEW — DO NOT MERGE.**

Fetched `origin/main` is **`36b3849aa1e6297e88de2ec1655777793ab0b59e`**. GitHub confirms [PR #765](https://github.com/t81dev/t81-foundation/pull/765) **merged** at that commit, on 2026-11-11T00:56:56Z. Its actual head is **`2dd3cc62e526942cdcbed6d100770c08c26ab2fe`**, base `dbf107fda83a3755fa21d6b95caa3eb8ac450dd3`. The PR body's claimed head `d80bc179…` is not the metadata head; use the actual GitHub/Git identities. The merged seven-file experimental/validator tree is byte-identical to the earlier R1 candidate `3969002e…` on those paths, established with `git diff --quiet`; this does not reproduce its arithmetic results.

This follow-up updates existing draft [#764](https://github.com/t81dev/t81-foundation/pull/764). It preserves the [R1 snapshot](T81_RESEARCH_STATUS_2026-11-10_R1.md), [82-record inventory](receipts/2026-11-10-r1/pr-inventory.json), and all R1 receipts byte-for-byte. The existing branch was merged with current main without conflicts, retaining original PR head `48dc1435e1753b426258eeb2bb14dfd036ad686b` as an ancestor. No experimental fix was reproduced in this branch; its review diff against current main remains documentation/status data only.

## Current lifecycle and acceptance

| Question | Observed disposition |
| --- | --- |
| #765 publication/integration | **VERIFIED — MERGED**, based on GitHub metadata and fetched history. The R1 statement “corrective PR publication pending” remains historically valid at its checkpoint and is superseded operationally here. |
| #765 Codex review | **Two unresolved findings**, review `COMMENTED` on actual head `2dd3cc62e5` at 2026-11-11T00:44:03Z. Review completion is not approval. |
| Evidence integrity | **BLOCKED**: incorrect source revision pin and scalar type-validation gap. |
| Gate 1 implementation correctness / independent acceptance | **Acceptance withheld**. Earlier accounting and same-run evidence-linkage concerns remain in R1; metadata and integration do not clear them. No arithmetic execution in this follow-up. |
| Scientific acceptance | **WITHHELD**. No ternary runtime, device, energy or performance advantage established. |
| Gate 1 authorization provenance | **MISSING EVIDENCE**, as scoped in R1: Addendum-06/original authoritative decision not located. No approval inferred from merge. |
| Phase 0 readiness | Statistical allocation reconciliation (136,500 sequences versus the `10^7`-step criterion) remains conditional; #761 publication/acceptance is not refreshed here. |
| Gate 2 | **WITHHELD**; no experiment, benchmark or campaign authorized or executed. |
| Any newer Jules correction | **MISSING EVIDENCE / UNVERIFIED**. No subsequent corrective PR or advanced SCG-08 branch tip found in the retained discovery. Do not label a prepared correction published, merged or verified without a later exact-source observation. |

The two current Codex findings are:

1. **P1 — pin evidence to corrected implementation revision.** `gate1_evidence_record.json` still names pre-correction `dbf107fda…` as `head_sha`, which cannot reproduce its changed counters. [Finding](https://github.com/t81dev/t81-foundation/pull/765#discussion_r4532430598), thread `PRRT_kwDOQajdIs61-Dl4`.
2. **P2 — restore scalar band integer validation.** Presence/counter checks omit scalar field types; malformed `operand_trits` and `ternary_limbs_op` can pass despite schema violations. [Finding](https://github.com/t81dev/t81-foundation/pull/765#discussion_r4532430605), thread `PRRT_kwDOQajdIs61-Dl8`.

The retained [GitHub receipt](receipts/2026-11-10-r2/github-observations.json) includes exact metadata, findings, reviews, branch tips, current open-list metadata and head workflow results. At the checkpoint, Format Check reported success, Foundation CI was queued, and CodeQL/cross-platform determinism were in progress. These are GitHub reports, not local test execution or a completed CI pass. #765's body reports passing arithmetic tests and 11 negative cases; those claims are source-reported. R1 actually observed 12 schema rejection fixtures, and its negative schema findings are preserved.

The complete current open-list metadata contains **70 open PRs / 40 drafts, including #764**. This is a lightweight lifecycle checkpoint, not a replacement for the original R1 82-record audit or an assertion that all reviews/checks were refreshed. The visualization now adds #765 to the existing 24 mapped R1 records. Those 24 retain original observation timestamps and receipts; the new top-level checkpoint explicitly describes the mixed dates. Historical architecture baselines, coverage metrics, topology, H01–H08 canonical fields and all unrelated research findings remain unchanged. Track R/F/M and the single-active-repository boundary continue unchanged.

`CURRENT_STATE.json` appends `2026-11-10-r2`; every earlier view, historical snapshot, top-level authorization field and `selected_view` is preserved. Entry points prepend this follow-up above R1. No new governance framework or acceptance authority is introduced.

**Next action:** obtain and inspect an exact published successor addressing #765's two findings; require source/run-bound evidence, scalar type checks and resolution of retained accounting concerns, followed by independent exact-head Gate 1 acceptance. Keep authoritative contract/authorization and Phase 0 statistical-readiness gaps visible. Do not initiate Gate 2. Review the CI/policy stack separately as Track F maintenance.

See [follow-up validation](receipts/2026-11-10-r2/VALIDATION.md) for exact commands, preservation evidence, platform limits and the separate Bash 3.2 baseline discrepancy. The same Codex assistant authors this continuity update and has the prior user/Jules/Codex reports as context; neither role separation nor passing document tests establishes independent scientific validation.
