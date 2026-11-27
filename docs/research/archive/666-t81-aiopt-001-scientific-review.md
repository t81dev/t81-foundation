> **Historical Research Archive (Non-Normative)**
> - **Source PR:** `#666`
> - **Source Commit:** `bb38deddf`
> - **Original Path:** `docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md`
> - **Status:** Archived read-only historical record; does not alter accepted specs, frozen locks, or active research holds.

---

# Scientific & Mathematical Review: T81-AIOPT-001 (Fixed-Catalog Ternary Pattern Filtering)

**Document ID:** `docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md`
**Date:** November 2026
**Status:** AI-assisted mathematical source assessment; independent experimental replication not established
**Review Target:** T81-AIOPT-001 Milestone M1, unmerged PR #665 head `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db` (branch `research/aiopt-001-m1-protocol`; that head was unchanged on 2026-11-23). Protocol: `docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md`. Implementation: `experiments/aiopt_001_fixed_catalog/m1.py`. Neither file is on `main`.
**Investigator:** Jules (AI-assisted scientific analyst; not an authenticated independent human reviewer)
**Scientific Disposition:** `M2_REDESIGN_REQUIRED`

**Review provenance:** The initial Jules assessment did not pin an inspected M1 SHA, preserve an execution log, or establish that Jules executed M1. Independent experimental reproduction and independent-person peer review are **not claimed**. This review and the M1 commits are both recorded under the repository owner login; shared Git attribution is not independence. A correction pass on 2026-11-23 read the M1 protocol, `m1.py`, and `experiments/aiopt_001_fixed_catalog/M1_VERIFICATION_REPORT.md` at the SHA above and checked the mathematical claims against that source. It did not execute `m1.py`. That verification report recommends `READY_FOR_INDEPENDENT_ACCEPTANCE` for local checks of source commit `32d7f9a885d527772227d0d13d108f5de3483497`, and it states that independent-person replication is not claimed. `m1.py` emits `LOCAL_TESTS_PASS` only when its own assertions pass. The mathematical critique of the stated $C_{40}$ semantics does not depend on executing M1.

The operational index on `main` pins an earlier revision of this file at `b2b9d3c73df96d6b4227ee0061c56bc2ee04a8fe` (`docs/ai-context/CURRENT_STATE.json`, observation `AIOPT-001-M1`). Commit `a14cf48a9ec8494f876819ec457be5478dfc08d5` restored the uncorrected text and dropped those bounds. This revision restores them and corrects additional mismatches against current `main`. It does not rewrite the pinned historical receipt.

**Governing record:** Maintainer decision, 2026-11-23: `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md`, already on `main`, governs. It critiques wildcard linear filtering as a premise for a ternary-specific advantage and leaves M2 execution blocked. Section 6 is retained as a non-binding alternative. It does not authorize execution and does not override that assessment.

---

## 1. Executive Assessment

Milestone M1 of T81-AIOPT-001 proposes an isolated, correctness-only verification framework for evaluating ternary pattern filtering across three software representations: an independent scalar integer oracle, a byte-oriented unpacked adapter, and an 8-bit SIMD-Within-A-Register (SWAR) packed 2-bit adapter. The operational domain is bounded to 4-trit vectors $X = \{-1, 0, +1\}^4$ ($|X| = 81$) filtered against a 40-pattern catalog ($C_{40}$) formed by the first 40 lexicographically ordered vectors.

### Core Review Findings

1. **Exact Representation Equivalence Only (Correctness Is Not Optimization):**
   M1 defines a finite correctness contract for equivalent representations. The M1 harness reports matching predicate outputs, and this review did not independently execute or reproduce those tests. The contract does not, and a finite equality check cannot, demonstrate computational speedup, representation superiority, hardware efficiency, or AI-guided optimization.

2. **Severe Degeneracy of the Proposed Workload:**
   Because all patterns in $C_{40}$ are fully specified without wildcards, the 40 patterns are mutually exclusive singletons. Consequently, for any valid input $x \in X$, the output set of matching indices has cardinality $|M(x)| \le 1$. Specifically, $|M(x)| = 1$ for exactly 40 inputs and $|M(x)| = 0$ for 41 inputs. Furthermore, because $C_{40}$ is defined as the prefix of the lexicographical enumeration of $X$, the match function collapses mathematically to:
   $$\text{filter}(x, C_{40}) = \begin{cases} [\text{lex\_index}(x)] & \text{if } \text{lex\_index}(x) < 40 \\ [] & \text{otherwise} \end{cases}$$
   Evaluating this problem via an $O(|C|)$ linear scan over 40 patterns in a loop (whether scalar, byte, or SWAR) evaluates an algorithm that is asymptotically obsolete compared with a constant-time scalar integer comparison or direct array lookup. No cycle count was measured.

