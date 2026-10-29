# Statistical counterexamples — exact synthetic series

**Status: PROPOSED / NOT APPROVED.** Prepared October 29, 2026. This packet authorizes no implementation or measurement. Every proposed rule is prospective and requires the approvals below. No existing branch, PR, frozen document, or governance activation was modified.

These are algebraic diagnostics only, never empirical measurements. The four-outcome results below are conditional on otherwise valid setup and observations when testing statistical semantics. They do not make these synthetic CSVs eligible measurement runs. At full-protocol level #635 already contains inconsistent PRNG vectors; this document isolates its statistical clauses rather than pretending a complete conformant run existed.

Source decisions: [Original #635:464–509](https://github.com/t81dev/t81-foundation/blob/b37005ceda6a3f4349ec9027b2a9be84ea176aa9/docs/research/TERNARY_ADVANTAGE_CANDIDATES.md#L464-L509); [statistics.hpp:89–172](https://github.com/t81dev/t81-foundation/blob/988b18ca9ebdb4a6fb76fe0be30115470e5637f0/experiments/cmp3_decision_advantage/statistics.hpp#L89-L172); [analyze.cpp:62–103](https://github.com/t81dev/t81-foundation/blob/988b18ca9ebdb4a6fb76fe0be30115470e5637f0/experiments/cmp3_decision_advantage/analyze.cpp#L62-L103). Reconciliation findings 4–10 are the source of C1–C4; the additional cases exercise the original P1 example, degeneracy, latency ambiguity and prospective multiplicity handling. Actual #637 outputs were rechecked with the unchanged existing reconciliation analyzer; the proposal was evaluated independently in `verify_proposal.py`, not by editing the experiment engine. Results and full vectors are also in `VERIFICATION_RESULTS.json` and `C*.csv`.

For C1–C11, each displayed mode has exactly ten observations, and the identical synthetic mode set is repeated for patterns A/B/C. Pair index is the vector index, fixed in advance. I=instructions, B=branch misses, T=elapsed_ns; baseline 1–4 identities remain fixed. Synthetic CSVs use checksum=42 only as a parity placeholder; it is not a corpus-derived performance checksum. Do not infer independent physical samples from these examples.

A single exact expected #635 outcome is impossible for some fixtures: its original procedure omits degenerate-test definitions and conflicts on “any primary metric” and fastest/all-baseline latency. Those cells explicitly retain ambiguity rather than invent a repaired original. #637 and proposed outputs below are precise for the stated models.

| Case | Original #635 statistical consequence | #637 actual classifier | Proposed r1 mathematical result |
| --- | --- | --- | --- |
| C1 | INCONCLUSIVE | NOT SUPPORTED | INCONCLUSIVE |
| C2 | INCONCLUSIVE | NOT SUPPORTED | INCONCLUSIVE |
| C3 | INCONCLUSIVE under the literal any-primary-metric reading; otherwise underdefined | NOT SUPPORTED | NOT SUPPORTED |
| C4 | INCONCLUSIVE | SUPPORTED | SUPPORTED |
| C5 | Undefined for the zero-variance tests; NOT SUPPORTED if an added deterministic-limit convention is used | NOT SUPPORTED | NOT SUPPORTED |
| C6 | Undefined for equal constant instructions; INCONCLUSIVE if the additional p=1 convention is assumed | NOT SUPPORTED | INCONCLUSIVE |
| C7 | Conflicting: NOT SUPPORTED under the fastest-baseline formula; no matching disposition under the literal “higher than all” prose | NOT SUPPORTED | NOT SUPPORTED |
| C8 | NOT SUPPORTED under the stated per-metric Welch/significance reading | NOT SUPPORTED | INCONCLUSIVE |
| C9 | Undefined for constant Welch series; INCONCLUSIVE if equality of constant misses is assigned p=1 under literal precedence | SUPPORTED | SUPPORTED |
| C10 | INCONCLUSIVE under the nonsignificant-miss rule; primary constant-data convention is otherwise unspecified | INCONCLUSIVE | INCONCLUSIVE |
| C11 | INCONCLUSIVE under the original nonsignificant-primary-instruction precedence | NOT SUPPORTED | NOT SUPPORTED |

## C1_small_positive

**#635:** INCONCLUSIVE. **#637:** NOT SUPPORTED. **Proposed:** INCONCLUSIVE.

A precisely positive 5% gain is below target. Original explicit small-positive handling is preserved; #637 instead rejects the minimum-effect hypothesis through U<.15.

Exact series:

```text
candidate
  I=[951, 949, 951, 949, 951, 949, 951, 949, 951, 949]
  B=[81, 79, 81, 79, 81, 79, 81, 79, 81, 79]
  T=[451, 449, 451, 449, 451, 449, 451, 449, 451, 449]
baseline 1
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 2
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 3
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 4
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
```

Paired instruction p=0; Welch instruction p=1.2572407650281474e-26; proposed bootstrap [L,U]=[0.04997001798920648, 0.05003001801080648]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C2_nonsignificant

**#635:** INCONCLUSIVE. **#637:** NOT SUPPORTED. **Proposed:** INCONCLUSIVE.

Both Welch and paired instruction p≈.16785 fail .01; the proposed table does not convert a bootstrap upper bound below .15 into regression evidence.

Exact series:

```text
candidate
  I=[1050, 850, 1050, 850, 1050, 850, 1050, 850, 1050, 850]
  B=[81, 79, 81, 79, 81, 79, 81, 79, 81, 79]
  T=[451, 449, 451, 449, 451, 449, 451, 449, 451, 449]
baseline 1
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 2
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 3
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 4
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
```

Paired instruction p=0.16785065605707344; Welch instruction p=0.16785065605707344; proposed bootstrap [L,U]=[-0.01, 0.11]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C3_instruction_regression

**#635:** INCONCLUSIVE under the literal any-primary-metric reading; otherwise underdefined. **#637:** NOT SUPPORTED. **Proposed:** NOT SUPPORTED.

Unchanged variable misses have Welch p=1. Original precedence can hide a strong instruction regression. Proposed instruction simultaneous lower bound is +200, so negative evidence prevails.

Exact series:

```text
candidate
  I=[1201, 1199, 1201, 1199, 1201, 1199, 1201, 1199, 1201, 1199]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[451, 449, 451, 449, 451, 449, 451, 449, 451, 449]
baseline 1
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 2
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 3
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 4
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
```

Paired instruction p=0; Welch instruction p=1.8531207936620982e-37; proposed bootstrap [L,U]=[-0.20012007204322593, -0.19988007195682592]. Proposed regression contrast indices passing simultaneous lower>0: [0] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C4_covariant_pairs

**#635:** INCONCLUSIVE. **#637:** SUPPORTED. **Proposed:** SUPPORTED.

Welch instruction p≈.44551 but paired p≈.00073897 because block covariance is retained. Given valid prospectively matched blocks, the paired model is the preferred estimand; metadata alone does not prove its assumptions.

Exact series:

```text
candidate
  I=[1280, 320, 1280, 320, 1280, 320, 1280, 320, 1280, 320]
  B=[81, 79, 81, 79, 81, 79, 81, 79, 81, 79]
  T=[451, 449, 451, 449, 451, 449, 451, 449, 451, 449]
baseline 1
  I=[1600, 400, 1600, 400, 1600, 400, 1600, 400, 1600, 400]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 2
  I=[1600, 400, 1600, 400, 1600, 400, 1600, 400, 1600, 400]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 3
  I=[1600, 400, 1600, 400, 1600, 400, 1600, 400, 1600, 400]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 4
  I=[1600, 400, 1600, 400, 1600, 400, 1600, 400, 1600, 400]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
```

Paired instruction p=0.00073896790980326613; Welch instruction p=0.44551145426299621; proposed bootstrap [L,U]=[0.2, 0.2]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C5_original_miss_counterexample

**#635:** Undefined for the zero-variance tests; NOT SUPPORTED if an added deterministic-limit convention is used. **#637:** NOT SUPPORTED. **Proposed:** NOT SUPPORTED.

All observations are constant. #635 does not define 0/0 or infinite Welch behavior. Under the explicitly proposed limit, the miss difference interval is [+1,+1], including under multiplicity control, so it vetoes the instruction gain.

Exact series:

```text
candidate
  I=[70, 70, 70, 70, 70, 70, 70, 70, 70, 70]
  B=[11, 11, 11, 11, 11, 11, 11, 11, 11, 11]
  T=[50, 50, 50, 50, 50, 50, 50, 50, 50, 50]
baseline 1
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 2
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 3
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 4
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
```

Paired instruction p=0; Welch instruction p=undefined in the original zero-variance formula; proposed bootstrap [L,U]=[0.3, 0.3]. Proposed regression contrast indices passing simultaneous lower>0: [1] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C6_equal_counts

**#635:** Undefined for equal constant instructions; INCONCLUSIVE if the additional p=1 convention is assumed. **#637:** NOT SUPPORTED. **Proposed:** INCONCLUSIVE.

Equal instructions are not an observed regression. #637 rejects via U=0<.15; proposal reports minimum-effect shortfall separately and returns INCONCLUSIVE.

Exact series:

```text
candidate
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[50, 50, 50, 50, 50, 50, 50, 50, 50, 50]
baseline 1
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 2
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 3
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 4
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
```

Paired instruction p=1; Welch instruction p=undefined in the original zero-variance formula; proposed bootstrap [L,U]=[0.0, 0.0]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C7_fastest_latency

**#635:** Conflicting: NOT SUPPORTED under the fastest-baseline formula; no matching disposition under the literal “higher than all” prose. **#637:** NOT SUPPORTED. **Proposed:** NOT SUPPORTED.

Candidate T=500±1 is slower than baseline 2 at 450±1 but faster than baselines 1/3/4. Proposal explicitly chooses regression versus any baseline with simultaneous coverage, not the contradictory all-baselines reading.

Exact series:

```text
candidate
  I=[701, 699, 701, 699, 701, 699, 701, 699, 701, 699]
  B=[81, 79, 81, 79, 81, 79, 81, 79, 81, 79]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 1
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[551, 549, 551, 549, 551, 549, 551, 549, 551, 549]
baseline 2
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[451, 449, 451, 449, 451, 449, 451, 449, 451, 449]
baseline 3
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[601, 599, 601, 599, 601, 599, 601, 599, 601, 599]
baseline 4
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[101, 99, 101, 99, 101, 99, 101, 99, 101, 99]
  T=[651, 649, 651, 649, 651, 649, 651, 649, 651, 649]
```

Paired instruction p=0; Welch instruction p=1.2544903973451196e-40; proposed bootstrap [L,U]=[0.29982010793523883, 0.3001801080648389]. Proposed regression contrast indices passing simultaneous lower>0: [3] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C8_multiplicity

**#635:** NOT SUPPORTED under the stated per-metric Welch/significance reading. **#637:** NOT SUPPORTED. **Proposed:** INCONCLUSIVE.

The branch regression is significant at unadjusted .01 but not at .01/18. New result is INCONCLUSIVE: mean miss guard fails, but no simultaneous regression is established. This changed conclusion is solely the prospective M3 multiplicity decision.

Exact series:

```text
candidate
  I=[701, 699, 701, 699, 701, 699, 701, 699, 701, 699]
  B=[999, 1001, 1002, 1003, 1004, 1006, 1007, 1008, 1009, 1011]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 1
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  T=[551, 549, 551, 549, 551, 549, 551, 549, 551, 549]
baseline 2
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  T=[551, 549, 551, 549, 551, 549, 551, 549, 551, 549]
baseline 3
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  T=[551, 549, 551, 549, 551, 549, 551, 549, 551, 549]
baseline 4
  I=[1001, 999, 1001, 999, 1001, 999, 1001, 999, 1001, 999]
  B=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  T=[551, 549, 551, 549, 551, 549, 551, 549, 551, 549]
```

Paired instruction p=0; Welch instruction p=1.2544903973451196e-40; proposed bootstrap [L,U]=[0.29982010793523883, 0.3001801080648389]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).
Branch contrast mean=5, SD=3.8297084310253524, p=0.0025647809345641271, simultaneous lower=-1.311284654467447. The .01/18 threshold is new and needs M3 approval.

