# T81 SCG-08 Phase 0 — Final Independent Acceptance Review Report R2

**Document ID:** `T81-SCG-08-PHASE0-ACCEPTANCE-001-R2`  
**Target Document:** `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md`  
**Document Title:** *SCG-08 Phase 0 Research Charter: Architectural Advantage Investigation*  
**Review Profile:** Read-Only Independent Acceptance Review  
**Authoritative Provenance:**
- **Observed Review Head:** `d62a7c988139a0defd125dca16dba0f32366e0b1` (PR #745)
- **Integration Merge Commit:** `0bd0817e4b9dc879fffce18c59ea9ff781f7acb3` (PR #753 merging into `origin/main`)
- **Current Baseline:** `9a3e307ad21135749d35a4f1a6a0d5820d0cc698` (PR #754)
- **Changed-File Inventory:** Exactly 1 file modified in charter baseline (`docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md`, +546 lines, 0 deletions)
- **Working Tree State:** Strictly clean, read-only, 0 uncommitted modifications.

---

## Executive Summary & Final Review Disposition

An independent, read-only acceptance review of the SCG-08 Phase 0 Research Charter (`docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md`) was conducted across mathematical derivations, statistical sampling methodologies, historical continuity, hypothesis registration, experimental isolation, and research governance.

**Final Review Disposition:** **`READY FOR REVIEW`**  
*(Finding Evaluation: **`PASS`** on theoretical and mathematical derivations; **`CONDITIONAL ACCEPTANCE`** on sampling and operational cost qualifications; **`NO BLOCKERS`** identified).*

---

## 1. Reconciled Statistical Sampling Quantities

To prevent conflation between the frozen experimental protocol in the charter and proposed empirical implementation designs, sampling quantities are reconciled:

| Sampling Quantity | Defined Level | Scope & Identity | Count / Specification |
|:---|:---|:---|:---|
| **Experimental Trials** | Frozen Protocol | Preregistered in charter Section 5.3 under H02-B | $M = 10^7$ steps |
| **Independent Sequences** | Implementation Design | Proposed sequence clusters with boundary reset ($c_0 = 0$) | $M = 136,500$ clusters |
| **Digit-Level Observations** | Mathematical Process | Individual balanced-ternary digit positions $k$ | Total steps evaluated across sequences |
| **Transition Observations** | Statistical Indicator | Consecutive digit-pair state transitions $(c_k, c_{k+1})$ | Sample transitions for $f_{0\to 1}, f_{1\to 1}, f_{1\to 0}$ |

### Critical Reconciliation Note:
> **Explicit Qualification:** The proposed 136,500-sequence implementation allocation across the 6 length bands has **not yet been numerically reconciled** with the charter's registered $10^7$-step criterion. Compliance with the registered sample size cannot be inferred, and the frozen protocol remains unaltered. Full numerical reconciliation remains a prerequisite for any future Gate 2 experimental campaign protocol authorization.

---

## 2. Qualification of Effective Sample Size & Serial Dependence

Section 5.3 of the charter defines a $3\sigma$ threshold for statistical falsification. Methodological factors are qualified as follows:

1. **Asymptotic Approximation Scope:**
   The effective sample size relation:
   $$M_{\text{eff}} \approx M \cdot \frac{1 - \lambda_3}{1 + \lambda_3} = M \cdot \frac{1 - 1/9}{1 + 1/9} = 0.8 M$$
   is an **asymptotic approximation** derived under the stationary Markov chain assumption specifically for the stationary active-carry indicator $I(c_k \neq 0)$.
2. **Domain-Specific Non-Universality:**
   This $0.8M$ approximation must **not** be applied universally to:
   - Transition-frequency estimators (such as $f_{0 \to \text{active}}$), which depend on bivariate joint state probabilities rather than univariate indicators.
   - Finite-length sequence boundaries where the transient component $(1/9)^k$ dominates before stationary mixing occurs.
3. **Cluster-Level Variance:**
   In empirical evaluation using independent sequences, variance should be estimated using cluster-robust delta methods on linearized sequence residuals ($z_{bm} = U_{bm} - \hat{R} V_{bm}$) rather than assuming homogeneous i.i.d. sampling.
4. **Statistical Match vs. Physical Advantage:**
   Empirical conformance of carry counts to the Markov model proves mathematical validity of the adder logic; it does not constitute evidence of computational throughput advantage or hardware efficiency.

---

## 3. Independent Mathematical Recomputation & Reconstruction

The balanced-ternary addition of two independent, uniformly distributed balanced-ternary digits $A_k, B_k \in \{-1, 0, +1\}$ with incoming carry $c_k \in \{-1, 0, +1\}$ yields intermediate sum $S_k = A_k + B_k + c_k$. Outgoing carry $c_{k+1}$ satisfies:
$$c_{k+1} = \begin{cases} +1 & \text{if } S_k \ge 2 \\ -1 & \text{if } S_k \le -2 \\ 0 & \text{if } -1 \le S_k \le 1 \end{cases}$$

### Complete Truth-Table Enumeration (All $3 \times 9 = 27$ Cases):
- **For $c_{\text{in}} = -1$:**
  - $S_k = -3 \implies c_{\text{out}} = -1$ (1 pair: $(-1, -1)$)
  - $S_k = -2 \implies c_{\text{out}} = -1$ (2 pairs: $(-1, 0), (0, -1)$)
  - $S_k = -1 \implies c_{\text{out}} = 0$ (3 pairs: $(-1, 1), (0, 0), (1, -1)$)
  - $S_k = 0 \implies c_{\text{out}} = 0$ (2 pairs: $(0, 1), (1, 0)$)
  - $S_k = 1 \implies c_{\text{out}} = 0$ (1 pair: $(1, 1)$)
  - *Totals:* $c_{\text{out}} = -1: 3/9 = 1/3$; $c_{\text{out}} = 0: 6/9 = 2/3$; $c_{\text{out}} = 1: 0/9 = 0$.
- **For $c_{\text{in}} = 0$:**
  - $S_k = -2 \implies c_{\text{out}} = -1$ (1 pair: $(-1, -1)$)
  - $S_k = -1 \implies c_{\text{out}} = 0$ (2 pairs: $(-1, 0), (0, -1)$)
  - $S_k = 0 \implies c_{\text{out}} = 0$ (3 pairs: $(-1, 1), (0, 0), (1, -1)$)
  - $S_k = 1 \implies c_{\text{out}} = 0$ (2 pairs: $(0, 1), (1, 0)$)
  - $S_k = 2 \implies c_{\text{out}} = 1$ (1 pair: $(1, 1)$)
  - *Totals:* $c_{\text{out}} = -1: 1/9$; $c_{\text{out}} = 0: 7/9$; $c_{\text{out}} = 1: 1/9$.
- **For $c_{\text{in}} = +1$:**
  - $S_k = -1 \implies c_{\text{out}} = 0$ (1 pair: $(-1, -1)$)
  - $S_k = 0 \implies c_{\text{out}} = 0$ (2 pairs: $(-1, 0), (0, -1)$)
  - $S_k = 1 \implies c_{\text{out}} = 0$ (3 pairs: $(-1, 1), (0, 0), (1, -1)$)
  - $S_k = 2 \implies c_{\text{out}} = 1$ (2 pairs: $(0, 1), (1, 0)$)
  - $S_k = 3 \implies c_{\text{out}} = 1$ (1 pair: $(1, 1)$)
  - *Totals:* $c_{\text{out}} = -1: 0/9 = 0$; $c_{\text{out}} = 0: 6/9 = 2/3$; $c_{\text{out}} = 1: 3/9 = 1/3$.

The exact transition matrix is:
$$\mathbf{P} = \begin{pmatrix} 1/3 & 2/3 & 0 \\ 1/9 & 7/9 & 1/9 \\ 0 & 2/3 & 1/3 \end{pmatrix}$$

### Derived Invariants:
1. **Stationary Distribution:** $\boldsymbol{\pi} = (1/8, 3/4, 1/8)$, verified by $\boldsymbol{\pi}\mathbf{P} = \boldsymbol{\pi}$.
2. **Stationary Carry Occurrence:** $\pi(c \neq 0) = 1/4 = 0.250000$.
3. **Zero-to-Active Transition Frequency:** $f_{0 \to \text{active}} = \pi_0 \cdot (2/9) = (3/4)(2/9) = 1/6 \approx 0.166667$.
4. **Active-to-Zero Transition Frequency:** $f_{\text{active} \to 0} = (\pi_{-1} + \pi_1)(2/3) = (1/4)(2/3) = 1/6 \approx 0.166667$.
5. **Total Transition Frequency:** $f_{\text{total\_trans}} = 1/6 + 1/6 = 1/3 \approx 0.333333$.
6. **Expected Active Run Length:** $E[L] = 1 / (1 - 1/3) = 1.500$ trits.
7. **1-Tryte Carry Probability:** $P(c_3 \neq 0 \mid c_0 = 0) = \frac{1}{4}(1 - (1/9)^3) = 182/729 \approx 0.249657$.
8. **Finite-$N$ Active Carry Expectation:**
   $$E[N_{\text{active}}] = \frac{N}{4} - \frac{1 - (1/9)^N}{32}$$
   At $N=27$: $E[27_{\text{active}}] = 6.75 - 0.03125 = 6.71875$ carries.

---

## 4. Preservation of Historical Governance & Hypothesis Register

1. **Alignment with Hypothesis Register:**
   Traced directly to `HYPOTHESIS_REGISTER_v1.json` (H01, H02, H04, H05, H06). Preserves historical negative results:
   - Hanoi CWEB tryte addition $10.6\times$ slower than binary host addition (STOP).
   - Packed MatVec AVX2 achieving 75.3% of INT8 throughput (STOP).
   - TISC artifact size expansion of $7.898\times$ relative to Wasm.
2. **Historical Governance Separation:**
   - **Phase 0 Authorization:** The charter preregisters research hypotheses and theoretical derivations; it authorized neither Gate 1 implementation nor Gate 2 campaign execution.
   - **Phase 1 Gate 1 Continuity:** Subsequent proposals (`T81-SCG-08-PHASE1-PROP-001-R1/R2/R3` and Addenda 01–05) were authorized under separate maintainer directives. Citing Phase 0 boundaries preserves governance without retroactively invalidating subsequent activities.
   - **Gate 2 Status:** Benchmark campaign execution remains strictly unauthorized.

---

## 5. Evidentiary Classification Matrix

| Claim / Component | Evidentiary Status | Verification Reference |
|:---|:---|:---|
| Full Truth Table & Matrix $\mathbf{P}$ | **Mathematically Verified** | 27-state exhaustive combinatorial enumeration |
| Stationary Distribution $\boldsymbol{\pi}$ | **Mathematically Verified** | Exact rational nullspace solving $(\mathbf{I} - \mathbf{P}^T)\boldsymbol{\pi}^T = 0$ |
| Carry Frequencies ($1/4, 1/6, 1/3$) | **Mathematically Verified** | Exact rational stationary state products |
| Run Length $E[L] = 1.500$ trits | **Mathematically Verified** | Geometric sum with continuation $p = 1/3$ |
| Finite-$N$ Sum $E[N]$ | **Mathematically Verified** | Exact geometric progression summation |
| 1-Tryte Carry $182/729$ | **Mathematically Verified** | Exact rational evaluation at $N=3$ |
| CWEB Scalar Addition Slowdown ($10.6\times$) | **Historically Reported** | `experiments/radix_comparison/` |
| Packed MatVec Throughput (75.3%) | **Historically Reported** | `experiments/packed_ternary_matvec/` |
| TISC Code Expansion ($7.898\times$) | **Historically Reported** | `experiments/tisc_vs_wasm/` |
| Actual Limb Layout (`alignas(16) int8_t`) | **Source-Verified** | `include/t81/types/T81Limb.hpp:42` |
| Hardware EDP Model | **Not Independently Verified** | Analytic model parameterized without physical ternary IC |
| Proposed `EXP-SCG08-01` Execution | **Proposed / Not Authorized** | Benchmark execution withheld |

---

## 6. Reproducibility Appendix: Exact Rational Verification Code

### Verification Command:
```bash
python3 -c "
import fractions
F = fractions.Fraction

digits = [-1, 0, 1]
carries = [-1, 0, 1]
P_reconstructed = [[F(0, 1) for _ in range(3)] for _ in range(3)]

for i, c_in in enumerate(carries):
    counts = {-1: 0, 0: 0, 1: 0}
    for a in digits:
        for b in digits:
            s = a + b + c_in
            c_out = 1 if s >= 2 else (-1 if s <= -2 else 0)
            counts[c_out] += 1
    for j, c_out in enumerate(carries):
        P_reconstructed[i][j] = F(counts[c_out], 9)

expected_P = [
    [F(1, 3), F(2, 3), F(0, 1)],
    [F(1, 9), F(7, 9), F(1, 9)],
    [F(0, 1), F(2, 3), F(1, 3)]
]
assert P_reconstructed == expected_P

pi = [F(1, 8), F(3, 4), F(1, 8)]
for j in range(3):
    assert sum(pi[i] * P_reconstructed[i][j] for i in range(3)) == pi[j]

f_0_to_active = pi[1] * (P_reconstructed[1][0] + P_reconstructed[1][2])
assert f_0_to_active == F(1, 6)

f_active_to_0 = (pi[0] + pi[2]) * F(2, 3)
assert f_active_to_0 == F(1, 6)

f_total_transitions = f_0_to_active + f_active_to_0
assert f_total_transitions == F(1, 3)

p_continuation = F(1, 3)
expected_run_length = F(1, 1) / (F(1, 1) - p_continuation)
assert expected_run_length == F(3, 2)

def p_active_at_k(k: int) -> fractions.Fraction:
    return F(1, 4) * (F(1, 1) - F(1, 9)**k)

assert p_active_at_k(3) == F(182, 729)

def expected_active_total(N: int) -> fractions.Fraction:
    return F(N, 4) - (F(1, 1) - F(1, 9)**N) / F(32, 1)

for N in range(1, 55):
    assert sum(p_active_at_k(k) for k in range(1, N + 1)) == expected_active_total(N)

print('Python verification script successfully asserted all modeled discrete quantities.')
"

```

### Verification Output:

```
Python verification script successfully asserted all modeled discrete quantities.

```

*Statement of Scope:* The verification script specifically asserts the 27 combinatorial additions, the reconstructed transition matrix $\mathbf{P}$, stationary distribution $\boldsymbol{\pi}$, transition frequencies ($1/6, 1/3$), run length ($1.500$), 3-trit carry probability ($182/729$), and finite-$N$ expectation formula for widths $N \in [1, 54]$.

---

## 7. Review Findings & Final Acceptance Conclusion

- **Finding 1 (Transition Frequency Disambiguation):** Resolved and clarified; $1/6$ is the zero-to-active transition frequency, while total transition frequency is $1/3$.
- **Finding 2 (Sampling Quantities & Effective Sample Size):** Reconciled and qualified; $M=10^7$ is the registered protocol criterion, while $M\_{\text{eff}} \approx 0.8M$ is an asymptotic approximation for the stationary active-carry indicator. The 136,500-sequence allocation is not yet numerically reconciled with the $10^7$-step criterion.
- **Finding 3 (Governance Separation):** Reconciled; Phase 0 boundaries preserved without invalidating subsequent separately authorized Phase 1 activities.

**Final Review Disposition:** **`READY FOR REVIEW`**

