# Research Protocol 001 — Methodological Defect Traceability Matrix

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/TRACEABILITY.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Approved Methodological Traceability Contract

---

## 1. Overview

This document provides a comprehensive, auditable traceability matrix mapping every methodological defect identified in PR #561 review to its affected documents, applied corrections, and validation methods under **Research Protocol 001**.

---

## 2. Defect Traceability & Corrective Action Matrix

| Defect ID | Defect Summary | Affected Document(s) | Corrective Action Applied | Validation Method |
| :--- | :--- | :--- | :--- | :--- |
| **DEF-01** | Control NC-01 conflated JSON syntax validation with JCS key-reordering canonicalization. | `NEGATIVE_CONTROLS.md` | Explicitly defined the wire/storage trust model requiring bit-exact RFC 8785 JCS bytes. Clarified that NC-01 fails with `ERR_CANONICAL_HASH_MISMATCH` under byte-level JCS verification, distinguished from `ERR_JSON_SYNTAX_ERROR` in NC-01B. | Synthetic JSON canonicalization unit tests in `verify.py`. |
| **DEF-02** | Ed25519 verification dependency was inaccurately presented as part of standard Python stdlib. | `BASELINE_SPECIFICATION.md`, `REPRODUCIBILITY.md`, `EXPERIMENT_PROTOCOL.md` | Specified that Ed25519 signature verification in Python 3.12 requires the external `cryptography` PyPI package (`pip install cryptography`), with stdlib HMAC-SHA256 as an optional fallback. | Dependency check script in `verify.py`. |
| **DEF-03** | Outcome B performance and host complexity thresholds were contradictory across documents ($2\times$ vs $5\times$). | `HYPOTHESES.md`, `STOPPING_CRITERIA.md`, `RESEARCH_PROPOSAL.md`, `CLAIM_REGISTER.md` | Harmonized threshold across all files: Arm (iii) achieves equal or lower decision execution latency AND $\ge 5\times$ lower host runtime LOC footprint. | Cross-document automated grep and text consistency audit. |
| **DEF-04** | Negative controls lacked a structured threat taxonomy and explicit trust boundary audit. | `NEGATIVE_CONTROLS.md` | Categorized all controls into Evidence Boundary (NC-01, NC-01B, NC-04, NC-05), Guest Boundary (NC-02, NC-03, NC-06), and Host Boundary (NC-07, NC-08). | Control matrix threat taxonomy audit. |
| **DEF-05** | Hypotheses H1, H2, and H3 were presented as mutually exclusive competing outcomes. | `HYPOTHESES.md`, `RESEARCH_PROPOSAL.md`, `STOPPING_CRITERIA.md` | Reframed H1, H2, and H3 as nonexclusive explanatory models. Added explicit reporting rules for mixed, inconclusive (`INCONCLUSIVE_INSUFFICIENT_DELTA`), and unsupported outcomes. | Hypothesis logic and reporting rules audit. |
| **DEF-06** | Stopping criteria permitted automatic ecosystem-wide architectural pivots and ISA deprecation. | `STOPPING_CRITERIA.md`, `RESEARCH_CHARTER.md` | Removed automatic ecosystem-wide pivots. Restricted directives strictly to the evaluated workload class; broader shifts require separate research and explicit review. | Governance scope review. |
| **DEF-07** | OPA policy engine transport bias (IPC sidecar vs in-process FFI) was unresolved. | `BASELINE_SPECIFICATION.md`, `EXPERIMENT_PROTOCOL.md`, `HYPOTHESES.md` | Specified in-process OPA C/Rust FFI bindings for primary Arm (iii) benchmark. Asynchronous sidecar IPC transport is evaluated separately as a secondary variable under H3. | Harness configuration execution check. |
| **DEF-08** | Mid-flight policy revocation injection mechanism lacked deterministic execution specification. | `EXPERIMENT_PROTOCOL.md`, `BASELINE_SPECIFICATION.md` | Injected a synchronous `post_calc_pre_commit` callback hook directly inside the dispatch sequence prior to policy evaluation and storage write. | Revocation sequence trace audit. |
| **DEF-09** | Common evidence format canonicalization vs native tryte format measurements were unresolved. | `BASELINE_SPECIFICATION.md`, `EXPERIMENT_PROTOCOL.md`, `MEASUREMENT_PLAN.md` | Standardized primary comparative benchmarks on RFC 8785 Canonical JSON (JCS) conforming to internal RFC-00D4 draft schema. Native tryte/binary formats are recorded as secondary metrics. | Verification pipeline check. |
| **DEF-10** | Unverified claims presented RFC-00D4 as an established industry standard. | All 10 protocol documents | Labeled RFC-00D4 accurately across all files as an internal experimental specification draft proposal (`spec/rfcs/RFC-00D4-accountable-decision-bundle.md`). | Repository-wide documentation search audit. |
| **DEF-11** | Sample size $N=30$ was falsely claimed to establish zero failure probability ($p=0$). | `MEASUREMENT_PLAN.md`, `STOPPING_CRITERIA.md`, `RESEARCH_CHARTER.md`, `EXPERIMENT_PROTOCOL.md` | Added Clopper-Pearson 95% confidence bounds ($N=30 \le 9.50\%$, $N=300 \le 0.99\%$). Defined Student's t / bootstrap $CI_{95}$ and non-parametric hypothesis tests. | Statistical formula audit. |
| **DEF-12** | Inconsistent dates, versions, cross-references, and terminology across research documents. | All 10 protocol documents | Normalized all headers to Date: April 2026, Version: v1.1.0, Location: `experiments/research_001/`, and standardized Markdown links. | Metadata normalization audit. |