3. **Absence of SWAR Vectorization Advantage:**
   In M1, the SWAR adapter serializes pattern matching across 40 iterations. The SWAR predicate `((payload ^ target) & 0xFF) == 0` evaluates an exact 8-bit integer equality `payload == target`. The SWAR encoding performs no parallel lane filtering in this predicate. It is an injective 8-bit packed code for the 81 valid vectors, with the least-significant pair holding coordinate 0, matching `encode_swar` / `filter_swar` in `m1.py`.

4. **Severe Selection Bias in $C_{40}$:**
   Truncating the lexicographical enumeration at 40 patterns introduces extreme structural asymmetry. In $C_{40}$, position 0 contains 27 $-1$ trits, 13 $0$ trits, and exactly 0 $+1$ trits. The $+1$ state is completely absent from the leading lane, invalidating any claim that $C_{40}$ represents balanced ternary behavior.

5. **AI-Optimization Vacuum:**
   PR #665 contains zero AI optimization, zero heuristic search, and zero adaptive selection logic. Attempting to deploy "AI-guided optimization" on a 3-way choice between trivial representations for an 81-state domain represents an extreme mismatch of problem complexity to optimization machinery.

### Disposition

**`M2_REDESIGN_REQUIRED`**: Milestone M1 specifies fail-closed correctness and mutual consistency among its three adapters. Those properties were not independently reproduced by this review. The protocol cannot proceed to an optimization milestone on this workload. Measuring execution time or training a selection agent on it would time loop overhead and compiler register allocation rather than representation efficiency. The governing next record is the merged M2 assessment. Section 6's wildcard grid is a non-binding alternative and does not authorize the next experiment.

---

## 2. Mathematical Analysis of the Proposed Workload

### 2.1 Domain Cardinality, Information Content, and Packing Density

Let the ternary alphabet be $\mathbb{T} = \{-1, 0, +1\}$, with cardinality $|\mathbb{T}| = 3$. The input vector space is $X = \mathbb{T}^4$, with total domain cardinality:
$$|X| = 3^4 = 81$$

The exact information-theoretic entropy of a uniformly distributed 4-trit vector is:
$$H(X) = \log_2(81) \approx 6.33985 \text{ bits}$$

In conventional digital binary architectures:
- **Theoretical Minimum Storage:** $\lceil \log_2 81 \rceil = 7 \text{ bits}$.
- **Byte-Oriented Encoding:** Allocates 1 byte (8 bits) per trit, consuming $4 \times 8 = 32 \text{ bits}$. The utilized state space is $81 / 2^{32} \approx 1.886 \times 10^{-8}$ (99.999998% wasted state space).
- **SWAR 2-Bit Packed Encoding:** Allocates 2 bits per trit ($\{00_2 \to -1, 01_2 \to 0, 10_2 \to +1\}$), packing 4 trits into an 8-bit unsigned integer ($2^8 = 256$ states). Of the 256 states, exactly $3^4 = 81$ are valid ternary representations, leaving $256 - 81 = 175$ invalid states (68.36% unused code space).

While SWAR compresses the memory footprint from 4 bytes to 1 byte (a $4\times$ memory reduction), this is a trivial compression property of binary packaging. It does not establish computational advantage.

### 2.2 Workload Degeneracy: Mutual Exclusivity and Structural Collapse

The catalog $C_{40} = (p_0, p_1, \dots, p_{39})$ is defined as the first 40 elements of $X$ ordered lexicographically under $-1 < 0 < +1$. In `m1.py` that order is `itertools.product((-1, 0, 1), repeat=4)`, which varies coordinate 3 fastest and coordinate 0 slowest. Position 0 in the bias table below is that first coordinate.

**Theorem 1 (Mutual Exclusivity):**
*For any input $x \in X$, $|\{i \mid p_i = x, p_i \in C_{40}\}| \le 1$.*

*Proof:* By definition, $X$ is a set of distinct 4-tuples. $C_{40} \subset X$ is an ordered sequence of 40 distinct elements without replacement. Since matching requires $x_j = p_{i, j}$ for all $j \in \{0,1,2,3\}$, $match(x, p_i) = 1 \iff x = p_i$. Because all $p_i$ are pairwise distinct, $x$ can equal at most one pattern $p_i \in C_{40}$. $\blacksquare$

**Corollary 1.1:**
Across all 81 inputs in $X$:
- 40 inputs produce an output list of length 1 (a single matching index).
- 41 inputs produce an output list of length 0 (empty list).
- Exactly 0 inputs produce multi-match lists.

