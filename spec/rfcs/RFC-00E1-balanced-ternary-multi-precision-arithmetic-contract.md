# RFC-00E1: Balanced-Ternary Multi-Precision Arithmetic Evaluation Contract & Accounting Model

**Status:** draft
**Type:** standards-track
**Applies-To:** `experiments/scg08_arithmetic_work/`, multi-precision evaluation harnesses
**Created:** 2026-11-11
**Updated:** 2026-11-11
**Author:** Jules (T81 Research Continuity Assistant)
**Supersedes:** none; prospective specification for existing arithmetic implementation

---

## 1. Scope, Purpose & Governance Authority

This specification defines the multi-precision arithmetic representation, operand domains, accumulator-width sizing, operation-accounting model, and independent oracle validation rules for research program **SCG-08 (Balanced-Ternary Carry Propagation and Arithmetic Work)**.

### Governance Notice & Boundary
1. **Prospective Specification Only:** This document prospectively standardizes the requirements implemented in `experiments/scg08_arithmetic_work/` (PR #760, #763, #765). It does **not** assert or backdate historical maintainer approval for `T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06`.
2. **Historical Status Classified as NOT VERIFIED:** As documented in repository continuity audits, the physical frozen document text for `T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06` and dated maintainer Gate 1 implementation authorization are classified as **`NOT VERIFIED`**.
3. **Gate 1 Acceptance Status:** Formal Gate 1 implementation acceptance remains pending independent review and maintainer signature.
4. **Gate 2 Execution Status:** Benchmark execution, large-scale statistical campaigns, and hardware timing remain **`STRICTLY WITHHELD / STRICT NO-GO`**.
5. **Emulated Substrate Notice:** All algorithmic evaluations operate on binary host processors (x86_64 / AArch64). Operation counts reflect software emulation on binary ALUs, not native ternary semiconductor hardware.

---

## 2. Standardized Information Capacity & Operand Domains

Multi-precision arithmetic algorithms MUST be evaluated across five standardized signed information capacity bands $B \in \{64, 128, 256, 512, 1024\}$ bits over identical mathematical integer ranges:
$$\mathcal{D}_{\text{signed},B} = \left[-2^{B-1}, \, 2^{B-1} - 1\right]$$

### 2.1 Accumulator-Width Sizing & Limb Allocation Table
Ternary representations use 27-trit limbs (`T81Limb27` from `include/t81/types/T81Limb.hpp`), while binary controls use 64-bit limbs (`std::uint64_t`).

In balanced ternary with $T$ trits, the representable signed symmetric range is:
$$\left[-\frac{3^T - 1}{2}, \, \frac{3^T - 1}{2}\right]$$
To represent the asymmetric signed binary range boundary $[-2^{B-1}, 2^{B-1}-1]$ without truncation or overflow, $T$ must satisfy $(3^T - 1) / 2 \ge 2^{B-1}$, giving exact required trits:
$$T_{\text{signed}} = \left\lceil \log_3\left(2^B + 1\right) \right\rceil$$

For addition of two operands from $\mathcal{D}_{\text{signed},B}$, the extreme sum is $-2^B$, requiring:
$$T_{\text{add\_min}} = \left\lceil \log_3\left(2^{B+1} + 1\right) \right\rceil$$

For multiplication of two operands from $\mathcal{D}_{\text{signed},B}$, the exact product range is $\left[-2^{B-1}(2^{B-1}-1), \, 2^{2B-2}\right]$ with maximum magnitude $(-2^{B-1})^2 = 2^{2B-2}$, requiring:
$$T_{\text{mul\_min}} = \left\lceil \log_3\left(2^{2B-1} + 1\right) \right\rceil$$

The exact minimum trit requirements and corresponding limb allocations are:

| Band ($B$ bits) | Signed Domain Range | Operand Trits ($T_{\text{signed}}$) | Add Min Trits ($T_{\text{add\_min}}$) | Mul Min Trits ($T_{\text{mul\_min}}$) | Ternary Limbs (Op / Add / Mul) | Binary Limbs (Op / Add / Mul) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **64** | $[-2^{63}, 2^{63}-1]$ | 41 | 42 | 81 | 2 / 2 / 3 | 1 / 2 / 2 |
| **128** | $[-2^{127}, 2^{127}-1]$ | 81 | 82 | 161 | 3 / 4 / 6 | 2 / 3 / 4 |
| **256** | $[-2^{255}, 2^{255}-1]$ | 162 | 163 | 323 | 6 / 7 / 12 | 4 / 5 / 8 |
| **512** | $[-2^{511}, 2^{511}-1]$ | 324 | 324 | 646 | 12 / 12 / 24 | 8 / 9 / 16 |
| **1024** | $[-2^{1023}, 2^{1023}-1]$ | 647 | 647 | 1292 | 24 / 24 / 48 | 16 / 17 / 32 |

### 2.2 Mathematical Notes on Bands 512 and 1024
Notice that for $B=512$, $512 \log_3(2) \approx 323.036 \implies T_{\text{signed}} = 324$, and $513 \log_3(2) \approx 323.667 \implies T_{\text{add\_min}} = 324$. Similarly, for $B=1024$, $1024 \log_3(2) \approx 646.072 \implies T_{\text{signed}} = 647$, and $1025 \log_3(2) \approx 646.703 \implies T_{\text{add\_min}} = 647$. Because the logarithmic increment is less than the distance to the next integer ceiling, the minimum required trits for addition match the operand trit count at these specific widths, while limb allocation boundaries ($\text{Limbs}_{\text{add}} = \lceil 324/27 \rceil = 12$, $\text{Limbs}_{\text{add}} = \lceil 647/27 \rceil = 24$) ensure sufficient capacity.

---

## 3. Operation-Accounting Model, Instrumentation Boundaries & $W_{\text{ops}}$ Proxy

### 3.0 Distinction Among SCG-08 Evaluation Models & Scope of $W_{\text{ops}}$
Research program SCG-08 maintains three distinct, non-interchangeable evaluation models:
1. **H02 Carry-Resolution Timing Model (Phase 0 Charter §9, §10.3 Tier 3):** Evaluates host CPU execution time and whether balanced-ternary carry normalization overhead exceeds binary carry resolution time by $> 2.0\times$ across $B \in [64, 1024]$ bits.
2. **Elementary Operations-per-Bit Model (Phase 0 Charter §10.3 Tier 2, §10.5):** Evaluates primitive additions, table lookups, carry propagation steps, and normalizations normalized per bit of information ($\le 1.10\times$ parity target at $\ge 512$ bits; $> 2.0\times$ universal falsification threshold).
3. **Stipulated Weighted Kernel Event Proxy ($W_{\text{ops}}$) (Addendum 06 / RFC-00E1):** Aggregates five instrumented kernel event counters under the fixed linear formula:
   $$W_{\text{ops}} = \text{OP}_{\text{ALU\_ADD\_SUB}} + 3 \cdot \text{OP}_{\text{ALU\_MUL}} + \text{OP}_{\text{TABLE\_LOOKUP}} + \text{OP}_{\text{BRANCH\_COND}} + \text{OP}_{\text{NORM\_PASS}}$$

#### Methodological Qualifications
- **Not Unbiased Total Computational Work:** $W_{\text{ops}}$ is a stipulated, representation-specific kernel event proxy. It does **not** measure unbiased total computational work, instruction count, or physical hardware energy/latency, because it collapses multi-trit normalization sweeps into `op_norm_pass = 1`, excludes intra-harness representation unpacking/repacking (`to_trits()`, `from_trits()`, `from_trits_window()`), excludes internal index/encoding arithmetic inside `T81Limb27::addc()`, and excludes bitwise half-word shifts/masks inside `mul64()`.
- **Stipulated Multiplication Weight ($3 \cdot \text{OP}_{\text{ALU\_MUL}}$):** The weight factor $3$ assigned to `op_alu_mul` is a stipulated modeling convention that prices each counted primitive multiplication event (`ai * bj` on non-zero trits in `mul_ternary`, or a 32x32-bit partial product in `mul64`) at three nominal additive units. It is **not** an empirically established or microarchitecturally universal cost ratio across radices or CPUs.

### 3.1 Instrumentation Boundaries & Work Taxonomy
Every operation in `tier2_work_harness.hpp` is partitioned into four explicit categories:

1. **Category A — Algorithmic Kernel Work (Aggregated into $W_{\text{ops}}$):**
   - `op_alu_add_sub`: Counted limb `addc()` calls and scalar carry combinations (`c1 + c2`) in `add_ternary`; active trit product accumulations in `mul_ternary`; 64-bit word additions/subtractions in `add_binary`; and `mul64()` helper additions plus caller accumulator and carry-propagation additions in `mul_binary`.
   - `op_alu_mul`: Active non-zero trit multiplications (`ai * bj`) in `mul_ternary` and 32x32-bit half-word multiplications in `mul64()`.
   - `op_table_lookup`: Accesses to the three named static lookup tables (`detail::ADD_TABLE`, `detail::COMPOSITION_TABLE`, `detail::CARRY_FROM_ZERO`) inside `T81Limb27::addc()`.
   - `op_branch_cond`: Evaluated data and control predicates inside the harness kernel loops.
   - `op_norm_pass`: Whole-accumulator carry-normalization sweeps in `mul_ternary` (1 per invocation).
2. **Category B — Representation Conversion, Unpacking & Repacking (Excluded from $W_{\text{ops}}$):**
   - Test-harness oracle conversions (`TernaryMultiLimb::from_oracle`, `trits_to_oracle`, `BinaryMultiLimb::from_oracle`, `BinaryMultiLimb::to_oracle`) execute outside the kernel boundary.
   - Intra-harness ternary format conversions (`a.to_trits()` and `b.to_trits()` at `mul_ternary` entry, `TernaryMultiLimb::from_trits()` at `mul_ternary` exit, and `T81Limb27::from_trits_window()` carry-limb construction in `add_ternary`) are excluded from $W_{\text{ops}}$ and recorded in non-aggregating diagnostic counters:
     - `conv_limb_unpacks`: Number of `T81Limb27` $\to$ 27-trit limb unpacks.
     - `conv_limb_repacks`: Number of 27-trit window $\to$ `T81Limb27` limb packs via `from_trits_window`.
     - `conv_tryte_decodes`: Number of tryte-to-3-trits decode operations (`detail::TRYTE_TO_TRITS` and `t81::decode_tryte`).
     - `conv_tryte_encodes`: Number of 3-trit-to-tryte `t81::encode_tryte` operations.
3. **Category C — Normalization Internal Primitive Work (Excluded from $W_{\text{ops}}$):**
   - Tracked in non-aggregating diagnostic counters `norm_alu_add_sub`, `norm_alu_mul`, `norm_table_lookup`, and `norm_branch_cond`.
   - Constant-radix modulo and division operations (`(sum + 1) % 3` and `(sum - rem) / 3`) inside `mul_ternary` normalization are excluded from both $W_{\text{ops}}$ and `norm_alu_add_sub`.
4. **Category D — Memory Traffic & Data Copies (Excluded from $W_{\text{ops}}$):**
   - Nominal operand limb loads (`2 * InLimbs`) and result limb stores (`OutLimbs`) are recorded in `mem_loads`, `mem_stores`, `bytes_read`, and `bytes_written`.
   - Pure data movement and zero-fill operations (`res.limbs[i] = carry` in `add_binary` extension limbs, `res.limbs[i] = c_limb` in `add_ternary` extension limbs, and `res.limbs[i] = 0` / `T81Limb27{}`) are data copies and MUST NOT increment `op_alu_add_sub`.

### 3.2 Ternary Arithmetic Accounting Rules
1. **Limb Addition (`add_ternary<InLimbs, OutLimbs>`):**
   - **Named-Table Lookups per `T81Limb27::addc()`:** As verified against `include/t81/types/T81Limb.hpp:688-722`, each `T81Limb27::addc()` invocation across `TRYTES = 9` trytes accounts for **38 named-table lookups**:
     - 9 lookups in `detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13]` ($i = 0 \dots 8$)
     - 20 lookups in `detail::COMPOSITION_TABLE` across the three unrolled Kogge-Stone prefix stages ($8 + 7 + 5$)
     - 9 lookups in `detail::CARRY_FROM_ZERO` ($8$ internal tryte carries `map_ids[0..7]` + $1$ `carry_out` lookup `map_ids[8]`)
   - **Disclosed Exclusions Inside `T81Limb27::addc()`:** The coarse `op_alu_add_sub += 1, op_table_lookup += 38` accounting per `addc()` call deliberately excludes:
     - 18 index-offset additions (`trytes_[i] + 13`, `other.trytes_[i] + 13`) and 9 carry-index additions (`carries[i] + 1`),
     - 9 base-3 carry-map polynomial encodings `map_ids[i] = (e.cout[0] + 1) + 3 * (e.cout[1] + 1) + 9 * (e.cout[2] + 1)` (45 additions and 18 constant multiplications),
     - 9 local array selections `sum_vals[i][carries[i] + 1]` and 27 scalar copies into `sum_vals`,
     - Fixed-trip loop counter increments and loop-bound comparisons across the 5 internal loops.
   - **Input-Limb Loop ($0 \le i < \text{InLimbs}$):**
     - Primary limb addition `a.limbs[i].addc(b.limbs[i])` increments `op_alu_add_sub` by 1 and `op_table_lookup` by 38.
     - Carry-in check `if (carry_in != 0)` increments `op_branch_cond` by 1 on every limb.
     - On the $K_{\text{cin}}$ limbs where `carry_in != 0`:
       - Packing `carry_in` via `T81Limb27::from_trits_window(c_buf, 0)` increments `conv_limb_repacks` by 1 and `conv_tryte_encodes` by 9.
       - Carry-in limb addition `sum_ab.addc(c_limb)` increments `op_alu_add_sub` by 1 and `op_table_lookup` by 38.
       - Scalar carry combination `carry_in = c1 + c2` increments `op_alu_add_sub` by 1.
   - **Extension-Limb Loop ($\text{InLimbs} \le i < \text{OutLimbs}$):**
     - Carry check `if (carry_in != 0)` increments `op_branch_cond` by 1 on every extension limb.
     - When `carry_in != 0` ($C_{\text{ext}} \in \{0, 1\}$), `from_trits_window` increments `conv_limb_repacks` by 1 and `conv_tryte_encodes` by 9, and `res.limbs[i] = c_limb; carry_in = 0;` performs a limb copy (**0** `op_alu_add_sub` increments).
   - **Closed-Form Expected Counts:**
     $$\text{op\_alu\_add\_sub} = \text{InLimbs} + 2 K_{\text{cin}}, \quad \text{op\_table\_lookup} = 38(\text{InLimbs} + K_{\text{cin}}), \quad \text{op\_branch\_cond} = \text{OutLimbs}$$
     $$W_{\text{ops}}(\text{add\_ternary}) = 39\,\text{InLimbs} + 40\,K_{\text{cin}} + \text{OutLimbs}$$
     $$\text{conv\_limb\_repacks} = K_{\text{cin}} + C_{\text{ext}}, \quad \text{conv\_tryte\_encodes} = 9(K_{\text{cin}} + C_{\text{ext}})$$
2. **Limb Multiplication (`mul_ternary<InLimbs, OutLimbs>`):**
   - **Entry Unpacking (`a.to_trits()`, `b.to_trits()`):** Unpacks $2\,\text{InLimbs}$ limbs (`conv_limb_unpacks = 2 * InLimbs`). Because each `TernaryMultiLimb::to_trits()` invocation constructs a temporary `unit_limb` via `T81Limb27::from_trits_window` (1 `conv_limb_repacks` and 9 `encode_tryte` calls per `to_trits()` call) and evaluates `limbs[l] * unit_limb` (18 `TRYTE_TO_TRITS` lookups + 18 `encode_tryte` calls in `T81Limb54::from_trits`) followed by `l54.to_trits()` (18 `decode_tryte` calls) per limb, entry unpacking records `conv_limb_repacks += 2`, `conv_tryte_decodes = 72 * InLimbs`, and `conv_tryte_encodes += 18 + 36 * InLimbs`.
   - **Convolution Kernel:** Let $T_{\text{in}} = 27\,\text{InLimbs}$, $T_{\text{out}} = 27\,\text{OutLimbs}$, and let $N_a, N_b$ be the counts of non-zero trits in $a$ and $b$.
     - Outer loop evaluates `ai == 0` $T_{\text{in}}$ times (`op_branch_cond += T_in`).
     - For each of the $N_a$ non-zero $a_i$, inner loop evaluates `bj == 0` $T_{\text{in}}$ times (`op_branch_cond += N_a * T_in`).
     - For each of the $N_a N_b$ active pairs, `i + j < ACCUM_TRITS` is evaluated (`op_branch_cond += N_a * N_b`), and `accum[i + j] += ai * bj` increments `op_alu_mul` by 1 and `op_alu_add_sub` by 1.
   - **Normalization Pass:** Increments `op_norm_pass` by 1 and records `norm_branch_cond = T_out` and `norm_alu_add_sub = 4 * T_out + N_neg_rem` as non-aggregating diagnostics.
   - **Exit Repacking (`TernaryMultiLimb<OutLimbs>::from_trits`):** Records `conv_limb_repacks += OutLimbs` (total `conv_limb_repacks = OutLimbs + 2`) and `conv_tryte_encodes += 9 * OutLimbs` (total `conv_tryte_encodes = 18 + 36 * InLimbs + 9 * OutLimbs`).
   - **Closed-Form Expected Kernel Counts (for in-domain operands where active $i + j < T_{\text{out}}$):**
     $$\text{op\_alu\_mul} = N_a N_b, \quad \text{op\_alu\_add\_sub} = N_a N_b, \quad \text{op\_branch\_cond} = T_{\text{in}}(1 + N_a) + N_a N_b, \quad \text{op\_norm\_pass} = 1$$
     $$W_{\text{ops}}(\text{mul\_ternary}) = 5 N_a N_b + T_{\text{in}}(1 + N_a) + 1$$

### 3.3 Binary Arithmetic Accounting Rules
1. **Primitive Multiplier (`mul64`):**
   - 64x64-bit multiplication decomposed into 32-bit halves (`u0, u1, v0, v1`) SHALL account for **4** primitive 32x32-bit multiplications (`u0*v0`, `u1*v0`, `u0*v1`, `u1*v1`) in `op_alu_mul` and **4** helper 64-bit additions (`u1*v0 + (w0 >> 32)`, `w1 += u0*v1`, `w2 += (w1 >> 32)`, `hi = u1*v1 + w2`) in `op_alu_add_sub`.
   - Bitwise shifts (`>> 32`, `<< 32`) and masks (`& 0xFFFFFFFFULL`, `|`) used for 32-bit half-word extraction and reassembly are excluded from $W_{\text{ops}}$.
2. **Multi-Precision Multiplication (`mul_binary<InLimbs, OutLimbs>`):**
   - For each of $M_a$ non-zero outer limbs `ai` (`InLimbs` outer `ai == 0` checks in `op_branch_cond`), the inner loop over `j in [0, InLimbs)` evaluates `i + j >= OutLimbs` (1 branch), calls `mul64` (4 muls, 4 adds), and accumulates `sum1 = cur + lo`, `sum2 = sum1 + carry`, `carry = hi + c1 + c2` (4 caller additions into `op_alu_add_sub` and 2 carry predicates `sum1 < cur`, `sum2 < sum1` into `op_branch_cond`), totaling **4 multiplications, 8 additions, and 3 branch evaluations per inner product step**.
   - The carry propagation loop (`k >= i + InLimbs`) adds 1 `op_alu_add_sub` and 2 `op_branch_cond` (`carry == 0` false and `sum < cur`) per active carry step ($C_{\text{prop}}$ total), plus 1 `op_branch_cond` when terminating on `carry == 0` ($B_{\text{break}}$ times).
   - Post-convolution sign normalization evaluates `res.limbs[i] != 0` until the first non-zero limb ($Z_{\text{scan}}$ checks) plus 1 `all_zero` check.
   - **Closed-Form Expected Counts ($\text{OutLimbs} = 2\,\text{InLimbs}$):**
     $$\text{op\_alu\_mul} = 4 M_a \text{InLimbs}, \quad \text{op\_alu\_add\_sub} = 8 M_a \text{InLimbs} + C_{\text{prop}}$$
     $$\text{op\_branch\_cond} = \text{InLimbs} + 3 M_a \text{InLimbs} + 2 C_{\text{prop}} + B_{\text{break}} + Z_{\text{scan}} + 1$$
3. **Multi-Precision Addition/Subtraction (`add_binary<InLimbs, OutLimbs>`):**
   - Evaluates sign equality `a.is_negative == b.is_negative` at entry (`op_branch_cond += 1`).
   - **Same-Sign Addition Path:**
     - Each input limb $i \in [0, \text{InLimbs})$ executes **2** primitive 64-bit additions (`sum_ab = a.limbs[i] + b.limbs[i]` and `sum = sum_ab + carry`, incrementing `op_alu_add_sub` by 2) and **2** conditional evaluations (`carry != 0` and the selected carry predicate `sum <= a.limbs[i]` or `sum < a.limbs[i]`, incrementing `op_branch_cond` by 2).
     - Extension limbs $i \in [\text{InLimbs}, \text{OutLimbs})$ copy `res.limbs[i] = carry; carry = 0;` as pure data movement (**0** `op_alu_add_sub` and **0** `op_branch_cond` increments).
     - **Closed-Form Same-Sign Expected Counts:**
       $$\text{op\_alu\_add\_sub} = 2\,\text{InLimbs}, \quad \text{op\_branch\_cond} = 1 + 2\,\text{InLimbs}, \quad W_{\text{ops}} = 4\,\text{InLimbs} + 1$$
   - **Opposite-Sign Subtraction Path:**
     - Calls `cmp_binary_limbs(a.limbs, b.limbs, acc)` (1 `va != vb` check per compared limb from MSB, plus 1 `va < vb` check on the first differing limb), followed by `c == 0` (`op_branch_cond += 1`).
     - If `c != 0`, evaluates `c > 0` (`op_branch_cond += 1`) and for each input limb $i \in [0, \text{InLimbs})$ computes `sub = smaller.limbs[i] + borrow` and `res.limbs[i] = larger.limbs[i] - sub` (`op_alu_add_sub += 2`), evaluating borrow predicates `sub < smaller.limbs[i]` (+1 branch) and short-circuit `larger.limbs[i] < sub` when `!(sub < smaller.limbs[i])` (+1 branch). Extension limbs are zero-filled as data copies (0 ALU ops).
4. **Historical Evidence Preservation Notice:**
   - The committed artifact `experiments/scg08_arithmetic_work/gate1_evidence_record.json` records historical observations tied to `evaluated_implementation_sha: 2dd3cc62e526942cdcbed6d100770c08c26ab2fe` (prior to the `add_ternary` `c1 + c2` / extension-copy and `add_binary` `a + b + carry` / carry-predicate / extension-copy counter reconciliations above). In accordance with scientific continuity rules, those historical recorded numbers are preserved unaltered in `gate1_evidence_record.json`.

---

## 4. Decoupled Exact Integer Oracle Validation Contract

Every arithmetic operation in evaluation harnesses MUST be validated at the result level against the structurally decoupled base-$10^9$ arbitrary-precision oracle (`ExactBigIntOracle`) across five mandatory dimensions, populating all fields of `OracleValidationReport`:

1. **Numerical Equality (`numerical_equality`):** Converted input operands (`ta`, `tb`, `ba`, `bb`) and computed multi-precision addition and multiplication results (`t_sum`, `t_prod`, `b_sum`, `b_prod`) MUST evaluate to the exact mathematical integer values of `oa`, `ob`, `ExactBigIntOracle::add(oa, ob)`, and `ExactBigIntOracle::mul(oa, ob)`.
2. **Overflow Classification (`overflow_classification`):** Input operands MUST be verified within $\mathcal{D}_{\text{signed},B} = [-2^{B-1}, 2^{B-1}-1]$, and computed ternary and binary results MUST be classified against $\mathcal{D}_{\text{signed},B}$ (`!is_in_signed_b_domain(result, B)`), matching both the oracle overflow classification and deterministic analytical expectations while remaining within exact accumulator bounds ($[-2^B, 2^B-2]$ for addition; $[-2^{B-1}(2^{B-1}-1), 2^{2B-2}]$ for multiplication).
3. **Canonical Digits (`canonical_digits`):** Both input and output balanced-ternary representations MUST have all stored `T81Limb27` trytes validated within $[-13, +13]$ (`has_canonical_trytes()`) **before** any table-indexed decoding, multiplication, or addition operation (`TRYTE_TO_TRITS[tryte + 13]` or `ADD_TABLE[tryte + 13]`) is executed, and all extracted trits MUST lie within $\{-1, 0, +1\}$, matching the unique canonical balanced-ternary expansion `oracle_to_canonical_trits<N>(expected)` with no unnormalized borrows or carries.
4. **Sign Handling (`sign_handling`):** Oracle values, ternary digit vectors (sign of the most significant non-zero trit), and binary sign-magnitude limbs (`is_negative` flag and non-zero limb invariants, strictly rejecting negative zero `-0`) MUST agree across positive, negative, zero, and asymmetric boundary values.
5. **Absence of Truncation (`absence_of_truncation`):** Allocated limb widths MUST satisfy `AccumulatorSpec` minimums, oracle-to-representation decompositions MUST leave zero high-order quotient residual, and all high-order extension trits ($\ge T_{\text{signed}}$) and binary extension limbs ($\ge B_{\text{In}}$) MUST be preserved without truncation to the $B$-bit input domain.

### Fail-Closed Acceptance & Negative-Test Requirement
- Evaluation harnesses MUST populate all five fields of `OracleValidationReport` and enforce `OracleValidationReport::all_passed()` as an active acceptance condition.
- Any discrepancy in oracle evaluation SHALL immediately abort execution with exit code 1 and emit:
```
INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)
```
- Deterministic negative tests MUST deliberately corrupt each of the five validation dimensions (numerical result, overflow classification, canonical output digits/trytes, sign handling, and high-limb/high-trit truncation) and verify both `!report.all_passed()` and fail-closed termination.

---

## 5. Execution Budget & Gate Separation

1. **Gate 1 Execution Ceiling:**
   - Standalone smoke testing and deterministic test suites SHALL enforce a hard ceiling of **$\le 1,000$ cumulative trials** via `TrialBudgetEnforcer`.
   - The ceiling is classified as `IMPLEMENTED / HISTORICAL — PREREGISTRATION NOT VERIFIED`.
   - Exceeding 1,000 recorded trials aborts execution fail-closed.
2. **Gate 2 Pre-requisite:**
   - Progression to Gate 2 benchmarking requires a separate, dated maintainer authorization record.
   - Gate 2 execution remains **STRICTLY WITHHELD / STRICT NO-GO**.
