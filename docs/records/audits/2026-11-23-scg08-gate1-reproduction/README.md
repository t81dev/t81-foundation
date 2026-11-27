# SCG-08 Phase 1 Gate 1 reproduction

**Label: AI-assisted reproduction — not an independent review**

**Document:** `T81-SCG-08-GATE1-REPRO-2026-11-23`
**Checked against:** `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md` (`T81-SCG-08-GATE1-REPORT-001`)
**Roadmap order:** 2 in `docs/ROADMAP.md` (schema validations and smoke tests). This record does not authorize Gate 2 and does not accept Gate 1.
**Tested harness revision:** `5a005b2dea5e1eb262d4af45698f8338d16b137c` (`origin/main` at execution)
**Record commit:** the pull-request head that adds this directory. It changes no harness, schema, `W_ops` definition, preregistration, threshold, `spec/` file, or `include/t81/**` file.

## Result

The current Gate 1 schema validations and the registered smoke, rejection, and CTest suite passed on this host. The parent smoke process recorded **179 / 1000** trials and printed `Gate 2 Status: STRICTLY WITHHELD`. No Gate 2 campaign was run. No benchmark binary was run. Durations printed by CTest and unittest were discarded and are not results.

Five report claims were **NOT REPRODUCED** on this tree. They are wording, registration size, a trial-count split, and the C++ language mode. The passing gates are listed below. Historical macOS logs, the original evidence-generation execution, and Gate 2 results were **NOT TESTED**.

## Environment

Recorded in `environment.txt`.

| Item | Observed |
|---|---|
| OS | Ubuntu 24.04.5 LTS, Linux 6.12.113+, x86_64 |
| Compiler | GCC 13.3.0, `/usr/bin/g++`, target `x86_64-linux-gnu` |
| CMake | 3.28.3, `CMAKE_BUILD_TYPE=Release` |
| Compile flags | `-O3 -DNDEBUG -std=gnu++23 -fPIE -Wall -Wextra -Wpedantic` |
| Defines | `-DT81_DETERMINISTIC -DT81_HAS_NATIVE_INT128=1` |
| System Python | 3.12.3; `jsonschema` not installed |
| Validator Python | 3.12.3 with `jsonschema==4.26.0` from `scripts/ci/scg08-validator-requirements.txt` |

`T81_USE_CXX23` defaults ON, so the documented Release configure selects `-std=gnu++23`. The report's prose says GNU C++20. That difference is claim C17.

Configure and the target build both exited 0. Built targets were `t81_tracing`, `t81_core`, and `t81_exp_scg08_01_test`. GCC emitted one `-Wclass-memaccess` warning at `test_exp_scg08_01.cpp:665`. The warning is in `transcripts/build-warning.txt`. The source was left unchanged. The smoke binary still exited 0.

## Commands

Exact command list: `commands.txt`.

Differences from the report's historical cmake line, recorded so they are not hidden:

- `-DCMAKE_C_COMPILER=gcc` and `-DCMAKE_CXX_COMPILER=g++` select the compiler named above.
- `-DPython3_EXECUTABLE` points CTest at the virtualenv so the optional Draft 7 parity check runs. The system interpreter has no `jsonschema`.
- The build directory and virtualenv were outside the worktree.

Schema commands were run with and without `python -O`. Each optimized stdout was byte-identical to its unoptimized stdout.

## Counts

| Check | Count | Exit |
|---|---|---|
| Positive schema validation, normal and `-O` | 1 committed record, both runs | 0 |
| Negative schema cases | 94 rejected | 0 |
| Draft 7 parity inside the negative suite | 61 rejections and 2 positive records | 0 |
| JSON-file cases | 57 (49 exit 1, 8 exit 0), normal and `-O` | 0 |
| Parent smoke process | 179 cumulative trials, ceiling 1000 | 0 |
| Real subprocess rejections | 12 cases, each exit 1 with the required diagnostic | 0 |
| Rejection-driver unit tests | 3 methods | 0 |
| CTest `^t81_exp_scg08_01` | 8 passed, 0 failed | 0 |