**Theorem 2 (Index Triviality):**
*Let $\phi: X \to \{0, 1, \dots, 80\}$ be the canonical base-3 lexicographical bijection $\phi(x) = \sum_{j=0}^3 (x_j + 1) \cdot 3^{3-j}$. Then for any $x \in X$:*
$$\text{filter}(x, C_{40}) = \begin{cases} [\phi(x)] & \text{if } \phi(x) < 40 \\ [] & \text{if } \phi(x) \ge 40 \end{cases}$$

*Proof:* Follows directly from the construction of $C_{40}$ as the prefix of length 40 of the lexicographically sorted sequence of $X$. $\blacksquare$

**Scientific Implication:**
The problem posed in T81-AIOPT-001 M1 as "pattern filtering" is mathematically isomorphic to testing whether an integer index $\phi(x)$ is strictly less than 40. Solving this via 40 iterative vector comparisons in a loop is an algorithmic artifact, not an intrinsic computational requirement.

### 2.3 Catalog Selection Bias in $C_{40}$

The protocol defines $C_{40}$ by truncating the lexicographical generation of $\mathbb{T}^4$ at $N = 40$. In balanced ternary, the three states $\{-1, 0, +1\}$ are symmetric around 0. However, lexicographical truncation introduces severe structural bias across the pattern catalog:

| Position $j$ | Weight $3^{3-j}$ | Count of $-1$ | Count of $0$ | Count of $+1$ | Marginal Skew |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **Lane 0** (MSB) | 27 | **27 (67.5%)** | **13 (32.5%)** | **0 (0.0%)** | $+1$ completely absent |
| **Lane 1** | 9 | **18 (45.0%)** | **13 (32.5%)** | **9 (22.5%)** | Heavy negative bias |
| **Lane 2** | 3 | **15 (37.5%)** | **13 (32.5%)** | **12 (30.0%)** | Moderate negative bias |
| **Lane 3** (LSB) | 1 | **14 (35.0%)** | **13 (32.5%)** | **13 (32.5%)** | Near uniform |

**Consequences of Selection Bias:**
1. **Lane Degeneracy:** Any branch or short-circuit logic evaluating Lane 0 will never observe a $+1$ in any catalog entry.
2. **Asymmetric Error Sensitivity:** An input with $x_0 = +1$ will fail all 40 patterns on the first trit comparison. An input with $x_0 = -1$ will survive Lane 0 for 27 patterns (67.5% of the catalog), creating artificial variance in branch predictors that is an artifact of the lexicographical prefix rather than ternary logic.

### 2.4 Mechanics of SWAR Pattern Matching in M1

In M1, the SWAR filter evaluates:
```python
target = sum((p[j] + 1) << (2 * j) for j in range(4))
if ((payload ^ target) & 255) == 0:
    result.append(i)
```
Notice that:
1. `payload` is an 8-bit integer.
2. `target` is an 8-bit integer.
3. The bitwise operation `(payload ^ target) & 0xFF == 0` is mathematically and logically identical to:
   $$\text{payload} == \text{target}$$
4. Because each trit is encoded with an injective 2-bit code, two 4-trit vectors are equal if and only if their 8-bit SWAR integers are equal:
   $$x = p \iff \text{encode\_swar}(x) = \text{encode\_swar}(p)$$
5. The SWAR adapter does not perform SIMD parallel sub-word arithmetic or relational comparisons. It executes an 8-bit scalar integer comparison inside a 40-iteration loop.

### 2.5 Intrinsic vs. Encoding Costs

To understand what is being computed, we separate intrinsic costs from representation artifacts:

| Dimension | Intrinsic Problem Cost | Representation Artifact (M1) |
| :--- | :--- | :--- |
| **Information Bound** | Distinguishing the 81 inputs requires $\log_2(81) \approx 6.33985$ bits. Membership in this prefix is then a comparison of that index with 40. | Byte adapter allocates 32 bits; SWAR allocates 8 bits. |
| **Decision Complexity** | Membership in this static 40-element prefix is one index comparison, a binary search of $\lceil \log_2 40 \rceil = 6$ comparisons, or an $O(1)$ table lookup. | M1 evaluates 40 linear iterations of per-trit comparisons or 40 sequential integer XOR comparisons. |
| **Validation Cost** | Zero if the input is trusted or guaranteed by construction. | `decode_swar` rejects any 2-bit lane equal to $11_2$ before matching. Whether that check costs more than the comparison was not measured. |
| **Loop Overhead** | Absent from the index comparison. | M1 recorded no timings. Any later timing of the 40-iteration loop would include counter increments, indexing, and branches. |

---

## 3. Relationship to the H01–H08 Hypothesis Program

