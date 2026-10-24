# T81 Experimental Research Charter: Research Protocol 001

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/RESEARCH_CHARTER.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Active Research Governance Directive

---

## 1. Preamble & Context

This Charter governs all experimental inquiries under **Research Protocol 001** of the T81 Foundation.

The T81 Foundation explicitly re-frames the T81 computational stack (comprising TISC ISA v1.9.0, T81VM deterministic runtime, Axion Policy Kernel, CanonFS immutable file system, and balanced ternary SWAR emulation) not as a static product specification to be defended, but as **one specific empirical hypothesis under active test**.

The primary purpose of Research Protocol 001 is to evaluate whether T81's vertically integrated, deterministic artifact execution pipeline provides measurable technical or operational advantages over a conventional, off-the-shelf content-addressed execution pipeline when executing identical computational workloads under identical governance constraints.

---

## 2. Core Research Principles

All research executed under this protocol MUST adhere strictly to seven governing principles:

### 2.1 Falsifiability
Every architectural claim made regarding T81 or conventional alternatives MUST be expressed as a falsifiable hypothesis. For every claim, explicit empirical observations MUST be specified in advance that, if observed, will formally invalidate the claim. Claims that cannot be falsified by empirical observation are classified as non-scientific and excluded from evaluation.

### 2.2 Fair Comparison
Comparative benchmarks MUST NOT use T81-specific primitives or specialized API constructs to define success in ways that artificially disadvantage conventional reference baselines.
- Both systems MUST execute logically equivalent workloads using identical input data corpora.
- Both systems MUST operate under identical host resource limits (CPU quotas, memory allocations, thread counts) and security privileges.
- Verification requirements MUST be defined in runtime-agnostic terms (e.g., bit-exact output agreement, policy compliance, signature validity, verification latency).

### 2.3 Independent Validation
Every experimental result MUST be independently verifiable by a third party operating outside the producer environment:
- Verification procedures MUST NOT require importing producer-specific libraries or running in-process runtime components.
- Verification MUST rely exclusively on standalone verification scripts operating on public, detached evidence artifacts (e.g., JSON schemas, canonical hashes, standard signatures).

### 2.4 Evidence Preservation
Raw measurement data, environment manifests, compiler flags, and complete canonical evidence bundles MUST be recorded and immutably preserved for every experimental run.
- Truncated summary statistics without raw observations are forbidden.
- Flawed, anomalous, or unexpected test runs MUST be preserved in evidence registers rather than silently discarded or replaced.

### 2.5 Negative-Result Publication
Negative or unexpected results—including cases where conventional baselines outperform T81, or where T81 mechanisms incur unacceptable performance or complexity taxes—MUST be documented and published with equal prominence as positive results. The suppression or selective reporting of benchmark data is strictly prohibited.

### 2.6 Bounded Claims
Claims derived from experimental outcomes MUST be bounded strictly to the domain, workload, and platform configurations tested.
- Passing a synthetic integer workload test MUST NOT be generalized into claims of general OS superiority or floating-point determinism.
- Empirical advantages observed on one target architecture (e.g., x86_64 AVX2) MUST NOT be asserted for other target architectures (e.g., ARM64 or RISC-V) without direct experimental measurement.

### 2.7 Stopping Discipline & Statistical Validity
Every experiment MUST operate under predefined stopping criteria, explicit statistical confidence intervals, and minimum sample size bounds ($N \ge 30$). Experimental runs MUST terminate immediately upon reaching predefined statistical confidence thresholds or falsification conditions. Iterating tests selectively until a desired result is achieved ("p-hacking" or benchmark overfitting) is forbidden. Zero observed failures in $N=30$ trials yield a Clopper-Pearson 95% upper confidence bound of $\le 9.50\%$ failure rate; expanded testing ($N=300$) is required to bound zero-failure rates to $\le 0.99\%$.

---

## 3. Scope and Governance Boundaries

1. **Production Code Immutability:** Research Protocol 001 is a research-design and empirical evaluation protocol. It DOES NOT modify production implementations (`src/`, `include/t81/`), frozen ISAs (`spec/tisc/`), or existing operational contracts (`RFC-0054`, `RFC-0002`).
2. **Experimental Isolation:** All protocol specifications, baseline models, negative controls, measurement plans, and evidence manifests reside strictly within `experiments/research_001/`.
3. **Restricted Pivoting Scope:** If experimental results establish Outcome B (Conventional Equivalence) or Outcome C (Environmental Dependence) under the predefined stopping criteria, conclusions and engineering pivots MUST be restricted strictly to the evaluated workload and measured security/performance guarantees. Automatic ecosystem-wide architectural pivots are forbidden; broader changes require separate empirical research and explicit review.

