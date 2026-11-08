# H04-R03 — Dated Acceptance-Finding Reconciliation (2026-11-06; Reconciled 2026-11-07)

> **Non-normative continuity addendum.** This is a repository/review-record observation, not an amendment to the accepted protocol, historical receipts, scientific measurements, or `CURRENT_STATE.json`. The three findings below were independently **reported by Codex on PR #679** and addressed via the H04-R03-FC1 corrective implementations merged at `50087972e078a09d7aff79cf6d8ae15db45c17ba` (PR #724), reconciled documentation at `0a547536649edf311ffbc1b85ebafccfb73604bb` (PR #726), and finalized retention enforcement merged at `64941caff9372f2e76dd3a0fb79aa6711a48b171` (PR #727). Overall milestone disposition is **VERIFIED (CORRECTNESS & RETENTION BOUNDARY ENFORCED)**.

## Exact repository and review identities

- [PR #679](https://github.com/t81dev/t81-foundation/pull/679): **merged 2026-11-02**, merge commit `d8e1376a7a77902cc26269c6e8d4991bcde117a7`; Codex reviewed earlier candidate `e79f26e5a68b3b0a46cf3d50aca035493b8e549b`. Three review threads remained **unresolved and not outdated** at the 2026-11-06 inspection.
- [PR #680](https://github.com/t81dev/t81-foundation/pull/680): **merged 2026-11-02**, merge commit `927972f876d7be53b37e119a0be5b91d623edd95`. Its acceptance report records **ACCEPT / 6-of-6 PASS**, bounded to implementation correctness and protocol fidelity; this is a historical report, **not proof of closure of #679's Codex findings**.
- [PR #724](https://github.com/t81dev/t81-foundation/pull/724) (Issue #723): **merged 2026-11-07**, authoritative merge commit `50087972e078a09d7aff79cf6d8ae15db45c17ba` (base `1103356ac867a88b3c3873a7e2e696d48378f424`). Implemented H04-R03-FC1 corrective fixes for Findings 1 and 2, and honest reporting correction for Finding 3.
- [PR #726](https://github.com/t81dev/t81-foundation/pull/726): **merged 2026-11-07**, merge commit `0a547536649edf311ffbc1b85ebafccfb73604bb` (reconciling PR #722 documentation atop PR #724).
- [PR #727](https://github.com/t81dev/t81-foundation/pull/727): **merged 2026-11-07**, authoritative merge commit `64941caff9372f2e76dd3a0fb79aa6711a48b171` (rebasing PR #725 atop PR #724 & PR #726). Implemented Phase 4 wire-frame retention enforcement and published dated evidence receipt `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`.
- **Open PRs #722 and #725**: Both are fully superseded by PR #726 and PR #727 respectively, with zero unique unmerged functional changes.
- **Phase C performance measurement remains strictly FROZEN / unauthorized.**

## Three acceptance-related findings and FC1 corrective resolution

| Finding | Codex severity | Reviewed behavior / counterexample | Scientific consequence | Corrective resolution in PR #724 (`50087972`) & PR #727 (`64941caf`) | Final disposition |
|---|---|---|---|---|---|
| Missing statistics yield fabricated confidence intervals | P1 | `evaluate_decision_rules` substituted `(1.0, 1.0, 1.0)` for missing lifecycle/query confidence intervals; Codex reported incomplete inputs can produce `REJECT` or `ACCEPT` instead of `INVALID`. | A confidence-shaped number was emitted without the required observation; 38/38 passing tests could not establish complete statistical validity. | Removed all 5 dead fallback tuples in PR #724. Implemented `_is_valid_ci` validating 3-element positive, finite, non-boolean numeric tuples with `ci[1] <= ci[2]`; fail closed as `INVALID`. Preserved Step 2 CV > 5.0% early noise stop (`INCONCLUSIVE`). Tests added in `test_harness.py`. | **FIXED (DEFENSIVE HARDENING)** |
| Random-access reader accepts nonzero alignment padding | P2 | Codex example: encode `[0]` with `k=5, A=8`, set byte 10 to 1, query index 0; `random_access` returned 0 while `decode` rejected. Also test `A=64` header-gap padding at byte 8. | A malformed frame could be treated as valid by one required reader, violating preregistered padding validation. | Verified Counterexamples 1, 2, and multi-microblock corruption in PR #724. Implemented `ValidatedFrame` (conventionally immutable token-guarded admission boundary for in-process callers with $O(\text{total\_bytes})$ copy and $O(N)$ canonical payload validation accounted in Phase 5) and `admitted_random_access()` preserving $\le 6$ payload byte access limit. Raw `random_access()` documented as unvalidated primitive. Tests added in `test_b729_u5.py` and `test_matched_codec.py`. | **FIXED** |
| Phase 4 retention marked verified without enforcement | P1 | Dry-run Phase 4 was labeled verified, but original `symbols` remained live through queries/decodes and `self.u_symbols` remained resident. | An unverified lifecycle property was reported as satisfied; passing tests did not prove wire-frame-only retention. | Removed `self.u_symbols` in PR #724. Functionally enforced in PR #727: registered Python-owned input buffers monitored via weak references (`OwnedSymbols`, `OwnedBuffer`), mandatory native C11 ordering fence (`retention_fence.c`), independent precomputed witness digests for Phase 6 queries and Phase 7 decode without retaining a second raw symbol array, and fail-closed alias assertions (`live_owned_buffers == 0`). Negative tests in `test_harness.py` (`test_11`, `test_12`, `test_13`). | **VERIFIED (WIRE-FRAME-ONLY RETENTION ENFORCED)** |

### Status classification

**CORRECTIVE MERGE INTEGRATED — OVERALL DISPOSITION: VERIFIED (CORRECTNESS & RETENTION BOUNDARY ENFORCED) (`64941caf`).**
All three Codex review findings are fully resolved with deterministic regression tests. Across the active suites, 82 tests pass (82/82 OK: 56 unit/harness tests and 26 infrastructure/enforcement tests) with zero regressions.

**Verification boundary distinctions:**
1. **Implemented correctness vs Reproduction:** Implemented correctness and author-reported 82/82 passing tests were executed in-environment by Jules on Linux x86_64 container. Clean code review was achieved. Independent third-party reproduction by an external entity has not taken place.
2. **Software retention scope vs Physical memory reclamation:** Weak references track only registered Python-owned objects, and the native ordering fence enforces CPU memory ordering. OS-level physical memory reclamation, allocator cache retention (e.g. ptmalloc arena retention), and undiscovered aliases are not guaranteed or established.
3. **Phase C freeze intact:** Phase C performance measurement remains strictly FROZEN absent separate explicit maintainer authorization. No performance claim, benchmark, hardware advantage, or throughput conclusion is authorized.

## Other bounded findings to preserve

- Historical **4096×4096 packed 2-bit ternary AVX2 MatVec**: **12.91 GFLOP/s** vs **17.15 GFLOP/s** for native INT8 AVX2; packed ternary reached approximately **75.3%** of INT8 throughput, with **75% less weight storage** in the tested representation. The precise cache/DRAM working-set characterization must be checked against the primary benchmark packet; do not assert a memory regime from this summary alone. This is a negative CPU-speed result for the tested workload, not universal radix superiority.
- **CPA-004/005**: 25 matching observation boundaries in the bounded hosted/freestanding comparison; correctness/determinism scope only, **not a ternary-vs-binary speed win**.
- `CURRENT_STATE.json` remains an independently dated machine-readable inventory; **do not overwrite it** without a separate complete, evidence-backed regeneration.

## FC1 milestone status summary

**H04-R03-FC1 status: VERIFIED (CORRECTNESS & RETENTION BOUNDARY ENFORCED).** See primary receipt in `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`.

1. **Statistical evidence ordering:** Resolved in PR #724. All five fallback intervals removed; defensive fail-closed validation enforced; Step 2 early noise stop preserved before CI computation.
2. **Alignment and bounded-access contract:** Resolved in PR #724. Explicit immutable admission boundary (`ValidatedFrame` and `admitted_random_access()`) separates $O(B)$ admission validation from $O(1)$ random access, preserving the $\le 6$ payload byte access limit while guaranteeing zero unvalidated/corrupted padding bits reach admitted queries.
3. **Retention claim versus enforcement:** Resolved in PR #727. Weak references monitor registered Python-owned input buffers; native C11 ordering fence enforces execution barriers; Phase 6 queries and Phase 7 decode verify strictly against precomputed witness digests without retaining any raw symbol array; negative tests verify fail-closed alias detection. Bounded to registered-object scope.

**Closure rule compliance:** Pre-fix reproducers, post-fix regressions, exact source SHAs (`50087972`, `0a547536`, `64941caf`), executed commands, observed outcomes, protocol compatibility, and trust boundaries are fully documented. Issue #723 is closed; open PRs #722 and #725 are superseded.

## Next bounded action

Maintain Phase C measurement freeze. Any future advancement of H04-R03 requires:
1. Formal maintainer authorization before any Phase C benchmark execution or timing runs.
2. Independent third-party reproduction of the FC1 evidence receipt and test artifacts.
3. Evaluation against the existing H01–H08 research roadmap to prioritize next research decisions without unapproved benchmarking.
