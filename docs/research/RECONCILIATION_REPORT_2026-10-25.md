# T81 Foundation — Research & Hypothesis Continuity Reconciliation Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation — Research & Hypothesis Continuity Reconciliation Report](#t81-foundation-—-research-&-hypothesis-continuity-reconciliation-report)
  - [1. Executive Summary & Reconciliation Purpose](#1-executive-summary-&-reconciliation-purpose)
  - [2. Restored Continuity Artifacts](#2-restored-continuity-artifacts)
  - [3. Canonical Hypothesis Register (H01–H08) Reconciliation](#3-canonical-hypothesis-register-h01–h08-reconciliation)
  - [4. Historical Experiment Evidence Mapping](#4-historical-experiment-evidence-mapping)
  - [5. Research Sequence Integration (H04-R01, H04-R02, and Candidate 3)](#5-research-sequence-integration-h04-r01-h04-r02-and-candidate-3)
  - [6. Verification and Build Integrity Summary](#6-verification-and-build-integrity-summary)

<!-- T81-TOC:END -->


**Document ID:** `docs/research/RECONCILIATION_REPORT_2026-10-25.md`
**Date:** October 25, 2026
**Status:** Complete / Authoritative Reconciliation
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Reference Merge Commit:** `2c96164096a4c1424efe2be0faa5663fbe650cc3` (PR #567)
**Continuity Baseline Commit:** `860558b4ad1cf4160af30860b00b15c46165acd5`

---

## 1. Executive Summary & Reconciliation Purpose

This report documents the post-merge research continuity correction following PR #567 (`2c96164096a4c1424efe2be0faa5663fbe650cc3`).

PR #567 provided a valuable full-repository reassessment report (`docs/research/T81_FULL_REASSESSMENT_2026-10-25.md`) and reconstructed the root `README.md`. However, it deleted the research continuity directory (`docs/research/continuity/`) and renumbered/re-defined the canonical hypothesis register (H01–H08), creating discrepancies with the **Master Continuity Charter** (`MASTER_CONTINUITY_CHARTER_v1.0.md`) and the **H04-R01 experiment packet** (`H04_RADIX_ADAPTATION_DISCRIMINATOR.md`).

This correction reconciles all research documentation without erasing historical evidence or rewriting scientific conclusions:
1. Restores the deleted `docs/research/continuity/` directory and its evidence artifacts from commit `860558b`.
2. Preserves the canonical H01–H08 register definitions and statements.
3. Maps empirical benchmark findings into a separate **Historical Experiment Evidence Mapping**.
4. Restores the three-track research structure (**Track R**, **Track F**, and **Track M**).
5. Scopes categorical claims ("falsified", "established", "100% verified") strictly to tested software and emulated environments.
6. Integrates H04-R01 as conditional mathematical model support and preserves H04-R02 as the next Track R experiment.

---

## 2. Restored Continuity Artifacts

The following files under `docs/research/continuity/` were restored from commit `860558b`:

| Restored File Path | Purpose & Research Function |
| :--- | :--- |
| `docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md` | Master research charter governing Track R, Track F, and Track M. |
| `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json` | Master canonical hypothesis register (H01–H08) with source pins and evidence tiers. |
| `docs/research/continuity/H04_RADIX_ADAPTATION_DISCRIMINATOR.md` | H04-R01 $2\times2$ exact trace discriminator specification. |
| `docs/research/continuity/LINEAGE.md` | Historical CWEB Hanoi source inheritance mapping (41 shared blobs). |
| `docs/research/continuity/SESSION_HANDOFF.md` | Session handoff protocol and state ledger. |
| `docs/research/continuity/CHANGELOG.md` | Append-only research continuity log. |
| `docs/research/continuity/evidence/*` | JSON evidence files, source pins, isolated CWEB probes, and dry-run logs. |
| `docs/research/continuity/probes/*` | Python probes verifying lineage and finite-state storage boundaries. |

---

## 3. Canonical Hypothesis Register (H01–H08) Reconciliation

The table below details the canonical hypothesis definitions restored across `docs/research/T81_FULL_REASSESSMENT_2026-10-25.md` and `README.md`:

| Hypothesis ID | Canonical Statement & Domain | Reconciled Status & Scoped Evidence |
| :--- | :--- | :--- |
| **H01** | **Native Ternary Storage & Logic Cost Reduction**<br>*(Track R: Scientific Computing)* | Mathematical density established ($\log_2 3 \approx 1.585$). Software tryte addition on binary ALUs is **falsified for speed and frozen (STOP)** ($10.6\times$ slower than native binary integer addition). Physical hardware claims remain open. |
| **H02** | **Balanced Ternary Signed Arithmetic & Rounding**<br>*(Track R: Scientific Computing)* | Algebraic symmetry around zero established. Software SWAR SIMD emulation on binary vector units is **falsified for speed and frozen** due to digit-unpacking tax. |
| **H03** | **Base-81 Digit Grouping & Representation Interface**<br>*(Track R: Scientific Computing)* | 4-trit grouping reduces software tryte addition latency from $105.3\text{ ns}$ to $11.7\text{ ns}$. 2-bit weight MatVec is **validated for storage reduction ($75\%$ DRAM savings)**, but **falsified for CPU MatVec speed** ($75.3\%$ of INT8 AVX2) in L1/L2 cache. |
| **H04** | **Adaptive Representation Tiering**<br>*(Track R: Scientific Computing)* | Hanoi tiering heuristic exists. Refined by **H04-R01** exact trace discriminator. Stage 1 yields **conditional model support with explicit counterexamples** (e.g. $M=256$, $C_3/C_2=1.5$ yields higher cost for ternary), not physical hardware proof. |
| **H05** | **Three-State Algebra Primitives & Architectures**<br>*(Track R: Scientific Computing)* | Exploratory. In-VM pre-dispatch policy evaluation (`eval_axion_call`) prevents unauthorized side effects ($0.0\%$ breach rate) in T81VM, but host capability boundaries in Wasm provide equivalent protection. |
| **H06** | **Ternary Weights & Activations for AI Inference**<br>*(Track R: Scientific Computing)* | On-disk 2-bit weight format (`.t81w`) validated for storage; CPU speedup disproven on binary SIMD. |
| **H07** | **Representation-Independent Governed Execution**<br>*(Track F: Foundation Systems)* | **Established for DCP determinism, CanonFS byte identity, and detached evidence auditability (RFC-00D4)**. Proves accountable computation is a runtime-agnostic protocol, not a custom VM requirement. |
| **H08** | **AI-Assisted Iterative Research & Trustworthy Discovery**<br>*(Track M: AI Methodology)* | **Established for prompt-to-code provenance, session continuity, and CanonFS artifact identity**. |

---

## 4. Historical Experiment Evidence Mapping

Empirical benchmark findings are explicitly separated into an evidence mapping section in `T81_FULL_REASSESSMENT_2026-10-25.md`:

| Historical Benchmark | Target Surface | Evaluated Hypotheses | Empirical Finding & Scope |
| :--- | :--- | :--- | :--- |
| **Radix Comparison** | `experiments/radix_comparison/` | **H01, H03** | Software tryte addition is $10.6\times$ slower than native 64-bit binary ALU addition. Falsifies software CPU ternary arithmetic speedup. |
| **Packed Ternary MatVec** | `experiments/packed_ternary_matvec/` | **H02, H03, H06** | 2-bit packed weight format saves $75\%$ memory footprint, but SIMD bit-shuffle decode tax limits execution speed to $75.3\%$ of INT8 AVX2 in cache. |
| **Accountable Computation (Baseline-X)** | `examples/governance/accountable-computation/` & `experiments/research_001/` | **H04, H05, H06, H07** | Wasmtime + OPA host FFI capability baseline (Arm iii) achieves identical $0.0\%$ breach rates and $100\%$ detached evidence auditability with $10\times$ lower host C++ LOC. Proves accountable computation is a protocol, not a runtime requirement. |
| **Cross-Platform Determinism (DCP)** | `tests/cpp/vm_determinism_property_test.cpp` | **H07** | Bit-exact cross-platform execution and RFC-0030 soft-float transcendentals pass $100\%$ of property-based tests across x86_64, ARM64, and WebAssembly. |
| **CanonFS Byte Identity & Interchange** | `src/core/fs/` & CTest #84–85 | **H07, H08** | Content-addressed storage (CanonHash-81) and foreign filesystem interchange (RFC-00D1) deliver immutable byte identity across systems. |

---

## 5. Research Sequence Integration (H04-R01, H04-R02, and Candidate 3)

The research sequence in Section H of `T81_FULL_REASSESSMENT_2026-10-25.md` was reconciled as follows:

1. **H04-R01 Packet (Conditional Model Support):** Recognized as a $2\times2$ exact trace model providing mathematical model support under explicit symbol assumptions, containing explicit counterexamples where ternary cost exceeds binary cost ($M=256$, $C_3/C_2=1.5$).
2. **H04-R02 Next Experiment (Four-Arm Exact Trace Ledger):** Preserved as the next planned Track R experiment to evaluate exact symbol-slot counts and mode transition work vectors across declared integer trace families.
3. **Candidate 3 (Deterministic AI Execution Parity):** Listed as a separate candidate under Track F / DCP rather than replacing H04-R02 in Track R.

---

## 6. Verification and Build Integrity Summary

- **Research Regressions Test Suite:** Executed `python3 scripts/ci/run_research_regressions.py --build-dir build`. Result: 8/8 tests passed (100% pass rate).
- **Probes Execution:** Executed `python3 docs/research/continuity/probes/storage_boundary.py`. Result: Verified exact storage boundary calculations and counterexamples.
- **Link Integrity Check:** Validated all Markdown links in `README.md`, `docs/research/T81_FULL_REASSESSMENT_2026-10-25.md`, and `docs/research/README.md`. Result: 0 broken links.
