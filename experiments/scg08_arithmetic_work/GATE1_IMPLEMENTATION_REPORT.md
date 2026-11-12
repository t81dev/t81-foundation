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

## 2. Repository Context & Provenance

- **Repository:** https://github.com/t81dev/t81-foundation
- **Baseline Git SHA:** `dbf107fda83a3755fa21d6b95caa3eb8ac450dd3` (merging PR #763)
- **Evaluated Implementation SHA:** `2dd3cc62e526942cdcbed6d100770c08c26ab2fe` (corrected source revision reviewed; not an independently established historical execution revision)
- **Original PR #766 base/head:** `36b3849aa1e6297e88de2ec1655777793ab0b59e` / `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766`.
- **Isolated reconciliation baseline:** `89223724038ff7dec349343469df4ea8389ede26`.
- **Actual historical evidence-generation revision:** **UNVERIFIED** (`evidence_generation_sha: null`); no independently traceable execution receipt binds the observations to a generating source revision.
- **PR #753 Integration:** Verified integrated into mainline history (`0bd0817e4b9dc879fffce18c59ea9ff781f7acb3`).

### Changed File Inventory
| File | Action | Purpose |
| :--- | :--- | :--- |
| `experiments/scg08_arithmetic_work/evidence_schema.json` | Modified | Updated provenance model (`baseline_sha`, `evaluated_implementation_sha`) and enforced scalar band property integer types |
| `experiments/scg08_arithmetic_work/gate1_evidence_record.json` | Modified | Recorded explicit revision provenance separating baseline and evaluated implementation revisions |
| `scripts/ci/validate_scg08_gate1_evidence.py` | Modified | Enforced strict non-negative integer type checks on scalar band properties; added negative tests reproducing `operand_trits: null` and `ternary_limbs_op: "not an integer"` |
| `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` | Modified | Documented findings resolution, exact provenance, and reproduction instructions |

**Production Impact:** Zero modifications to production arithmetic files (`include/t81/types/`, `src/core/`, etc.). All additions are isolated under `experiments/scg08_arithmetic_work/` and `scripts/ci/`.

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
1. **Finding 1 (P1 - Binary Primitive Arithmetic Accounting in `mul64()` and caller):**
   - *Audit:* Evaluated `mul64()` and caller accumulator additions in `mul_binary()`. `mul64()` decomposes 64x64 into 4 32x32 multiplications (`u0*v0`, `u1*v0`, `u0*v1`, `u1*v1`) and 4 helper 64-bit additions (`u1*v0 + (w0 >> 32)`, `w1 += u0*v1`, `w2 += (w1 >> 32)`, `hi = u1*v1 + w2`). In the caller loop, each limb product adds `lo` and `carry` into the result accumulator (`sum1 = cur + lo`, `sum2 = sum1 + carry`) and accumulates carry (`carry = hi + c1 + c2`), totaling 4 caller additions per inner iteration (8 additions per inner product step before carry propagation).
   - *Resolution:* Reconciled `acc.op_alu_mul` and `acc.op_alu_add_sub` in `mul64()` and `mul_binary()` to track all executed primitive arithmetic operations without distorting accounting semantics.
2. **Finding 2 (P2 - Binary Condition Evaluation Accounting):**
   - *Audit:* Audited executed conditional evaluations across zero, sparse, dense, and carry-propagating inputs.
   - *Resolution:* Tracked condition evaluations consistently:
     - In `mul_binary()`: outer loop `ai == 0`, inner loop `i + j >= OutLimbs`, inner carry predicates `sum1 < cur` and `sum2 < sum1`, carry propagation checks `carry == 0` and `sum < cur`, result-zero limb check `res.limbs[i] != 0`, and final sign normalization `if (all_zero)`.
     - In `add_binary()`: sign check `a.is_negative == b.is_negative`, carry check `carry != 0` and predicate `sum <= a.limbs[i]` vs `sum < a.limbs[i]`, and subtraction magnitude comparisons (`va != vb`, `va < vb`, `c == 0`, `c > 0`, borrow predicates).
3. **Finding 3 (P1 - Evidence Trial-Count Integrity & Provenance):**
   - *Audit:* Evaluated committed evidence record against standalone test execution. The initial draft stored 30 trials from an isolated band run, conflicting with the 179 trials executed by the smoke suite.
   - *Resolution:* Reconciled `gate1_evidence_record.json` to record the exact 179 cumulative trials executed by the smoke suite (`actual_cumulative_trials_executed: 179`), verified against the ceiling of 1,000 trials, with recorded timestamp preserved. The baseline and corrected source identities are recorded separately; the actual historical evidence-generation revision remains unverified.
4. **Finding 4 (P2 - Evidence Schema Validation & Negative Test Suite):**
   - *Resolution:* Hardened `scripts/ci/validate_scg08_gate1_evidence.py` to enforce strict regex pattern matching on commit SHAs (40-char hex) and ISO-8601 UTC timestamps, exact specification version string matching, strictly positive costs, and non-empty dimension strings. Added a comprehensive negative test suite (`--test-negative`) registered in CTest (`t81_exp_scg08_01_schema_negative_validation`).
   - *Post-PR #765 Review Findings Resolution:*
     - **P1 (Evidence revision provenance):** Repaired `git_context` to separate `baseline_sha` (`dbf107fda83a3755fa21d6b95caa3eb8ac450dd3`) from `evaluated_implementation_sha` (`2dd3cc62e526942cdcbed6d100770c08c26ab2fe`), avoiding self-referential commit hashes by documenting the publication revision relationship non-cyclically.
     - **P2 (Scalar schema validation):** Hardened validator to enforce explicit scalar integer type checks on all band properties (`bit_width`, `operand_trits`, `add_trits_min`, `mul_trits_min`, `ternary_limbs_op`, `ternary_limbs_add`, `ternary_limbs_mul`, `w_ops_*`), rejecting malformed properties like `operand_trits: null` and `ternary_limbs_op: "not an integer"`. JSON Schema parity is checked when `jsonschema` is available, except that Draft 7 accepts integral floats as mathematical integers while the strict Python validator rejects all floats.
5. **Table Lookup Accounting Reconciliation (P1):**
   - *Source Verification:* In `include/t81/types/T81Limb.hpp`, each `T81Limb27::addc()` performs:
     - 9 lookups in `detail::ADD_TABLE[trytes_[i] + 13][other.trytes_[i] + 13]` (1 per tryte)
     - 8 lookups in `detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 1]]` (for $i=1..8$)
     - 7 lookups in `detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 2]]` (for $i=2..8$)
     - 5 lookups in `detail::COMPOSITION_TABLE[map_ids[i]][map_ids[i - 4]]` (for $i=4..8$)
     - 8 lookups in `detail::CARRY_FROM_ZERO[map_ids[i]]` (carries array for $i=1..8$)
     - 1 lookup in `detail::CARRY_FROM_ZERO[map_ids[8]]` (carry_out computation)
     - **Exact Sum:** $9 + 8 + 7 + 5 + 8 + 1 = 38$ table lookups per `T81Limb27::addc()` invocation.
   - *Resolution:* Reconciled `acc.op_table_lookup` in `ArithmeticWorkHarness::add_ternary` to increment by 38 per `addc()` call (for both initial limb pairs and nested carry-in limbs `sum_ab.addc(c_limb)`).

Accounting guarantees enforced:
- **Diagnostic Non-Aggregating Counters:** `diagnostic_convolution_ops` and `diagnostic_reduction_ops` are recorded separately and confirmed **not** to aggregate into $W_{\text{ops}}$.
- **Internal Normalization Primitive Operations:** Primitive operations within normalization passes (`norm_alu_add_sub`, `norm_alu_mul`, `norm_table_lookup`, `norm_branch_cond`) are tracked to provide full cost visibility without altering $W_{\text{ops}}$.
- **Memory Traffic:** Tracked independently via `mem_loads`, `mem_stores`, `bytes_read`, `bytes_written`.

---

## 6. Execution Budget Accounting & Evidence Provenance

The executable code enforces a hard ceiling of **1,000 cumulative trials maximum** via `TrialBudgetEnforcer`:
- **Allowed Ceiling:** 1,000 cumulative trials.
- **Historical Recorded Output in Evidence Artifact:** Exactly **179 trials** (10 Tier 1 sequences + 15 Tier 1 deterministic replay sequences + 140 Tier 2 band trials + 14 deterministic exact-count trials = 179 trials, well within the budget).
- **Enforcement Mechanism:** Exceeding 1,000 calls to `record_trial()` aborts execution fail-closed via `std::exit(1)`.

### 6.1 Evidence Provenance & Historical Artifact Boundary
- **Historical Output Designation:** The 179 cumulative trials and measured operation counters stored in `gate1_evidence_record.json` are historical recorded observations. Commit `2dd3cc62e526942cdcbed6d100770c08c26ab2fe` (`evaluated_implementation_sha`) identifies corrected source reviewed, not a proven generating execution. The actual evidence-generation revision remains **UNVERIFIED** (`evidence_generation_sha: null`). They are retained for schema conformance and non-normative review, not claimed as a newly executed full campaign.
- **Historical Reported Execution Command (not an independently traceable receipt):**
  ```bash
  cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build --target t81_exp_scg08_01_test
  ./build/experiments/scg08_arithmetic_work/t81_exp_scg08_01_test
  ```
- **Historically Claimed Environment:** Ubuntu Linux x86_64, GNU C++20 (GCC 13.3.0), Python 3.12. This reconciliation does not independently establish that historical environment.
- **Non-Self-Referential Provenance:** The evidence artifact omits `evidence_publication_sha` to prevent circular self-referential hash cycles, recording `baseline_sha` and `evaluated_implementation_sha` as source context. Optional publication identity is validated when present; omission is valid. Publication identity is not an execution receipt.

---

## 7. Historical Reported Verification Outcomes (not rerun receipts)

### Command:
```bash
cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target t81_exp_scg08_01_test
./build/experiments/scg08_arithmetic_work/t81_exp_scg08_01_test
```

### Historical Reported Output (preserved; not a new execution receipt):
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
1/3 Test #390: t81_exp_scg08_01_test .........................   Passed    0.40 sec
    Start 391: t81_exp_scg08_01_schema_validation
2/3 Test #391: t81_exp_scg08_01_schema_validation ............   Passed    0.31 sec
    Start 392: t81_exp_scg08_01_schema_negative_validation
3/3 Test #392: t81_exp_scg08_01_schema_negative_validation ...   Passed    0.31 sec

100% tests passed, 0 tests failed out of 3
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


## 9. Isolated PR #766 Reconciliation Verification

This section records the bounded reconciliation run separately from the historical
output above. It neither ratifies draft RFC-00E1 nor establishes historical Gate 1
authorization, formal Gate 1 acceptance, or Gate 2 execution authorization.

- Remote `main` was confirmed at `89223724038ff7dec349343469df4ea8389ede26`;
  the original PR #766 branch remained at `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766`.
  The original PR comparison base is `36b3849aa1e6297e88de2ec1655777793ab0b59e`.
- The four-file repair was applied with `git merge --squash --no-commit` in
  `/tmp/t81-scg08-evidence-reconciliation` on
  `codex/scg08-gate1-evidence-reconciliation`, based on the pinned main revision.
  Git reported a clean merge; no unmerged index entries or conflict resolutions.
- Historical JSON values outside `git_context` were compared structurally with
  the pinned baseline and are unchanged, including timestamp
  `2026-11-10T22:30:00Z`, measured counters, and the 179-trial record.
- Validation environment: macOS ARM64, Apple clang 21.0.0
  (`clang-2100.1.1.101`, target `arm64-apple-darwin25.5.0`), Python 3.14.3.

Executed commands (from the isolated worktree):

```bash
python3 scripts/ci/validate_scg08_gate1_evidence.py
python3 scripts/ci/validate_scg08_gate1_evidence.py --test-negative
clang++ -std=c++20 -O2 -DNDEBUG -I include experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp -o /tmp/t81-scg08-gate1-reconciliation-test
/tmp/t81-scg08-gate1-reconciliation-test
```

Results:

- Stdlib evidence validation: PASS.
- Negative schema suite: 94 rejection cases PASS, retaining the source PR's cases
  and adding all 11 scalar fields crossed with boolean, null, string, fractional
  float, negative integer, and integral-float inputs. Optional publication SHA
  omission and a valid SHA both PASS; malformed values, including a trailing
  newline, are rejected. Missing/invalid generation revision fields are rejected;
  explicit null preserves unverified historical provenance.
- Optional `jsonschema.Draft7Validator` parity: NOT RUN, dependency unavailable
  in the available Python environments. The suite retains conditional parity
  checks for 61 malformed scalar/publication cases and two positive records.
  Draft 7 accepts integral floats as mathematical integers; the strict Python
  validator deliberately rejects every float. Full validator/schema equivalence
  is not claimed.
- Targeted standalone arithmetic regression: PASS under `-DNDEBUG`, all five
  bands, exact accounting, oracle checks, replay checks, and both existing
  fail-closed subprocess tests. Parent workload: 179/1,000 trials. The budget
  negative subprocess attempts counter 1,001 and exits as required; this is not
  an additional arithmetic campaign. No full CMake/CTest integration run was made.
- Fresh Tier 1 smoke observations differ from the historical artifact: ternary
  `f_gen|0=0.1875`, `f_occ=0.2`, mean run `1.33333`; binary
  `f_gen|0=0.326923`, `f_occ=0.51`, mean run `3`. The source uses
  `std::uniform_int_distribution`; this run does not establish the cause of the
  discrepancy or cross-library reproducibility. Historical values were retained
  unchanged. No comparative scientific conclusion is drawn from either smoke run.

Raw local validation logs (temporary review artifacts; not committed):

- `/tmp/t81-scg08-reconciliation-schema.log` — SHA-256 `b6bfdd65661223786be3592b412ba9125da7161133adca418b13ab9a4cd27e8e`.
- `/tmp/t81-scg08-reconciliation-negative.log` — SHA-256 `532d3f1107843cbbf566eff335dea162c76655d8af0761bada605b992d3e225f`.
- `/tmp/t81-scg08-reconciliation-arithmetic.log` — SHA-256 `ce3982d46aa5066fb23e11e1c4e78c17bc961473e61e0b0d333cf1f8384b794d`.


## 10. RFC-00E1 Scientific Contract Corrections (`d3a6cf55517a3098d7bae91b07c72694bc3472da`)

Following independent Codex review of baseline `d3a6cf55517a3098d7bae91b07c72694bc3472da` (`CORRECTIONS REQUIRED`), the five substantive findings were resolved without altering historical evidence in `gate1_evidence_record.json` or retrospectively changing preregistered thresholds:

1. **P1 — Oracle Validation (`exact_oracle_adapter.hpp`, `tier2_work_harness.hpp`, `test_exp_scg08_01.cpp`):**
   - Implemented `evaluate_oracle_validation_report()` populating all five `OracleValidationReport` fields (`numerical_equality`, `overflow_classification`, `canonical_digits`, `sign_handling`, `absence_of_truncation`) across both input operands and computed ternary/binary addition and multiplication results.
   - Enforced result-level overflow classification against $\mathcal{D}_{\text{signed},B} = [-2^{B-1}, 2^{B-1}-1]$ and accumulator bounds, stored `T81Limb27` tryte range $[-13, +13]$ plus canonical output trit vector equality against `oracle_to_canonical_trits<N>()`, result sign correctness (leading non-zero trit sign and binary sign-magnitude invariants rejecting `-0`), and absence of high-order trit/limb truncation.
   - Exercised `oracle_rep.all_passed()` as an active fail-closed acceptance condition and added deterministic negative tests for all five dimensions.
2. **P1 — Operation-Accounting Consistency (`RFC-00E1`, `tier2_work_harness.hpp`):**
   - Removed the claim that $W_{\text{ops}}$ represents unbiased total computational work, documenting it as a stipulated kernel event proxy alongside the separate H02 carry-resolution timing model and elementary operations-per-bit model.
   - Documented the multiplication weight $3$ on `op_alu_mul` as a stipulated modeling convention and disclosed the operations inside `T81Limb27::addc()`, `mul64()`, normalization, and conversion that are excluded from $W_{\text{ops}}$.
3. **P1 — Counter Correctness (`tier2_work_harness.hpp`, `test_exp_scg08_01.cpp`):**
   - Reconciled `add_binary` same-sign addition to count both primitive additions in `a.limbs[i] + b.limbs[i] + carry` (`2 * InLimbs` in `op_alu_add_sub`) and both conditional evaluations per input limb (`carry != 0` and the selected carry predicate `sum <= a.limbs[i]` or `sum < a.limbs[i]`, giving `1 + 2 * InLimbs` in `op_branch_cond`), while removing the `op_alu_add_sub` increment from pure extension-limb data copies (`res.limbs[i] = carry`).
   - Reconciled `add_ternary` to count the scalar carry combination `carry_in = c1 + c2` (`op_alu_add_sub += 1` per active carry-in limb $K_{\text{cin}}$) and removed the `op_alu_add_sub` increment from extension-limb copy placement (`res.limbs[i] = c_limb`), giving closed-form $\text{op\_alu\_add\_sub} = \text{InLimbs} + 2 K_{\text{cin}}$ and $W_{\text{ops}} = 39\,\text{InLimbs} + 40\,K_{\text{cin}} + \text{OutLimbs}$.
4. **P1 — Ternary Conversion Costs (`tier2_work_harness.hpp`, `RFC-00E1`, `test_exp_scg08_01.cpp`):**
   - Added non-aggregating diagnostic conversion counters (`conv_limb_unpacks`, `conv_limb_repacks`, `conv_tryte_decodes`, `conv_tryte_encodes`) to `OperationAccounting` to audit `to_trits()`, `from_trits_window()`, and `from_trits()` repacking costs without silently altering $W_{\text{ops}}$ or historical evidence.
5. **P2 — Binary Multiplication (`RFC-00E1`, `tier2_work_harness.hpp`, `test_exp_scg08_01.cpp`, `GATE1_IMPLEMENTATION_REPORT.md`):**
   - Reconciled contract and explanatory text with `mul64()` control flow: 4 primitive 32x32 multiplications and 4 helper 64-bit additions in `mul64()`, plus 4 caller accumulator additions in `mul_binary()`, totaling 8 additions per inner product step before carry propagation.

### Preserved Historical Evidence vs. Reconciled Deterministic Expected Counts (`val_max`, `val_max`)
| Band ($B$) | Historical `add_ternary` $W_{\text{ops}}$ (`2dd3cc62e5`) | Reconciled `add_ternary` $W_{\text{ops}}$ (`op_alu_add_sub`) | Historical `add_binary` $W_{\text{ops}}$ (`2dd3cc62e5`) | Reconciled `add_binary` $W_{\text{ops}}$ (`op_alu_add_sub` / `op_branch_cond`) | `mul_ternary` $W_{\text{ops}}$ (Unchanged) | `mul_binary` $W_{\text{ops}}$ (Unchanged) |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **64** | 80 | 80 (2) | 4 | 5 (2 / 3) | 5,826 | 29 |
| **128** | 161 | 161 (5) | 6 | 9 (4 / 5) | 17,218 | 103 |
| **256** | 359 | 361 (12) | 10 | 17 (8 / 9) | 79,750 | 389 |
| **512** | 714 | 720 (24) | 18 | 33 (16 / 17) | 296,182 | 1,513 |
| **1024** | 1,350 | 1,360 (44) | 34 | 65 (32 / 33) | 1,208,742 | 5,969 |

*Governance Posture:* `RFC-00E1` remains **DRAFT**; formal Gate 1 acceptance remains **WITHHELD**; Gate 2 remains **STRICT NO-GO**.