## C9_exact_15_percent

**#635:** Undefined for constant Welch series; INCONCLUSIVE if equality of constant misses is assigned p=1 under literal precedence. **#637:** SUPPORTED. **Proposed:** SUPPORTED.

The proposal uses the explicit degenerate-limit convention and descriptive equality guard; d=L=U=.15 satisfies the inclusive minimum. No artificial epsilon is allowed.

Exact series:

```text
candidate
  I=[85, 85, 85, 85, 85, 85, 85, 85, 85, 85]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[50, 50, 50, 50, 50, 50, 50, 50, 50, 50]
baseline 1
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 2
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 3
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 4
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
```

Paired instruction p=0; Welch instruction p=undefined in the original zero-variance formula; proposed bootstrap [L,U]=[0.15, 0.15]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C10_noisy_misses

**#635:** INCONCLUSIVE under the nonsignificant-miss rule; primary constant-data convention is otherwise unspecified. **#637:** INCONCLUSIVE. **Proposed:** INCONCLUSIVE.

A huge noisy miss mean is not a statistically demonstrated regression; it also fails the descriptive guard. Keep INCONCLUSIVE rather than use a fixed allowance or treat a point regression as proven.

Exact series:

```text
candidate
  I=[70, 70, 70, 70, 70, 70, 70, 70, 70, 70]
  B=[1000000000, 0, 0, 0, 0, 0, 0, 0, 0, 0]
  T=[50, 50, 50, 50, 50, 50, 50, 50, 50, 50]
baseline 1
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 2
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 3
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
baseline 4
  I=[100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[55, 55, 55, 55, 55, 55, 55, 55, 55, 55]
```

