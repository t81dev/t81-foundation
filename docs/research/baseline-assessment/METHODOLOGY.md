# T81 Foundation Baseline Assessment Methodology & Counting Rules

**Document Identifier:** T81-BASELINE-001-METHODOLOGY
**Repository Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Temporal Validity:** Evaluated as of pinned baseline commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Date:** November 2026
**Status:** Canonical Assessment Methodology

---

## 1. Scope, Limitations & Epistemic Boundaries

This document defines the evidence-based methodology, taxonomy, counting rules, and verification criteria used in the T81 Foundation Baseline Assessment (`T81-BASELINE-001`). The goal of this assessment is to reconstruct the true state of T81 research, specifications, engineering implementations, and benchmark evidence directly from primary repository artifacts, distinguishing verified implementations from aspirational design documents or generated summaries.

### Methodological Limitations & Explicit Downgrade
To maintain strict epistemic integrity, the methodology makes the following explicit admissions:
1. **Recountable Totals vs. Adjudicated Classifications:** Metric enumerations (file counts, catalog entries, registered CTest targets, benchmark family counts) are independently recountable via deterministic shell commands. However, capability maturities, evidence tiers, and RFC statuses require qualitative and manual adjudication against primary code, test definitions, and commits.
2. **Declared vs. Registered vs. Executed vs. Passed Tests:** Enumerating 159 static `add_test()` declarations across CMakeLists.txt files establishes that test targets are declared in build manifests. It does NOT establish that all declared tests are registered in every CTest configuration, executed in CI, or passing. Furthermore, external PR #662 reports that up to 26 tests were omitted from dynamic CTest discovery at this exact baseline and that certain CI regex patterns returned success while executing zero tests. Claims regarding DPE, TernaryOS, and Axion coverage are strictly scoped to declared target definitions, with runtime-wide verification pending independent adversarial reproduction.
3. **Oracle Correctness vs. Multi-Runner Reproduction:** Validating mathematical logic against an independent mathematical oracle script (such as Python scripts in EXP-H04-R01, EXP-S02, EXP-DURABLE-WITNESS) verifies model consistency and arithmetic correctness; it does NOT constitute independent multi-runner execution reproduction across distinct physical machines, operating systems, and compilers. Only benchmarks with attributable multi-environment evidence in tree (EXP-CMP3-R1 across Apple Clang21 / Linux Clang22 and EXP-FLOAT-CONVERSION across macOS ARM64 / Linux CI) are categorized as Tier 4, and their cross-environment execution status is treated as reported pending fresh independent adversarial rerun.
4. **Temporal Baseline:** All data, statuses, and metrics are evaluated **as of pinned historical baseline commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`**. Post-baseline events (e.g., PR #658 Phase B acceptance, PR #662 test discovery audits) are explicitly treated as dated post-baseline deltas.

---

## 2. Counting Rules and Denominators

To ensure full reproducibility and eliminate ambiguity, every quantitative metric in the baseline assessment is grounded in explicit counting rules and declared denominators.

### 2.1 RFC Corpus Inventory
- **RFC Files Denominator:** Total `.md` files matching `spec/rfcs/RFC-*.md`.
  - **Count:** 113 markdown files.
- **Unique RFC ID Denominator:** Unique logical RFC numbers identified across the corpus.
  - **Count:** 111 unique RFC IDs.
  - *Discrepancy Explanation:* Two RFC IDs have two markdown files each:
    1. `RFC-0055`: `RFC-0055-native-ternary-hardware-target-and-interop-contract.md` (contract) and `RFC-0055-hardware-target-profile-template.md` (companion profile template).
    2. `RFC-0056`: `RFC-0056-google-axion-t81-integration.md` (contract) and `RFC-0056-google-axion-hardware-profile.md` (companion target profile).
- **Catalog Indexed Entries Denominator:** Total RFC entries listed in the markdown table of `spec/rfcs/index.md`.
  - **Count:** 110 catalog entries.
  - *Breakdown by catalog row:* 94 accepted, 11 superseded, 5 draft.
  - *Delta against Files:* `RFC-00D4` exists in `spec/rfcs/` (`spec/rfcs/RFC-00D4-accountable-decision-bundle.md`) but was omitted from the catalog table in `index.md`.
- **Status Categories Reconciled:**
  - **Catalog Status Mapped Across All 113 Files:** 96 accepted (94 index rows + 2 companion template/profile files for RFC-0055 and RFC-0056), 11 superseded, 5 draft, 1 uncataloged (`RFC-00D4`) = 113 rows in `RFC_MATRIX.csv`.
  - **Frontmatter Status Across All 113 Files:** 83 accepted, 11 superseded, 11 unparsed, 8 draft = 113 files.
  - **Reconciled Maturity:** Realization maturity determined from primary source and test existence (`79 Tested, 23 Specified, 11 Superseded`). Specifications lacking code implementations (e.g. RFC-0056, RFC-00D0, RFC-00D2) are classified as SPECIFIED.

### 2.2 Engineering Capabilities Inventory
- **Capability Count Denominator:** 23 discrete capabilities mapped across 10 architectural domains.
- **Realization Maturity Breakdown:**
  - **TESTED:** 21 capabilities backed by compilable code and verified test suites.
  - **IMPLEMENTED:** 1 capability (Batch Inference Engine & Llama Model Bridge: class implemented in `inference/batch_inference_engine.cpp` but standalone test suite unintegrated).
  - **SPECIFIED:** 1 capability (Tier 4 Meta-Cognition: architecture specified, prototype unmerged).
- **Integration Status Breakdown:**
  - **INTEGRATED:** 18 capabilities integrated across core runtime and CLI.
  - **EXPERIMENTAL:** 4 capabilities under experimental research / freestanding kernel.
  - **UNINTEGRATED:** 1 capability (Tier 4 cognition).

### 2.3 Benchmark Families Inventory
- **Benchmark Family Denominator:** 26 total identifiable benchmark families across the repository:
  - 17 Google Benchmark suites located in `benchmarks/BM_*.cpp`.
  - 9 formal empirical experiment harnesses located in `experiments/`, `execution_reports/`, and `docs/research/`.
- **Execution Breakdown:**
  - **EXECUTED:** 18 benchmark families with attributable execution logs, reports, or raw data.
  - **SOURCE_ONLY:** 7 suites with C++ benchmark definitions but no raw results in tree (BM_Base81SIMD, BM_DPE, BM_DeterminismValidation, BM_GovernedWorkflows, BM_LlamaGguf, BM_NativeWeightsExecution, BM_TensorPromotion).
  - **BLOCKED:** 1 investigation (EXP-INV-10A physical storage data unavailable).
- **Classification Rubric & Grounded Counts (Tiers 1–6):**
  - **Tier 1 (Source-Only / Proposed):** 7 suites.
  - **Tier 2 (Executed, Insufficient Provenance):** 3 suites (BM_PackedTritVector, BM_DispatchOverhead, BM_VMSimulation: candidate runs preserved, but binary comparators are explicitly marked missing or below timer resolution).
  - **Tier 3 (Executed with Attributable Evidence):** 11 families with attributed raw logs, machine metadata, or mathematical oracle evaluations (BM_CanonFS, BM_InferenceComparison, BM_Lexer, BM_LimbMul, BM_TensorMatMul, BM_TritwiseWorkloads, EXP-H04-R01, EXP-S02, EXP-H09-E1, EXP-DURABLE-WITNESS, EXP-DIRECT-API-SPEEDUP).
  - **Tier 4 (Independently Reproduced):** 2 families with reported cross-environment artifacts in tree (EXP-CMP3-R1 across Apple Clang21 and Linux Clang22; EXP-FLOAT-CONVERSION across macOS ARM64 and Linux CI).
  - **Tier 5 (Methodologically Invalid / Superseded):** 2 families shown by adversarial audit to suffer from fatal methodology defects (BM_LlamaKernels, EXP-H04-R02).
  - **Tier 6 (Blocked / Incomplete):** 1 family (EXP-INV-10A physical solid-state storage data unavailable).

### 2.4 Scientific Hypotheses Inventory (H01–H08)
- **Canonical Register Denominator:** The 8 canonical hypotheses defined in `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`.
- **Dispositions:**
  - H01: PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE.
  - H02: CONDITIONAL MATHEMATICAL PROPERTY — REFUTED AS HARDWARE SPEEDUP ON BINARY SILICON.
  - H03: PARTIALLY SUPPORTED (CONVENIENCE ENCODING) — REFUTED AS OPTIMAL STORAGE DENSITY.
  - H04: REFUTED WITHIN GENERAL SCOPE — CONDITIONAL ON RESTRICTED MATHEMATICAL DOMAINS WITH UNEQUAL CAPACITY.
  - H05: THEORETICALLY PLAUSIBLE AT ABSTRACT LEVEL — UNVERIFIED ON PHYSICAL SILICON.
  - H06: REFUTED ON BINARY SILICON — CONDITIONAL ON SPECIALIZED HARDWARE WITHOUT HARDWARE MULTIPLIERS.
  - H07: DEMONSTRATED IN OPERATIONAL TESTS — COMPARATIVE AUDITABILITY ADVANTAGE UNTESTED AGAINST COMPETITIVE BOUNDARY CONTROL.
  - H08: UNTESTED (METHODOLOGICAL FRAMEWORK ACTIVE; COMPARATIVE TRUSTWORTHINESS OPEN).

---

## 3. Reproducible Extraction Pipeline & Commands

To ensure full transparency, every metric is verifiable using standard commands executed from repository root:

1. **Verify RFC Files and Catalog Denominators:**
   ```bash
   python3 -c "
   import glob, re
   files = sorted(glob.glob('spec/rfcs/RFC-*.md'))
   print('RFC files count:', len(files)) # 113
   index_text = open('spec/rfcs/index.md').read()
   index_rows = re.findall(r'\|\s*(RFC-[0-9A-Za-z\-]+)\s*\|', index_text)
   print('Index table count:', len(index_rows)) # 110
   "
   ```

2. **Verify Declared CTest Targets:**
   ```bash
   python3 -c "
   import re, glob
   cmake_files = glob.glob('**/CMakeLists.txt', recursive=True) + ['CMakeLists.txt']
   tests = set()
   for cf in set(cmake_files):
       content = open(cf, errors='ignore').read()
       tests.update(re.findall(r'add_test\s*\(\s*NAME\s+([A-Za-z0-9_\-]+)', content))
       tests.update(re.findall(r'add_test\s*\(\s*([A-Za-z0-9_\-]+)\s+COMMAND', content))
   print('Total declared add_test targets:', len(tests)) # 159
   "
   ```

3. **Verify Benchmark Artifacts & Ledgers:**
   ```bash
   python3 -c "
   import csv
   with open('docs/research/baseline-assessment/BENCHMARK_LEDGER.csv') as f:
       rows = list(csv.DictReader(f))
   print('Benchmark families count:', len(rows)) # 26
   from collections import Counter
   print('Tiers:', Counter(r['Classification_Tier'] for r in rows))
   print('Executed status:', Counter(r['Executed_Status'] for r in rows))
   "
   ```

4. **Verify Top-Level Repository Invariants:**
   ```bash
   python3 scripts/governance/check_root_structure.py
   python3 tests/ci/check_root_structure_test.py
   ```
