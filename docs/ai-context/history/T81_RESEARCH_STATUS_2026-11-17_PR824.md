# T81 — PR #824 Integration Reconciliation (2026-11-17)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 — PR #824 Integration Reconciliation (2026-11-17)](#t81-—-pr-#824-integration-reconciliation-2026-11-17)
  - [Integrated documentation and independent draft](#integrated-documentation-and-independent-draft)
  - [Unchanged STOP and qualification contract](#unchanged-stop-and-qualification-contract)
  - [Structured continuity and remaining issues](#structured-continuity-and-remaining-issues)

<!-- T81-TOC:END -->


**NON-NORMATIVE / DOCUMENTATION ONLY / NO OPERATIONAL AUTHORIZATION**

Freshly fetched `origin/main`: `ab15820a2dd8ebcadde0efecf1fbb0d43b57d3f9`.
This is a bounded reconciliation of PRs #823 and #824, not a repository-wide
backlog, CI refresh, scientific finding or host-qualification receipt. The
[retained observation receipt](../receipts/2026-11-17-pr824-reconciliation.json)
records collection methods, metadata, provenance limits and exact source hashes.

## Integrated documentation and independent draft

| Subject | Verified identities and status | Evidence boundary |
|---|---|---|
| [PR #824](https://github.com/t81dev/t81-foundation/pull/824) | Reviewed source `f803e15ebcf4896fc70608cb11d69505134e5578`; base `40695b0ba5cea3e43563b6ed2443c987d2738089`; squash merge `ab15820a2dd8ebcadde0efecf1fbb0d43b57d3f9`; merged at `2026-11-17T15:09:05Z` | Exactly one planning file integrated: [BOLQ migration review plan](../../research/proposals/RFC-00D7-F01-BOLQ-MIGRATION/REVIEW_PLAN.md). Reviewed-source and merged-plan bytes match. |
| [PR #823](https://github.com/t81dev/t81-foundation/pull/823) | Open, draft, unmerged; head `52604ee1d4b7d90f69b19651a942f68dd5653183`; branch `docs/ai-context-morning-2026-11-17`; observed base `40695b0ba5cea3e43563b6ed2443c987d2738089` | [Morning report at its own pinned head](https://github.com/t81dev/t81-foundation/blob/52604ee1d4b7d90f69b19651a942f68dd5653183/docs/ai-context/MORNING_STATUS_2026-11-17.md) remains on that independent branch. No discussion/review entries were returned; external review is unverified. Its synthetic test-merge identity is not integration. |

PR #823's next-action suggestion to prepare a migration review plan is now
historical relative to integrated #824. Its report is neither imported nor
marked merged by this reconciliation; its other PR statuses, CI observations
and scientific summaries are not refreshed here. Its independent review and
merge decision remain separate.

**Review and authorization provenance:** the user reports independent Gemini
review of corrected #824 and human authorization for its documentation-only
merge. Git/GitHub independently confirm the integration and source identities.
The original Gemini review and human decision artifacts were not supplied or
authenticated by this collector. The returned GitHub discussion contains a
Codex bot review summary; it is not substituted for Gemini evidence. This
reconciliation is authored by Codex with prior F01 authoring context and is
submitted for independent Gemini review, not presented as independent validation
of its own work. Content hashes establish identity, not authority or truth.

## Unchanged STOP and qualification contract

- **F01-HOST-BOLQ-v1 is NOT ADOPTED. Residual risks are NOT ACCEPTED. M1–M6 remain STOP. `linux_relock_required=true`.** The original PR #812 **UNSUPPORTED** universal failure-path finding remains intact; no measured enforcement evidence has been added.
- M0 permits read-only documentation review only. A documentation merge does not adopt the assurance model, accept residual physical-overrun risk or authorize future work. The plan's original drafting-time authorization text is preserved as history.
- M2 (complete prospective protocol), M3 (versioned receipt/schema/routing design) and M4 (provisioning applicability) must be reviewed, reconciled and co-pinned together before separately authorized M5 implementation. A component change requires renewed reconciliation. H0 host selection/acquisition/admin setup, H1 provider/build artifact acceptance, H2 one bounded offline provisioning attempt, H3 independent provisioning receipt review, H4 active re-lock change and H5 Linux qualification remain distinct. Preliminary H5 compatibility work requires its own approved source/budget; H6 one A–F run remains outside M5.
- Operative F01 protocol SHA-256 remains **`d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3`**, verified against the pinned [PR #809 protocol bytes](https://github.com/t81dev/t81-foundation/blob/0afbc339288b44cc7536af6c227f957257bb8ff6/docs/research/proposals/RFC-00D7-F01/PROTOCOL.md). The merged review plan does not import the unmerged harness/protocol sources into main or alter those bytes.
- Existing qualification semantics, including **`limits_demonstrated:true`**, remain unchanged. Candidate BOLQ receipts cannot satisfy, alias, auto-upgrade or reinterpret legacy qualification. No active schema, gate, lock or qualification artifact is changed.
- Preserve **49,408 suite states**, including controls/rotations; Area F **254 planned states within its 256-state cap**; F288-v1 remains deferred/unexecuted. Preserve 60 seconds wall, 50 seconds cumulative CPU, 256 MiB aggregate memory, 128 MiB runner/helper, provisioning's shared 64 MiB controller/guard allowance, zero swap, one CPU and 25 MiB artifacts/evidence, with existing attribution and no reset/retry. These are existing scope-specific limits, not newly demonstrated enforcement.
- Provisioning remains separate from A–F. Its proposed **1 GiB volume / 921 MiB allocation** (128 inputs + 512 installed payload + 256 staging + 25 evidence, in MiB) supplies **no additional A–F storage**. No provisioning, provider selection, installation, host acquisition, qualification, probes, proof checking, A–F execution or implementation is authorized here.
- Canonical H01–H08, historical Hanoi/CWEB lineage, negative and inconclusive findings and unrelated research/release holds are preserved. This update adds no scientific results or current repository-wide PR count.

## Structured continuity and remaining issues

[`CURRENT_STATE.json`](../CURRENT_STATE.json) adds
`operational_views.2026-11-17-pr824-reconciled` under the unchanged v2 schema.
All earlier views and snapshots are retained unchanged. The new view carries
the four historical scientific observations verbatim with their original dates,
source pins and limitations; they are not new observations or reruns.
`open_pr_count=1` applies only to the two selected PRs (#823 open, #824 merged).
An empty CI array means **not refreshed**, not passing.

The existing contract tests assume the historical `selected_view` and historical
PR membership. To preserve documentation-only scope, that selector remains
`2026-11-02-reconciled`; it is explicitly historical. Updated `next_gate` and
navigation point to this dated overlay. The previous top-level date, selector
and next-gate text are retained in the new receipt. Generalizing those test
assumptions is a separate scoped maintenance decision, not a gate bypass here.

Actual host/provider/interpreter identity, containment rights, latency, reaping,
durability, receipt feasibility and human risk decisions remain unresolved.
Source acceptance and documentation integration do not establish qualification.
See the [validation record](../receipts/2026-11-17-pr824-validation.md) for exact
commands, preservation checks and limitations. The only next step supplied by
this draft is independent documentation review; no new operational GO is granted.
