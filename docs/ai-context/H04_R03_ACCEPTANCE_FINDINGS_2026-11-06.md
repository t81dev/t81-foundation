# H04-R03 — Dated Acceptance-Finding Reconciliation (2026-11-06)

> **Non-normative continuity addendum.** This is a repository/review-record observation, not an amendment to the accepted protocol, historical receipts, scientific measurements, or `CURRENT_STATE.json`. The three findings below were independently **reported by Codex on PR #679**; their reproduction and closure have **not** been independently established by this addendum. Refresh current source and review state before acting.

## Exact repository and review identities

- [PR #679](https://github.com/t81dev/t81-foundation/pull/679): **merged 2026-11-02**, merge commit `d8e1376a7a77902cc26269c6e8d4991bcde117a7`; Codex reviewed earlier candidate `e79f26e5a68b3b0a46cf3d50aca035493b8e549b`. Three review threads remained **unresolved and not outdated** at the 2026-11-06 inspection. Review findings apply to the reviewed candidate; confirm applicability to the merged tree and any subsequent fixes before asserting current runtime defects.
- [PR #680](https://github.com/t81dev/t81-foundation/pull/680): **merged 2026-11-02**, merge commit `927972f876d7be53b37e119a0be5b91d623edd95`. Its acceptance report records **ACCEPT / 6-of-6 PASS**, bounded to implementation correctness and protocol fidelity; this is a historical report, **not proof of closure of #679's Codex findings**.
- Historical state **as of 2026-11-02 before the merges**: PRs #679/#680 open; implementation and correctness validation reported; Phase C performance trials **not authorized**. The later same-day merges do not themselves authorize Phase C. Recheck authorization independently before any measurement.

## Three unresolved acceptance-related findings

| Finding | Codex severity | Reviewed behavior / counterexample | Scientific consequence | Required closure evidence |
|---|---|---|---|---|
| Missing statistics yield fabricated confidence intervals | P1 | `evaluate_decision_rules` substitutes `(1.0, 1.0, 1.0)` for missing lifecycle/query confidence intervals; Codex reported incomplete inputs can produce `REJECT` or `ACCEPT` instead of `INVALID`. | A confidence-shaped number is emitted without the required observation; 38/38 passing tests cannot establish complete statistical validity. | Remove placeholder intervals, fail closed as `INVALID` for missing/nonfinite/incomplete required CV and CI inputs, and add tests for missing query and lifecycle arms, including favorable-looking incomplete inputs. |
| Random-access reader accepts nonzero alignment padding | P2 | Codex example: encode `[0]` with `k=5, A=8`, set byte 10 to 1, query index 0; `random_access` returns 0 while `decode` rejects. Also test `A=64` header-gap padding at byte 8. | A malformed frame can be treated as valid by one required reader, violating preregistered padding validation. | Deterministic malformed-frame regression tests across applicable header-gap and block-end padding; both readers reject; preserve valid-frame results. |
| Phase 4 retention marked verified without enforcement | P1 | Dry-run Phase 4 is labeled verified, but original `symbols` remains live through queries/decodes and `self.u_symbols` remains resident. | An unverified lifecycle property is reported as satisfied; passing tests do not prove wire-frame-only retention. | Enforce and independently check the actual retention boundary, **or** report it explicitly as unimplemented/unverified without asserting Phase 4 acceptance; maintain measurement freeze. |

### Status classification

**MERGED — ACCEPTANCE-RELATED REVIEW FINDINGS UNRESOLVED (as inspected 2026-11-06).** The reported **38/38** tests and **6/6 PASS** are bounded historical suite/review outcomes, not a finding-resolution certificate. Do not silently upgrade them to full correctness acceptance. The precise test coverage of the malformed padding and missing-statistics paths has **not been independently established** by this addendum; inspect the test matrix and document any blind spots.

**Phase C remains frozen absent separate explicit authorization.** No measurement, benchmark, hardware advantage, or comparative performance conclusion is authorized by these merges or this note.

## Other bounded findings to preserve

- Historical **4096×4096 cache-resident packed 2-bit ternary AVX2 MatVec**: **12.91 GFLOP/s** vs **17.15 GFLOP/s** for native INT8 AVX2; packed ternary reached approximately **75.3%** of INT8 throughput, with **75% less weight storage** in the tested representation. Verify the exact memory regime in the primary benchmark packet before generalizing to DRAM-bound workloads. This is a negative CPU-speed result for the tested workload, not universal radix superiority.
- **CPA-004/005**: 25 matching observation boundaries in the bounded hosted/freestanding comparison; correctness/determinism scope only, **not a ternary-vs-binary speed win**.
- `CURRENT_STATE.json` remains an independently dated machine-readable inventory; **do not overwrite it** without a separate complete, evidence-backed regeneration.

## Next bounded action

Perform a focused H04-R03 correctness repair against the current merged tree. First map the 38 reported tests to the three Codex paths, reproduce the counterexamples, and distinguish untested from failing behavior. Correct the missing-CI decision path, malformed-frame random access, and retention-reporting/enforcement discrepancy; add explicit negative regressions and preserve historical evidence. Do **not** execute Phase C, rewrite acceptance history, or claim independent experimental replication. A new, separately reviewed repair receipt must identify exact source SHA, test commands/results, and disposition of each finding.