The T81 Foundation maintains a formal hypothesis register (`docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`) and research charter. The proposed T81-AIOPT-001 experiment interacts with these canonical hypotheses:

### 3.1 Interaction with H03 (Base-81 Interfacing & Grouping)
- **Claim:** Grouping four trits as base-81 can provide a workload-specific interface or execution benefit beyond equivalent binary packing (`docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`, H03).
- **M1 Relationship:** The input domain $X = \mathbb{T}^4$ has exactly 81 states. Base-81 represents each 4-trit vector as a single digit $d \in [0, 80]$.
- **Finding:** If the base-81 digit is the lexicographic index $\phi(x)$, the pattern filter for this prefix catalog is a scalar range check: `return (d < 40) ? [d] : []`. M1 does not use that conversion. It uses SWAR 2-bit packing, whose code space has 256 states. For this prefix catalog, the base-81 index makes the filter a range test. That is a property of $C_{40}$, not a measured H03 result, and it does not show a benefit beyond equivalent binary packing.

### 3.2 Interaction with H04 (Adaptive Representation) & H04-R02 Findings
- **Claim:** An explicitly defined adaptive ternary representation policy can beat fixed ternary and an equally capable adaptive binary policy on a specified workload (`docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`, H04).
- **Historical Evidence:** Corrected H04-R02 (`experiments/h04_r02_adaptive_representation/RESULTS.md`) invalidates the original inference and retains conditional symbolic-model results. For the matched 10-bit control, the AT window narrows to $r < 5/3$, which includes $r = 1.60$. `experiments/h04_r02_adaptive_representation/H04_MODEL_GAP_ASSESSMENT.md` records AT = 1210 U and AB = 1250 U at that point. Modeled winners depend on metadata, mode boundaries, and transition and conversion assumptions. The historical 96% figure applies to one ratio-1 trace, not general observed variance. Actual encoded byte lengths were measured. No new CPU performance, bandwidth, area, or energy advantage or loss was established. H04 remains unresolved.
- **M1 Relationship:** The AIOPT-001 name suggests representation selection. The M1 protocol fixes the encoding in advance and forbids adaptive selection. Conversion and dispatch costs of a switching policy were not measured.

### 3.3 Prior CPU Ternary Arithmetic Closure (PRs #544–#549)
- **Recorded finding:** In PR #549 (`docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`), CPU-based ternary kernel optimization was formally **frozen**. That synthesis reports specific measured CPU-kernel comparisons and records a program-level freeze on further custom ternary CPU-kernel optimization. Its negative comparisons are bounded by the tested kernels, workloads, architectures, and execution conditions. They do not prove that no future packed-ternary algorithm can outperform a binary alternative on any CPU workload.
- **M1 relationship:** T81-AIOPT-001 risks reopening that frozen optimization program under an "AI optimization" label. The closure record does not supply a universal proof that would make every later packed-ternary experiment futile.

### 3.4 Interaction with H08 (AI-Assisted Research Trustworthiness)
- **Claim:** AI-assisted iteration improves research trustworthiness or discovery under an independently auditable process (`docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`, H08).
- **M1 Relationship:** The AIOPT-001 title refers to AI optimization. M1 contains no learned selector. Using a model to choose among three hand-coded loops on an 81-state problem, where an $O(1)$ lookup answers the same predicate, would not meet H08's stated measurement: a controlled comparison of trustworthiness or discovery. That conditional mismatch is not a recorded H08 result.

---

## 4. Meaningful Comparator Framework

A scientifically defensible comparison must not evaluate ternary representations against a strawman. Any future milestone (M2) must test the candidate representations against standard, high-performance conventional baselines under identical semantic contracts.

### 4.1 Specification of the Five Comparators

All comparators must accept the exact same input domain $X = \mathbb{T}^4$, filter against catalog $C_{40}$, and return identical ordered index lists.

```
                    +------------------------------------------+
                    |           Semantic Input (X in T^4)      |
                    +------------------------------------------+
                                         |
     +-----------------+-----------------+-----------------+-----------------+
     |                 |                 |                 |                 |
     v                 v                 v                 v                 v
[Comparator 1]   [Comparator 2]   [Comparator 3]   [Comparator 4]   [Comparator 5]
Conventional     Direct Table     Bitset Filter    Packed SWAR      Byte-Oriented
Binary Scalar    Lookup (LUT)     (Uint64 Mask)    Ternary (M1)     Ternary (M1)
Scalar Int       Single array     Shift + bitwise  8-bit XOR loop   4-byte loop
comparison       indexing O(1)    test O(1)        40 iterations    40 iterations
```

