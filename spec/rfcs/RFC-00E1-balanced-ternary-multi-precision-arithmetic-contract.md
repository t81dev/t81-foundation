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

For multiplication of two operands from $\mathcal{D}_{\text{signed},B}$, the extreme product is $(-2^{B-1})^2 = 2^{2B-2}$, requiring:
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

## 3. Weighted Operation Metric ($W_{\text{ops}}$) & Accounting Invariants

To avoid biasing algorithmic comparisons, both ternary and binary implementations SHALL account for primitive computational steps under the frozen weighted metric:
$$W_{\text{ops}} = \text{OP}_{\text{ALU\_ADD\_SUB}} + 3 \cdot \text{OP}_{\text{ALU\_MUL}} + \text{OP}_{\text{TABLE\_LOOKUP}} + \text{OP}_{\text{BRANCH\_COND}} + \text{OP}_{\text{NORM\_PASS}}$$

### 3.1 Ternary Arithmetic Accounting Rules
1. **Limb Addition (`add_ternary`):**
   - Each call to `T81Limb27::addc()` performs table-driven addition across 9 trytes. As verified against `include/t81/types/T81Limb.hpp`, each invocation SHALL account for exactly **38 table lookups**:
     - 9 lookups in `detail::ADD_TABLE`
     - 20 lookups in `detail::COMPOSITION_TABLE` ($8 + 7 + 5$)
     - 9 lookups in `detail::CARRY_FROM_ZERO` ($8 + 1$)
   - Primary limb addition increments `op_alu_add_sub` by 1 and `op_table_lookup` by 38.
   - If incoming carry is non-zero, a second `addc()` invocation absorbs the carry, incrementing `op_alu_add_sub` by 1 and `op_table_lookup` by 38.
   - Every carry check evaluates `op_branch_cond`.
2. **Limb Multiplication (`mul_ternary`):**
   - Convolutive outer and inner loops evaluate trits $a_i$ and $b_j$.
   - Conditional zero-checks `ai == 0` and `bj == 0` increment `op_branch_cond` on **every evaluation** regardless of branch outcome.
   - Active products (`ai != 0 && bj != 0`) increment `op_alu_mul` by 1 and `op_alu_add_sub` by 1.
   - Each multiplication pass executes a normalization pass, incrementing `op_norm_pass` by 1.
   - Primitive normalization operations (`norm_alu_add_sub`, `norm_branch_cond`) SHALL be tracked as non-aggregating diagnostic counters and MUST NOT alter $W_{\text{ops}}$.

### 3.2 Binary Arithmetic Accounting Rules
1. **Primitive Multiplier (`mul64`):**
   - 64x64-bit multiplication decomposed into 32-bit halves SHALL account for 4 primitive multiplications (`u0*v0`, `u1*v0`, `u0*v1`, `u1*v1`) into `op_alu_mul` and 3 intermediate 64-bit additions into `op_alu_add_sub`.
2. **Multi-Precision Multiplication (`mul_binary`):**
   - Accumulator additions in the caller loop (`sum1 = cur + lo`, `sum2 = sum1 + carry`, `carry = hi + c1`, `carry += c2`) SHALL account for 4 additions per inner step into `op_alu_add_sub` (totaling 7 additions per inner product).
   - Condition checks across limb zero-detection, carry predicates, carry propagation, result scanning, and sign normalization SHALL increment `op_branch_cond` on every evaluation.
3. **Multi-Precision Addition/Subtraction (`add_binary`):**
   - Same-sign additions account for limb additions and carry predicates.
   - Opposite-sign subtractions account for magnitude comparisons (`cmp_binary_limbs`) and borrow propagation additions.

---

## 4. Decoupled Exact Integer Oracle Validation Contract

Every arithmetic operation in evaluation harnesses MUST be validated against the structurally decoupled base-$10^9$ arbitrary-precision oracle (`ExactBigIntOracle`) across five mandatory dimensions:

1. **Numerical Parity:** Multi-precision balanced-ternary trits and binary words evaluate to identical integer values in the oracle.
2. **Overflow Classification:** Input operands and results are strictly checked against $[-2^{B-1}, 2^{B-1}-1]$.
3. **Canonical Digits:** Balanced-ternary digits are verified to be canonical $d_i \in \{-1, 0, +1\}$ with no non-canonical borrows or carries.
4. **Sign Handling:** Positive, negative, zero, and boundary asymmetric values are verified without sign corruption.
5. **Absence of Truncation:** Full mathematical intermediate and result precision is preserved without bit truncation.

### Fail-Closed Enforcement
Any discrepancy in oracle evaluation SHALL immediately abort execution with exit code 1 and emit:
```
INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)
```

---

## 5. Execution Budget & Gate Separation

1. **Gate 1 Execution Ceiling:**
   - Standalone smoke testing and deterministic test suites SHALL enforce a hard ceiling of **$\le 1,000$ cumulative trials** via `TrialBudgetEnforcer`.
   - The ceiling is classified as `IMPLEMENTED / HISTORICAL — PREREGISTRATION NOT VERIFIED`.
   - Exceeding 1,000 recorded trials aborts execution fail-closed.
2. **Gate 2 Pre-requisite:**
   - Progression to Gate 2 benchmarking requires a separate, dated maintainer authorization record.
   - Gate 2 execution remains **STRICTLY WITHHELD / STRICT NO-GO**.
