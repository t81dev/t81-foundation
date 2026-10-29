# EXP-T81-COMP3-01/r1 — final approval record candidate

**READY FOR MAINTAINER APPROVAL — NOT APPROVED; r1 NOT ACTIVE.**

Prepared October 29, 2026. M1–M4 were provisionally accepted for finalization, not formally activated. This immutable review record is a candidate for a separate signed maintainer approval; unchecked boxes and unset identities are intentional. Do not fill this packet in place and reuse its old hashes. The approval mechanism must bind the final immutable packet digests in a new signed approval payload.

## Exact authority and candidate identity

- Original repository/path: `t81dev/t81-foundation`, `docs/research/TERNARY_ADVANTAGE_CANDIDATES.md`.
- Original PR #635 commit: `b37005ceda6a3f4349ec9027b2a9be84ea176aa9`.
- Original Git blob: **`0fbe0b4da1614867bf51b1c3eaf200f82d789a65`**, recomputed and verified over the preserved 43,709 bytes.
- Original SHA-256: **`990f54a57d7ad34cfcd93c45147e81440ac8f2430eae3271d5f676d6947081f1`**.
- Existing implementation/evidence reference: PR #637 at `988b18ca9ebdb4a6fb76fe0be30115470e5637f0`; no repair or new evidence of Linux execution is claimed.
- Proposed amendment: **EXP-T81-COMP3-01-AMENDMENT-01, review draft 0.2 (final approval candidate)**.
- Proposed governing revision after all activation gates: **EXP-T81-COMP3-01/r1 = original #635 bytes + approved AMENDMENT-01**.
- Exact amendment SHA-256: **`f068bb0dab6425029436483f82487e0297cf86c92135935d62d00efbaf04f193`**.

Draft 0.1 and all previous artifacts are preserved under `history/draft-0.1/`. Original #635 is not rewritten. The original 27 findings remain traceable in `RECONCILIATION_SOURCE.md` and the matrix. All source line references in the four documents remain pinned; finalization fixes the previously empty vector-source reference and adds only the two requested clarifications plus their acceptance tests.

## Explicit maintainer approval text — proposed, unsigned

A maintainer may sign a separate canonical approval payload expressing:

> I approve EXP-T81-COMP3-01-AMENDMENT-01 draft 0.2 for prospective r1 use, bound to the exact original identity and final document hashes recorded here. I approve M1, M2, M3 and M4 below, including mandatory exact bootstrap decision arithmetic and the T1–T8 experiment-local trust model. I understand this does not retrospectively validate #637 against #635, resolve GitHub review threads, activate r1, or authorize performance measurement by itself. Measurement eligibility requires the later repaired implementation and Linux validation, an authenticated trust chain, and separately recorded measurement authorization.

| Approval | Exact prospective decision |
| --- | --- |
| M1 | Adopt the versioned authority/activation record, preserve #635 bytes and provenance, and accept the explicit PRNG, sampling, Pattern-B, PMU-name and registry errata/clarifications. Sampling/reset conventions and the interval contract require express approval, not just spelling-error treatment. |
| M2 | Use predeclared matched blocks and paired df=9 inference; instruction reduction against Baseline 1 is the sole benefit endpoint. Replace 1.96 intervals with Student-t reporting; add the nominal one-sided 99% paired bootstrap minimum-effect qualification. Retain 15% and p<.01; restrict other benefit comparisons to descriptive reports. |
| M3 | Reserve NOT SUPPORTED for demonstrated regressions; keep positive sub-15% and otherwise nonsignificant benefits INCONCLUSIVE, separately reporting minimum-effect evidence. Approve the 18-contrast, family-alpha=.01 Bonferroni regression rule, fastest-baseline interpretation, zero-variance convention and explicit per-pattern/overall precedence. This changes #637 as well as #635. |
| M4 | Approve exact CPU/RSS budget accounting, trusted completion evidence and its trust boundary, strict raw integer domains, failure-code separation, environmental controls and the Linux validation/activation gates. No new performance-success threshold is introduced. |

## Final statistical decision table — unchanged from draft 0.1

Ten matched blocks per pattern; sole benefit endpoint is instruction reduction versus Baseline 1. Student-t primary inference has df=9 and two-sided p<.01. Minimum point reduction and nominal one-sided bootstrap lower bound must each be ≥3/20. Branch/latency support guards compare means descriptively; they do not prove statistical non-inferiority. Regression family has exactly 18 contrasts across three patterns: per pattern I−I1, B−B1 and T−Tb for b=1..4, with family alpha .01, simultaneous two-sided intervals using `t(9,1−.01/(2*18))` and strictly positive lower endpoint as the regression criterion.

V=all applicable integrity conditions valid; G=at least one qualifying regression for that pattern; Q=all retained significance/minimum-effect/descriptive-guard conditions hold.