#### Comparator 1: Conventional Binary Scalar (Integer Lexicographical Index)
- **Data Structure:** Input $x \in \mathbb{T}^4$ mapped to standard integer $k = \sum_{j=0}^3 (x_j + 1) \cdot 3^{3-j}$, where $k \in [0, 80]$.
- **Execution:**
  ```c
  // Direct scalar evaluation
  if (k < 40) {
      out[0] = k;
      return 1;
  }
  return 0;
  ```
- **Cost:** 1 comparison, 1 conditional branch (predictable). $O(1)$ time, 0 bytes auxiliary memory.

#### Comparator 2: Direct Table Lookup (LUT-81 / LUT-256)
- **Data Structure:**
  - **LUT-81:** Array `int8_t lut[81]`, where `lut[k] = (k < 40) ? k : -1`. Size: 81 bytes.
  - **LUT-256:** Array `int8_t lut_swar[256]`, directly indexed by the 8-bit SWAR payload. Size: 256 bytes.
- **Execution:**
  ```c
  int8_t match = lut_swar[swar_payload];
  if (match >= 0) {
      out[0] = match;
      return 1;
  }
  return 0;
  ```
- **Cost:** One memory load and one comparison, in place of the 40-iteration loop. L1 residency was not measured.

#### Comparator 3: Bitset / 64-Bit Word Mask
- **Data Structure:** Membership of indices 0 through 39 fits in one `uint64_t`. A bitset of all 81 domain states needs two `uint64_t` words because 81 is greater than 64.
- **Execution:**
  ```c
  // k is the lexicographic index in [0, 80].
  // 0xFFFFFFFFFFULL is bits 0..39. Shifting a uint64_t by k is
  // defined only for k < 64, which holds for every catalog hit.
  uint64_t catalog_mask = 0xFFFFFFFFFFULL;
  if ((k < 64) && (catalog_mask & (1ULL << k)) != 0) {
      out[0] = (int)k;
      return 1;
  }
  return 0;
  ```
- **Cost:** One shift and one bitwise AND, or one comparison `k < 40`, which is equivalent for this prefix. Instruction selection was not measured.

#### Comparator 4: Packed SWAR Adapter (M1 Reconstructed)
- **Data Structure:** 8-bit packed unsigned integer; 2 bits per trit.
- **Execution:** Loop over 40 catalog entries, performing 8-bit XOR and zero-check.
- **Cost:** 40 loop iterations, 40 XOR operations, 40 branch evaluations.

#### Comparator 5: Byte-Oriented Adapter (M1 Reconstructed)
- **Data Structure:** 4 unpacked bytes (`uint8_t[4]`).
- **Execution:** Loop over 40 catalog entries, comparing 4 byte positions sequentially.
- **Cost:** 40 outer loop iterations, up to 160 byte comparisons, multiple conditional branches.

### 4.2 Accounting Requirements for Fair Comparison

Any empirical comparison must strictly account for the full execution pipeline:
$$\text{Cost}_{\text{total}} = \text{Cost}_{\text{encode}} + \text{Cost}_{\text{validate}} + \text{Cost}_{\text{filter}} + \text{Cost}_{\text{decode}}$$

1. **Validation Costs:** In M1, SWAR rejects any 2-bit field equal to $11_2$ before matching. Omitting that check violates the fail-closed contract. Whether the check dominates filtering cost was not measured.
2. **Metadata & Memory Alignment:** Allocating small arrays or tuples introduces substantial allocation and heap/stack alignment overhead.
3. **Cold vs. Warm State:** Table lookup baselines must account for cache footprint. An 81-byte table spans two 64-byte lines, and a 256-byte table spans four. Both are small relative to a typical L1 data cache. That residency was not measured here.

---

## 5. AI-Guided Optimization Research Boundaries

The stated long-term motivation for T81-AIOPT is exploring whether AI-guided systems can select optimal representations or execution strategies. To prevent pseudoscientific claims, we define rigorous boundaries separating trivial search from learned selection.

### 5.1 The Hierarchy of Optimization Autonomy

```
Level 0: Hardcoded / Static Assignment
   └─ Always use SWAR (or always use LUT), determined at compile time.

Level 1: Rule-Based / Threshold Heuristic
   └─ if (catalog_size < 16) use Unrolled_Byte; else use LUT;
      Determined by manual profiling and static rules.

Level 2: Exhaustive / Offline Search (Auto-Tuning)
   └─ Profile all N implementations across workload grid W; select argmin.
      (Standard ATLAS / FFTW / TVM approach).

Level 3: Learned Configuration Selection (Meta-Learning / Contextual Bandits)
   └─ Model M: Features(Workload, Hardware) -> Implementation.
      Must predict optimal kernel from workload features without exhaustive profiling.

Level 4: Genuine Dynamic Adaptive Representation
   └─ Online runtime monitors state transitions and dynamically migrates in-memory
      vector representations based on amortized cost modeling.
```