Paired instruction p=0; Welch instruction p=undefined in the original zero-variance formula; proposed bootstrap [L,U]=[0.3, 0.3]. Proposed regression contrast indices passing simultaneous lower>0: [] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## C11_uncertain_benefit_miss_regression

**#635:** INCONCLUSIVE under the original nonsignificant-primary-instruction precedence. **#637:** NOT SUPPORTED. **Proposed:** NOT SUPPORTED.

Instruction p≈.16785 is inconclusive, but the constant miss regression has simultaneous interval [+1,+1]. Proposed precedence preserves that negative evidence rather than allowing uncertain benefit to mask it.

Exact series:

```text
candidate
  I=[1050, 850, 1050, 850, 1050, 850, 1050, 850, 1050, 850]
  B=[11, 11, 11, 11, 11, 11, 11, 11, 11, 11]
  T=[451, 449, 451, 449, 451, 449, 451, 449, 451, 449]
baseline 1
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 2
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 3
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
baseline 4
  I=[1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
  B=[10, 10, 10, 10, 10, 10, 10, 10, 10, 10]
  T=[501, 499, 501, 499, 501, 499, 501, 499, 501, 499]
```

Paired instruction p=0.16785065605707344; Welch instruction p=0.16785065605707344; proposed bootstrap [L,U]=[-0.01, 0.11]. Proposed regression contrast indices passing simultaneous lower>0: [1] (0=I vs B1; 1=misses vs B1; 2–5=latency vs B1–B4).