CTest names, in order: `t81_exp_scg08_01_test`, `t81_exp_scg08_01_result_rejection`, `t81_exp_scg08_01_result_rejection_driver`, `t81_exp_scg08_01_schema_validation`, `t81_exp_scg08_01_schema_negative_validation`, `t81_exp_scg08_01_schema_json_files`, `t81_exp_scg08_01_schema_negative_optimized`, `t81_exp_scg08_01_schema_json_files_optimized`.

Current call-site trial sum inside the parent process, from source plus the printed total:

| Call site | `record_trial()` calls |
|---|---|
| `test_tier1_markov_dynamics`: two `simulate_ternary(5, …)` and one `simulate_binary(5, …)` | 15 |
| `test_tier2_all_bands`: 5 bands × 7 cases × 4 operations | 140 |
| `test_deterministic_exact_counts` | 14 |
| `test_deterministic_replay`: two `simulate_ternary(5, …)` | 10 |
| Total | 179 |

The rejection fixtures in the parent process do not call `record_trial()`. The budget subprocess resets the counter to 1000 and records one more trial in its own process.

## Claim ledger

Status meanings: **CONFIRMED** means this run's execution, git object, or source inspection at the tested SHA matches the claim. **NOT REPRODUCED** means this tree or this run disagrees with the claim. **NOT TESTED** means this reproduction did not execute that check. Gate 2, timing, and the macOS reconciliation logs were left untested on purpose.

### Sections 1 and 8 — scope

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C01 | An isolated Gate 1 harness is present under `experiments/scg08_arithmetic_work/` | CONFIRMED | Files exist at the tested SHA. Smoke banner names Gate 1 and Addendum 06. |
| C02 | Smoke-covered balanced-ternary and binary addition and multiplication match `ExactBigIntOracle` on the five bands | CONFIRMED | `t81_exp_scg08_01_test` exit 0 under `-DNDEBUG`. The five dimension assertions are in the binary that passed. |
| C03 | The oracle is a decoupled base-10^9 integer | CONFIRMED | `exact_oracle_adapter.hpp` sets `BASE` to `1000000000`. The evidence record names that oracle. Schema validation passed. |
| C04 | `W_ops = OP_ALU_ADD_SUB + 3*OP_ALU_MUL + OP_TABLE_LOOKUP + OP_BRANCH_COND + OP_NORM_PASS` | CONFIRMED | `OperationAccounting::compute_w_ops()` matches. The formula test requires 37 and still requires 37 after diagnostic and conversion counters are increased. |
| C05 | Diagnostic, normalization, conversion, and memory counters stay outside `W_ops` | CONFIRMED | Same formula assertion. Source lists those fields as excluded. |
| C06 | The executable ceiling is 1000 cumulative trials and overflow exits fail-closed | CONFIRMED | `GATE1_MAX_CUMULATIVE_TRIALS` is 1000. Parent printed `179 / 1000`. The budget subprocess exited 1 with `GATE 1 CUMULATIVE TRIAL BUDGET EXCEEDED`. |
| C07 | Seed `0x5C608001ULL` replay checks pass | CONFIRMED | Smoke reached and passed the replay section. |
| C08 | This harness run does not establish ternary performance superiority | CONFIRMED | No benchmark was executed. The smoke line says Gate 2 is withheld. Source text calls `W_ops` a stipulated proxy. |
| C09 | Gate 2 stays withheld | CONFIRMED | Not executed. Smoke prints `STRICTLY WITHHELD`. RFC-00E1 status text still says `STRICT NO-GO`. Roadmap order 2 says this review does not authorize Gate 2. |
| C10 | Evaluations here run on a binary host CPU | CONFIRMED | This host is x86_64. No native ternary device was used. |