### 5.2 Criteria for Demonstrating Legitimate Learned Selection

To claim that an AI-guided selector provides scientific value, an experiment must satisfy four mandatory criteria:

1. **Non-Trivial Search Space:**
   Selecting among 3 fixed implementations on a problem with 81 states is not an AI problem; it is a 3-way exhaustive test that can be resolved in microseconds. A legitimate search space must have high dimensionality (e.g., vector lengths $L \in [4, 1024]$, catalog sizes $M \in [10, 10^5]$, wildcard densities $W \in [0, 0.8]$, batch sizes $B \in [1, 10^4]$, and memory topologies).

2. **Amortized Selection Overhead:**
   Let $T_{\text{infer}}(M)$ be the time required for the AI model to predict the optimal representation, and let $T_{\text{exec}}(I, W)$ be the execution time of implementation $I$ on workload $W$. The AI selector is beneficial if and only if:
   $$T_{\text{infer}}(M) + T_{\text{exec}}(I_{\text{AI}}, W) < \min_{I \in \text{Baselines}} T_{\text{exec}}(I, W)$$
   For a micro-workload executing in nanoseconds (such as 4-trit filtering), any neural network, decision tree, or Python runtime invocation will incur microseconds of inference latency—rendering the system millions of times slower than simply running the default baseline.

3. **Generalization to Unseen Workloads:**
   The learned selector must be evaluated on unseen test distributions (different catalog structures, densities, and hardware architectures). If the model only reproduces the optimum on its training set, it is merely an over-parameterized lookup table.

4. **Advantage Over Simple Heuristics:**
   The AI selector must be benchmarked against a 1-line heuristic (e.g., `if size > threshold: use_lut else: use_swar`). If the learned selector does not achieve statistically significant speedup over the simple heuristic after accounting for model size and inference overhead, the AI component is unjustified.

---

## 6. Non-Binding Alternative for Milestone M2 (Specification Only)

*This section is a non-binding alternative. It is not implemented, executed, or authorized. The merged assessment `docs/research/aiopt/T81-AIOPT-001-M2-ADVERSARIAL-DESIGN-ASSESSMENT.md` governs.*

Maintainer decision, 2026-11-23: that assessment governs the M2 question. It argues that wildcard linear filtering cannot establish a ternary-specific computational advantage and leaves M2 execution blocked. The grid below remains this review's 2026-11-01 proposal, kept so the alternative is readable. It does not authorize a run.

### 6.1 Primary Research Question
*Under what combination of vector length ($L$), catalog size ($M$), and wildcard/don't-care density ($D$) does a packed ternary representation (SWAR or SIMD) exhibit measurable execution latency advantage over conventional binary scalar and direct lookup-table comparators on general-purpose binary CPUs?*

### 6.2 Formal Hypotheses
- **Null Hypothesis ($H_0$):**
  For all workload configurations $(L, M, D) \in \mathcal{W}$, the minimum execution latency of packed ternary adapters is greater than or equal to the latency of the best conventional binary baseline:
  $$\min_{T \in \{\text{Byte}, \text{SWAR}\}} \text{Latency}(T) \ge \min_{B \in \{\text{Scalar}, \text{LUT}, \text{Bitset}\}} \text{Latency}(B)$$
- **Alternative Hypothesis ($H_1$):**
  There exists a non-empty subset of workloads $\mathcal{W}^* \subset \mathcal{W}$ with vector length $L \ge 16$ and wildcard density $D > 0$ where packed ternary SIMD filtering achieves at least a 15% reduction in execution latency ($p < 0.01$) over the fastest conventional binary baseline.

### 6.3 Workload Families & Parameter Grid
To eliminate the degeneracy of M1, M2 must evaluate a parameterized workload space $\mathcal{W}$:
1. **Vector Length ($L$):** $L \in \{4, 16, 64, 256\}$ trits.
   - $L = 4$ ($3^4 = 81$ states): Tests the small-domain boundary.
   - $L = 16$ ($3^{16} \approx 4.3 \times 10^7$ states): Domain exceeds direct LUT capacity; requires algorithmic filtering.
   - $L = 64, 256$: Longer vectors. These lengths are not evidence that the workload matches a deployed network.
2. **Catalog Size ($M$):** $M \in \{10, 100, 1000, 10000\}$ patterns.
3. **Wildcard Density ($D$):** $D \in \{0.0, 0.1, 0.25, 0.5\}$.
   - Patterns include ternary wildcards $*$ (matches any trit $\in \{-1, 0, +1\}$), so a single input can match more than one pattern ($|M(x)|$ may exceed 1).