## Invalidity and aggregation fixtures

**C12 — fractional raw counters.** For every p=0..2, r=0..9: candidate `(I,B,T)=(70.5,10.5,50.5)`; each baseline `(100.25,12.25,55.25)`; checksum 42. These exact 150 rows were accepted by #637 as overall SUPPORTED in the reconciliation diagnostic. Under r1 they are BLOCKED—INVALID_INTEGER_OBSERVATION before statistics. Under #635 these are not valid hardware readings, so no valid statistical result exists; it defines no CSV parser. Source: [analyze.cpp:38–70](https://github.com/t81dev/t81-foundation/blob/988b18ca9ebdb4a6fb76fe0be30115470e5637f0/experiments/cmp3_decision_advantage/analyze.cpp#L38-L70).

**C13 — complete rows, failed completion.** Use every exact C4 series above for all 150 rows, and terminal runner evidence `exit_status=6, failure_code=BUDGET_EXCEEDED, failure_scope=global`. In the reconciliation diagnostic, #637 ignored the companion failure and returned SUPPORTED. Original #635 requires BLOCKED for budget failure; r1 requires BLOCKED before statistics regardless of rows. A forged or stale “success” manifest also blocks. The failed diagnostic status is synthetic, not an actual over-budget performance run.

**C14 — missing/malformed data.** Start from C1's exact mode series and remove baseline-4 observation 9 in pattern C, duplicate a row, or replace a counter by NaN, infinity, -1 or 2^64. r1 is BLOCKED for each defect. #637 blocks missing/duplicate/nonfinite/negative cases, but its double parser/range comparison can accept fractional values and 2^64 due to double(UINT64_MAX) rounding. #635 requires valid measurement/setup but does not specify ingestion. Hardware raw domain boundaries are proposed explicitly, not retroactively attributed to #635.

