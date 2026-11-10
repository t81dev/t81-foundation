# SCG-08 Phase 1 Gate 1 Implementation Report

**Document ID:** `T81-SCG-08-GATE1-REPORT-001`
**Specification:** `T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06`
**Parent Charter:** `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md` (`T81-SCG-08-PHASE0-CHARTER-001-R2`, merged via PR #753 at `0bd0817e4b9dc879fffce18c59ea9ff781f7acb3`)
**Status:** READY FOR INDEPENDENT GATE 1 IMPLEMENTATION REVIEW
**Gate 2 Status:** **STRICTLY WITHHELD**

---

## 1. Executive Summary

This report documents the isolated Phase 1 Gate 1 implementation of the experimental arithmetic evaluation harness for program **SCG-08: Balanced-Ternary Carry Propagation and Arithmetic Work**.

The primary objective of Gate 1 is to establish:
1. Mathematical correctness of balanced-ternary and binary multi-precision arithmetic algorithms.
2. Structurally decoupled independent oracle verification via a base-$10^9$ arbitrary-precision exact oracle (`ExactBigIntOracle`).
3. Exact operation accounting under the frozen weighted metric $W_{\text{ops}}$, with non-aggregating diagnostic counters and explicit normalization internal tracking.
4. Strict enforcement of execution constraints, specifically capping cumulative trials to $\le 1,000$ trials in executable code.
5. Deterministic replay and reproducibility.

**Crucial Research Boundary:** This implementation does not demonstrate or assert ternary performance superiority. Gate 2 experimental benchmarking remains strictly withheld.

---

## 2. Repository Context & Baseline

- **Repository:** https://github.com/t81dev/t81-foundation
- **Baseline Git SHA:** `cd490f5b5c0496350682542b56424dd77bea6963` (merging PR #756)
- **PR #753 Integration:** Verified integrated into mainline history (`0bd0817e4b9dc879fffce18c59ea9ff781f7acb3`).

### Changed File Inventory
| File | Action | Purpose |
| :--- | :--- | :--- |
| `CMakeLists.txt` | Modified | Added opt-in `T81_BUILD_EXPERIMENTS` option guarding `experiments/scg08_arithmetic_work` |
| `experiments/scg08_arithmetic_work/CMakeLists.txt` | Created | Declares target `t81_exp_scg08_01_test` and `t81_exp_scg08_01_schema_validation` CTest checks |
| `experiments/scg08_arithmetic_work/evidence_schema.json` | Created | Machine-readable JSON schema `t81.scg08.carry-experiment.v1` |
| `experiments/scg08_arithmetic_work/gate1_evidence_record.json` | Created | Measured counters, weighted totals, and validation outcomes conforming to evidence schema |
| `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp` | Created | Decoupled arbitrary-precision base-$10^9$ exact integer oracle & 5D validator |
| `experiments/scg08_arithmetic_work/tier1_markov_harness.hpp` | Created | Tier 1 scalar carry Markov chain simulator with decoupled event counters |
| `experiments/scg08_arithmetic_work/tier2_work_harness.hpp` | Created | Tier 2 multi-precision signed arithmetic, operation accounting, and trial budget enforcer |
| `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp` | Created | Unconditional fail-closed test harness executable active under `-DNDEBUG` |
| `scripts/ci/validate_scg08_gate1_evidence.py` | Created | Fail-closed stdlib-only validation script for `gate1_evidence_record.json` |
| `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` | Created | Gate 1 implementation report and verification receipts |

**Production Impact:** Zero modifications to production arithmetic files (`include/t81/types/`, `src/core/`, etc.). All additions are isolated under `experiments/scg08_arithmetic_work/`.

---

## 3. Accumulator-Width Sizing & Domain Sizing (Addendum 06)

Evaluated across the 5 standardized signed $B$-bit operand domains $\mathcal{D}_{\text{signed},B} = [-2^{B-1}, 2^{B-1}-1]$:

| Band ($B$ bits) | Signed Domain Range | Operand Trits ($T_{\text{signed}}$) | Add Min Trits ($T_{\text{add\_min}}$) | Mul Min Trits ($T_{\text{mul\_min}}$) | Ternary Limbs (Op / Add / Mul) | Binary Limbs (Op / Add / Mul) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **64** | $[-2^{63}, 2^{63}-1]$ | 41 | 42 | 81 | 2 / 2 / 3 | 1 / 2 / 2 |
| **128** | $[-2^{127}, 2^{127}-1]$ | 81 | 82 | 161 | 3 / 4 / 6 | 2 / 3 / 4 |
| **256** | $[-2^{255}, 2^{255}-1]$ | 162 | 163 | 323 | 6 / 7 / 12 | 4 / 5 / 8 |
| **512** | $[-2^{511}, 2^{511}-1]$ | 324 | 324 | 646 | 12 / 12 / 24 | 8 / 9 / 16 |
| **1024** | $[-2^{1023}, 2^{1023}-1]$ | 647 | 647 | 1292 | 24 / 24 / 48 | 16 / 17 / 32 |

*Verification:* Verified analytically and confirmed in executable code by `test_accumulator_width_specification()`.

---

## 4. Independent Exact Oracle Evidence

The test suite validates every arithmetic operation against the decoupled base-$10^9$ `ExactBigIntOracle` across 5 mandatory dimensions:
1. **Numerical Equality:** Computed multi-precision balanced-ternary trits and binary words evaluate to the identical mathematical integer value in the oracle.
2. **Overflow Classification:** Inputs and outputs are strictly classified against the signed domain boundary $[-2^{B-1}, 2^{B-1}-1]$.
3. **Canonical Balanced-Ternary Digits:** Output representations are verified to be canonical digits $d_i \in \{-1, 0, +1\}$ with no non-canonical carries.
4. **Correct Sign Handling:** Positive, negative, zero, and asymmetric negative boundary $-2^{B-1}$ are verified without sign corruption or unhandled borrow states.
5. **Absence of Truncation:** Full mathematical precision is retained even when intermediate or final values exceed original operand bit-widths.

Any violation triggers immediate termination with:
```
INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)
```

Negative tests via isolated subprocess forks confirm that deliberate oracle mismatches and budget violations terminate execution with exit status 1 and the mandatory diagnostic message.

---

## 5. Operation Accounting Reconciliation & Findings Resolution

The frozen weighted cost metric is preserved exactly:
$$W_{\text{ops}} = \text{OP}_{\text{ALU\_ADD\_SUB}} + 3 \cdot \text{OP}_{\text{ALU\_MUL}} + \text{OP}_{\text{TABLE\_LOOKUP}} + \text{OP}_{\text{BRANCH\_COND}} + \text{OP}_{\text{NORM\_PASS}}$$

### 5.1 Inline Review Findings Reconciliation
1. **Finding 1 (P1 - Table Lookup Accounting Reconciliation):**
   - *Source Verification:* In `include/t81/types/T81Limb.hpp`, each `T81Limb27::addc()` performs:
     - 9 lookups in `detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13]` (1 per tryte)
     - 8 lookups in `detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 1]]` (for $i=1..8$)
     - 7 lookups in `detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 2]]` (for $i=2..8$)
     - 5 lookups in `detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 4]]` (for $i=4..8$)
     - 8 lookups in `detail::CARRY_FROM_ZERO[map_ids[i]]` (carries array for $i=1..8$)
     - 1 lookup in `detail::CARRY_FROM_ZERO[map_ids[8]]` (carry_out computation)
     - **Exact Sum:** $9 + 8 + 7 + 5 + 8 + 1 = 38$ table lookups per `T81Limb27::addc()` invocation.
   - *Resolution:* Reconciled `acc.op_table_lookup` in `ArithmeticWorkHarness::add_ternary` to increment by 38 per `addc()` call (for both initial limb pairs and nested carry-in limbs `sum_ab.addc(c_limb)`).
2. **Finding 2 (P2 - Consistent Conditional Evaluations):**
   - *Source Verification:* Conditional branch evaluations must be counted consistently rather than branch-taken only. In both binary and ternary algorithms, condition evaluations are counted on each iteration:
     - Ternary multiplication: outer loop `if (ai == 0)` and inner loop `if (bj == 0)` and `if (i + j < ACCUM_TRITS)` evaluate condition checks on each iteration regardless of whether trits are zero or non-zero.
     - Binary multiplication: outer loop `if (ai == 0)` evaluates condition checks on each iteration, inner loop `if (i + j >= OutLimbs)` and carry continuation `if (carry == 0)` condition checks are tracked.
     - Binary addition: `if (a.is_negative == b.is_negative)`, `if (carry != 0)`, and magnitude comparison condition checks are tracked.
     - Zero, sparse, dense, and carry-propagating operands evaluate condition branches consistently without artificially equalizing operation costs.
3. **Finding 3 (P2 - Machine-Readable Evidence Artifact & Schema Conformance):**
   - *Resolution:* Persisted measured counters, weighted totals, and validation outcomes in `experiments/scg08_arithmetic_work/gate1_evidence_record.json`.
   - Built a fail-closed, stdlib-only validation script `scripts/ci/validate_scg08_gate1_evidence.py` verifying full conformance against `experiments/scg08_arithmetic_work/evidence_schema.json` (`t81.scg08.carry-experiment.v1`).
   - Integrated schema validation into CTest (`t81_exp_scg08_01_schema_validation`).

Accounting guarantees enforced:
- **Diagnostic Non-Aggregating Counters:** `diagnostic_convolution_ops` and `diagnostic_reduction_ops` are recorded separately and confirmed **not** to aggregate into $W_{\text{ops}}$.
- **Internal Normalization Primitive Operations:** Primitive operations within normalization passes (`norm_alu_add_sub`, `norm_alu_mul`, `norm_table_lookup`, `norm_branch_cond`) are tracked to provide full cost visibility without altering $W_{\text{ops}}$.
- **Memory Traffic:** Tracked independently via `mem_loads`, `mem_stores`, `bytes_read`, `bytes_written`.

---

## 6. Execution Budget Accounting

The executable code enforces a hard ceiling of **1,000 cumulative trials maximum** via `TrialBudgetEnforcer`:
- **Allowed Ceiling:** 1,000 cumulative trials.
- **Executed in Gate 1 Smoke Suite:** Exactly **179 trials** (10 Tier 1 sequences + 15 Tier 1 deterministic replay sequences + 140 Tier 2 band trials + 14 deterministic exact-count trials = 179 trials, well within the budget).
- **Enforcement Mechanism:** Exceeding 1,000 calls to `record_trial()` aborts execution fail-closed via `std::exit(1)`.

---

## 7. Verification Outcomes

### Command:
```bash
cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target t81_exp_scg08_01_test
./build/experiments/scg08_arithmetic_work/t81_exp_scg08_01_test
```

### Observed Output:
```
=====================================================================
 SCG-08 Phase 1 Gate 1: Carry Propagation & Arithmetic Work Harness
 Specification: T81-SCG-08-PHASE1-PROP-001-R3-ADDENDUM-06
 Parent Charter: SCG08_PHASE0_RESEARCH_CHARTER.md
=====================================================================
[SCG-08 GATE 1] Verifying exact accumulator-width tables per Addendum 06...
  Band B=64 bits: OpTrits=41, AddTrits=42 (2 limbs), MulTrits=81 (3 limbs)
  Band B=128 bits: OpTrits=81, AddTrits=82 (4 limbs), MulTrits=161 (6 limbs)
  Band B=256 bits: OpTrits=162, AddTrits=163 (7 limbs), MulTrits=323 (12 limbs)
  Band B=512 bits: OpTrits=324, AddTrits=324 (12 limbs), MulTrits=646 (24 limbs)
  Band B=1024 bits: OpTrits=647, AddTrits=647 (24 limbs), MulTrits=1292 (48 limbs)
[SCG-08 GATE 1] Testing Tier 1 Markov carry dynamics (smoke test)...
  Ternary f_gen|0: 0.243243, f_occ: 0.29, mean_run_len: 1.61111
  Binary f_gen|0: 0.272727, f_occ: 0.47, mean_run_len: 3.13333
[SCG-08 GATE 1] Testing Tier 2 multi-precision arithmetic across all 5 bands...
  Verifying Band B=64 bits (TernaryLimbs: In=2, Add=2, Mul=3; BinaryLimbs: In=1, Add=2, Mul=2)...
  Verifying Band B=128 bits (TernaryLimbs: In=3, Add=4, Mul=6; BinaryLimbs: In=2, Add=3, Mul=4)...
  Verifying Band B=256 bits (TernaryLimbs: In=6, Add=7, Mul=12; BinaryLimbs: In=4, Add=5, Mul=8)...
  Verifying Band B=512 bits (TernaryLimbs: In=12, Add=12, Mul=24; BinaryLimbs: In=8, Add=9, Mul=16)...
  Verifying Band B=1024 bits (TernaryLimbs: In=24, Add=24, Mul=48; BinaryLimbs: In=16, Add=17, Mul=32)...
[SCG-08 GATE 1] Verifying deterministic exact operation counts (zero, sparse, dense, carry-propagating)...
  Deterministic exact-count verifications passed.
[SCG-08 GATE 1] Verifying frozen weighted metric W_ops formula...
[SCG-08 GATE 1] Verifying PRNG seed 0x5C608001ULL deterministic replay...
[SCG-08 GATE 1] Running negative tests: deliberate oracle mismatch and budget violation fail closed...
  Negative tests passed: both oracle mismatch and budget overflow fail closed.
[SCG-08 GATE 1] Verifying cumulative trial budget accounting...
  Cumulative trials recorded so far: 179 / 1000
=====================================================================
 [GATE 1 RESULT] ALL SMOKE-TESTS AND VERIFICATIONS PASSED
 Cumulative Trials: 179 / 1000 (Budget Respected)
 Gate 2 Status: STRICTLY WITHHELD
=====================================================================
```

### CTest Execution:
```bash
ctest --test-dir build -R scg08 --output-on-failure
```
```
    Start 390: t81_exp_scg08_01_test
1/2 Test #390: t81_exp_scg08_01_test ................   Passed    0.39 sec
    Start 391: t81_exp_scg08_01_schema_validation
2/2 Test #391: t81_exp_scg08_01_schema_validation ...   Passed    0.28 sec

100% tests passed, 0 tests failed out of 2
```

### Schema Validation Output:
```bash
python3 scripts/ci/validate_scg08_gate1_evidence.py experiments/scg08_arithmetic_work/gate1_evidence_record.json experiments/scg08_arithmetic_work/evidence_schema.json
```
```
[SCG-08 SCHEMA VALIDATION PASSED] Artifact gate1_evidence_record.json conforms to t81.scg08.carry-experiment.v1
```

---

## 8. Remaining Limitations

1. **Emulated Binary Substrate:** All algorithms are executed on binary host CPU architectures (x86_64). Cycle and latency measurements reflect binary instruction translation overhead rather than hypothetical native ternary hardware.
2. **Gate 2 Execution Withheld:** Full-scale statistical campaigns (e.g., 136,500 sequences, cluster-robust delta variance estimation, empirical latency distributions) remain strictly withheld pending explicit maintainer authorization.
3. **No Automatic Merge:** This PR must be reviewed and accepted by a human maintainer without automatic merge.