4. **Pattern Generation:** Uniform random sampling over $\mathbb{T}^L$ with independent wildcard insertion, completely replacing the biased lexicographical prefix $C_{40}$.

### 6.4 Comparator Implementations
1. **Scalar Ternary:** Unrolled loops operating on `int8_t` values $\in \{-1, 0, +1\}$.
2. **Byte-Packed Binary:** Standard byte arrays with explicit wildcard masks.
3. **Packed SWAR 2-Bit:** 2 bits per trit packed into `uint64_t` words; parallel wildcard masking using bitwise logic:
   $$\text{match} = ((\text{input} \oplus \text{pattern}) \ \& \ \sim\text{wildcard\_mask}) == 0$$
4. **Direct LUT / Index Table:** (For $L \le 8$ only).
5. **Bit-Parallel SIMD (AVX2 / AVX-512):** Vectorized bit-manipulation baseline processing 128 or 256 trits per instruction.

### 6.5 Fairness Constraints
- **Identical Input & Semantics:** All comparators must process the exact same random seed workloads and return identical match indices.
- **End-to-End Accounting:** Timers must include conversion/encoding from raw input into the comparator's internal format, unless input is pre-compiled (in which case pre-compilation time must be reported separately).
- **Compilation Uniformity:** All C++ implementations compiled with identical flags (`-O3 -march=native`) under GCC and Clang.
- **Fail-Closed Verification:** All implementations must validate input payloads or operate within provably safe bounds.

### 6.6 Measurement Environment & Statistical Protocol
- **Environment:** Dedicated bare-metal x86_64 and ARM64 instances; CPU frequency pinning, turbo boost disabled, isolated CPU cores (`isolcpus`).
- **Warmup & Iterations:** $10^4$ warmup runs, followed by $10^5$ measured evaluations per configuration.
- **Metric:** Execution latency per batch (nanoseconds), reported as median and 99th percentile (tail latency). Secondary metrics: CPU cycles via PMU (`perf_event_open`), L1 data cache misses, and instructions per cycle (IPC).
- **Statistical Significance:** Mann-Whitney $U$ test with Bonferroni correction for multiple hypothesis testing; significance threshold $\alpha = 0.01$. Confidence intervals computed via 10,000 bootstrap resamples.

### 6.7 Stopping, Negative Evidence, and Inconclusive Outcomes
- **Early stopping (proposal only):** A sustained conventional-baseline advantage at $L = 16$ and $L = 64$ may prompt a documented resource review. It must not automatically close untested larger configurations. Record stopped configurations as not tested.
- **Positive evidence:** A predeclared workload shows a practically meaningful advantage against the strongest fair conventional comparator, with appropriate confidence bounds and multiplicity-adjusted inference.
- **Bounded negative evidence:** For configurations that were actually tested, an adequately powered confidence interval excludes the preregistered practically meaningful improvement threshold. Report uncertainty, sample size, conditions, and scope.
- **Inconclusive:** When uncertainty spans both negligible and meaningful improvement, or power is insufficient, record the result inconclusive. Failure to reject a null hypothesis is not proof of equivalence or of no advantage.
- **Preregistration required:** Effect-size thresholds, the confidence procedure, power, the workload grid, and stopping rules need independent review before any M2 execution. The 15% and $p < 0.01$ figures in Section 6.2 are illustrative proposal text, not an accepted analysis plan.

---

## 7. Threats to Validity

### 7.1 Internal Validity
- **Loop Overhead Dominance:** M1 recorded no timings. A later timing of 40 pattern iterations could be dominated by branch prediction, call overhead, and harness mechanics rather than representation differences. That dominance was not measured.
- **Compiler Optimizations:** A modern compiler (GCC/Clang `-O3`) may auto-vectorize the byte comparison loop or unroll it entirely, while the SWAR bitwise shift loop may encounter register pressure or suboptimal code generation, confounding representation efficiency with compiler heuristics.
- **Validation Asymmetry:** If the SWAR adapter checks 2-bit codes while the scalar oracle assumes validated input, the adapters are not doing the same work. Omitting the SWAR check accepts payloads the M1 contract rejects. Neither effect was timed.

### 7.2 External Validity
- **Non-Generalizability of 4-Trit Domain:** Findings on $L = 4$ ($3^4 = 81$) do not by themselves predict behavior at the lengths discussed for ternary neural networks or content-addressable memory. No such workload was tested.
- **Absence of Hardware Ternary Storage:** Emulating ternary logic on binary hardware inevitably incurs bit-manipulation overheads that would not exist on hypothetical ternary silicon (e.g., memristor crossbars). Results obtained on x86_64 CPUs cannot be generalized to claim hardware infeasibility.

