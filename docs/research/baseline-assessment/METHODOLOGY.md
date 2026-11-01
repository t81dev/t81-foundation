# T81 Foundation Baseline Assessment Methodology & Counting Rules

**Document Identifier:** T81-BASELINE-001-METHODOLOGY
**Repository Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Temporal Validity:** Evaluated as of pinned baseline commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Date:** November 2026
**Status:** Canonical Assessment Methodology

---

## 1. Scope and Objective

This document defines the rigorous, evidence-based methodology, taxonomy, counting rules, and verification criteria used in the T81 Foundation Baseline Assessment (`T81-BASELINE-001`). The goal of this assessment is to reconstruct the true state of T81 research, specifications, engineering implementations, and benchmark evidence directly from primary repository artifacts, distinguishing verified implementations from aspirational design documents or generated summaries.

All data, statuses, and metrics are evaluated **as of pinned historical baseline commit `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`**. Post-baseline events (e.g. PR #658 acceptance) are explicitly treated as dated post-baseline deltas.

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
  - **Frontmatter Status Across All 113 Files:** 82 accepted, 11 superseded, 11 unparsed, 8 draft, 1 status = 113 files.
  - **Reconciled Maturity:** Realization maturity determined from primary source and test existence (`80 Tested, 17 Specified, 11 Superseded, 5 Implemented`).

### 2.2 Engineering Capabilities Inventory
- **Subsystem Denominator:** 10 primary architectural domains:
  1. Core Ternary Arithmetic & Fundamental Types (`include/t81/types/`, `src/core/`)
  2. T81Lang Language & Compiler Toolchain (`lang/frontend/`, `src/lang/`, `include/t81/frontend/`)
  3. TISC ISA & HanoiVM Execution Runtime (`vm/`, `isa/`, `include/t81/isa/`, `include/t81/vm/`)
  4. Axion Policy Enforcement & Governance Engine (`src/axion/`, `kernel/axion/`, `include/t81/axion/`)
  5. CanonFS Content-Addressed Storage & Interchange (`fs/`, `include/t81/canonfs/`)
  6. Tensor, SIMD, SWAR & AI Inference Stack (`include/t81/tensor/`, `include/t81/simd/`, `include/t81/swar/`, `inference/`)
  7. Deterministic Parallel Execution (DPE) Subsystem (`vm/dpe/`, `include/t81/dpe/`)
  8. TernaryOS Microkernel, HAL & User Environment (`ternaryos/`, `kernel/`)
  9. Cryptographic & Post-Quantum Subsystems (`crypto/`, `include/t81/tensor/lattice_crypto.hpp`, `include/t81/tensor/ntru_kem.hpp`)
  10. Cognitive, Recursive & Tier 4+ Systems (`src/cognition/`, `include/t81/ai/`)

- **Implementation Realization Maturity Rubric:**
  - **SPECIFIED:** Defined in normative RFC or architectural specification; no active implementation exists.
  - **EXPERIMENTAL:** Prototype code exists in tree (or under `experimental/` / research branches) but is unintegrated or lacks complete deterministic bounds.
  - **IMPLEMENTED:** Compilable C++ or Python implementation exists in tree, but automated test coverage is incomplete or unverified.
  - **TESTED:** Compilable source code covered by registered unit, integration, or property test suites passing in CI.
  - **INTEGRATED:** End-to-end integration across compiler, VM, policy, and CLI surfaces with full regression protection.
  - **BLOCKED:** Implementation cannot proceed due to missing physical hardware, unverified literature assumptions, or upstream defects.

### 2.3 Benchmark Families Inventory
- **Benchmark Family Denominator:** 26 total identifiable benchmark families across the repository:
  - 17 Google Benchmark suites located in `benchmarks/BM_*.cpp`.
  - 9 formal empirical experiment harnesses located in `experiments/`, `execution_reports/`, and `docs/research/`.
- **Classification Rubric (Tiers 1–6):**
  - **Tier 1 (Source-Only / Proposed):** Benchmark source code exists in repository, but no raw execution results or environment metadata are recorded in tree (9 suites: BM_Base81SIMD, BM_DPE, BM_DeterminismValidation, BM_GovernedWorkflows, BM_LlamaGguf, BM_NativeWeightsExecution, BM_PackedTritVector, BM_TensorMatMul, BM_TensorPromotion).
  - **Tier 2 (Executed, Insufficient Provenance):** Workload executed and raw candidate values present, but comparator baseline is missing, unparsed, or marked not-implemented (1 suite: BM_TritwiseWorkloads in `benchmarks/results/archive/benchmark_raw.txt`).
  - **Tier 3 (Executed with Attributable Evidence):** Executed run with recorded raw output, machine metadata, compiler flags, and deterministic inputs preserved in tree (7 suites: BM_CanonFS, BM_DispatchOverhead, BM_InferenceComparison, BM_Lexer, BM_LimbMul, BM_VMSimulation, EXP-DIRECT-API-SPEEDUP).
  - **Tier 4 (Independently Reproduced):** Workload executed across multiple independent environments (e.g., Apple Silicon ARM64 and Linux x86_64) or validated against an independent mathematical oracle (6 suites: EXP-H04-R01, EXP-S02, EXP-H09-E1, EXP-CMP3-R1, EXP-DURABLE-WITNESS, EXP-FLOAT-CONVERSION).
  - **Tier 5 (Methodologically Invalid / Superseded):** Prior benchmark or experiment shown by adversarial audit to suffer from fatal methodology defects (e.g., comparator capacity asymmetry, metadata scaling errors, unoptimized controls, missing conversion costs) (2 suites: BM_LlamaKernels, EXP-H04-R02).
  - **Tier 6 (Blocked / Incomplete):** Benchmark cannot be executed due to missing physical devices, uncharacterized hardware, or unavailable datasets (1 suite: EXP-INV-10A).

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

To ensure full transparency and auditability, every metric and table is verifiable using standard POSIX shell and Python commands executed from the repository root:

1. **Verify RFC Files and Denominators:**
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

2. **Verify Registered CTest Targets:**
   ```bash
   python3 -c "
   import re, glob
   cmake_files = glob.glob('**/CMakeLists.txt', recursive=True) + ['CMakeLists.txt']
   tests = set()
   for cf in set(cmake_files):
       content = open(cf, errors='ignore').read()
       tests.update(re.findall(r'add_test\s*\(\s*NAME\s+([A-Za-z0-9_\-]+)', content))
       tests.update(re.findall(r'add_test\s*\(\s*([A-Za-z0-9_\-]+)\s+COMMAND', content))
   print('Total add_test targets:', len(tests)) # 159
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
   "
   ```

4. **Verify Top-Level Repository Invariants:**
   ```bash
   python3 scripts/governance/check_root_structure.py
   python3 tests/ci/check_root_structure_test.py
   ```