| Precedence | Outcome | Necessary and sufficient condition in this ordered table | Required reason/evidence |
| --- | --- | --- | --- |
| 1 | BLOCKED | not V | Enumerate integrity failures and their scope; do not run the classifier on invalid series. |
| 2 | NOT SUPPORTED | V and G | Report each demonstrated instruction, branch-miss or latency regression with its contrast, simultaneous interval and adjusted decision. A nonsignificant benefit metric cannot mask it. |
| 3 | SUPPORTED | V and not G and Q | Report d, bootstrap L/U, primary p, means and fastest baseline. This is a bounded software result only. |
| 4 | INCONCLUSIVE | V and not G and not Q | Report which qualification failed; retain positive direction, uncertainty, and evidence below the target separately. |

Overall precedence is **BLOCKED > NOT SUPPORTED > INCONCLUSIVE > SUPPORTED**. All three patterns must be SUPPORTED for overall support. A failed collector, signal, final budget overrun, tampering or unauthenticated/ambiguous status blocks globally even if every CSV row exists. Normal-exit authenticated pattern-local data failures cannot make overall valid; unaffected complete-pattern states may be retained only as explicitly scoped records. Positive insufficient gains and nonsignificant benefits remain INCONCLUSIVE unless a demonstrated regression vetoes. Report U<.15 as evidence against the minimum target separately; do not relabel it as an observed regression or hide it behind the composite outcome.

## Bootstrap verification summary

- Frozen PCG XSL-RR 128/64, E1 seed/stream/multiplier; old-state extraction; fresh identical bootstrap state per pattern, separate from corpus state.
- 10,000 resamples ×10 with-replacement accepted draws, outer resample then inner draw, bounded(10) rejection threshold 6, block identities ordered 0..9.
- Exact rational ratio-of-sums ordering; L rank 100/index 99, U rank 9900/index 9899; no interpolation or epsilon.
- All **100,000 indices** agree between direct recurrence and independent affine-jump state calculation. Fixed prefix: 100,000 raw draws, zero rejects; rejection logic separately checked with scripted helper values.
- Full index-byte SHA-256: **`8c5f235bc6d1df1c1fcdca99903abaaaa8201591d5521fe84ba7e72868770b64`**.
- Seven added fixtures pass using both pair sums and histogram sums. Nondegenerate lower bounds immediately below/at/above .15 are **149999/1000000, 3/20, 150001/1000000**. Corresponding states are INCONCLUSIVE, SUPPORTED, SUPPORTED with the other gates passing.
- Exact zero variance, nonzero constant differences, nonconstant differences with constant bootstrap ratio, and minimum positive integer variance at near-zero relative scale are distinguished. Near-zero fixture L=`37499999999999999/250000000000000000` is below .15 despite equal binary64 formatting. Mandatory exact comparison is the narrow correction to the old optional “where feasible” wording.
- Eleven historical statistical cases, all eight predicate combinations and all 64 aggregate-state triples retain their previous results. Failures: **0**.
- Nominal one-sided 99% bounds are not empirically demonstrated coverage. [L,U] is a central 98% percentile interval, not a two-sided 99% interval. Coverage simulations performed: **0**. No new test or acceptance threshold was introduced.

## Trusted-runner boundary — concrete profile, not a configured system

A named maintainer provisions/authenticates an approval public key out of band. A named operator provisions the intended Linux host and short-lived supervisor, with a distinct protected runner signing identity delegated by the maintainer. The measurement collector runs under a separate restricted UID, without access to the runner key or protected approval/output/journal files. A same-UID collector that can read the signing key is ineligible. There is no daemon or new T81 governance/signing service.

Verifier trust starts from the pre-pinned raw maintainer key/fingerprint, never a manifest-supplied key. Role-domain-separated canonical Ed25519 approval, delegation, eligibility and run payloads bind the exact packet/implementation/executable/corpus/environment/capture identities. The signed maintainer approval explicitly includes M1–M4=true and the five final document digests; independent eligibility later binds repaired-source and executable hashes plus actual Linux validation. The reviewer must not infer either record from a content hash.

Supervisor captures the exact child's raw wait4 exit/signal status and native integer user/system CPU and peak RSS; final limits remain **600,000,000 CPU microseconds** and **500,000,000 resident bytes**. It signs terminal evidence only after complete captures, child termination and final resource validation. It owns an atomically reserved CSPRNG run id/campaign/attempt journal and a protected admission journal; duplicate admission is rejected, read-only audit verification remains idempotent. Full artifact digests, block/order/PMU/control evidence and terminal failure scopes are bound to the run. Test keys and synthetic artifact roles are segregated; measurement verification has no test-key override. A signature proves signer/capture custody under the trusted-host assumptions, not truthful computation against a malicious host or independently validated performance.

