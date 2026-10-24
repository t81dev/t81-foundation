# T81 Experimental Research Charter: Research Protocol 001

**Date:** April 2026
**Status:** Active Research Governance Directive
**Location:** `experiments/research_001/RESEARCH_CHARTER.md`

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

### 2.7 Stopping Discipline
Every experiment MUST operate under predefined stopping criteria and sample size bounds ($N \ge 30$). Experimental runs MUST terminate immediately upon reaching predefined confidence intervals or falsification thresholds. Iterating tests selectively until a desired result is achieved ("p-hacking" or benchmark overfitting) is forbidden.

---

## 3. Scope and Governance Boundaries

1. **Production Code Immutability:** Research Protocol 001 is a research-design and empirical evaluation protocol. It DOES NOT modify production implementations (`src/`, `include/t81/`), frozen ISAs (`spec/tisc/`), or existing operational contracts (`RFC-0054`, `RFC-0002`).
2. **Experimental Isolation:** All protocol specifications, baseline models, negative controls, measurement plans, and evidence manifests reside strictly within `experiments/research_001/`.
3. **Pivoting Authority:** If experimental results establish Outcome B (Conventional Equivalence) or Outcome C (Environmental Dependence) under the predefined stopping criteria, the T81 Foundation will formally pivot architectural investment toward lightweight protocol standards (e.g., RFC-00D4 Accountable Decision Bundles) without requirement to complete unproven VM or ISA components.