### Section 2 — provenance

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C11 | Baseline `dbf107fda83a3755fa21d6b95caa3eb8ac450dd3` is the PR #763 merge and is in `main` history | CONFIRMED | `git log` subject is `Merge pull request #763`. `git merge-base --is-ancestor` is true. |
| C12 | `2dd3cc62e526942cdcbed6d100770c08c26ab2fe` is the corrected source revision named by the report, and it is in `main` history | CONFIRMED | Commit exists, is an ancestor, and is the `evaluated_implementation_sha` stored in `gate1_evidence_record.json`. This does not show that commit generated the historical counters. |
| C13 | PR #766 base/head are `36b3849aa1e6297e88de2ec1655777793ab0b59e` / `a763ab6876bf07b8ae7c529656cdd2e5d9cf3766` | CONFIRMED | GitHub PR #766 reports those OIDs. State is CLOSED, `mergedAt` is null. The head is not an ancestor of tested `main`. |
| C14 | Isolated reconciliation baseline `89223724038ff7dec349343469df4ea8389ede26` is in `main` history | CONFIRMED | Ancestor. Subject is `Merge pull request #772`. |
| C15 | `evidence_generation_sha` is null and the generating execution is unverified | CONFIRMED | JSON value is null. No execution receipt binding the historical counters to a generating SHA was found or created. |
| C16 | PR #753 is in `main` at `0bd0817e4b9dc879fffce18c59ea9ff781f7acb3` | CONFIRMED | GitHub reports that merge commit. It is an ancestor. |
| C17 | The historical environment sentence "GNU C++20 (GCC 13.3.0), Python 3.12" describes this documented configure | NOT REPRODUCED | GCC is 13.3.0 and Python is 3.12.3. The Release build's compile line is `-std=gnu++23`, because `T81_USE_CXX23` defaults ON. Whether a person on 2026-11-10 actually selected C++20 was not re-established (see C40). |
| C18 | The PR #766 range touches the four files named in the report inventory, and commit `2dd3cc62` does not modify `include/t81/**` or `src/core/**` | CONFIRMED | `git diff --name-only 36b3849 a763ab` is the report, schema, evidence record, and validator. `git diff-tree` of `2dd3cc62` has no `include/t81` or `src/core` path. |

### Section 3 — width table

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C19 | The five signed bands print operand/add/mul trit counts 41/42/81, 81/82/161, 162/163/323, 324/324/646, 647/647/1292 | CONFIRMED | `transcripts/smoke.stdout` lines for bands 64 through 1024. |
| C20 | Ternary limbs Op/Add/Mul and binary limbs Op/Add/Mul match the report table | CONFIRMED | Same smoke log: ternary `In/Add/Mul` and binary `In/Add/Mul` match every row. `test_accumulator_width_specification()` ran inside the passing binary. |

### Section 4 — oracle dimensions and fail-closed mismatches

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C21 | Numerical equality, overflow classification, canonical digits, sign handling, and absence of truncation are fail-closed acceptance checks | CONFIRMED | Passing parent assertions, plus 11 oracle subprocesses. |
| C22 | A deliberate oracle mismatch exits 1 and prints `INVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH)` | CONFIRMED | `transcripts/result-rejection.stdout`: 11 oracle cases, each "rejected with exit 1 and required diagnostic". |
| C23 | The parent smoke transcript says "Negative tests passed: both oracle mismatch and budget overflow fail closed." | NOT REPRODUCED | Current smoke text is "Checking deliberate 5-D oracle corruption fixtures" and "process rejection checks are registered separately in CTest." The fail-closed behavior itself is C22 and C24. |

