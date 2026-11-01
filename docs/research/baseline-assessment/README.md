# T81 Foundation — Research, RFC, Implementation & Benchmark Baseline Assessment

**Document Identifier:** T81-BASELINE-001
**Mission:** Baseline Assessment, Research Archaeology & Evidence Reconstruction
**Repository:** `https://github.com/t81dev/t81-foundation`
**Current Baseline Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Baseline Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
**Date:** November 2026
**Mode:** Read-only investigation with a bounded documentation deliverable

---

## 1. Executive Summary & Assessment Scope

This baseline assessment provides a comprehensive, ground-truth reconstruction of the T81 Foundation research program directly from primary repository artifacts, replacing historical AI summaries, promotional claims, and speculative architecture narratives with verified evidence.

T81 Foundation is an experimental computing architecture and systems research platform investigating balanced ternary representations, deterministic execution, content-addressed artifacts, language tooling, and policy-governed computation. Over months of intensive development, extensive work has been completed across RFC specification, C++ runtime engineering, compiler construction, and empirical experimentation.

This assessment establishes:
1. **Deterministic Core Profile (DCP) is Real and Operational:** TISC ISA v1.9.0 bytecode execution, bit-exact replay, deterministic mark-sweep GC, and Axion fail-closed policy enforcement are fully implemented, tested across 159 CMake test targets, and verified across x86_64 and ARM64 architectures.
2. **Binary Silicon Heavily Outperforms Emulated Ternary:** On conventional CPUs, highly optimized binary hardware accelerators (such as ARM NEON INT8 `sdot`) outperform ternary arithmetic by up to 14.5x. Early claims of "74x ternary AI speedup" were methodologically invalid artifacts comparing against unvectorized FP32 scalar baselines.
3. **Physical Storage Advantage is Unverified:** Investigation 10A demonstrated that all 10 literature citations cited for solid-state ternary memory density were unverified in primary literature. Hypothesis H01 remains blocked by unavailable matched characterization data, rather than experimentally falsified.
4. **Adaptive Representation Advantage is Conditional:** In H04-R01 and H04-R02, apparent adaptive ternary advantages over fixed binary stem from adaptive precision rather than ternary radix superiority. Under strictly equalized capacity bounds, adaptive binary matches or outperforms adaptive ternary across all tested symbol cost ratios.
5. **Recursive Cognition Suffers Catastrophic Overhead:** Experiment H09-E1 proved that multi-tier cognitive agent loops incur an overhead penalty exceeding 96%, rendering them 7x to 66x more expensive than centralized planners.

---

## 2. Quantitative Program Assessment Summary

Every total below is computed under the explicit counting rubrics defined in [METHODOLOGY.md](METHODOLOGY.md):

| Category / Metric | Total Count | Counting Rule / Ground-Truth Denominator | Key Breakdown |
| :--- | :---: | :--- | :--- |
| **Total RFC Markdown Files** | **113** | Markdown files matching `spec/rfcs/RFC-*.md` | 111 unique RFC IDs (RFC-0055 and RFC-0056 have companion profile templates) |
| **Catalog Indexed RFCs** | **110** | RFCs listed in markdown table of `spec/rfcs/index.md` | 94 accepted, 11 superseded, 5 draft (RFC-00D4 omitted from index) |
| **Reconciled RFC Maturity** | **113** | Ground-truth lifecycle based on actual code & tests | **80 Tested, 17 Specified, 11 Superseded, 5 Implemented** |
| **RFC Status (Catalog)** | **113** | Explicit status in catalog table | **96 accepted, 11 superseded, 5 draft, 1 uncataloged (RFC-00D4)** |
| **RFC Status (Frontmatter)**| **113** | Top-level metadata / frontmatter in RFC file | **82 accepted, 11 superseded, 11 unparsed, 8 draft, 1 status** |
| **Major Subsystems Audited** | **10** | Core architectural divisions in codebase | 10 audited (Core Arith, Compiler, VM, Axion, CanonFS, Tensor/SIMD, DPE, TernaryOS, Crypto, Cognitive) |
| **Engineering Capabilities** | **23** | Major architectural capabilities mapped | **16 Tested, 4 Experimental, 2 Implemented, 1 Specified** |
| **Benchmark Families** | **26** | 17 Google Benchmark suites + 9 formal experiment harnesses | 25 Executed, 1 Blocked (Physical Device Investigation 10A) |
| **Benchmark Quality Tiers** | **26** | Six-tier evidence classification rubric | **16 Tier 3 (Attributable), 7 Tier 4 (Reproduced), 2 Tier 5 (Invalid/Superseded), 1 Tier 6 (Blocked)** |
| **Canonical Hypotheses (H01–H08)**| **8** | Canonical H01–H08 hypothesis register | **1 Confirmed (H07), 3 Refuted in scope (H04, H06, H08), 2 Conditional (H02, H03), 1 Blocked (H01), 1 Plausible/Unverified (H05)** |

---

## 3. Repository Baseline Coordinates

- **Target Repository:** `t81dev/t81-foundation`
- **Current `main` Commit:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
- **Tree SHA:** `68fa5529b9d8244ba31c0f543006dee9283b248f`
- **Branch:** `main` (active assessment on working branch `jules-3194268086121102263-4fd1da7e`)
- **Historical Lineage:** Hanoi / CWEB prototypes (2024–2025) → T81 Foundation Architecture (early 2026) → Corrective Governance & Research Protocol Reconciliation (October 2026).
- **Active Governance Validators:** 27 scripts (24 in `scripts/governance/`, 3 in `scripts/ci/`).

---

## 4. Deliverables Directory Index

This baseline assessment is structured across seven purpose-built documentation deliverables located under `docs/research/baseline-assessment/`:

1. [README.md](README.md) — Executive assessment, scope, quantitative summary, and repository baseline (this document).
2. [METHODOLOGY.md](METHODOLOGY.md) — Rigorous counting rules, tier definitions, denominator explanations, and evidence extraction methods.
3. [RFC_MATRIX.csv](RFC_MATRIX.csv) — Complete 113-row RFC inventory mapping IDs, filenames, titles, dual statuses, dates, verified source paths, test references, and realization maturities.
4. [CAPABILITY_MATRIX.csv](CAPABILITY_MATRIX.csv) — Complete engineering implementation audit mapping 23 capabilities across 10 subsystems, linking source paths, test paths, integration status, and maturity.
5. [BENCHMARK_LEDGER.csv](BENCHMARK_LEDGER.csv) — Comprehensive 26-family benchmark ledger detailing workloads, candidate vs comparator, execution commands, hardware targets, raw evidence paths, numerical results, and tier classifications (Tiers 1–6).
6. [HYPOTHESIS_STATUS.md](HYPOTHESIS_STATUS.md) — Scientific reconciliation of H01–H08, S01–S02, H09, and Investigations 07, 08, and 10A, preserving negative results and Classification D audit findings.
7. [GAPS_AND_PRIORITIES.md](GAPS_AND_PRIORITIES.md) — Evidence-backed analysis of the ten most consequential evidence gaps and actionable development priorities.
