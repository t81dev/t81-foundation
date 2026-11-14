# SCG-08 Carry Propagation & Arithmetic Work Reconciliation Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [SCG-08 Carry Propagation & Arithmetic Work Reconciliation Report](#scg-08-carry-propagation-&-arithmetic-work-reconciliation-report)
  - [1. Executive Summary & Authoritative Dispositions](#1-executive-summary-&-authoritative-dispositions)
  - [2. Statistical Protocol Reconciliation: Contiguous Markov Stream vs. Stratified Clustered Sequences](#2-statistical-protocol-reconciliation-contiguous-markov-stream-vs-stratified-clustered-sequences)
    - [2.1 The Two Competing Experimental Designs](#21-the-two-competing-experimental-designs)
    - [2.2 Mathematical Non-Equivalence: Boundary Transient Carry Deficit](#22-mathematical-non-equivalence-boundary-transient-carry-deficit)
    - [2.3 Corrected Within-Stratum Centered Variance Estimator](#23-corrected-within-stratum-centered-variance-estimator)
      - [Sampling Assumptions](#sampling-assumptions)
      - [Estimator Definitions](#estimator-definitions)
      - [Stratified Within-Stratum Centered Variance Formula](#stratified-within-stratum-centered-variance-formula)
  - [3. Authoritative Provenance Register (Thresholds & Stopping Rules)](#3-authoritative-provenance-register-thresholds-&-stopping-rules)
  - [4. Evidence Inventory & Traceability Matrix](#4-evidence-inventory-&-traceability-matrix)
  - [5. Sequential Governance Roadmap to Gate 2](#5-sequential-governance-roadmap-to-gate-2)

<!-- T81-TOC:END -->


**Document ID:** `T81-SCG-08-RECONCILIATION-REPORT-20261111`
**Classification:** Research Continuity & Mathematical Reconciliation
**Date:** 2026-11-11
**Investigator:** Jules (T81 Research Continuity Assistant)
**Parent Charter:** `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md` (PR #753, commit `0bd0817e4b9dc879fffce18c59ea9ff781f7acb3`)
**Evaluated Branch & Evidence Archive:** PR #768 (`docs/scg08-jules-evidence-handoff-20261111`, commit `a7fca2fbc4c5c6dfdb861c37fe434c481ca5737b`)
**Historical Source Evidence:** [`docs/research/continuity/SCG08_JULES_COMPLETE_EVIDENCE_HANDOFF_2026-11-11.md`](SCG08_JULES_COMPLETE_EVIDENCE_HANDOFF_2026-11-11.md)
**Implementation Baseline:** `experiments/scg08_arithmetic_work/` (PR #760, #763, #765 on `main` at `36b3849aa1e6297e88de2ec1655777793ab0b59e`)
**Prospective Specification Draft:** [`spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md`](../../../spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md)

---

## 1. Executive Summary & Authoritative Dispositions

This document provides the independent mathematical and governance reconciliation for research program **SCG-08: Balanced-Ternary Carry Propagation and Arithmetic Work**, evaluating the evidence handoff archived in PR #768.

| Governance Dimension | Finding / Authoritative Disposition |
|:---|:---|
| **Authoritative Frozen Specification Text (`ADDENDUM-06`)** | **`NOT VERIFIED`** — The physical, frozen contract text does not exist as an independent specification document in Git history, historical branches, or merged spec directories. Exists strictly as an external citation string. |
| **Historical Dated Gate 1 Implementation Authorization** | **`NOT VERIFIED`** — No maintainer directive, approval record, or recorded decision granting Gate 1 implementation authorization exists. Parent Charter explicitly records Gate 1 as **NOT GRANTED**. |
| **Gate 1 1,000-Trial Execution Ceiling** | **`IMPLEMENTED / HISTORICAL — PREREGISTRATION NOT VERIFIED`** — Enforced in implementation code (`tier2_work_harness.hpp:67`) and reported in Gate 1 report; not present in the frozen Phase 0 Charter. |
| **Existing Gate 1 Implementation (`experiments/scg08_arithmetic_work/`)** | **`IMPLEMENTATION DESCRIPTION ONLY`** — Subject to separate independent technical and evidentiary acceptance; merging code does not establish specification authority or maintainer approval. |
| **Gate 2 Experimental Execution Status** | **`STRICT NO-GO / STRICTLY WITHHELD`** — No experimental campaign, benchmarking, or execution authorized or performed. |

---

## 2. Statistical Protocol Reconciliation: Contiguous Markov Stream vs. Stratified Clustered Sequences

### 2.1 The Two Competing Experimental Designs

| Parameter / Dimension | Registered Charter Protocol (§5.3, §10.3) | Proposed Clustered Design (Addendum 03 / Report) |
|:---|:---|:---|
| **Sampling Entity** | Contiguous elementary digit steps | Stratified sequence clusters across 6 length bands |
| **Sample Size** | $M = 10,000,000$ steps ($10^7$) | $K = 136,500$ clusters ($48,128,000$ total digits) |
| **Boundary Initialization** | Single initial $c_0 = 0$, asymptotic mixing | Boundary condition $c_0 = 0$ re-initialized $136,500$ times |
| **Serial Correlation** | Intra-stream mixing with $\lambda_3 = 1/9$ | Intra-sequence correlation; inter-sequence independence |
| **Variance Model** | Asymptotic mixing $M_{\text{eff}} \approx 0.8M$ | Stratified within-stratum centered variance estimator |
| **Governance Status** | **`PREREGISTERED`** | **`PROPOSED / UNRATIFIED`** |

### 2.2 Mathematical Non-Equivalence: Boundary Transient Carry Deficit
Starting from quiescent carry state $c_0 = 0$, the probability of an active carry at step $k$ follows:
$$P(c_k \neq 0 \mid c_0 = 0) = \frac{1}{4}\left(1 - \left(\frac{1}{9}\right)^k\right)$$
The transient deficit at step $k$ relative to the stationary distribution $\pi(c \neq 0) = 1/4$ is:
$$\Delta P_k = P(c_k \neq 0 \mid c_0 = 0) - \frac{1}{4} = -\frac{1}{4}\left(\frac{1}{9}\right)^k$$
Summing the transient deficit over a sequence of length $L \ge 64$:
$$\sum_{k=1}^L \Delta P_k = -\frac{1}{4} \sum_{k=1}^L \left(\frac{1}{9}\right)^k \approx -\frac{1}{4} \cdot \frac{1/9}{1 - 1/9} = -\frac{1}{32} \text{ carries per sequence}$$
Across the proposed allocation of 136,500 independent sequences, boundary resets induce a cumulative deficit of:
$$\Delta N_{\text{active, tot}} = 136,500 \times \left(-\frac{1}{32}\right) = -4,265.625 \text{ carries}$$

Because finite sequences with boundary reset $c_0 = 0$ systematically suppress early carries, the expected carry occurrence rate in finite sequences is strictly below $1/4$:
$$\mathbb{E}[\hat{R}_b] = \frac{1}{4} - \frac{1 - (1/9)^{L_b}}{32 L_b} < \frac{1}{4}$$
Therefore, the clustered sequence protocol measures a mixture of transient and stationary dynamics, whereas the charter's contiguous $10^7$-step protocol measures asymptotic stationary mixing. They are mathematically distinct experimental regimes and cannot be declared equivalent without a formal protocol amendment.

### 2.3 Corrected Within-Stratum Centered Variance Estimator

#### Sampling Assumptions
1. **Fixed-Allocation Partitioning:** Sampling allocates fixed cluster counts $K_b$ across $B = 6$ length strata $b \in \{1, \dots, B\}$ with sequence lengths $L_b \in \{64, 128, 256, 512, 1024, 2048\}$.
2. **Mutual Independence:** Every sequence $k \in \{1, \dots, K_b\}$ is generated independently via deterministic PRNG seeds ($0\text{x5C608001ULL}$). Sequences are mutually independent across and within strata.
3. **Digit Totals:** Total evaluated digits $V_{\text{tot}} = \sum_{b=1}^B K_b L_b = 48,128,000$. Stratum digit weights $w_b = \frac{K_b L_b}{V_{\text{tot}}}$.

#### Estimator Definitions
- Per-band sample mean carries: $\bar{U}_b = \frac{1}{K_b} \sum_{k=1}^{K_b} U_{bk}$.
- Per-band carry frequency estimator: $\hat{R}_b = \frac{\bar{U}_b}{L_b}$.
- Pooled digit-weighted frequency estimator:
  $$\hat{R} = \frac{\sum_{b=1}^B \sum_{k=1}^{K_b} U_{bk}}{V_{\text{tot}}} = \sum_{b=1}^B w_b \hat{R}_b$$

#### Stratified Within-Stratum Centered Variance Formula
Under fixed allocation, weights $w_b$ and lengths $L_b$ are deterministic constants. Because strata are sampled independently:
$$\mathbb{V}(\hat{R}) = \sum_{b=1}^B w_b^2 \mathbb{V}(\hat{R}_b) = \sum_{b=1}^B \left(\frac{K_b L_b}{V_{\text{tot}}}\right)^2 \frac{1}{L_b^2} \frac{\mathbb{V}(U_{bk})}{K_b} = \frac{1}{V_{\text{tot}}^2} \sum_{b=1}^B K_b \mathbb{V}(U_{bk})$$
Estimating $\mathbb{V}(U_{bk})$ with the unbiased sample variance $s_{U,b}^2 = \frac{1}{K_b - 1} \sum_{k=1}^{K_b} (U_{bk} - \bar{U}_b)^2$, where **each cluster is centered on its own stratum sample mean $\bar{U}_b$**:
$$\hat{\mathbb{V}}_{\text{strat}}(\hat{R}) = \frac{1}{V_{\text{tot}}^2} \sum_{b=1}^B \frac{K_b}{K_b - 1} \sum_{k=1}^{K_b} \left(U_{bk} - \bar{U}_b\right)^2 = \frac{1}{V_{\text{tot}}^2} \sum_{b=1}^B \frac{K_b}{K_b - 1} \sum_{k=1}^{K_b} \left(U_{bk} - \hat{R}_b L_b\right)^2$$

**Correction Note:** Centering on the stratum mean $\bar{U}_b = L_b \hat{R}_b$ (rather than the global pooled quantity $L_b \hat{R}$) ensures that between-strata differences in mean carry counts (induced by varying sequence lengths $L_b$ and transient boundary effects) are **not** inflated into the variance estimator.

---

## 3. Authoritative Provenance Register (Thresholds & Stopping Rules)

| Criterion / Parameter | Status | Authoritative Source Citation | Governance Assessment |
|:---|:---|:---|:---|
| **$f_{\text{gen}|0} \in [0.217, 0.227]$** | **`PREREGISTERED`** | Charter §10.5 (line 499) | Theoretical $2/9 \approx 0.2222$; frozen in Phase 0. |
| **$f_{\text{trans}} \in [0.161, 0.172]$** | **`PREREGISTERED`** | Charter §10.5 (line 500) | Theoretical $1/6 \approx 0.1667$; frozen in Phase 0. |
| **$f_{\text{occ}} \in [0.245, 0.255]$** | **`PREREGISTERED`** | Charter §10.5 (line 501) | Theoretical $1/4 = 0.2500$; primary stationary frequency. |
| **$f_{\text{cont}|\text{act}} \in [0.328, 0.338]$** | **`PREREGISTERED`** | Charter §10.5 (line 502) | Theoretical $1/3 \approx 0.3333$; continuation rate. |
| **$\bar{L}_{\text{run}} \in [1.450, 1.550]$ trits** | **`PREREGISTERED`** | Charter §10.5 (line 503) | Theoretical $E[L] = 1.500$ trits; geometric expectation. |
| **$W_{\text{ops}}$ parity $\le 1.10\times$ at $\ge 512$ bits** | **`PREREGISTERED`** | Charter §10.5 (line 504) | Algorithmic operation work parity target for Tier 2. |
| **Falsification: $W_{\text{ops}} > 2.0\times$ across all bands** | **`PREREGISTERED`** | Charter §10.5 (line 506), §9 Table | Universal stopping rule: immediate H02 falsification. |
| **Gate 1 Ceiling: $\le 1,000$ cumulative trials** | **`IMPLEMENTED / HISTORICAL — PREREGISTRATION NOT VERIFIED`** | `tier2_work_harness.hpp:67`; `GATE1_IMPLEMENTATION_REPORT.md` | Absent from Charter; introduced directly into implementation code. |
| **Falsification: Ternary $f_{\text{occ}} > 0.35$ or $\bar{L}_{\text{run}} > 2.0$** | **`PROPOSED`** | None in Charter; derived in Assessment | **NOT PREREGISTERED**. Must be ratified in Gate 2 authorization. |
| **$M_{\text{eff}} \approx 0.8M$ effective sample variance** | **`PROPOSED`** | Phase 0 Review R2 §2 | Asymptotic qualification; not in charter threshold table. |
| **Signed Domain $[-2^{B-1}, 2^{B-1}-1]$** | **`PROPOSED`** | Addendum 03; Status Report | Prospective standardization drafted in RFC-00E1. |

---

## 4. Evidence Inventory & Traceability Matrix

Every candidate source referencing `T81-SCG-08-PHASE1-PROP-001`, `ADDENDUM-06`, or Gate 1 authorization was inspected and categorized:

| Source Reference | Commit SHA | Date | Document Identity / Path | Category | Establishes Authorization? |
|:---|:---|:---|:---|:---|:---:|
| **Parent Charter (PR #753)** | `0bd0817e4b` (head `09c666e36a`) | 2026-11-10 17:02:14Z | `T81-SCG-08-PHASE0-CHARTER-001-R2` / `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md` | **Frozen Specification Text** (Phase 0) | **NO** — Explicitly specifies Gate 1: **NOT GRANTED**, Gate 2: **NOT GRANTED**. |
| **PR #760 Initial Gate 1 Harness** | `dab59a63e4` / `525554a193` | 2026-11-10 20:07:36Z | `T81-SCG-08-GATE1-REPORT-001` / `experiments/scg08_arithmetic_work/` | **Implementation Description** | **NO** — Open, unmerged PR citing Addendum 06 as an external spec reference string; contains no authorization grant. |
| **PR #761 Independent Acceptance Review R2** | `5c69f11579` | 2026-11-10 21:06:23Z | `SCG08_PHASE0_INDEPENDENT_ACCEPTANCE_REPORT.md` | **Retrospective Reporting** | **NO** — Open draft PR. Retrospectively asserts Phase 1 was authorized under separate directives, but cites no artifact SHA or maintainer record. |
| **PR #763 Gate 1 Evidence Repair** | `dbf107fda8` (head `93a3d1bc36`, commit `afa0714c4e`) | 2026-11-10 22:18:11Z | PR #763 Merge / `experiments/scg08_arithmetic_work/` | **Implementation Description** | **NO** — Merged corrective PR resolving table lookups and conditions; merging code does not infer governance authorization. |
| **PR #765 Codex Findings Resolution** | `36b3849aa1` (head `2dd3cc62e5`) | 2026-11-11 00:56:56Z | PR #765 Merge / `experiments/scg08_arithmetic_work/` | **Implementation Description** | **NO** — Merged corrective PR resolving Codex findings; PR body explicitly requires: `READY FOR INDEPENDENT REVIEW — ACCEPTANCE WITHHELD`. |
| **PR #768 Evidence Handoff Archive** | `a7fca2fbc4` | 2026-11-10 22:39:33 EST | `docs/research/continuity/SCG08_JULES_COMPLETE_EVIDENCE_HANDOFF_2026-11-11.md` | **Retrospective Evidence Archive** | **NO** — Archives unedited historical reports (Appendices A, B, C) for cross-agent reconciliation. |
| **RFC-00E1 (This Proposal)** | Current Branch Tip | 2026-11-11 | `spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md` | **Prospective Standards-Track RFC Draft** | **NO** — Awaiting formal maintainer review and signature. |

---

## 5. Sequential Governance Roadmap to Gate 2

To proceed lawfully toward Gate 2 without fabricating historical approvals:

```
[CURRENT MAIN BASELINE: 36b3849] (Gate 2 STRICT NO-GO)
       │
       ▼
[STEP 1: Merge PR #766 / Fix Schema & Evidence Binding]
       ├── Decouples baseline_sha from evaluated_implementation_sha.
       └── Fixes scalar integer type validation in validate_scg08_gate1_evidence.py.
       │
       ▼
[STEP 2: Ratify Prospective Specification RFC-00E1]
       ├── Formally codifies signed domain [-2^(B-1), 2^(B-1)-1], accumulator sizing table,
       │   frozen W_ops metric, 38 table lookups per addc(), and budget constraints.
       └── Requires explicit maintainer review and approval (closing the missing Addendum 06 gap).
       │
       ▼
[STEP 3: Issue Formal Gate 1 Acceptance Decision]
       └── Maintainer signs formal decision confirming implementation & accounting correctness.
       │
       ▼
[STEP 4: Formalize Preregistration Protocol Amendment]
       └── Explicitly authorize either Option A (10^7 contiguous steps) or Option B (136,500 clustered
           sequences with within-stratum centered variance estimation).
       │
       ▼
[STEP 5: Execute Gate 2 Authorization Record]
       └── Maintainer signs Gate 2 authorization template. Gate 2 transitions to GO.
```

*Status: Delivered as a read-only governance and reconciliation report under draft PR. Zero production code edits, zero experimental campaigns, and zero automatic merges performed.*