### Section 5 — accounting

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C24 | Budget overflow exits 1 with `GATE 1 CUMULATIVE TRIAL BUDGET EXCEEDED` | CONFIRMED | Twelfth rejection case. Source resets the counter to 1000 and then records one trial. |
| C25 | `mul64` counts 4 multiplications and 4 helper additions, and the caller counts 4 accumulator additions before carry propagation | CONFIRMED | Source increments match that split. Sparse mul requires 4 muls and 8 adds. Dense mul requires 4 muls and 9 adds, the extra add being the carry loop. |
| C26 | Zero, sparse, dense, and carry-propagating exact-count checks pass, including binary same-sign `W_ops == 4 * B_In + 1` | CONFIRMED | Smoke printed "Deterministic exact-count verifications passed." Those literals are `SCG08_REQUIRE` checks in the binary that exited 0. |
| C27 | `T81Limb27::addc()` performs 38 named table lookups, and `add_ternary` adds 38 per `addc()` | CONFIRMED | At `include/t81/types/T81Limb.hpp` `T81Limb27::addc`, `TRYTES` is 9: 9 `ADD_TABLE`, 8 + 7 + 5 `COMPOSITION_TABLE`, 8 `CARRY_FROM_ZERO` on `map_ids[0]` through `map_ids[7]`, and 1 `CARRY_FROM_ZERO[map_ids[8]]`. The harness adds 38. Passing checks require 76 (`2 * 38`) and 114 (`3 * 38`). This inspection did not edit `include/t81/**`. |
| C28 | The committed evidence record stores 179 trials, timestamp `2026-11-10T22:30:00Z`, and the historical `W_ops` column | CONFIRMED | JSON `actual_cumulative_trials_executed` is 179. Historical `w_ops_add_ternary` is 80, 161, 359, 714, 1350. Historical `w_ops_add_binary` is 4, 6, 10, 18, 34. `w_ops_mul_ternary` is 5826, 17218, 79750, 296182, 1208742. `w_ops_mul_binary` is 29, 103, 389, 1513, 5969. The file was not modified. |
| C29 | The report's split "10 Tier 1 + 15 Tier 1 replay + 140 Tier 2 + 14 exact-count" describes the current call sites | NOT REPRODUCED | The sum is 179, and the fresh parent printed 179. Current sites are 15 + 10 + 140 + 14. The 10-versus-15 labels do not match `test_tier1_markov_dynamics` and `test_deterministic_replay`. |
| C30 | The negative schema suite rejects `operand_trits: null` and `ternary_limbs_op: "not an integer"`, and Draft 7 parity runs when `jsonschema` is present | CONFIRMED | 94 rejections. Stderr contains both of those property errors. Parity printed "61 rejection cases and 2 positive records passed" and the integral-float boundary sentence. `-O` stdout matched. |

### Section 6 — budget and historical artifact

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C31 | A fresh parent smoke execution on this host prints 179 / 1000 | CONFIRMED | `transcripts/smoke.stdout`. |
| C32 | The JSON's 179 trials and counters are a retained historical artifact, not a newly written campaign, and publication SHA omission is valid | CONFIRMED | This run did not rewrite the JSON. `evidence_publication_sha` is absent. Positive validation exited 0. The negative suite accepts omission and one valid SHA. |
| C33 | Tier 1 smoke figures on this GCC run match the figures quoted in section 7 | CONFIRMED | Ternary `f_gen\|0=0.243243`, `f_occ=0.29`, mean run `1.61111`. Binary `f_gen\|0=0.272727`, `f_occ=0.47`, mean run `3.13333`. |
| C34 | The validator success sentence is `conforms to t81.scg08.carry-experiment.v1` | NOT REPRODUCED | Current sentence, normal and `-O`: `passes supported schema checks and domain rules for t81.scg08.carry-experiment.v1`. Exit code is still 0. |

### Section 7 — quoted historical transcripts

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C35 | The section 7 CTest transcript is the current registered suite: exactly 3 tests, numbers 390–392 | NOT REPRODUCED | Current CTest selected 8 tests, numbers 399–406, all passed. The three historical names are the first, fourth, and fifth of those eight. |
| C36 | Section 7 elapsed times (0.40 s, 0.31 s, 0.31 s) are reproduced | NOT TESTED | This reproduction discarded runner durations and made no timing comparison. |
| C37 | The quoted section 7 block is an authenticated execution receipt for a specific past process | NOT TESTED | The report labels that block as preserved output, not a new receipt. This run did not recover an original process record. Current smoke text matches the width, Tier 1, band, exact-count, replay, 179/1000, and Gate 2 lines, and differs at C23 and C34. |