**C15 — aggregate states.** These are exact symbolic inputs to the aggregator, separate from measurement vectors:

| A,B,C states | #635 | #637 convention | Proposed r1 |
| --- | --- | --- | --- |
| SUPPORTED, SUPPORTED, SUPPORTED | No explicit all-three aggregation | SUPPORTED | SUPPORTED |
| SUPPORTED, INCONCLUSIVE, SUPPORTED | No explicit all-three aggregation | INCONCLUSIVE | INCONCLUSIVE |
| SUPPORTED, INCONCLUSIVE, NOT SUPPORTED | Original experimental precedence is not a specified per-pattern reducer | NOT SUPPORTED | NOT SUPPORTED |
| NOT SUPPORTED, SUPPORTED, BLOCKED | Infrastructure failure dominates, details unspecified | BLOCKED; current analyzer does not retain all pattern outputs | BLOCKED, retaining unaffected diagnostic records with validity scope |

## Precision, direction and variance boundaries to test after approval

C9 fixes inclusive d=.15. Add exact integer ratios just below/above 3/20 without altering a tolerance. p=.01 does not pass the primary strict test; simultaneous regression lower=0 does not pass the strict regression test. A very small *positive* variance is not the same as exact zero variance. Equal constants receive p=1/point zero; nonzero constant differences receive explicitly flagged limiting p=0/point intervals. A zero denominator or missing observation is invalid, not a limit case.

C1 has U<.15: r1 must state evidence against attaining the minimum target, even though its deliberately reserved composite label is INCONCLUSIVE. C2 also demonstrates that “no significant improvement” is not proof of a regression. C3/C5/C7 demonstrate that an actual qualifying regression cannot be suppressed by an unrelated uncertain benefit. C8 demonstrates why adopting multiplicity control can reverse an unadjusted negative conclusion. None of these diagnoses establish a software or hardware performance advantage.


Finalization note — draft 0.2: M1–M4 are provisionally accepted for preparing this approval candidate only. They are not formally approved. Draft 0.1 and all its evidence are retained in `history/draft-0.1/`. The finalization adds bootstrap verification and the concrete experiment-local runner profile without changing the paired design, 15% target, primary p<.01 threshold, 18-contrast family, alpha .01 or four-outcome predicates.

## Final bootstrap verification fixtures — additive draft 0.2 evidence

The original C1–C15 discussion above is retained. These additional mathematical fixtures instantiate the already selected M2 rule; they are not performance observations or new statistical tests. Exact resampling indices, all rational bounds and inputs are frozen in `FINAL_BOOTSTRAP_VERIFICATION.json` and `BOOTSTRAP_INDICES.u8`. The old-state recurrence and independent affine-jump calculation agree on every accepted index; direct pair sums and independent histogram-weighted sums agree on both order statistics for all seven fixtures.

