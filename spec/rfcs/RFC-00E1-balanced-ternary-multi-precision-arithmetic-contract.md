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

Accounting revision **correction-v2** defines selected algorithmic kernel events:
$$W_{\text{ops}} = \text{OP}_{\text{ALU\_ADD\_SUB}} + 3 \cdot \text{OP}_{\text{ALU\_MUL}} + \text{OP}_{\text{TABLE\_LOOKUP}} + \text{OP}_{\text{BRANCH\_COND}} + \text{OP}_{\text{NORM\_PASS}}$$

The formula is retained. Multiplication weight **3 is a stipulated modeling
choice**, not an empirically established universal relative cost. This mixed
abstraction metric is **not unbiased total computational work**, instruction
count, timing, energy, or a native hardware measurement. It does not supersede
H02 timing or the separate elementary operations-per-bit model. Corrected counters
must be labeled correction-v2; historical observations must not be recomputed or
silently interpreted under these definitions.

### 3.1 Event definitions and boundaries

| Event | Included boundary | Excluded work |
| --- | --- | --- |
| `op_alu_add_sub` | One ternary `addc` macro invocation; each scalar carry combination `c1+c2`; binary value additions/subtractions in `add_binary`, `mul64`, and multiplication accumulation; one multiply-accumulate addition per active ternary trit product | Internals of `addc`, conversions, normalization internals, index/address arithmetic, loop increments, assignments and copies |
| `op_alu_mul` | Each active ternary trit product; each of four binary 32x32 products in `mul64` | Products within decoding/repacking, `addc` internals, oracle and setup arithmetic |
| `op_table_lookup` | Only accesses to the three named tables in `T81Limb27::addc` | Local array indexing, `e.sum_value`, `e.cout`, `sum_vals[i][carries[i]+1]`, conversion tables, cache effects |
| `op_branch_cond` | Explicit arithmetic decisions/predicates in the four harness kernels and magnitude comparator, including selected ternary carry tests, sign comparisons, carry comparisons, borrow decisions and result-zero scans | Loop control, indexing/padding selection in the comparator, boolean short-circuit plumbing, library/conversion/internal-`addc` conditions, trial and validation checks; normalization predicates are diagnostic |
| `op_norm_pass` | One pass per ternary multiplication | Its internal operations are not added again to W_ops |
| `diagnostic_convolution_ops`, `diagnostic_reduction_ops` | One algorithmic pass each | Not per scalar product and never added to W_ops |
| `norm_alu_add_sub`, `norm_branch_cond` | Four scalar additions/subtractions per normalized trit, one extra when the remainder is negative; one remainder-sign comparison per trit | Modulo/division, loop/index arithmetic and memory; other `norm_*` fields are unused placeholders, not complete cost visibility |
| `mem_loads`, `mem_stores`, byte totals | Logical input/output limb transfers once per invocation | Sign metadata, initialization, repeated accesses, temporary arrays, allocations, internal helper/conversion traffic, caches; these are not measured host traffic |

Binary borrow decisions are: `sub < smaller`, `!b1`, conditional `larger < sub`,
and the combined `b1 || b2` selection. The selected comparison is counted once;
boolean operands are not additional comparisons. Both `c > 0` operand selections
are counted. Same-sign carry counts the carry selector and its selected `<` or
`<=` predicate. Result-zero scans count each visited limb and the final decision.

**Conversion boundary (both controls):** oracle operand preparation and validation
are outside W_ops. Ternary `to_trits()` currently constructs a unit limb, multiplies
each packed limb by that unit using `T81Limb27::operator*`, then decodes the 54-trit
result and copies 27 trits. All that arithmetic, normalization and allocation is
excluded. `from_trits_window()` (tryte encoding), carry-limb construction,
`from_trits()` buffers and output repacking are also excluded, even when called
inside a harness kernel. Only the explicit convolution on the decoded arrays and
one normalization-pass event are included for ternary multiplication. These costs
are **unmeasured and unverified as total costs**, not zero. The boundary is an
explicit model limitation; it cannot justify end-to-end comparative advantage.

### 3.2 Ternary addition and multiplication

Each `addc` invokes exactly **38 named-table lookups**: 9 `ADD_TABLE`,
8+7+5 `COMPOSITION_TABLE`, and 8+1 `CARRY_FROM_ZERO`. Internal arithmetic and
local array accesses are excluded as specified above. A nonzero incoming carry
executes a second `addc` (one macro event and 38 lookups) **and** a scalar
`c1+c2` addition. Writing a carry into an otherwise empty extension limb is a
representation copy, not an arithmetic addition.

Convolution counts each outer `ai == 0`, each visited inner `bj == 0`, and each
active-pair output-bound predicate. Each active in-bounds pair contributes one
product and one accumulation addition. Normalization contributes one pass and
separate partial diagnostic counters.

### 3.3 Binary arithmetic (correction to former §3.2)

`mul64` always executes four products and **four additions**:
`u1*v0 + (w0>>32)`, `w1 += u0*v1`, `w2 += (w1>>32)`, and
`hi = u1*v1 + w2`. The caller adds four more (`cur+lo`, `sum1+carry`,
`hi+c1`, then `+c2`): **eight additions per visited limb product**, plus one
for each actually executed extension carry addition. A zero carry breaks before
that addition. Shifts, masks and bitwise assembly are excluded.

Same-sign binary addition executes **two** additions per input limb in
`a+b+carry`. Copying a carry to an extension limb executes no arithmetic.
Opposite-sign subtraction reuses `sub=smaller+borrow` for its overflow predicate,
then executes `larger-sub`: two arithmetic events per visited input limb.
Magnitude comparison and the decisions listed in §3.1 are included. Binary zero
results are normalized to a positive sign in addition and multiplication.

---

## 4. Decoupled Exact Integer Oracle Validation Contract

Every arithmetic operation in evaluation harnesses MUST be validated against the structurally decoupled base-$10^9$ arbitrary-precision oracle (`ExactBigIntOracle`) across five mandatory dimensions:

1. **Numerical Parity:** Multi-precision balanced-ternary trits and binary words evaluate to identical integer values in the oracle.
2. **Overflow Classification:** Input operands must fit the domain; results are classified (not rejected merely for exceeding it) against $[-2^{B-1}, 2^{B-1}-1]$.
3. **Canonical Digits:** Balanced-ternary digits are verified to be canonical $d_i \in \{-1, 0, +1\}$ with no non-canonical borrows or carries.
4. **Sign Handling:** Positive, negative, zero, and boundary asymmetric values are verified without sign corruption.
5. **Absence of Truncation:** Full mathematical intermediate and result precision is preserved without bit truncation.

Each result validator populates all five `OracleValidationReport` fields and
`all_passed()` is the acceptance condition. Equality compares the decoded result
with the full oracle sum/product. Overflow compares the observed signed-B domain
classification with that of the mathematical result; the kernels return widened
values and have no separate overflow status flag. Sign checks include rejection
of binary negative zero. Ternary digit checks inspect every decoded output digit;
internal packed tryte validity before decoding is not independently verified.
Truncation requires both equality and enough output capacity for the exact value
(residual balanced quotient zero, or binary magnitude below `2^(64*limbs)`). This
is a final-result check, not a proof about every internal transient.

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
