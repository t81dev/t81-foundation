# T81-BASELINE-001 Final Evidence Reconciliation Report

**Document Identifier:** T81-BASELINE-001-FINAL-RECONCILIATION
**Pinned Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Candidate Evaluation Commit:** `bcd1a70260123c6eb1f0dfdb78b15599ef79c8da`
**Intended Submission Branch:** `jules-9454522404953220260-f8b072a2` (PR #663 candidate branch)
**Date:** November 2026
**Scope:** `docs/research/baseline-assessment/`
**Status:** Independent Review Candidate

---

## 1. Executive Summary

This final evidence reconciliation closes the remaining evidence attribution, benchmark provenance, numerical verification, and reproducibility defects identified during independent review of Mission T81-BASELINE-001. All 7 deliverable documents under `docs/research/baseline-assessment/` have been reconciled against primary repository artifacts, raw benchmark logs, and verified CMake build definitions at pinned baseline commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`.

Crucially, this reconciliation:
1. Withdraws unverified claims of recursive cognition coordination tax (>96% tax, 7x–66x overhead) in H09-E1 due to absence of primary execution artifacts or runner scripts in tree.
2. Reconciles tritwise workload benchmarks against primary archive logs, correcting fabricated numbers (1.48 µs, 45.2 ns) with ground-truth archive timings across `BM_MaskApply`, `BM_PatternMatch`, `BM_CanonPipeline`, and `BM_TensorLane`.
3. Distinguishes floating-point conversion corpora (24 diagnostic vectors in `float_conversion_audit/corpus.json` vs 104 checked-contract cases in `checked_float_contract/corpus.json`), marks the disposition as `UNRESOLVED`, and clarifies that the claimed 10,000-vector corpus was an unpersisted synthetic/declared claim.
4. Corrects CMP3 decision advantage claims to reflect canonical `RESULTS.json` evidence (`measurements_collected: false, measurement_authorized: false, pmu_cycles: null, linux: null`), categorizing performance results as `PMU_UNAVAILABLE` and eliminating claims of hardware counter parity or multi-runner reproduction.
5. Replaces nonexistent `./build/benchmarks/t81_benchmarks` invocations with the declared CMake executable target `./build/benchmarks/benchmark_runner` and aligns benchmark filters to registered benchmark names in C++ sources.
6. Reconciles workload descriptions in `BM_LimbMul` (Booth multiplication on limbs vs 64-bit uint64 scalar multiplication across 4,096 ops/iter) and `BM_VMSimulation` (synthetic 1,000-operation `OP_TAND` dispatch loop rather than an application-level benchmark).
7. Documents explicit RFC counting rules separating catalog status (110 rows in `index.md`), frontmatter status (83 accepted, 11 superseded, 11 unparsed, 8 draft across 113 files), and implementation maturity (79 Tested, 23 Specified, 11 Superseded).

---

## 2. Auditable Findings Reconciliation Table

| Finding ID | Original Claim | Primary Evidence | Corrected Disposition | Affected Files | Validation Method | Remaining Limitation |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **FINDING-01 (H09-E1)** | H09-E1 was EXECUTED (Tier 3); claimed Arm C incurs >96% coordination tax and 7x–66x overhead vs centralized planner | `docs/research/H09_RECURSIVE_PROCESS_INTELLIGENCE.md`; no directory `experiments/h09_process_recursion` or runner script exists at baseline | **UNEXECUTED / PROPOSED (Tier 1)**; unverified numerical claims withdrawn; classified as protocol specification only | `BENCHMARK_LEDGER.csv`, `METHODOLOGY.md`, `HYPOTHESIS_STATUS.md`, `GAPS_AND_PRIORITIES.md`, `README.md` | Inspected repository tree; verified absence of runner script and execution artifacts | Primary execution runner script was never committed to repository tree |
| **FINDING-02 (Tritwise Workloads)** | `BM_CanonPipeline_Library`: 1.48 µs vs `BM_CanonPipeline_Scalar`: 3.12 µs; `BM_TensorLane_Library`: 45.2 ns vs `BM_TensorLane_Scalar`: 89.1 ns | `benchmarks/results/archive/benchmark_results_clean.txt`, `benchmark_results_fastpath.txt`, `benchmark_tensorlane.txt` | **RECONCILED TO RAW LOGS**: `BM_MaskApply`: 1.07 µs vs 2.76 µs; `BM_PatternMatch`: 2.83 µs vs 4.88 µs; `BM_CanonPipeline`: 3.18 µs vs 2.88 µs (fastpath: 3.12 µs vs 3.97 µs); `BM_TensorLane`: 10.84 µs vs 7.35 µs (tensorlane log: 10.81 µs vs 9.70 µs). Replaced unverified 1.48 µs and 45.2 ns claims. | `BENCHMARK_LEDGER.csv` | Exact substring and number match against primary archive txt logs | Archive logs reflect historical unvectorized baseline variations across multiple dates |
| **FINDING-03 (Float Conversion)** | EXP-FLOAT-CONVERSION independently reproduced across environments (Tier 4); verified 10,000 corpus vectors against exact reference oracle | `experiments/float_conversion_audit/corpus.json` (24 vectors); `experiments/checked_float_contract/corpus.json` (104 cases); `provenance-v2-macos-arm64.json` | **UNRESOLVED (Tier 2)**: 24 diagnostic vectors in audit and 104 checked cases in contract; claimed 10,000 vectors was unpersisted synthetic claim; canonical evidence status is `conversion_status: UNRESOLVED: excluded from canonical evidence` with known platform divergence | `BENCHMARK_LEDGER.csv`, `METHODOLOGY.md`, `README.md` | Inspected JSON corpus sizes and `conversion_status` field in provenance-v2 record | IEEE-754 binary64 round-trip conversion remains noncanonical and unresolved across architectures |
| **FINDING-04 (CMP3 Decision Advantage)** | EXP-CMP3-R1 independently reproduced across environments (Tier 4); PMU cycle counts show hardware parity on ARM64 `csel`/`cset` and Linux | `experiments/cmp3_decision_advantage/evidence-r1-publication/RESULTS.json` (`measurements_collected: false`, `measurement_authorized: false`, `pmu_cycles: null`, `linux: null`) | **PMU_UNAVAILABLE (Tier 2)**: Compiler assembly validation exists for ARM64/x86_64, but PMU cycle counts were unauthorized and uncollected; Linux PMU data is null. No Tier 4 reproduction. | `BENCHMARK_LEDGER.csv`, `METHODOLOGY.md`, `HYPOTHESIS_STATUS.md`, `README.md` | Inspected canonical `RESULTS.json` fields | Hardware PMU performance counter access unconfigured in Linux CI container |
| **FINDING-05 (Benchmark Executables)** | Execution command invoked nonexistent `./build/benchmarks/t81_benchmarks` with unmapped filter names | `benchmarks/CMakeLists.txt` (declares `add_executable(benchmark_runner ...)`), `benchmarks/BM_*.cpp` | **CORRECTED COMMANDS**: Replaced with `./build/benchmarks/benchmark_runner` and mapped exact registered benchmark filters (`BM_Base81_`, `BM_Overhead_`, `BM_Governed`, `BM_Limb`, etc.). Marked `BM_TensorPromotion` as `SOURCE_ONLY (No BENCHMARK declarations registered)` | `BENCHMARK_LEDGER.csv` | Cross-referenced declared CMake target names and `BENCHMARK(...)` source macros | `benchmark_runner` requires building with Google Benchmark target dependencies |
| **FINDING-06 (Workload Identities)** | `BM_LimbMul` described as Karatsuba vs O(N^2) schoolbook multiplication; `BM_VMSimulation` described as Fibonacci / Ackermann workloads | `benchmarks/BM_LimbMul.cpp`, `benchmarks/BM_VMSimulation.cpp`, `benchmarks/BM_DispatchOverhead.cpp` | **ALIGNED WORKLOAD IDENTITIES**: `BM_LimbMul` measures Booth limb multiplication vs native binary 64-bit uint64 scalar multiplication (4,096 ops/iter); `BM_VMSimulation` measures synthetic 1,000-op `OP_TAND` dispatch loop followed by `OP_HALT` (not application code) | `BENCHMARK_LEDGER.csv` | Inspected C++ source implementations of benchmark loops | Synthetic microbenchmarks do not measure real application workloads |
| **FINDING-07 (RFC Counting Rules)** | Frontmatter status counts (83 accepted, 11 superseded, 11 unparsed, 8 draft) inconsistent with catalog table counts (94 accepted, 11 superseded, 5 draft) | `spec/rfcs/index.md`, `spec/rfcs/RFC-*.md`, `scripts/governance/check_rfc_lifecycle_hygiene.py` | **DOCUMENTED COUNTING RULES**: Catalog table has 110 entries; RFC files total 113 (111 unique IDs due to 2 companion profiles + 1 uncataloged `RFC-00D4`). The 11 unparsed files use markdown table formatting (`\| Status \| Accepted \|`) not matched by line-prefix regex. | `METHODOLOGY.md`, `README.md` | Ran deterministic parser script evaluating table syntax vs header prefix syntax | Governance script `check_rfc_lifecycle_hygiene.py` rejects table syntax until updated |
| **FINDING-08 (Direct API Speedup Simulation)** | EXP-DIRECT-API-SPEEDUP was Tier 3 (Attributable Evidence); claimed 864.4x speedup of Direct API over CLI baseline | `src/test_production_direct_api.cpp` (lines 184, 187: hardcoded `std::this_thread::sleep_for(2650us)` + `200us`) | **SIMULATED WORKLOAD (Tier 2)**: Direct API benchmark uses hardcoded sleep loops simulating execution duration rather than running real T81 model inference or policy checks; downgraded from Tier 3 to Tier 2 | `BENCHMARK_LEDGER.csv`, `METHODOLOGY.md`, `README.md` | Inspected C++ harness source code in `src/test_production_direct_api.cpp` | Real model inference latency unmeasured in harness; reflects mock sleep loop only |
| **FINDING-09 (H04-R01 Artifact Attribution)** | EXP-H04-R01 was Tier 3; attributed 4-arm adaptive trace sweep to `experiments/radix_comparison/results.json` | `docs/research/continuity/H04_RADIX_ADAPTATION_DISCRIMINATOR.md`; `experiments/radix_comparison/results.json` | **UNEXECUTED / PROPOSED (Tier 1)**: H04-R01 is explicitly a selected specification, not benchmarked; `radix_comparison/results.json` is a separate 64-bit microbenchmark across radices 2/3/9/27/81, not the 4-arm adaptive sweep | `BENCHMARK_LEDGER.csv`, `METHODOLOGY.md`, `HYPOTHESIS_STATUS.md`, `README.md` | Inspected JSON schema of `results.json` vs protocol specification in `H04_RADIX_ADAPTATION_DISCRIMINATOR.md` | 4-arm adaptive capacity protocol was never executed in tree at baseline |
| **FINDING-10 (H04-R02 Historical vs Model)** | EXP-H04-R02 conflated invalid historical reports with corrected results; did not separate physical-data limits | `experiments/h04_r02_adaptive_representation/` (`HISTORICAL_RESULTS.md`, `CORRECTED_RESULTS.md`, `LIMITATIONS.md`) | **SEPARATED INVALID HISTORICAL FROM SYMBOLIC MODEL (Tier 5)**: Disentangled historical Classification D defects from corrected symbolic model (AT=1210 < AB=1250 on boundary_oscillation under 10-bit Tier-0 control); explicitly preserved physical limits (offline DP, stipulated selection costs, unequal capacity) | `BENCHMARK_LEDGER.csv`, `HYPOTHESIS_STATUS.md` | Inspected `DISCREPANCY_LEDGER.md` and `LIMITATIONS.md` | Real encoder, conversion, and hardware silicon costs remain unmeasured |
| **FINDING-11 (H06 Scope Restriction)** | H06 declared universally refuted on binary silicon based on microbenchmark | `benchmarks/results/inference_comparison.md` (ARM64 NEON dot product 72.2 vs 4.96 Gops/s) | **RESTRICTED TO MEASURED ARM64 KERNELS**: INT8 14.5x advantage is strictly restricted to measured ARM64 CPU vector kernel microbenchmarks; model-level inference, non-ARM64 platforms, and custom hardware architectures left unresolved | `HYPOTHESIS_STATUS.md`, `GAPS_AND_PRIORITIES.md`, `README.md` | Inspected raw benchmark data in `inference_comparison.md` | End-to-end full model inference benchmarks across heterogeneous hardware remain open |

---

## 3. Evidence Classification Tiers (Reconciled)

Across the 26 audited benchmark families in `BENCHMARK_LEDGER.csv`:

| Evidence Tier | Grounded Criteria | Reconciled Families Count | Benchmark Families |
| :--- | :--- | :--- | :--- |
| **Tier 1 (Source-Only / Proposed)** | Benchmark suite or protocol defined in tree; no raw execution results or primary artifacts archived | **9** | `BM_Base81SIMD`, `BM_DPE`, `BM_DeterminismValidation`, `BM_GovernedWorkflows`, `BM_LlamaGguf`, `BM_NativeWeightsExecution`, `BM_TensorPromotion`, `EXP-H09-E1`, `EXP-H04-R01` |
| **Tier 2 (Executed, Insufficient Provenance)** | Candidate runs preserved, but binary comparators missing/below resolution, measurements unauthorized, simulated workloads, or evidence unresolved | **6** | `BM_PackedTritVector`, `BM_DispatchOverhead`, `BM_VMSimulation`, `EXP-CMP3-R1` (PMU_UNAVAILABLE), `EXP-FLOAT-CONVERSION` (UNRESOLVED), `EXP-DIRECT-API-SPEEDUP` (SIMULATED) |
| **Tier 3 (Executed with Attributable Evidence)** | Attributed raw execution logs, machine metadata, or mathematical oracle evaluations in tree | **8** | `BM_CanonFS`, `BM_InferenceComparison`, `BM_Lexer`, `BM_LimbMul`, `BM_TensorMatMul`, `BM_TritwiseWorkloads`, `EXP-S02`, `EXP-DURABLE-WITNESS` |
| **Tier 4 (Independently Reproduced)** | Attributable multi-runner execution reproduction across distinct physical machines, operating systems, and compilers | **0** | *None qualify at baseline (downgraded CMP3 and Float Conversion to Tier 2)* |
| **Tier 5 (Methodologically Invalid / Superseded)** | Demonstrated by adversarial audit to suffer from fatal methodology defects or capacity asymmetry | **2** | `BM_LlamaKernels` (invalid unvectorized baseline artifact), `EXP-H04-R02` (Classification D: capacity asymmetry, metadata scaling defect) |
| **Tier 6 (Blocked / Incomplete)** | Empirical characterization blocked by absence of matched physical hardware data | **1** | `EXP-INV-10A` (physical solid-state ternary storage assessment) |
| **Total** | | **26** | **16 Executed, 9 Source-Only, 1 Blocked** |

---

## 4. Deliverable Package Integrity

All seven required deliverables under `docs/research/baseline-assessment/` are present, mutually consistent, and reconciled:
1. `README.md` — Executive overview, scorecard, tier totals, and canonical directory structure.
2. `METHODOLOGY.md` — Epistemic principles, counting rules, denominators, extraction pipeline, and limitation admissions.
3. `RFC_MATRIX.csv` — Complete 113-row RFC inventory with dual status tracking (catalog vs frontmatter) and realization maturity.
4. `CAPABILITY_MATRIX.csv` — 23-row engineering capability mapping across 10 architectural domains with test linkages.
5. `BENCHMARK_LEDGER.csv` — 26-row benchmark inventory with reconciled commands, workload identities, raw evidence paths, and quality tiers.
6. `HYPOTHESIS_STATUS.md` — Formal scientific synthesis for canonical hypotheses H01–H08 and auxiliary investigations.
7. `GAPS_AND_PRIORITIES.md` — Strategic architectural roadmap distinguishing critical engineering gaps from deprioritized research lanes.

---

## 5. Verification Commands and Validation Results

The following deterministic commands were executed to validate markdown, CSV, and inventory integrity:

```bash
# 1. Verify CSV row counts and structure:
python3 -c "
import glob, csv
for p in sorted(glob.glob('docs/research/baseline-assessment/*.csv')):
    with open(p) as f:
        r = list(csv.reader(f))
    print(f'{p}: {len(r)} rows')
"
# Output:
# docs/research/baseline-assessment/BENCHMARK_LEDGER.csv: 27 rows (1 header + 26 data)
# docs/research/baseline-assessment/CAPABILITY_MATRIX.csv: 24 rows (1 header + 23 data)
# docs/research/baseline-assessment/RFC_MATRIX.csv: 114 rows (1 header + 113 data)

# 2. Verify tier counts in BENCHMARK_LEDGER.csv:
python3 -c "
import csv
from collections import Counter
with open('docs/research/baseline-assessment/BENCHMARK_LEDGER.csv') as f:
    r = list(csv.DictReader(f))
print(Counter(x['Classification_Tier'] for x in r))
"
# Output:
# Counter({
#   'Tier 1 (Source-Only / Proposed)': 9,
#   'Tier 3 (Executed with Attributable Evidence)': 8,
#   'Tier 2 (Executed, Insufficient Provenance)': 6,
#   'Tier 5 (Methodologically Invalid / Superseded)': 2,
#   'Tier 6 (Blocked / Incomplete)': 1
# })

# 3. Verify top-level structure policy:
python3 scripts/governance/check_root_structure.py
# Output: root structure policy PASSED
```

---

## 6. Pre-Existing / Inherited CI Failures (Baseline Separation)

The changes in this reconciliation are strictly limited to documentation within `docs/research/baseline-assessment/`. No runtime C++, Python scripts, or CMake build configurations have been modified.

Pre-existing baseline CI failures at commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05` are inherited and completely independent of this documentation candidate:
- `cli_contract_test` fails due to missing fixture `models/tiny-random-llama/model.safetensors`.
- Google Benchmark compilation fails under GCC 15 on `benchmark_register.cc.o` (`-Werror=maybe-uninitialized`).
- MSVC / clang-cl CMake configuration fails due to missing `development_tests/test_real_direct_api.cpp`.
- `scripts/governance/check_rfc_lifecycle_hygiene.py` reports table formatting in 11 RFC files and profile template status divergence.

---

## 7. Submission Branch and PR #663 Discrepancy Resolution

- **Discrepancy Identification:** PR #663's pull request description historically reported preliminary benchmark reconciliations that diverged from the actual tree artifacts at candidate commit `bcd1a70260123c6eb1f0dfdb78b15599ef79c8da` (specifically regarding H09-E1 execution status, CMP3 PMU counter parity, and float conversion corpus size).
- **Resolution:** The final review must inspect this candidate commit and its committed deliverable documents, which supersede all prior pull request descriptions.
- **Submission Branch:** `jules-9454522404953220260-f8b072a2` (tracking candidate PR branch).

---

## 8. Final Disposition

**Disposition:** `READY`

The baseline assessment package `docs/research/baseline-assessment/` is fully reconciled against repository ground truth, contains no unsupported empirical claims, aligns all benchmark commands and workload identities with primary sources, and is submitted for independent review.