| Fixture | Exact L | Sample variance of paired differences | Mathematical result with favourable other guards |
| --- | --- | --- | --- |
| B_below | 149999/1000000 | 11000/3 | INCONCLUSIVE |
| B_at | 3/20 | 11000/3 | SUPPORTED |
| B_above | 150001/1000000 | 11000/3 | SUPPORTED |
| Z_nonzero_constant | 3/20 | 0/1 | SUPPORTED |
| Z_equal_constant | 0/1 | 0/1 | INCONCLUSIVE |
| Z_constant_ratio_nondegenerate_difference | 3/20 | 9000000000/1 | SUPPORTED |
| NZ_near_zero_relative_variance | 37499999999999999/250000000000000000 | 1/10 | INCONCLUSIVE |

Exact raw instruction series (baseline 2–4 repeat baseline 1 for these diagnostic fixtures; branch and latency series use the unchanged favourable `series()` defaults from `verify_proposal.py`):

```text
B_below
  Ic=[850049, 850029, 850009, 849989, 849969, 849949, 849929, 849909, 849889, 849869]
  I1=[1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000]
B_at
  Ic=[850048, 850028, 850008, 849988, 849968, 849948, 849928, 849908, 849888, 849868]
  I1=[1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000]
B_above
  Ic=[850047, 850027, 850007, 849987, 849967, 849947, 849927, 849907, 849887, 849867]
  I1=[1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000]
Z_nonzero_constant
  Ic=[850000, 850000, 850000, 850000, 850000, 850000, 850000, 850000, 850000, 850000]
  I1=[1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000]
Z_equal_constant
  Ic=[1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000]
  I1=[1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000]
Z_constant_ratio_nondegenerate_difference
  Ic=[340000, 1360000, 340000, 1360000, 340000, 1360000, 340000, 1360000, 340000, 1360000]
  I1=[400000, 1600000, 400000, 1600000, 400000, 1600000, 400000, 1600000, 400000, 1600000]
NZ_near_zero_relative_variance
  Ic=[85000000000000000, 85000000000000000, 85000000000000000, 85000000000000000, 85000000000000000, 85000000000000000, 85000000000000000, 85000000000000000, 85000000000000000, 85000000000000001]
  I1=[100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000, 100000000000000000]
```

All B_below/B_at/B_above fixtures are nondegenerate: their paired variance is 11000/3, and the lower-bound classification changes precisely at 3/20. The NZ fixture is not zero variance: exact variance is 1/10 and normalized variance 10^-35. Its lower bound is smaller than 3/20 by 1/250000000000000000 even though binary64 formatting rounds both to the same value. Mandatory exact ordering/comparison is the narrow fix to the old optional “where feasible” phrase. Zero-difference constants remain INCONCLUSIVE; nonzero constants and varying pairs with constant .15 bootstrap ratio follow the existing explicit degenerate-limit convention. No variance cutoff, p threshold, minimum-effect target or NOT SUPPORTED rule changes.

Rechecked existing cases: C1_small_positive=INCONCLUSIVE, C2_nonsignificant=INCONCLUSIVE, C3_instruction_regression=NOT SUPPORTED, C4_covariant_pairs=SUPPORTED, C5_original_miss_counterexample=NOT SUPPORTED, C6_equal_counts=INCONCLUSIVE, C7_fastest_latency=NOT SUPPORTED, C8_multiplicity=INCONCLUSIVE, C9_exact_15_percent=SUPPORTED, C10_noisy_misses=INCONCLUSIVE, C11_uncertain_benefit_miss_regression=NOT SUPPORTED. All 11 are unchanged. The eight V/G/Q combinations still partition into exactly four outcomes and all 64 overall state triples obey BLOCKED > NOT SUPPORTED > INCONCLUSIVE > SUPPORTED. Full failed-collector CSVs remain globally BLOCKED at the integrity gate, before these numerical tests.

No repeated-sampling coverage calibration was performed. The frozen 1% and 99% order statistics are separate nominal one-sided 99% bounds; their pair is a central 98% percentile interval, not a jointly demonstrated 99% coverage result. Fixture correctness is not a coverage guarantee.
