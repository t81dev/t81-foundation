# Independent Scientific & Mathematical Review: T81-AIOPT-001 (Fixed-Catalog Ternary Pattern Filtering)

**Document ID:** `docs/research/aiopt/T81-AIOPT-001-SCIENTIFIC-REVIEW.md`
**Date:** November 2026
**Status:** AI-assisted mathematical source assessment; independent experimental replication not established
**Review Target:** T81-AIOPT-001 Milestone M1 (PR #665; branch `research/aiopt-001-m1-protocol`)
**Investigator:** Jules (AI-assisted scientific analyst; not an authenticated independent human reviewer)
**Scientific Disposition:** `M2_REDESIGN_REQUIRED`

**Review provenance:** The initial Jules assessment did not pin an inspected M1 SHA, preserve an execution log, or establish that Jules executed M1. Independent experimental reproduction and independent-person peer review are **not claimed**. The exact inspected M1 revision is unverified from the initial review record. Subsequent Codex verification at `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db` reports local tests, explicitly without independent-person scientific acceptance; this is later context, not execution by Jules. The mathematical critique addresses the stated C40 semantics and does not depend on executing M1.

---

## 1. Executive Assessment

Milestone M1 of T81-AIOPT-001 proposes an isolated, correctness-only verification framework for evaluating ternary pattern filtering across three software representations: an independent scalar integer oracle, a byte-oriented unpacked adapter, and an 8-bit SIMD-Within-A-Register (SWAR) packed 2-bit adapter. The operational domain is bounded to 4-trit vectors $X = \{-1, 0, +1\}^4$ ($|X| = 81$) filtered against a 40-pattern catalog ($C_{40}$) formed by the first 40 lexicographically ordered vectors.

### Core Review Findings

1. **Exact Representation Equivalence Only (Correctness Is Not Optimization):**
   M1 establishes that three distinct binary memory representations of ternary vectors can compute identical predicate outputs across the finite domain of 81 inputs. It does not—and mathematically cannot—demonstrate computational speedup, representation superiority, hardware efficiency, or AI-guided optimization.

2. **Severe Degeneracy of the Proposed Workload:**
   Because all patterns in $C_{40}$ are fully specified without wildcards, the 40 patterns are mutually exclusive singletons. Consequently, for any valid input $x \in X$, the output set of matching indices has cardinality $|M(x)| \le 1$. Specifically, $|M(x)| = 1$ for exactly 40 inputs and $|M(x)| = 0$ for 41 inputs. Furthermore, because $C_{40}$ is defined as the prefix of the lexicographical enumeration of $X$, the match function collapses mathematically to:
   $$\text{filter}(x, C_{40}) = \begin{cases} [\text{lex\_index}(x)] & \text{if } \text{lex\_index}(x) < 40 \\ [] & \text{otherwise} \end{cases}$$
   Evaluating this problem via an $O(|C|)$ linear scan over 40 patterns in a loop (whether scalar, byte, or SWAR) evaluates an algorithm that is asymptotically and practically obsolete compared to a 1-cycle $O(1)$ scalar integer comparison or direct array lookup.

3. **Absence of SWAR Vectorization Advantage:**
   In M1, the SWAR adapter serializes pattern matching across 40 iterations. The SWAR predicate `((payload ^ target) & 0xFF) == 0` evaluates an exact 8-bit integer equality `payload == target`. The SWAR encoding performs zero parallel lane filtering; it merely functions as an 8-bit integer hash code.

4. **Severe Selection Bias in $C_{40}$:**
   Truncating the lexicographical enumeration at 40 patterns introduces extreme structural asymmetry. In $C_{40}$, position 0 contains 27 $-1$ trits, 13 $0$ trits, and exactly 0 $+1$ trits. The $+1$ state is completely absent from the leading lane, invalidating any claim that $C_{40}$ represents balanced ternary behavior.

5. **AI-Optimization Vacuum:**
   PR #665 contains zero AI optimization, zero heuristic search, and zero adaptive selection logic. Attempting to deploy "AI-guided optimization" on a 3-way choice between trivial representations for an 81-state domain represents an extreme mismatch of problem complexity to optimization machinery.

### Disposition

**`M2_REDESIGN_REQUIRED`**: Milestone M1 specifies fail-closed correctness and mutual consistency among its three adapters; those properties were not independently reproduced by this scientific review. However, the protocol cannot proceed to M2 as currently conceived. Measuring execution time or training a selection agent on this workload would benchmark loop overhead and compiler register allocation rather than representation efficiency. Milestone M2 requires complete redesign around meaningful vector lengths, wildcard/don't-care predicates, conventional lookup-table/bitset comparators, and a realistic search space.

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

The catalog $C_{40} = (p_0, p_1, \dots, p_{39})$ is defined as the first 40 elements of $X$ ordered lexicographically under $-1 < 0 < +1$.

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
| **Information Bound** | Determining membership in a 40-element subset of an 81-element domain requires $\log_2(81) \approx 6.34$ bits of information. | Byte adapter allocates 32 bits; SWAR allocates 8 bits. |
| **Decision Complexity** | Testing membership in a static 40-element set requires exactly 1 set membership test or binary search ($\lceil \log_2 40 \rceil = 6$ comparisons, or $O(1)$ table lookup). | M1 evaluates 40 linear iterations containing per-trit comparisons or 40 sequential integer XOR comparisons. |
| **Validation Cost** | Zero (if input is trusted or guaranteed by construction). | SWAR requires checking all four 2-bit lanes for code $11_2$ ($3$), requiring bit masking and range checks that exceed the cost of the comparison itself. |
| **Loop Overhead** | Absent in optimal algorithms. | Dominates M1 execution time (loop counter increments, array indexing, conditional branches across 40 iterations). |

---

## 3. Relationship to the H01–H08 Hypothesis Program

The T81 Foundation maintains a formal hypothesis register (`docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`) and research charter. The proposed T81-AIOPT-001 experiment interacts directly with several active and closed hypotheses:

### 3.1 Interaction with H03 (Base-81 Interfacing & Grouping)
- **Claim:** Grouping four trits as base-81 can provide a workload-specific interface or execution benefit beyond trivial 4-trit concatenation.
- **M1 Relationship:** The input domain $X = \mathbb{T}^4$ has exactly 81 states. Base-81 represents each 4-trit vector as a single digit $d \in [0, 80]$.
- **Finding:** If base-81 representation were used, the pattern filter for $C_{40}$ would be a single scalar range check: `return (d < 40) ? [d] : []`. M1 avoids base-81 integer conversion and instead uses SWAR 2-bit packing (which spans 256 states). This demonstrates that grouping 4 trits into base-81 trivializes the filtering task, whereas 2-bit SWAR packing creates an artificially sparse 8-bit domain.

### 3.2 Interaction with H04 (Adaptive Representation) & H04-R02 Findings
- **Claim:** An explicitly defined adaptive representation policy can outperform fixed representations when evaluated against equally optimized baselines.
- **Historical Evidence:** Corrected H04-R02 (`experiments/h04_r02_adaptive_representation/RESULTS.md`) invalidates the original inference and retains conditional symbolic-model ternary advantages. For the matched 10-bit control, the AT window narrows to `r < 5/3`, including `r = 1.60`; modeled winners depend on metadata, mode boundaries, transition and conversion assumptions. The historical 96% figure applies to one ratio-1 trace, not general observed variance. Actual encoded byte lengths were measured, but no new CPU performance, bandwidth, area or energy advantage or loss was established. H04 remains unresolved.
- **M1 Relationship:** T81-AIOPT-001 purports to study representation selection (byte vs. SWAR). However, M1 does not adapt dynamically. Encoding is fixed a priori. If an adaptive switching policy were introduced, the switching overhead (encoding, validation, branch dispatch) would dwarf the computation cost by orders of magnitude for a 4-trit vector.

### 3.3 Prior CPU Ternary Arithmetic Closure (PRs #544–#549)
- **Established Finding:** In PR #549 (`docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`), CPU-based ternary kernel optimization was formally **frozen**. That comprehensive investigation proved that software-emulated ternary bitwise operations (scalar, SWAR, LUT, AVX2 VPSHUFB) cannot match native binary SIMD instructions on general-purpose CPUs.
- **M1 Hazard:** T81-AIOPT-001 risks reopening this frozen line of inquiry under the guise of "AI optimization" without introducing any new architectural mechanism to circumvent the fundamental limits established in PR #549.

### 3.4 Interaction with H08 (AI-Assisted Research Trustworthiness)
- **Claim:** AI-assisted exploration improves research velocity and trustworthiness under verifiable human-auditable constraints.
- **M1 Relationship:** AIOPT-001 is marketed as "AI optimization." If AIOPT merely uses machine learning to select between three hand-coded loops on an 81-state problem where an $O(1)$ direct lookup table exists, it violates H08 by introducing artificial complexity and unprovable machine learning components where elementary discrete mathematics provides the closed-form optimal solution.

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
- **Cost:** 1 memory load (L1 cache resident), 1 comparison. Completely eliminates the 40-iteration loop.

#### Comparator 3: Bitset / 64-Bit Word Mask
- **Data Structure:** Since $|C_{40}| = 40 \le 64$, the catalog can be encoded as a 64-bit bitmask or two 64-bit integers representing valid inputs in the 81-state domain.
- **Execution:**
  ```c
  // For domain index k in [0, 80]:
  // Mask indicates whether index k is present in the catalog
  uint64_t catalog_mask = 0x000000FFFFFFFFFFULL; // bits 0..39 set
  if (k < 40) { // or (catalog_mask & (1ULL << k))
      out[0] = k;
      return 1;
  }
  return 0;
  ```
- **Cost:** 1 bit-test instruction (`BT` on x86, or shift + AND).

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

1. **Validation Costs:** In M1, SWAR requires verifying that no 2-bit field contains $11_2$. In Python or C++, this validation requires multiple shift and bitwise operations. If the validation is omitted during benchmarking, the adapter violates memory safety. If included, validation costs dominate the filtering cost.
2. **Metadata & Memory Alignment:** Allocating small arrays or tuples introduces substantial allocation and heap/stack alignment overhead.
3. **Cold vs. Warm State:** Table lookup (LUT) baselines must account for cache footprint (81 bytes vs. 256 bytes), although both fit comfortably within a single 64-byte or two 64-byte L1 cache lines.

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

## 6. Preregistered Design for Milestone M2 (Specification Only)

*In strict accordance with repository restrictions, Milestone M2 is specified here as a preregistered experimental protocol. It is not implemented or executed.*

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
   - $L = 64, 256$: Representative of embedding quantization and neural network weight lanes.
2. **Catalog Size ($M$):** $M \in \{10, 100, 1000, 10000\}$ patterns.
3. **Wildcard Density ($D$):** $D \in \{0.0, 0.1, 0.25, 0.5\}$.
   - Patterns include ternary wildcards $*$ (matches any trit $\in \{-1, 0, +1\}$), creating non-trivial subset matching where $|M(x)| \ge 0$.
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

### 6.7 Stopping, Negative Evidence and Inconclusive Outcomes
- **Early stopping (proposal only):** A sustained conventional-baseline advantage at L=16 and L=64 may prompt a documented resource review; it must not automatically close untested larger configurations. Record stopped configurations as NOT TESTED.
- **Positive evidence:** A predeclared workload shows a practically meaningful advantage against the strongest fair conventional comparator, with appropriate confidence bounds and multiplicity-adjusted inference.
- **Bounded negative evidence:** For actually tested configurations, an adequately powered confidence interval excludes the preregistered practically meaningful improvement threshold. Report uncertainty, sample size, conditions, and scope.
- **Inconclusive:** When uncertainty spans both negligible and meaningful improvement, or power is insufficient, record INCONCLUSIVE. Failure to reject a null hypothesis is not proof of equivalence or absence of advantage.
- **Preregistration required:** Effect-size thresholds, confidence procedure, power, workload grid, and stopping rules must be independently reviewed before any M2 execution.

---

## 7. Threats to Validity

### 7.1 Internal Validity
- **Loop Overhead Dominance:** In M1, evaluating 40 patterns takes only tens of nanoseconds. Measured time will be dominated by loop branch prediction, call overhead, and test framework harness mechanics rather than representation differences.
- **Compiler Optimizations:** A modern compiler (GCC/Clang `-O3`) may auto-vectorize the byte comparison loop or unroll it entirely, while the SWAR bitwise shift loop may encounter register pressure or suboptimal code generation, confounding representation efficiency with compiler heuristics.
- **Validation Asymmetry:** If the SWAR adapter performs 2-bit range checks while the scalar oracle assumes validated input, SWAR is penalized for safety. Conversely, if SWAR omits validation, it receives an unfair speed advantage while introducing undefined behavior.

### 7.2 External Validity
- **Non-Generalizability of 4-Trit Domain:** Findings on $L = 4$ ($3^4 = 81$) have zero predictive power for real-world ternary applications (e.g., ternary neural networks where $L \in [10^3, 10^7]$ or ternary content-addressable memory where $L \ge 64$).
- **Absence of Hardware Ternary Storage:** Emulating ternary logic on binary hardware inevitably incurs bit-manipulation overheads that would not exist on hypothetical ternary silicon (e.g., memristor crossbars). Results obtained on x86_64 CPUs cannot be generalized to claim hardware infeasibility.

### 7.3 Construct Validity
- **Mislabeling Linear Search as "Filtering":** In computer science, pattern filtering on static catalogs is implemented via tries, Aho-Corasick automata, hash tables, or inverted indices. Evaluating linear scans over 40 elements conflates "representation quality" with "naive algorithm selection."
- **Equating Packaging with Adaptation:** Switching between byte and SWAR encodings is a static data layout choice, not "adaptive representation." Calling this study an adaptive representation experiment misrepresents the construct.

### 7.4 Conclusion Validity
- **Microbenchmarking Noise:** On modern superscalar CPUs, measuring operations that complete in $<100 \text{ ns}$ is highly vulnerable to cacheline alignment, branch target buffers (BTB), and thermal throttling. Without cycle-level PMU counters and kernel isolation, conclusions drawn from wall-clock timers are statistically invalid.

---

## 8. Final Recommendation & Answers to Guiding Questions

### 8.1 Answers to Mandatory Review Questions

1. **Is T81-AIOPT-001 a meaningful experiment?**
   **No, not in its current form.** While Milestone M1 is technically competent as an isolated exercise in multi-representation functional testing, the underlying problem (filtering 4-trit inputs against 40 exact patterns) is mathematically degenerate. It reduces to testing whether an 81-state index is $<40$.

2. **What can M1 actually establish?**
   M1 can establish exactly one fact: that 4-trit vectors can be reversibly mapped into 4-byte arrays and 8-bit integers, and that exhaustive evaluations across all 81 inputs produce identical predicate outputs across the three hand-coded implementations. It proves functional correctness of equivalent binary encodings.

3. **What cannot be inferred from M1?**
   M1 cannot infer:
   - Computational speedup or performance advantage of ternary representations.
   - Efficiency on physical hardware.
   - Memory bandwidth or cache benefits.
   - Viability of AI-guided optimization or representation selection.
   - Generalization to vectors of length $L > 4$.

4. **Is the proposed workload suitable for an optimization study?**
   **No.** The workload space ($3^4 = 81$ states) is so small that the optimal solution is an 81-byte lookup table or a 1-cycle integer comparison. Optimizing a 40-iteration loop over this domain is optimizing an algorithm that should not exist in production code.

5. **What is the smallest scientifically useful next experiment?**
   The smallest useful experiment is **not** to benchmark M1, but to implement a **Parameterized Wildcard Matching Experiment** with vector lengths $L \in \{16, 64\}$, catalog size $M = 100$, and wildcard density $D = 0.2$, comparing packed 2-bit SWAR against a 64-bit binary bitmask baseline. This introduces non-trivial filtering while remaining small enough for rigorous manual and statistical control.

6. **Should M2 proceed, be redesigned, or be abandoned?**
   **M2 must be REDESIGNED (`M2_REDESIGN_REQUIRED`).**
   Proceeding directly from M1 to an M2 benchmarking run or AI-selector milestone on the current 4-trit catalog $C_{40}$ would generate misleading results, waste research capacity, and directly conflict with prior findings in PR #549. M2 should only proceed after adopting the parameterized protocol specified in Section 6.

---

## 9. Deliverable Summary Ledger

| Review Dimension | Determination / Finding |
| :--- | :--- |
| **Review Target** | PR #665 (`T81-AIOPT-001-M1-PROTOCOL.md`, `m1.py`) |
| **Scientific Disposition** | **`M2_REDESIGN_REQUIRED`** |
| **Domain Cardinality** | $3^4 = 81$ states ($H \approx 6.34 \text{ bits}$); fits in 7 bits; SWAR uses 8 bits (68.4% unused). |
| **Catalog Structure ($C_{40}$)** | Severely skewed prefix: Lane 0 contains 0 positive trits; all patterns are mutually exclusive singletons. |
| **Algorithmic Collapse** | Problem is isomorphic to $\text{index}(x) < 40$; solvable in $O(1)$ scalar time vs $O(M)$ loop. |
| **SWAR Nature** | 8-bit integer comparison (`payload == target`); no SIMD sub-lane parallel arithmetic. |
| **Key Comparator** | Conventional binary scalar range check (`k < 40`) and 81-entry Direct LUT (`lut[k]`). |
| **Most Serious Threat** | Conflating naive $O(M)$ linear scan loop overhead with representation efficiency. |
| **Actionable Next Step** | Do not benchmark M1. Preregister parameterized M2 with wildcards and $L \ge 16$. |
