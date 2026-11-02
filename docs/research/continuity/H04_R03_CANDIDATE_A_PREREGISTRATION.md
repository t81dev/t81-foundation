# EXP-H04-R03 Candidate A Preregistration Design
## Fixed-Codec Space/Access Tradeoff & Bounded Microblock Discriminator

**Status:** DESIGN ONLY / PREREGISTRATION PROPOSAL / PHASE C EXECUTION FROZEN
**Date:** 2026-11-02
**Parent Study:** EXP-H04-R03 Matched Codec Space-Time Tradeoff & Bounded-Access Discriminator
**Reference Document:** `experiments/h04_r03_matched_codec/SPECIFICATION.md` (Git tree `6c3d9a10`, PR #658)
**Parent Phase B Status:** ACCEPTED — DETERMINISTIC CORRECTNESS VERIFIED (PR #658, commit `0a48054510e79e753d5152ccb3f9f130c17bbafd`)
**Authority & Scope:** STRICTLY DESIGN ONLY. No implementation, benchmark harness creation, test execution, timing measurements, or Phase C activation are authorized by this document.

---

## 1. Preamble, Research Context & Scientific Boundaries

### 1.1 Source Anchors and Revisions
This preregistration design grounds strictly upon the following immutable repository source revisions:
- Remote `main` HEAD: Commit `1a6d5a36473004961248f0a85b2e06361ea2a38f`.
- Phase B Accepted Implementation Head: PR #658, Commit `0a48054510e79e753d5152ccb3f9f130c17bbafd`.
- Upstream Review Baseline: Commit `6d20d381322e4859e33524f4fc18d4c6569539ce` (Adversarial Specification Review).
- Pinned Phase B Assets:
  - `experiments/h04_r03_matched_codec/SPECIFICATION.md` (SHA-256: `b8c514f76d6834aecc61e0caee312f807bf57f3da55862ab2c8d4c33b7fd42ba`)
  - `experiments/h04_r03_matched_codec/codec.py`
  - `experiments/h04_r03_matched_codec/oracle.py`
  - `experiments/h04_r03_matched_codec/independent_witness.py`
  - `experiments/h04_r03_matched_codec/VALIDATION_REPORT.md`

### 1.2 Research Question & Scientific Contrast
Does Base-243 packing (T243) offer a practical complete-lifecycle performance advantage over binary encodings of identical or superior compression compactness when evaluated under a fixed-size, bounded-block access contract?

**Essential Scientific Contrast:**
- The primary evaluation must test T243 against **both** equally compact binary controls:
  1. **Whole-block B729:** Demonstrates maximum compaction (77 payload bytes at $k=64$) but incurs a multi-precision block decoding penalty.
  2. **Proposed B729-µ5 Microblock:** Demonstrates identical compaction (exactly 6 bytes per 5 values; 77 payload bytes at $k=64$) while providing bounded 6-byte random-access extraction.
- **Competence Rule:** Showing an advantage against B10 alone (a fixed-field baseline with 80 bytes) or against whole-block B729 alone (whose full multi-precision block decode is computationally uncompetitive) is **insufficient**. Base-243 cannot be declared practically advantageous unless it outperforms the competent, bounded-access, equally compact binary microblock control (B729-µ5) across the complete lifecycle.

### 1.3 Preservation of Scientific Integrity and Historical Findings
1. **Reaffirmation of Prior Dispositions:**
   - H04-R02 original methodology remains **Classification D (Invalid Experimental Methodology)** due to historical metadata scaling defects, division math errors, and comparator capacity asymmetry.
   - H04-R02 corrected findings ($AT = 1210 < AB = 1250$ at $r = 1.60$, crossover $5/3$ under 10-bit Tier-0 control) remain strictly a **conditional discrete-model result**, not a hardware execution claim.
   - Scientific Investigation 10A disposition remains **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE**.
   - Ternary CPU arithmetic research closure (PRs #544–#549) remains active: commodity CPU arithmetic slowdowns for ternary ALUs are established; this study does not evaluate general-purpose ternary computation or arithmetic execution.
2. **Exclusion of Hardware Superiority Claims:**
   - Any observed storage density or access efficiency in T243 is a **mathematical and software codec property** of radix representation, not evidence of native ternary silicon or hardware superiority.
   - Constructive identity: B243-CLONE proves that a conventional binary machine executes the identical byte language bit-for-bit.
   - Analytical size bound: Under the accepted framing, the analytical theorem $L_{\text{B729}}(m) \le L_{\text{T243}}(m)$ holds unconditionally for all block lengths $m \ge 1$. Fixed-radix block integer coding is never larger than Base-243.
3. **No DRAM-Bound Presumption:**
   - The design explicitly rejects any unevidenced assumption that random-access lookup under memory storage is DRAM-bandwidth-bound or cache-miss-dominated. Both computational ALU decode cost and memory hierarchy access costs are accounted without bias.

---

## 2. Accepted Phase B Contract and Reference Codecs

The study strictly adopts the Phase B deterministic wire and memory contract accepted on PR #658. Correctness acceptance of these reference implementations is fully settled and is not reopened:

### 2.1 Common Domain and Alphabet
- **Logical Element Domain:** Discrete signed integers $x \in [-364, 364]$, cardinality $|S| = 729$.
- **Wire Mapping:** Bijective unsigned offset $u = x + 364$, with $u \in [0, 728]$.
- **Injective Mapping Guarantees:** Every integer $u$ corresponds bijectively to a 6-digit base-3 expansion $u = \sum_{j=0}^5 d_j \cdot 3^j$ where $d_j \in \{0, 1, 2\}$, matching signed trits $t_j = d_j - 1 \in \{-1, 0, 1\}$.
- **Fail-Closed Domain Validation:** Any input containing non-integers, values $x < -364$ or $x > 364$ is rejected before encoding.

### 2.2 Canonical Framing and Layout (RA-FIXED)
For primary comparison, all arms adhere to the canonical RA-FIXED wire structure:
- **Header:** Exactly 8-byte little-endian unsigned 64-bit integer $N$ (`<Q`). If $N = 0$, the frame is exactly 8 bytes.
- **Index:** RA-FIXED contains **no offset table** ($H = 8$ bytes).
- **Data Offset:** $D = \text{align\_up}(H, A)$. For alignment $A = 1$, $D = 8$.
- **Block Layout:** The logical sequence is divided into $B = \lceil N / k \rceil$ blocks of nominal capacity $k$. Block $j \in [0, B-1]$ contains $m_j = \min(k, N - j \cdot k)$ values.
- **Block Offsets:** In RA-FIXED, block address arithmetic is purely analytical:
  $$\text{Offset}(j) = D + j \cdot \text{align\_up}(L_c(k), A)$$
  Each full preceding block has fixed byte stride $\text{Stride} = \text{align\_up}(L_c(k), A)$, calculated in $O(1)$ space and time without table allocations.

---

## 3. Four Matched Comparison Arms

The study establishes four explicit comparison arms. Three arms use accepted Phase B reference codecs, while the fourth arm introduces a proposed, equally compact binary microblock architecture:

| Arm ID | Implementation Name | Architectural Category | Exact Payload Bytes $L_c(m)$ | Acceptance Status | Role in Discriminator |
|---|---|---|---|---|---|
| **Arm 1** | **T243** | Base-243 Packing | $\lceil 6m / 5 \rceil$ | Accepted (Phase B) | Target ternary codec |
| **Arm 2** | **B10** | Packed 10-bit Binary | $\lceil 10m / 8 \rceil$ | Accepted (Phase B) | Baseline fixed-field binary control |
| **Arm 3** | **B729** | Whole-Block Enumerative Binary | $\lceil \text{bit\_length}(729^m - 1) / 8 \rceil$ | Accepted (Phase B) | Strongest compaction binary control |
| **Arm 4** | **B729-µ5** | 5-Value Microblock Binary | See Section 4 ($\lceil 6m / 5 \rceil$ for full µ-blocks) | **PROPOSED** | Bounded-access matched-density binary control |

### 3.1 Arm 1: T243 (Accepted Phase B Reference)
- Groups base-3 digits into chunks of 5 trits per byte ($3^5 = 243 \le 256$).
- Each 6-trit symbol requires 1.2 bytes on average. Full block of $k=64$ values requires $\lceil 384 / 5 \rceil = 77$ payload bytes.
- Bounded random access: Symbol $i$ starts at trit $6i$. Digit extraction spans at most 2 bytes ($b_{\text{start}} = \lfloor 6i/5 \rfloor$, $b_{\text{end}} = \lfloor (6i+5)/5 \rfloor$), touching at most 2 payload bytes per query.

### 3.2 Arm 2: B10 (Accepted Phase B Reference)
- Packs each $u \in [0, 728]$ into 10 bits ($2^9 < 729 \le 2^{10}$). Unused states $729..1023$ are strictly rejected upon decoding.
- Full block of $k=64$ values requires $\lceil 640 / 8 \rceil = 80$ payload bytes.
- Bounded random access: Symbol $i$ starts at bit $10i$. Bitfield extraction spans at most 3 payload bytes ($b_{\text{start}} = \lfloor 10i/8 \rfloor$, $b_{\text{end}} = \lfloor (10i+9)/8 \rfloor$), touching at most 3 payload bytes per query.

### 3.3 Arm 3: B729 Whole-Block (Accepted Phase B Reference)
- Evaluates the multi-precision block integer $I = \sum_{j=0}^{m-1} u_j \cdot 729^j$ using exact arbitrary-precision arithmetic.
- Full block of $k=64$ values requires $\lceil \text{bit\_length}(729^{64} - 1) / 8 \rceil = \lceil 609 / 8 \rceil = 77$ payload bytes.
- Compaction theorem: $L_{\text{B729}}(64) = 77$ bytes $\le L_{\text{T243}}(64) = 77$ bytes.
- Bounded random access: Because the entire block is encoded as a single multi-precision integer, extracting any individual element requires decoding the entire 64-value block (touching all 77 bytes and performing full multi-precision division).

### 3.4 Arm 4: B729-µ5 (Proposed Five-Value Microblock)
- Evaluates the hypothesis that binary integer enumerative coding over small, 5-value microblocks matches T243 byte-for-byte in payload density while eliminating the multi-precision decoding penalty of whole-block B729.

---

## 4. Proposed B729-µ5 Microblock Specification

**Status:** PROPOSED ARCHITECTURAL SPECIFICATION. Not previously accepted under Phase B. Requires independent implementation, validation, and witness verification before eligibility for measurement.

### 4.1 Mathematical Foundations & Density Equivalence
For any alphabet of 729 states, grouping into chunks of $M = 5$ values yields:
$$729^5 = (3^6)^5 = 3^{30} = 205,891,132,094,649$$
Evaluating binary bit capacity:
$$2^{47} = 140,737,488,355,328 < 3^{30} \le 281,474,976,710,656 = 2^{48}$$
$$\text{bit\_length}(729^5 - 1) = 48 \text{ bits} = \text{exactly } 6 \text{ bytes}$$

Crucially, in T243, 5 values contain $5 \times 6 = 30$ trits. Grouped 5 trits per byte, 30 trits require exactly $30 / 5 = 6$ bytes.
**Exact Equivalence:** For every group of 5 values, both T243 and B729-µ5 require **identically 6 bytes**.

### 4.2 Wire Grammar and Layout
Within a block of $m$ values ($m \le k$):
- The block is partitioned into $G = \lceil m / 5 \rceil$ microblocks indexed $g \in [0, G-1]$.
- Microblock $g$ contains $v_g = \min(5, m - 5g)$ values: $u_{5g}, \dots, u_{5g + v_g - 1}$.
- Microblocks $g \in [0, G-2]$ (and $g = G-1$ when $m$ is a multiple of 5) are full ($v_g = 5$). Each full microblock is encoded into exactly 6 bytes.
- The microblocks are laid out contiguously without inner padding:
  $$\text{Block Payload} = \mu_0 \mathbin{\Vert} \mu_1 \mathbin{\Vert} \dots \mathbin{\Vert} \mu_{G-1}$$

### 4.3 Microblock Integer Encoding Grammar
For a microblock with $v$ values ($v \in \{1, 2, 3, 4, 5\}$):
1. Compute the microblock integer:
   $$I_{\mu} = \sum_{p=0}^{v-1} u_{5g + p} \cdot 729^p$$
   Bound: $0 \le I_{\mu} < 729^v$.
2. Bit length requirement:
   $$b_{\mu}(v) = \text{bit\_length}(729^v - 1)$$
   Byte length requirement:
   $$L_{\mu}(v) = \left\lceil \frac{b_{\mu}(v)}{8} \right\rceil$$
3. Microblock length table:
   - $v = 1$: $729^1 - 1 = 728 \implies 10 \text{ bits} \implies 2 \text{ bytes}$
   - $v = 2$: $729^2 - 1 = 531,440 \implies 20 \text{ bits} \implies 3 \text{ bytes}$
   - $v = 3$: $729^3 - 1 = 387,420,488 \implies 29 \text{ bits} \implies 4 \text{ bytes}$
   - $v = 4$: $729^4 - 1 = 282,429,536,480 \implies 39 \text{ bits} \implies 5 \text{ bytes}$
   - $v = 5$: $729^5 - 1 = 205,891,132,094,648 \implies 48 \text{ bits} \implies 6 \text{ bytes}$
4. Integer serialization:
   $I_{\mu}$ is emitted as an unsigned, little-endian byte sequence of exactly $L_{\mu}(v)$ bytes:
   $$\text{byte}_k = \left( \lfloor I_{\mu} / 256^k \rfloor \right) \bmod 256, \quad k \in [0, L_{\mu}(v) - 1]$$
   For full microblocks ($v=5$), $L_{\mu}(5) = 6$ bytes, fitting directly into a standard 64-bit register (`uint64_t`).

### 4.4 Tail Handling for Arbitrary Block Lengths
For nominal block capacity $k = 64$:
- $64 = 12 \times 5 + 4$.
- A full block of $k=64$ values consists of 12 full microblocks ($12 \times 6 = 72$ bytes) plus 1 tail microblock of $v = 4$ values ($L_{\mu}(4) = 5$ bytes).
- Total block payload: $72 + 5 = 77$ bytes.
- Comparison with T243 at $k=64$: $L_{\text{T243}}(64) = \lceil 384 / 5 \rceil = 77$ bytes.
- Comparison with whole-block B729 at $k=64$: $L_{\text{B729}}(64) = \lceil 609 / 8 \rceil = 77$ bytes.
- **Exact Size Parity:** At $k=64$, T243, B729 whole-block, and B729-µ5 have **identical 77-byte block payloads**.

For partial final blocks where $m < 64$:
- Let $G = \lceil m / 5 \rceil$, full microblocks $G_{\text{full}} = \lfloor m / 5 \rfloor$, tail values $r = m \bmod 5$.
- If $r = 0$: Payload length is $6 \times G_{\text{full}}$ bytes.
- If $r > 0$: Payload length is $6 \times G_{\text{full}} + L_{\mu}(r)$ bytes.

### 4.5 Random Access Extraction Algorithm
To retrieve symbol $x_i$ at logical index $i \in [0, N-1]$ under RA-FIXED:
1. Block identification ($O(1)$ arithmetic):
   $$j = \lfloor i / k \rfloor, \quad p_{\text{block}} = i \bmod k$$
2. Microblock identification ($O(1)$ arithmetic):
   $$g = \lfloor p_{\text{block}} / 5 \rfloor, \quad p_{\mu} = p_{\text{block}} \bmod 5$$
3. Microblock byte offset:
   - For all microblocks prior to $g$, each was a full microblock occupying 6 bytes.
   - Offset of microblock $g$ relative to block start:
     $$\Delta_{\mu}(g) = 6 \cdot g$$
   - Absolute frame offset:
     $$\text{Offset}(i) = \text{Offset}(j) + \Delta_{\mu}(g) = D + j \cdot \text{align\_up}(L_c(k), A) + 6g$$
4. Bounded byte read:
   - The reader inspects $v_g = \min(5, m_j - 5g)$.
   - Length to read: $L_{\mu}(v_g) \le 6$ bytes.
   - Slices exactly $L_{\mu}(v_g)$ bytes directly from the frame:
     $$I_{\mu} = \sum_{b=0}^{L_{\mu}(v_g)-1} \text{blob}[\text{Offset}(i) + b] \cdot 256^b$$
5. Value extraction:
   - Decode value at position $p_{\mu}$:
     $$u = \left( \lfloor I_{\mu} / 729^{p_{\mu}} \rfloor \right) \bmod 729$$
     $$x_i = u - 364$$
- **Bounded Touch Guarantee:** B729-µ5 reads at most **6 contiguous payload bytes** per query. It touches **zero** bytes from other microblocks in the same block.

### 4.6 Canonical Validation Rules
Decoders and random access readers must enforce fail-closed rejection:
1. **Unused High Padding Bits:** For $v \in \{1, 2, 3, 4\}$, the top unused bits in byte $L_{\mu}(v)-1$:
   $$\text{Unused bits} = 8 \cdot L_{\mu}(v) - b_{\mu}(v)$$
   All unused bits must be strictly zero. Any nonzero bit triggers immediate frame rejection.
2. **Range Bounds:** $I_{\mu} < 729^{v_g}$. Any integer $I_{\mu} \ge 729^{v_g}$ represents an illegal out-of-range state and triggers rejection.
3. **Alignment Gap & Tail Padding:** Zero padding in alignment gaps ($D - H$) and end-of-block alignment must be strictly zero.

### 4.7 Independent Correctness Obligations
Before any benchmark execution:
1. An independent witness implementation (`independent_witness_b729_u5.py`) must be authored without shared code.
2. Exhaustive round-trip tests must pass for all $729^1 = 729$ single symbols, all $729^2 = 531,441$ pairs of symbols, and boundary sequences across all block sizes.
3. Adversarial test cases verifying non-zero padding rejection, integer overflow rejection, and asymmetric digit reversal rejection must pass 100%.

---

## 5. Frozen Synthetic Workload & Deterministic Dataset

To prevent dataset shopping and post-hoc configuration selection, both the input dataset and the query schedule are frozen deterministically:

### 5.1 Primary Workload Parameters
- **Dataset Size ($N$):** $N = 2^{20} = 1,048,576$ logical symbols.
- **Nominal Block Size ($k$):** $k = 64$ symbols per block.
- **Total Blocks ($B$):** $B = 2^{20} / 64 = 16,384$ blocks (exact division, zero partial final block).
- **Frame Alignment ($A$):** $A = 1$ byte (no inter-block padding waste).
- **Access Protocol:** **RA-FIXED** (no index table; analytical $O(1)$ block addressing).
- **Query Count ($Q$):** $Q = N = 1,048,576$ queries.

### 5.2 Deterministic Input Dataset Specification (Full-Domain Control)
- **Target Distribution:** Discrete uniform distribution over all 729 states: $x \in [-364, 364]$.
- **PRNG Algorithm:** SplitMix64 initialization followed by standard xoshiro256++ (or standard Python `random.Random` with pinned 64-bit seed).
- **Dataset Seed:** `DATASET_SEED = 0x5438315F44415441` (ASCII `"T81_DATA"`).
- **Exact Bounded Rejection Sampling Procedure:**
  To guarantee zero modular bias across the 729 states from raw 64-bit unsigned integers:
  ```python
  # Unbiased bounded sampling over [0, 728]:
  # 2^64 = 729 * 25,302,403,348,724,534 + 694
  # Rejection threshold = 2^64 - (2^64 % 729) = 18,446,744,073,709,550,922
  LIMIT_729 = 18446744073709550922

  def generate_symbols(N=1048576, seed=0x5438315F44415441):
      rng = xoshiro256plusplus(seed)
      symbols = []
      while len(symbols) < N:
          raw_u64 = rng.next_u64()
          if raw_u64 < LIMIT_729:
              u = raw_u64 % 729
              symbols.append(u - 364)
      return symbols
  ```
- **Canonical Byte Serialization & Pinned SHA-256 Digest Procedure:**
  - Before input mapping, the generated array of $N$ signed integers is packed as a little-endian signed 16-bit array (`<h`):
    $$\text{Array Bytes} = \text{struct.pack}(f'<{N}h', *symbols)$$
  - Expected length: exactly $2 \times 1,048,576 = 2,097,152$ bytes.
  - The SHA-256 digest of this canonical byte sequence is computed and recorded as `CANONICAL_DATASET_SHA256`.
  - Every authorized benchmark run must verify that `sha256(input_bytes) == CANONICAL_DATASET_SHA256` before admitting execution.

### 5.3 Independent Query Schedule Generation
- **Target Distribution:** Discrete uniform distribution over indices $i \in [0, N-1]$ drawn **with replacement**.
- **Query Seed:** `QUERY_SEED = 0x5438315F51554552` (ASCII `"T81_QUER"`), initialized completely independently of the dataset PRNG stream.
- **Sampling Procedure:**
  ```python
  # Rejection threshold for N = 2^20:
  # Since 2^64 is an exact multiple of 2^20 (2^44 * 2^20), raw_u64 & 0xFFFFF is perfectly unbiased.
  def generate_queries(Q=1048576, seed=0x5438315F51554552):
      rng = xoshiro256plusplus(seed)
      queries = []
      for _ in range(Q):
          idx = rng.next_u64() & 0xFFFFF  # 0 <= idx < 2^20
          queries.append(idx)
      return queries
  ```
- **Canonical Serialization & Hash:**
  Packed as little-endian unsigned 32-bit array (`<I`, exactly 4,194,304 bytes). The SHA-256 digest is verified before each run as `CANONICAL_QUERY_SHA256`.

### 5.4 Synthetic Workload Disclaimer
**EXPLICIT MANDATORY NOTICE:**
This workload is **purely synthetic** and designed exclusively as an algorithmic stress-test for bounded-block random access. It does **not** represent, model, or demonstrate an operational production storage access pattern, database record trace, or machine learning checkpoint workload. Claims of real-world application performance based on this synthetic benchmark are strictly prohibited.

### 5.5 Prohibition of Caches and State Leaks
- **No Block Offset Index:** Encodings must not precompute, persist, or look up block addresses in an index array.
- **No Decoded-Block Cache:** Decoders must not retain previously decoded blocks, cached microblocks, or memoized values across queries. Every query must execute independent byte fetching and decoding.
- **No Warm-Up Query Overlap:** Warm-up cycles (if any) must use a distinct, disjoint seed schedule whose state is explicitly purged before recorded trials.

---

## 6. Complete and Equal Lifecycle Accounting

To eliminate evaluation bias, benchmarking must not measure query access in isolation from the costs of preparing and retaining the data structure. The lifecycle is divided into eight discrete phases, all of which are charged:

```
[Phase 1: Input Mapping]
       │
       ▼
[Phase 2: Frame Allocation]
       │
       ▼
[Phase 3: Encoding & Packing]
       │
       ▼
[Phase 4: Retention Boundary] ─── (Free original raw inputs; enforce memory barrier)
       │
       ▼
[Phase 5: Frame Validation] ─── (Canonical structure & size verification)
       │
       ▼
[Phase 6: Timed Random Access Queries]
       │
       ▼
[Phase 7: Output Buffer Assembly & Witness Checksum]
       │
       ▼
[Phase 8: Deallocation & Scratch Cleanup]
```

### 6.1 Phase Definitions and Accounting Requirements
1. **Phase 1: Input Mapping:** Translates raw user symbols $x_i \in [-364, 364]$ to wire format $u_i = x_i + 364$.
2. **Phase 2: Frame Allocation:** Evaluates layout geometry via $O(1)$ oracle formulas and allocates the destination byte buffer.
3. **Phase 3: Encoding & Packing:** Encodes all $N$ symbols into canonical wire format (including headers, block payloads, and padding).
4. **Phase 4: Retention Boundary:** Frees original input arrays. Memory barrier confirms only the encoded wire frame remains resident.
5. **Phase 5: Frame Validation:** Validates header $N$, total byte length, and boundary alignments in $O(1)$ space.
6. **Phase 6: Timed Random Access Queries:** Executes $Q = 1,048,576$ queries from the frozen schedule.
7. **Phase 7: Output Buffer Assembly & Checksum:** Writes decoded values to a 64-bit array and computes a non-cryptographic checksum (xxHash64). Must match witness checksum.
8. **Phase 8: Deallocation & Cleanup:** Tears down frames and buffers; verifies final memory reclamation.

### 6.2 Universal Charging Rule
Every arm must be charged for:
- All static lookup tables (e.g., powers of 3, powers of 729, division lookup tables) charged to peak resident footprint.
- All scratch memory allocated during encoding, validation, or decoding.
- All bitfield shifting, multi-precision division, or arithmetic operations performed during retrieval.

---

## 7. Resource Envelope, Limits, and Caps

### 7.1 Single-Host Hardware Resource Cap
Testing is restricted to exactly **one dedicated host runner** meeting the following pinned profile:
- **Processor:** x86_64 architecture, minimum 4 physical cores, pinned CPU core frequency (performance governor active, dynamic turbo boost disabled).
- **Core Affinity:** Runner process pinned to isolated core(s) (e.g., via `taskset` or `numactl`) to eliminate context switches and migration noise.
- **Memory Ceiling:** Maximum 16 GB physical RAM allocated to the runner. Swap space disabled.
- **Cache Isolation:** L1/L2/L3 cache configuration documented. Cache state cleared between independent process repetitions.
- **Operating System:** Linux x86_64, pinned LTS kernel version.

### 7.2 Storage and Scratch Limits
- **Frame Memory Limit:** Wire frame size must conform to analytical bounds:
  - B10: $8 + 16,384 \times 80 = 1,310,728$ bytes ($\approx 1.25 \text{ MB}$).
  - T243: $8 + 16,384 \times 77 = 1,261,576$ bytes ($\approx 1.20 \text{ MB}$).
  - B729: $8 + 16,384 \times 77 = 1,261,576$ bytes ($\approx 1.20 \text{ MB}$).
  - B729-µ5: $8 + 16,384 \times 77 = 1,261,576$ bytes ($\approx 1.20 \text{ MB}$).
- **Scratch Memory Ceiling:** At no point during Phase 6 (Random Access Queries) may temporary scratch allocations exceed **64 KB** across the entire query loop.
- **Peak Resident Memory Cap:** Total resident set size (RSS) during any trial must not exceed **32 MB** ($4 \times$ raw symbol array footprint). Any execution exceeding 32 MB is immediately terminated and marked `INVALID`.

---

## 8. Statistical Protocol & Metric Formulations

### 8.1 Primary and Secondary Estimands
1. **Primary Estimand ($\theta_{\text{life}}$ — Complete-Lifecycle Throughput Ratio):**
   Defined over the complete end-to-end elapsed time $T_{\text{life}}$ (sum of Phases 1 through 7):
   $$\theta_{\text{life}}(C) = \frac{\text{Median}(T_{\text{life}}(C))}{\text{Median}(T_{\text{life}}(\text{T243}))}$$
   where $C \in \{\text{B10}, \text{B729}, \text{B729-}\mu 5\}$. Values $\theta_{\text{life}} > 1.0$ indicate T243 completes the lifecycle faster than comparator $C$.
2. **Secondary Estimand ($\theta_{\text{query}}$ — Isolated Random Access Query Throughput Ratio):**
   $$\theta_{\text{query}}(C) = \frac{\text{Median}(\text{Throughput}_{\text{T243}})}{\text{Median}(\text{Throughput}_C)}$$
   Evaluated strictly as supporting descriptive evidence; cannot override the primary lifecycle estimand.

### 8.2 Statistical Estimation & Confidence Intervals
- **Process Repetitions:** $n = 30$ independent process-level paired runs per arm.
- **Randomized Execution Order:** Within each of the 30 paired blocks, the execution order of the 4 arms is randomized uniformly.
- **Confidence Intervals:** 95% bootstrap confidence intervals computed using 10,000 paired resamples of whole-run observations:
  $$\text{CI}_{0.95}(\theta) = [\text{CI}_{\text{lower}}, \text{CI}_{\text{upper}}]$$

### 8.3 Preregistered Decision Thresholds
- **Practical Advantage Threshold:** $\tau_{\text{win}} = 1.10$ (minimum 10% throughput advantage or latency reduction).
- **Non-Inferiority Margin:** $\tau_{\text{equiv}} = 0.95$ (performance within 5% of comparator).
- **Inferiority Threshold:** $\tau_{\text{inferior}} = 0.95$.

---

## 9. Exhaustive Outcome Classifications & Decision Rules

To eliminate ambiguity, every valid, stable trial outcome maps deterministically to exactly one of three primary scientific classifications: **ACCEPT**, **REJECT**, or **INCONCLUSIVE**. The label **INVALID** is strictly reserved for protocol violations, verification failures, or uncharged resource leaks.

### 9.1 Distinguishing Advantage Rejection from Inferiority
- **Rejecting Practical Advantage:** Occurs when the upper confidence bound falls below the win threshold ($\text{CI}_{\text{upper}} < \tau_{\text{win}} = 1.10$). The evidence conclusively rules out a $\ge 10\%$ win, even if T243 is slightly faster or equivalent.
- **Establishing Inferiority:** Occurs when the upper confidence bound falls below the equivalence threshold ($\text{CI}_{\text{upper}} < \tau_{\text{equiv}} = 0.95$). The evidence conclusively proves T243 is strictly slower than the comparator by $\ge 5\%$.

### 9.2 Complete Decision Criteria
1. **`ACCEPT` (Practical Advantage Confirmed):**
   - The lower confidence bound for complete-lifecycle throughput establishes a practical win against **all three** binary baselines:
     $$\text{CI}_{0.95}(\theta_{\text{life}}(\text{B10}))_{\text{lower}} \ge 1.10 \quad \text{AND} \quad \text{CI}_{0.95}(\theta_{\text{life}}(\text{B729}))_{\text{lower}} \ge 1.10 \quad \text{AND} \quad \text{CI}_{0.95}(\theta_{\text{life}}(\text{B729-}\mu 5))_{\text{lower}} \ge 1.10$$
   - Secondary random-access query throughput of T243 is non-inferior ($\text{CI}_{\text{lower}} \ge 0.95$) against B729-µ5 and B10.
   - All deterministic checksums, wire layouts, and memory caps verified without exception.

2. **`REJECT` (Hypothesis Falsified / Advantage Disproved):**
   - Conclusive rejection of practical advantage against the primary microblock control:
     $$\text{CI}_{0.95}(\theta_{\text{life}}(\text{B729-}\mu 5))_{\text{upper}} < 1.10$$
   - OR, establishment of lifecycle inferiority against any binary control:
     $$\text{CI}_{0.95}(\theta_{\text{life}}(C))_{\text{upper}} < 0.95 \quad \text{for any } C \in \{\text{B10}, \text{B729}, \text{B729-}\mu 5\}$$
   - OR, failure of query non-inferiority against B729-µ5 ($\text{CI}_{0.95}(\theta_{\text{query}}(\text{B729-}\mu 5))_{\text{upper}} < 0.95$).

3. **`INCONCLUSIVE` (Insufficient Effect Resolution or High Noise):**
   - The confidence interval straddles the practical advantage threshold:
     $$\text{CI}_{\text{lower}} < 1.10 \le \text{CI}_{\text{upper}}$$
     (while satisfying non-inferiority $\text{CI}_{\text{lower}} \ge 0.95$).
   - OR, run-to-run coefficient of variation (CV) exceeds $5.0\%$ across repetitions.

4. **`INVALID` (Reserved for Protocol & Verification Breach):**
   - Checksum mismatch between decoded output and reference witness.
   - Any resident memory overflow ($> 32 \text{ MB}$) or query scratch allocation ($> 64 \text{ KB}$).
   - Any uncharged block cache, memoization, precomputed offset table, or hidden pointer leak.
   - Any violation of dataset seed, query schedule, or sample count.

### 9.3 Exhaustive Decision Table for Confidence Interval Profiles

The table below exhaustively categorizes all possible interval relations relative to the key boundaries ($0.95$ and $1.10$):

| Case | 95% Bootstrap Interval $[\text{CI}_{\text{lower}}, \text{CI}_{\text{upper}}]$ vs B729-µ5 | Lifecycle Non-Inferiority vs B10 & B729 | Deterministic Classification | Scientific Interpretation |
|---|---|---|---|---|
| **1** | $[1.12, 1.25]$ ($\text{CI}_{\text{lower}} \ge 1.10$) | Confirmed ($\text{CI}_{\text{lower}} \ge 0.95$) | **ACCEPT** | Conclusive practical win ($\ge 10\%$) over competent microblock control. |
| **2** | $[1.12, 1.25]$ ($\text{CI}_{\text{lower}} \ge 1.10$) | Failed ($\text{CI}_{\text{upper}} < 0.95$) | **REJECT** | Lifecycle non-inferiority failure: faster queries cannot offset large encoding/setup tax. |
| **3** | $[1.02, 1.08]$ ($\text{CI}_{\text{upper}} < 1.10$, $\text{CI}_{\text{lower}} \ge 0.95$) | Confirmed ($\text{CI}_{\text{lower}} \ge 0.95$) | **REJECT** | Practical advantage conclusively rejected: T243 is marginally faster (2–8%) but fails the 10% threshold. |
| **4** | $[0.98, 1.03]$ (Entirely within $[0.95, 1.10)$) | Confirmed ($\text{CI}_{\text{lower}} \ge 0.95$) | **REJECT** | Practical advantage conclusively rejected: T243 is equivalent to B729-µ5; no practical advantage. |
| **5** | $[0.82, 0.92]$ ($\text{CI}_{\text{upper}} < 0.95$) | Any | **REJECT** | Conclusive inferiority: T243 is strictly slower than B729-µ5. |
| **6** | $[1.05, 1.15]$ (Straddles $1.10$; $\text{CI}_{\text{lower}} \ge 0.95$) | Confirmed ($\text{CI}_{\text{lower}} \ge 0.95$) | **INCONCLUSIVE** | Effect resolution insufficient: interval spans both sub-threshold and win regions. |
| **7** | $[0.92, 1.04]$ (Straddles $0.95$; $\text{CI}_{\text{upper}} < 1.10$) | Confirmed ($\text{CI}_{\text{lower}} \ge 0.95$) | **REJECT** | Practical advantage rejected ($\text{CI}_{\text{upper}} < 1.10$); inconclusive on strict inferiority. |
| **8** | Any interval with run-to-run CV $> 5.0\%$ | Any | **INCONCLUSIVE** | Environmental noise excessive; cannot resolve true distribution medians. |
| **9** | Output checksum mismatch or scratch $> 64 \text{ KB}$ | Any | **INVALID** | Protocol breach / correctness defect. |

---

## 10. Design Choices Requiring Maintainer Approval

The following specific design decisions remain unapproved and require explicit maintainer sign-off prior to any Phase C scheduling:

1. **Approval of B729-µ5 Microblock as a Valid Binary Comparator:**
   - Maintainers must authorize whether B729-µ5 represents a legitimate domain-matched binary baseline or whether comparison must remain limited to whole-block B729.
2. **Approval of the 10% Practical Improvement Threshold:**
   - Maintainers must confirm whether a $10\%$ throughput improvement ($\theta \ge 1.10$) constitutes a sufficient practical advantage to justify non-standard base-3 wire encoding.
3. **Approval of the Single-Host Runner Specification:**
   - Maintainers must verify that the designated single-host runner environment meets reproducibility and cache control requirements.

---

## 11. Maintainer Recommendation: DEFER Criteria

**EXPLICIT RECOMMENDATION:**
If any of the following conditions occur during maintainer review:
1. Maintainers do not approve B729-µ5 as a valid binary control;
2. Independent implementation of the B729-µ5 reference codec and witness cannot be completed within minimal source complexity limits;
3. Dedicated single-host hardware with pinned core affinity and disabled dynamic frequency scaling cannot be secured;
4. The synthetic nature of the workload is judged of insufficient operational relevance to warrant computational credit expenditure;

**The maintainer must DEFER Phase C execution indefinitely rather than weakening the experimental controls, expanding resource budgets, or substituting uncontrolled shared runners.**

---

## 12. Maintainer Authorization Checklist

Prior to activating Phase C or merging execution harnesses, maintainers must verify and sign off each item:

- [ ] **Contract Integrity:** Accepted Phase B reference implementations are preserved untouched.
- [ ] **Comparator Definition:** All four comparison arms (T243, B10, whole-block B729, B729-µ5) are clearly defined.
- [ ] **Microblock Specification:** B729-µ5 grammar, tail handling, bounded-byte extraction, and validation rules are fully specified.
- [ ] **Workload Freeze:** Synthetic parameters ($N=2^{20}, k=64, A=1, \text{RA-FIXED}, Q=N$) are frozen and labeled synthetic.
- [ ] **Dataset Freeze:** Input symbols and query schedule algorithms, seeds, rejection sampling, and SHA-256 verification procedures are pinned.
- [ ] **Lifecycle Accounting:** Complete 8-phase lifecycle accounting is mandated; scratch and table memory charged.
- [ ] **Statistical Protocol:** Pinned 30-run paired protocol, 95% bootstrap CIs, upper-bound advantage rejection, and exhaustive decision rules are established.
- [ ] **Execution Freeze:** No benchmark harness, timing scripts, C++ test targets, or runner executions have been created or modified in this task.