### Section 9 — macOS reconciliation packet

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C38 | macOS ARM64, Apple clang 21.0.0, Python 3.14.3, and the three `/tmp` log SHA-256 values were re-executed | NOT TESTED | This host is Linux GCC. Those log files are not in the tree. No clang reproduction was run. |
| C39 | That packet's statement that Draft 7 parity was NOT RUN | NOT TESTED | Scoped there to a missing dependency. This run installed `jsonschema==4.26.0` and parity ran (C30). |
| C40 | The 2026-11-10 command/environment pair is an authenticated historical execution | NOT TESTED | The report says that reconciliation does not establish it. This run did not create that missing receipt. |
| C41 | The macOS fresh Tier 1 figures `f_gen\|0=0.1875`, `f_occ=0.2`, mean `1.33333` and binary `0.326923` / `0.51` / `3` | NOT TESTED | Different OS and standard library. This GCC run matched the retained artifact instead (C33). The cause of the earlier difference was not investigated. |
| C42 | That packet ran no full CMake/CTest integration | NOT TESTED | Statement about that worktree. This record's own CTest run is separate and passed 8/8. |

### Section 10 — reconciled counts and governance

| ID | Claim | Status | Evidence |
|---|---|---|---|
| C43 | Reconciled B=64 `val_max` counts in the section 10 table: ternary add `W_ops` 80, binary add 5, ternary mul 5826, binary mul 29 | CONFIRMED | Dense-case `SCG08_REQUIRE` literals in the passing binary. |
| C44 | Reconciled same-sign binary add `W_ops` is 5, 9, 17, 33, 65 across the five bands | CONFIRMED | The band loop requires `4 * B_In + 1` for same-sign inputs, including `val_max + val_max`, inside the passing binary. |
| C45 | Reconciled ternary-add `W_ops` integers 161, 361, 720, and 1360 for bands 128, 256, 512, and 1024 | NOT TESTED | The band loop enforces the closed form `op_alu_add_sub = InLimbs + 2*K_cin` and table lookups in multiples of 38. It does not require those four integers. They were not printed. The JSON still stores the historical column 161, 359, 714, 1350. |
| C46 | Absolute `mul_ternary` / `mul_binary` `W_ops` for bands 128, 256, 512, and 1024 | NOT TESTED | No literal assertion prints or requires 17218, 79750, 296182, 1208742 or 103, 389, 1513, 5969. Those values are present as the unmodified historical JSON (C28). B=64 is C43. |
| C47 | RFC-00E1 remains draft, formal Gate 1 acceptance remains withheld, and Gate 2 remains strict no-go | CONFIRMED | `spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` status is `draft`, and its governance notice says acceptance is pending and Gate 2 is `STRICTLY WITHHELD / STRICT NO-GO`. This record does not change that file and does not sign the gate. |
| C48 | Section 10's code-behavior repairs (five-field oracle report, same-sign binary add counts, ternary add `InLimbs + 2*K_cin`, conversion counters excluded from `W_ops`, mul64's 4+4 split) hold in the passing smoke binary | CONFIRMED | Those checks are `SCG08_REQUIRE` conditions in the binary that exited 0, and the corresponding source is present in `tier2_work_harness.hpp`. |

## What was left untested

- Gate 2, including any 136,500-sequence campaign, cluster-robust variance, latency distribution, or hardware timing.
- Every CTest or unittest duration. The retained CTest transcript in `transcripts/ctest.stdout` has the elapsed column removed. The driver summary keeps `Ran 3 tests` and `OK` and drops the duration clause.
- The macOS clang reconciliation, its three log hashes, and any claim that depends on Apple libc++.
- Authentication of the null `evidence_generation_sha`.
- A second configure with `T81_USE_CXX23=OFF`.
- Absolute reconciled ternary-add and wider-band multiply `W_ops` integers in C45 and C46.
- Hosted CI for this record. That result belongs to the pull request, not to this local packet.

## Boundaries kept

No file under `spec/` or `include/t81/**` was edited. `W_ops`, the preregistration, accepted thresholds, and `gate1_evidence_record.json` were not edited. No gate assertion was weakened or skipped. The optional Draft 7 check was executed by installing the repository's existing pin. The parent trial count stayed at 179, under the 1000 ceiling. The budget failure ran in a child process.

`reproducibility-report.json` is a proposed Research Commons schema instance stored with this packet. Its `repository_revision` is the tested harness SHA, not the commit that adds the packet. Status remains `proposed`. It is not an accepted finding and it is not an independent review.
