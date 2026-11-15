# H04-R03 — Dated Acceptance-Finding Reconciliation (2026-11-06; Reconciled 2026-11-07)

## Operational overlay — 2026-11-10

The historical snapshot below describes PR #724 at `50087972`. It predates
the later Phase 4 implementation integrated by [PR #727](https://github.com/t81dev/t81-foundation/pull/727)
at `64941caff9372f2e76dd3a0fb79aa6711a48b171`, which is present in the inspected
`main` baseline `e5c30560aa77c31184a14b03ed152a6deef5c1a3`.

Current source includes the registered-object retention boundary in
[retention.py](../../experiments/h04_r03_matched_codec/retention.py) and the
wire-frame-only handoff in
[benchmark_harness.py](../../experiments/h04_r03_matched_codec/benchmark_harness.py).
The [later FC1 receipt](../research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md)
reports 82 passing tests and the disposition
`VERIFIED (CORRECTNESS & RETENTION BOUNDARY ENFORCED)`. These are the receipt
author's reported results, not a new execution or independent reproduction by
this overlay. The source was inspected; no research tests or measurements were run.

Do not interpret the historical `UNIMPLEMENTED` / `OPEN` labels below as the
current implementation inventory or restart that repair from this snapshot.
The later boundary tracks registered Python-owned objects and checks a native
ordering fence; it does not establish absence of untracked aliases, allocator
page reclamation, physical erasure, or cache flushing. Those limits remain open.

**Phase C measurement remains frozen and requires separate explicit human
authorization.** This overlay changes neither the protocol nor historical
receipts, canonical H01–H08 identities, or `CURRENT_STATE.json`.

## Historical snapshot — PR #724 (`50087972`)

> **Non-normative continuity addendum.** This is a repository/review-record observation, not an amendment to the accepted protocol, historical receipts, scientific measurements, or `CURRENT_STATE.json`. The three findings below were independently **reported by Codex on PR #679** and addressed via the H04-R03-FC1 corrective implementation merged at `50087972e078a09d7aff79cf6d8ae15db45c17ba` (PR #724). Overall milestone disposition is **FC1 PARTIAL**; Phase 4 retention enforcement remains unimplemented. Refresh current source and review state before acting.

## Exact repository and review identities

- [PR #679](https://github.com/t81dev/t81-foundation/pull/679): **merged 2026-11-02**, merge commit `d8e1376a7a77902cc26269c6e8d4991bcde117a7`; Codex reviewed earlier candidate `e79f26e5a68b3b0a46cf3d50aca035493b8e549b`. Three review threads remained **unresolved and not outdated** at the 2026-11-06 inspection.
- [PR #680](https://github.com/t81dev/t81-foundation/pull/680): **merged 2026-11-02**, merge commit `927972f876d7be53b37e119a0be5b91d623edd95`. Its acceptance report records **ACCEPT / 6-of-6 PASS**, bounded to implementation correctness and protocol fidelity; this is a historical report, **not proof of closure of #679's Codex findings**.
- [PR #724](https://github.com/t81dev/t81-foundation/pull/724) (Issue #723 / PR #722): **merged 2026-11-07**, authoritative merge commit `50087972e078a09d7aff79cf6d8ae15db45c17ba` (base `1103356ac867a88b3c3873a7e2e696d48378f424`). Implemented H04-R03-FC1 corrective fixes and published dated evidence receipt `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`.
- Historical state **as of 2026-11-02 before the merges**: PRs #679/#680 open; implementation and correctness validation reported; Phase C performance trials **not authorized**. Subsequent merges do not authorize Phase C. Recheck authorization independently before any measurement.

## Three acceptance-related findings and FC1 corrective resolution

| Finding | Codex severity | Reviewed behavior / counterexample | Scientific consequence | Corrective resolution in PR #724 (`50087972`) | Final disposition |
|---|---|---|---|---|---|
| Missing statistics yield fabricated confidence intervals | P1 | `evaluate_decision_rules` substituted `(1.0, 1.0, 1.0)` for missing lifecycle/query confidence intervals; Codex reported incomplete inputs can produce `REJECT` or `ACCEPT` instead of `INVALID`. | A confidence-shaped number was emitted without the required observation; 38/38 passing tests could not establish complete statistical validity. | Removed all 5 dead fallback tuples. Implemented `_is_valid_ci` validating 3-element positive, finite, non-boolean numeric tuples with `ci[1] <= ci[2]`; fail closed as `INVALID`. Preserved Step 2 CV > 5.0% early noise stop (`INCONCLUSIVE`). Tests added in `test_harness.py`. | **FIXED (DEFENSIVE HARDENING)** |
| Random-access reader accepts nonzero alignment padding | P2 | Codex example: encode `[0]` with `k=5, A=8`, set byte 10 to 1, query index 0; `random_access` returned 0 while `decode` rejected. Also test `A=64` header-gap padding at byte 8. | A malformed frame could be treated as valid by one required reader, violating preregistered padding validation. | Verified Counterexamples 1, 2, and multi-microblock corruption. Implemented `ValidatedFrame` (conventionally immutable token-guarded admission boundary for in-process callers with $O(\text{total\_bytes})$ copy and $O(N)$ canonical payload validation accounted in Phase 5) and `admitted_random_access()` preserving $\le 6$ payload byte access limit. Raw `random_access()` documented as unvalidated primitive. Tests added in `test_b729_u5.py` and `test_matched_codec.py`. | **FIXED** |
| Phase 4 retention marked verified without enforcement | P1 | Dry-run Phase 4 was labeled verified, but original `symbols` remained live through queries/decodes and `self.u_symbols` remained resident. | An unverified lifecycle property was reported as satisfied; passing tests did not prove wire-frame-only retention. | Removed `self.u_symbols` caching. Honest reporting correction: dry-run returns `PARTIAL — PHASE 4 UNVERIFIED / UNIMPLEMENTED`, structured `phase_status` reports Phase 4 as `UNVERIFIED / UNIMPLEMENTED`, and `phases_verified` excludes Phase 4 (7 verified). Functional retention requirement remains open. | **REPORTING CORRECTED — FUNCTIONAL REQUIREMENT OPEN** |

### Status classification

**CORRECTIVE MERGE INTEGRATED — OVERALL DISPOSITION: FC1 PARTIAL (`50087972`).**
Findings 1 and 2 are FIXED with deterministic regression tests. Finding 3 was honestly REPORTING CORRECTED, withdrawing unsupported verification claims while leaving the functional wire-frame retention enforcement requirement explicitly OPEN. Across the active suites, 74 tests pass (including 53 standard unit tests and 21 infrastructure/enforcement tests) with zero regressions.

**Phase C remains strictly frozen absent separate explicit authorization.** No measurement, benchmark, hardware advantage, or comparative performance conclusion is authorized by these merges, this addendum, or the FC1 evidence receipt.

## Other bounded findings to preserve

- Historical **4096×4096 packed 2-bit ternary AVX2 MatVec**: **12.91 GFLOP/s** vs **17.15 GFLOP/s** for native INT8 AVX2; packed ternary reached approximately **75.3%** of INT8 throughput, with **75% less weight storage** in the tested representation. The precise cache/DRAM working-set characterization must be checked against the primary benchmark packet; do not assert a memory regime from this summary alone. This is a negative CPU-speed result for the tested workload, not universal radix superiority.
- **CPA-004/005**: 25 matching observation boundaries in the bounded hosted/freestanding comparison; correctness/determinism scope only, **not a ternary-vs-binary speed win**.
- `CURRENT_STATE.json` remains an independently dated machine-readable inventory; **do not overwrite it** without a separate complete, evidence-backed regeneration.

## FC1 milestone status summary

**H04-R03-FC1 status: FC1 PARTIAL.** See primary receipt in `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`.

1. **Statistical evidence ordering:** Resolved. All five fallback intervals removed; defensive fail-closed validation enforced; Step 2 early noise stop preserved before CI computation.
2. **Alignment and bounded-access contract:** Resolved. Explicit immutable admission boundary (`ValidatedFrame` and `admitted_random_access()`) separates $O(B)$ admission validation from $O(1)$ random access, preserving the $\le 6$ payload byte access limit while guaranteeing zero unvalidated/corrupted padding bits reach admitted queries.
3. **Retention claim versus enforcement:** Reporting corrected. Unsupported claims withdrawn; Phase 4 labeled `UNVERIFIED / UNIMPLEMENTED`; functional wire-frame-only retention enforcement remains **OPEN**.

**Closure rule compliance:** Pre-fix reproducers, post-fix regressions, exact source SHA (`50087972`), executed commands, observed outcomes, protocol compatibility, and trust boundaries are fully documented in `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`. Because Phase 4 was addressed via reporting correction rather than functional retention enforcement, the overall milestone disposition is bounded strictly to **FC1 PARTIAL**.

## Next bounded action

Maintain Phase C measurement freeze. Any future advancement of H04-R03 requires:
1. Formal maintainer authorization before any Phase C benchmark execution or timing runs.
2. Addressing the open Phase 4 functional retention requirement if end-to-end wire-frame memory isolation is to be verified.
3. Conducting independent review of the FC1 evidence receipt and test artifacts.