### 7.3 Construct Validity
- **Mislabeling Linear Search as "Filtering":** In computer science, pattern filtering on static catalogs is implemented via tries, Aho-Corasick automata, hash tables, or inverted indices. Evaluating linear scans over 40 elements conflates "representation quality" with "naive algorithm selection."
- **Equating Packaging with Adaptation:** Byte and SWAR encodings in M1 are fixed data layouts. The M1 protocol forbids adaptive selection. Reading M1 as an adaptive-representation experiment misreads that protocol.

### 7.4 Conclusion Validity
- **Microbenchmarking Noise:** On modern superscalar CPUs, measuring operations that complete in $<100 \text{ ns}$ is highly vulnerable to cacheline alignment, branch target buffers (BTB), and thermal throttling. Without cycle-level PMU counters and kernel isolation, conclusions drawn from wall-clock timers are statistically invalid.

---

## 8. Final Recommendation & Answers to Guiding Questions

### 8.1 Answers to Mandatory Review Questions

1. **Is T81-AIOPT-001 a meaningful experiment?**
   **No, not in its current form.** While Milestone M1 is technically competent as an isolated exercise in multi-representation functional testing, the underlying problem (filtering 4-trit inputs against 40 exact patterns) is mathematically degenerate. It reduces to testing whether an 81-state index is $<40$.

2. **What can M1 actually establish?**
   M1 can support one bounded claim if its harness checks pass: 4-trit vectors round-trip through 4-byte arrays and 8-bit integers, and the three hand-coded filters agree on all 81 inputs. This review did not execute that harness. The verification report at the pinned SHA records local checks and withholds independent-person acceptance.

3. **What cannot be inferred from M1?**
   M1 cannot infer:
   - Computational speedup or performance advantage of ternary representations.
   - Efficiency on physical hardware.
   - Memory bandwidth or cache benefits.
   - Viability of AI-guided optimization or representation selection.
   - Generalization to vectors of length $L > 4$.

4. **Is the proposed workload suitable for an optimization study?**
   **No.** The workload space ($3^4 = 81$ states) is small enough that a correct solution is an 81-byte lookup table or a scalar comparison of the lexicographic index with 40. No cycle count was measured. Optimizing a 40-iteration loop over this domain studies an algorithm the index comparison replaces.

5. **What is the smallest scientifically useful next experiment?**
   Do not benchmark M1. Section 6's wildcard comparison ($L \in \{16, 64\}$, $M = 100$, $D = 0.2$) is a non-binding alternative. The governing record is the merged M2 assessment, which rejects wildcard linear filtering as a way to establish a ternary-specific advantage and leaves execution blocked.

6. **Should M2 proceed, be redesigned, or be abandoned?**
   **M2 must be REDESIGNED (`M2_REDESIGN_REQUIRED`).**
   Proceeding from M1 to a timing run or an AI selector on $C_{40}$ would time the degenerate index test and would reopen the frozen CPU-kernel program recorded in PR #549 without a new workload. The merged M2 assessment governs the redesign question and leaves execution blocked. Section 6 is a non-binding alternative, not the authorized next protocol.

---

## 9. Deliverable Summary Ledger

| Review Dimension | Determination / Finding |
| :--- | :--- |
| **Review Target** | Unmerged PR #665 head `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db` (`docs/research/aiopt/T81-AIOPT-001-M1-PROTOCOL.md`, `experiments/aiopt_001_fixed_catalog/m1.py`; not on `main`) |
| **Scientific Disposition** | **`M2_REDESIGN_REQUIRED`** |
| **Domain Cardinality** | $3^4 = 81$ states ($H \approx 6.33985 \text{ bits}$); fits in 7 bits; SWAR uses 8 bits (175/256 = 68.36% unused). |
| **Catalog Structure ($C_{40}$)** | Severely skewed prefix: Lane 0 contains 0 positive trits; all patterns are mutually exclusive singletons. |
| **Algorithmic Collapse** | Problem is isomorphic to $\text{index}(x) < 40$; solvable in $O(1)$ scalar time vs $O(M)$ loop. |
| **SWAR Nature** | 8-bit integer comparison (`payload == target`); no SIMD sub-lane parallel arithmetic. |
| **Key Comparator** | Conventional binary scalar range check (`k < 40`) and 81-entry Direct LUT (`lut[k]`). |
| **Most Serious Threat** | Conflating naive $O(M)$ linear scan loop overhead with representation efficiency. |
| **Actionable Next Step** | Do not benchmark M1. The merged M2 assessment governs. Section 6 is a non-binding wildcard alternative. Execution remains blocked. |