---

## 4. Methodological Traceability & Defect Resolution Matrix

The following table maps all methodological corrections made under this protocol update to their affected documents, corrective actions, and verification methods:

| Defect ID | Summary Description | Affected Documents | Corrective Action Taken | Validation Method |
| :--- | :--- | :--- | :--- | :--- |
| **DEF-01** | NC-01 JSON canonicalization vs syntax error conflation. | `NEGATIVE_CONTROLS.md` | Separated valid JSON key-reordering (`ERR_CANONICAL_HASH_MISMATCH`) from malformed JSON syntax errors (`ERR_JSON_SYNTAX_ERROR`). | Synthetic JSON unit tests in `verify.py`. |
| **DEF-02** | Ed25519 verification dependency unspecified in Python stdlib. | `BASELINE_SPECIFICATION.md`, `REPRODUCIBILITY.md` | Specified Python `cryptography` package as explicit external dependency with stdlib HMAC-SHA256 fallback option. | Dependency check in `verify.py`. |
| **DEF-03** | Contradictory Outcome B latency/complexity thresholds ($2\times$ vs $5\times$). | `HYPOTHESES.md`, `STOPPING_CRITERIA.md`, `RESEARCH_PROPOSAL.md` | Harmonized threshold: Arm (iii) achieves equal or lower decision latency and $\ge 5\times$ lower host runtime LOC footprint. | Cross-document consistency review. |
| **DEF-04** | Negative controls lacked clear fault taxonomy and trust boundaries. | `NEGATIVE_CONTROLS.md` | Classified controls into Malformed Evidence, Malicious Guest Behavior, and Malicious Host Behavior; added NC-08 host boundary control. | Control matrix audit. |
| **DEF-05** | Hypotheses H1, H2, H3 presented as mutually exclusive. | `HYPOTHESES.md`, `RESEARCH_PROPOSAL.md` | Reframed H1, H2, H3 as nonexclusive explanatory models; added explicit reporting rules for mixed/inconclusive outcomes. | Hypothesis logic review. |
| **DEF-06** | Automatic ecosystem-wide architectural pivots in stopping criteria. | `STOPPING_CRITERIA.md`, `RESEARCH_CHARTER.md` | Restricted conclusions and pivots strictly to evaluated workload; required separate research for broader ecosystem shifts. | Governance scope review. |
| **DEF-07** | Unresolved design: OPA IPC overhead vs in-process execution. | `BASELINE_SPECIFICATION.md`, `EXPERIMENT_PROTOCOL.md` | Primary Arm (iii) uses in-process OPA C/Rust library; sidecar IPC evaluated as secondary measurement. | Harness execution flag check. |
| **DEF-08** | Unresolved design: Mid-flight revocation hook mechanism. | `EXPERIMENT_PROTOCOL.md`, `BASELINE_SPECIFICATION.md` | Injected explicit synchronous `post_calc_pre_commit` callback hook in workload dispatch sequence. | Revocation sequence trace audit. |
| **DEF-09** | Unresolved design: Evidence format canonicalization bias. | `BASELINE_SPECIFICATION.md`, `MEASUREMENT_PLAN.md` | Standardized primary benchmarks on common RFC 8785 JCS JSON; recorded native formats as secondary metrics. | Verification pipeline check. |
| **DEF-10** | Unverified claims regarding RFC-00D4 standards status. | All documents | Labeled RFC-00D4 accurately as an internal experimental specification draft proposal (`spec/rfcs/RFC-00D4-accountable-decision-bundle.md`). | Documentation search audit. |
| **DEF-11** | $N=30$ sample size claimed zero failure probability. | `MEASUREMENT_PLAN.md`, `EXPERIMENT_PROTOCOL.md` | Added Clopper-Pearson 95% confidence bounds ($N=30 \le 9.50\%$, $N=300 \le 0.99\%$), $CI_{95}$, and non-parametric hypothesis tests. | Statistical formula validation. |
| **DEF-12** | Inconsistent dates, versions, cross-references, and terms. | All documents | Normalized all metadata to April 2026, Version v1.1.0, and standardized cross-file link references. | Document audit. |
