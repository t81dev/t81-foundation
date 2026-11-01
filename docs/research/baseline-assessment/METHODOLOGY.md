# T81 Foundation Baseline Assessment Methodology & Counting Rules

**Document Identifier:** T81-BASELINE-001-METHODOLOGY
**Repository Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Date:** November 2026
**Status:** Canonical Assessment Methodology

---

## 1. Scope and Objective

This document defines the rigorous, evidence-based methodology, taxonomy, counting rules, and verification criteria used in the T81 Foundation Baseline Assessment (`T81-BASELINE-001`). The goal of this assessment is to reconstruct the true state of T81 research, specifications, engineering implementations, and benchmark evidence directly from primary repository artifacts, distinguishing verified implementations from aspirational design documents or generated summaries.

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
- **Catalog Indexed Denominator:** Total RFC entries listed in the markdown table of `spec/rfcs/index.md`.
  - **Count:** 110 entries.
  - *Delta against Files:* `RFC-00D4` exists in `spec/rfcs/` but was omitted from the catalog table in `index.md`.
- **Status Categories:**
  - **Catalog Status:** The status explicitly assigned in `spec/rfcs/index.md` (`accepted`, `superseded`, `draft`, `proposed`, `integrated`, `rejected`).
  - **Frontmatter Status:** The status declared in the YAML frontmatter or top-level metadata block of the individual RFC markdown file.
  - **Reconciled Status:** The ground-truth lifecycle state determined after inspecting the normative contract, linked code, and tests.

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
- **Benchmark Family Denominator:** Discrete benchmark suites located across `benchmarks/BM_*.cpp`, `experiments/`, and historical test runners.
  - **Count:** 17 compiled Google Benchmark suites in `benchmarks/BM_*.cpp` plus 9 formal empirical experiment benchmark harnesses in `experiments/`.
- **Classification Rubric (Tiers 1–6):**
  - **Tier 1 (Source-Only / Proposed):** Benchmark source code or execution script exists in repository, but no raw execution results or environment metadata are recorded.
  - **Tier 2 (Executed, Insufficient Provenance):** Numerical results are quoted in markdown or summaries, but raw logs, command lines, compiler flags, or seeds are missing or untraceable.
  - **Tier 3 (Executed with Attributable Evidence):** Executed run with recorded raw output, machine metadata, compiler flags, and deterministic inputs preserved in tree.
  - **Tier 4 (Independently Reproduced):** Workload executed across multiple independent environments (e.g., Apple Silicon ARM64 and Linux x86_64) or validated against an independent mathematical oracle.
  - **Tier 5 (Methodologically Invalid / Superseded):** Prior benchmark or experiment shown by adversarial audit to suffer from fatal methodology defects (e.g., comparator capacity asymmetry, metadata scaling errors, unoptimized controls, missing conversion costs).
  - **Tier 6 (Blocked / Incomplete):** Benchmark cannot be executed due to missing physical devices, uncharacterized hardware, or unavailable datasets.

### 2.4 Scientific Hypotheses Inventory (H01–H08)
- **Canonical Register Denominator:** The 8 canonical hypotheses defined in `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`.
- **Disposition Rubric:**
  - **SUPPORTED:** Empirical evidence under competent controls confirms the hypothesis within tested scope.
  - **REFUTED (Within Tested Scope):** Empirical evidence under matched controls falsifies the claimed advantage within the tested domain.
  - **CONDITIONAL:** Advantage appears only under specific non-universal parameter regimes (e.g., specific symbol cost ratios $C_3/C_2$ or particular boundary oscillations) while failing elsewhere.
  - **BLOCKED / UNVERIFIED:** Empirical evaluation cannot be completed due to unverified literature data, missing hardware, or uncharacterized physical technology.
  - **METHODOLOGICALLY DEFECTIVE (Classification D):** Prior supporting claims were invalidated due to experimental or computational defects.

---

## 3. Evidence Extraction & Verification Pipeline

All data entered into the baseline assessment CSV ledgers and status registers are extracted via automated scripts:
1. `parse_rfcs.py` traverses all files in `spec/rfcs/`, cross-referencing catalog entries in `spec/rfcs/index.md` with file headers, implementation files, and test definitions.
2. `parse_capabilities.py` verifies file existence across `include/`, `src/`, `vm/`, `fs/`, `lang/`, `ternaryos/`, and checks CMake `add_test()` declarations.
3. `parse_benchmarks.py` maps benchmark suites to raw results in `benchmarks/results/archive/` and `experiments/*/results.json`.
4. Integrity is verified using Python scripts asserting:
   - Zero missing or broken relative links.
   - Exact column counts across all CSV files.
   - Zero unregistered top-level files per `scripts/governance/check_root_structure.py`.
