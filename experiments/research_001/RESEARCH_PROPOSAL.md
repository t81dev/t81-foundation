# Research Proposal: Evaluation of Deterministic Artifact Execution vs. Conventional Content-Addressed Pipelines

**Protocol ID:** Research Protocol 001
**Target Directory:** `experiments/research_001/`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Status:** Under Proposal Review

---

## 1. Problem Statement

Modern enterprise and agentic software architectures increasingly depend on policy-governed, automated decision-making pipelines. In high-stakes domains—such as autonomous financial transactions, medical classification, and supply chain ordering—executing computational workloads requires strict guarantees of **reproducibility**, **auditability**, **pre-side-effect policy enforcement**, and **tamper-evident provenance**.

To achieve these guarantees, systems typically adopt one of two competing paradigms:

1. **Vertically Integrated Deterministic Runtimes (e.g., T81 Stack):** A custom, tightly coupled architecture where determinism, content addressing (CanonFS), and policy evaluation (Axion Kernel) are baked directly into the low-level virtual machine dispatch loop (T81VM/TISC).
2. **Conventional Content-Addressed Execution Pipelines:** A decoupled stack utilizing off-the-shelf binary runtimes (e.g., WebAssembly / Wasmtime or OCI containers), standard cryptographic hashing (SHA256 / BLAKE3), detached policy sidecars (e.g., Open Policy Agent / eBPF), and standardized evidence schemas (e.g., RFC-00D4 Accountable Decision Bundles).

While vertically integrated runtimes claim superior safety and non-bypassable policy interception, they introduce significant engineering complexity, custom ISA maintenance taxes, and non-standard toolchain friction. Conversely, decoupled conventional pipelines offer lower latency and standard tooling, but are often presumed to be vulnerable to side-effect bypass, mid-flight policy mutations, or environmental non-determinism.

To date, these competing claims have not been subjected to a rigorous, fair, and head-to-head empirical comparison under a standardized adversarial workload.

---

## 2. Core Research Question

> **Does T81's vertically integrated deterministic artifact execution pipeline provide measurable technical advantages in reproducibility, policy enforcement, and auditability over a conventional content-addressed execution pipeline under identical adversarial conditions?**

---

## 3. Key Objectives

1. **Formalize a Minimum Viable Workload:** Establish a realistic, production-relevant computational task ("Inventory Replenishment Under Constraint") executable by both T81 and conventional reference architectures.
2. **Construct a Standardized Conventional Reference Baseline:** Define and specify an off-the-shelf reference baseline using WebAssembly (Wasmtime), SQLite, Open Policy Agent (OPA), and RFC-00D4 evidence schemas.
3. **Execute Adversarial Stress Protocols:** Evaluate both systems against four concrete threat scenarios:
   - Adversarial prompt/parameter injection attempting spending limit bypass.
   - Mid-flight policy revocation between calculation and side-effect execution.
   - Stale or corrupted data/provenance injection.
   - Detached, offline third-party audit verification.
4. **Quantify Performance and Complexity Taxes:** Measure execution latency, memory footprint, evidence bundle size, lines of code (LOC), dependency overhead, and cross-platform verification agreement ($N \ge 30$ samples per host).
5. **Establish Falsification and Pivoting Boundaries:** Provide a definitive, data-driven basis for deciding whether T81's bespoke VM/ISA kernel coupling is strictly necessary, or whether computational accountability is best delivered as a runtime-agnostic protocol.

---

## 4. Scope and Non-Scope

### In Scope
- Comparative analysis of T81VM + Axion + CanonFS (Arm ii) vs. Wasmtime + OPA + RFC-00D4 (Arm iii) vs. Python + FastAPI + OPA (Arm i).
- Verification of bit-exact determinism across x86_64 and ARM64 platforms.
- Pre-side-effect policy intercept and fail-closed behavior under policy mutation.
- Detached, offline replay verification of execution evidence bundles.
- Empirical quantification of engineering complexity and execution overhead.

### Out of Scope
- Modifying production T81VM interpreter C++ code or frozen TISC v1.9.0 ISA opcodes.
- Redesigning hardware ternary SIMD or AVX2/NEON packed arithmetic kernel internals.
- Implementing production benchmarking suites prior to protocol approval.
- Evaluating non-deterministic floating-point or stochastic LLM inference models (the workload uses deterministic fixed-point / integer decision arithmetic).

---

## 5. Expected Scientific Outcomes

1. **Outcome A (T81 Architectural Advantage):** Empirical proof that T81's in-kernel policy evaluation prevents mid-flight side-effect breaches that decoupled pipelines fail to stop, justifying custom VM/ISA overhead.
2. **Outcome B (Conventional Equivalence / Protocol Pivot):** Empirical proof that a conventional Wasmtime + RFC-00D4 pipeline provides bit-exact reproducibility and equivalent fail-closed safety with $\ge 5\times$ lower latency and zero custom VM tax, proving that computational accountability is a **protocol, not a runtime**.
3. **Outcome C (Environmental Dependence):** Empirical proof that observed differences stem strictly from serialization formats or host platform configurations rather than intrinsic VM/kernel properties.