No signing key, public trust anchor, named operator account, host profile, formal approval, eligibility record or real signature has been provisioned or claimed. T1–T8 specify the details and negative-path acceptance requirements; their Linux/cryptographic implementation tests have not yet run.

## Outstanding human-controlled configuration

| Required action | Current state |
| --- | --- |
| Name the approving maintainer and approval-key custodian; authenticate/pin the actual public key and fingerprint by a known channel. | **UNCONFIGURED** |
| Name the operator/host, collector UID and protected directories; provision the separate protected runner key and maintainer-signed production delegation. | **UNCONFIGURED** |
| Select the exact Linux GCC/Clang host/configurations, campaign identity and bounded whole-run attempt/retry policy; configure the protected journals and verification trust store. | **UNCONFIGURED** |
| Issue formal signed M1–M4 approval bound to this immutable packet; after repairs/validation issue eligibility and separately authorize any measurement execution. | **NOT APPROVED / NOT AUTHORIZED** |

These are concrete provisioning/authorization decisions. The statistical design, target, p threshold, bootstrap sequence and regression family are not open choices in this finalization.

## Precise activation gates and remaining Linux requirements

1. Formal signed maintainer approval must bind the exact original and all five final document digests, with M1–M4=true, named identity and UTC timestamp, and a valid delegated runner/campaign scope. An unsigned completed checklist does not satisfy this gate.
2. Separately authorize and perform only the planned experiment-local repairs. Verify strict integer parsing, reason separation, resource checks, same-object launch/capture, trusted-runner negative paths and exact bootstrap/numerical fixtures; do not change the approved statistical rules to pass tests.
3. Obtain actual intended-host GNU GCC and Clang strict builds, boundary/correctness and independent PCG/corpus checks, ASan/UBSan where supported, full/per-block assembly inventories including ret, confirmed PMU smoke, affinity/governor/boost/isolation evidence, budget/status tests and authenticated eligibility. Keep compiler configurations separate and report all failures/skips. No existing macOS pass substitutes for these Linux lanes.
4. The first eligible r1 run must start strictly after both approval and eligibility records and only under separate applicable measurement authorization. Its authenticated manifest binds those records, exact artifacts and unique run identity. Earlier/synthetic data cannot be relabelled as r1 measurements. All failures remain archived; no selective retries or row splicing.

## Maintainer checklist

- [ ] Verify original commit/blob/SHA-256 and the amendment/companion hashes below.
- [ ] Review the two clarifications: B0–B5 exact bootstrap verification and T1–T8 trust model.
- [ ] Confirm M1–M4 approval text, unchanged decision table, 15%/p<.01 settings and fixed 18-contrast family.
- [ ] Accept mandatory exact decision arithmetic; distinguish nominal coverage from measured calibration.
- [ ] Identify/provision the human-controlled trust/custody/host/campaign settings; do not accept invented credentials.
- [ ] Record formal approval in a separately signed payload; leave r1 inactive until repairs, Linux eligibility and separate measurement authorization exist.
- [ ] Confirm neither PR nor the benchmark implementation was modified in this finalization; no performance measurements ran.

## Exact final review-document digests

| Document | SHA-256 |
| --- | --- |
| `PROPOSED_PROTOCOL_AMENDMENT.md` | `f068bb0dab6425029436483f82487e0297cf86c92135935d62d00efbaf04f193` |
| `PROTOCOL_DECISION_MATRIX.md` | `26e683aa40c19a72359135696f608004dd45a8d6cf29f4f4a591bbe0ab9f19b3` |
| `STATISTICAL_COUNTEREXAMPLES.md` | `64c11fdf13f6daf88fbd660fa72b5a7ba06ab2347d8f553ed9a1cc07f70e496c` |
| `IMPLEMENTATION_REPAIR_PLAN.md` | `2674c55f0dc08d55a4561839478869f82389c4fd0524763d4238878448913757` |

The SHA-256 of this approval-record candidate itself is listed externally in `FINAL_DOCUMENT_HASHES.txt` and `FINAL_PACKET_MANIFEST.json`, avoiding a self-hash cycle. Formal approval binds all five documents.

**No experimental performance measurements have been collected in this work.** All numerical results here are synthetic arithmetic/statistical diagnostics. The supplied earlier evidence contains no full PMU measurement dataset; this record does not invent claims about unobserved external activity. Neither PR #635 nor #637 was modified during finalization. No benchmark implementation, runtime, ISA, CanonFS or governance activation changed; no GitHub thread was marked resolved.

**Final preparation disposition: READY FOR MAINTAINER APPROVAL. Formal approval: PENDING. r1 activation: NONE. Stop at this boundary.**
