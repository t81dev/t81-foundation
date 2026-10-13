# T81 Research Charter: Accountable Computation Substrate

**Date:** April 2026
**Status:** Operational Research Directive
**Target Surface:** `docs/research/RESEARCH_CHARTER.md`

---

## 1. Context and Strategic Shift

This Charter establishes a temporary suspension of roadmap pressure to "complete" or "finish" the current T81 stack architecture (including TISC v1.9.0 ISA, T81VM runtime, Axion Policy OS kernel, CanonFS file system, and balanced ternary SWAR emulation).

Rather than treating the existing C++ codebase as a product specification awaiting completion, the T81 Foundation re-frames the entire stack as **one specific, empirical hypothesis under active test**.

### Core Research Question
> **Can we create a computing environment in which the identity, authority, evidence, and consequences of computation are first-class, composable objects—regardless of whether the computation itself is deterministic, probabilistic, binary, or ternary?**

### The Six Primitives of an Accountable Computational Act
Every execution unit under evaluation is decomposed into six primitive properties:
1. **Meaning:** What state transition or decision was intended by the operation?
2. **Evidence:** What verifiable artifact proves what actually executed?
3. **Method:** Under what runtime, model, or transformation rule was the output produced?
4. **Authority:** What capability or policy permitted this action prior to side effects?
5. **Accountability:** Who or what entity bears legal, financial, or operational responsibility?
6. **Challengeability:** Can an independent party replay, audit, or falsify the output?

---

## 2. Falsification Conditions

To maintain scientific objectivity, the project defines explicit, falsifiable conditions under which core architectural commitments will be retired or superseded:

### (a) Custom Virtual Machine Necessity
* **Hypothesis:** A bespoke ISA/VM (TISC/T81VM) is required to guarantee pre-side-effect policy evaluation and deterministic evidence generation.
* **Falsification Condition:** If standard off-the-shelf runtimes (e.g., WebAssembly / Wasmtime, eBPF, standard OCI Linux containers) paired with host FFI capability boundaries produce identical policy enforcement and bit-exact evidence bundles with lower latency and zero bespoke VM maintenance tax, the custom VM hypothesis is falsified.

### (b) Ternary Substrate Advantage
* **Hypothesis:** Balanced ternary representation and SWAR SIMD emulation (`TNOT_SWAR`, `TAND_SWAR`, `TOR_SWAR`) provide structural or computational advantages over binary SIMD formats for governed computation.
* **Falsification Condition:** If comparative benchmarks reveal no statistically significant advantage in energy efficiency, bit-density, or formal verification tractability compared to standard 2-bit packed binary SIMD, the ternary advantage hypothesis is falsified.

### (c) Governance-Inside-Kernel Coupling
* **Hypothesis:** Policy enforcement (`eval_axion_call`) must occur inside the inner dispatch loop of the execution kernel.
* **Falsification Condition:** If policy interception at external network gateways, OS system-call interfaces, or host FFI boundaries provides equivalent fail-closed guarantees against unauthorized side effects during mid-flight policy mutations, in-kernel governance coupling is declared unnecessary.

### (d) Integrated Stack vs. Detached Protocol
* **Hypothesis:** Computational accountability requires a vertically integrated, monolithic stack (TISC + Axion + CanonFS).
* **Falsification Condition:** If detached evidence protocols (such as RFC-00D4 bundles verified via standalone CLI tools) achieve equal auditability across heterogeneous hosts without requiring the producer to run the T81 VM stack, the integrated stack requirement is falsified.

---

## 3. Comparative Protocol & Allowed Outcomes

All architectural claims are evaluated against the empirical benchmark specified in [`docs/research/ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md`](./ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md) ("Inventory Replenishment Under Constraint").

The framework evaluates three distinct arms:
1. **(i) Conventional Baseline:** Python + FastAPI + PostgreSQL + Open Policy Agent (OPA).
2. **(ii) Current T81 Substrate:** T81VM + Axion Kernel + CanonFS + TISC.
3. **(iii) Lightweight Alternative ("Project Baseline-X"):** Wasmtime + SQLite + Cosign/Sigstore + RFC-00D4 Evidence Protocol.

### Allowed Outcomes
* **Outcome A (Current T81 Wins):** Integrated T81 VM proves necessary to prevent side-effect breaches or state divergence under mid-flight policy mutation.
* **Outcome B (Protocol Wins):** The lightweight alternative (Arm iii) meets all six accountability primitives with 10× lower complexity and latency, proving **Accountable Computation is a Protocol, Not a Runtime**.
* **Outcome C (Missing Primitive Discovered):** All three arms fail to support dynamic multi-party capability delegation, redirecting effort toward transferable authorization mechanisms.

---

## 4. Definition of Success under "Protocol, Not Runtime"

If the empirical results favor Outcome B, success for the T81 Foundation is explicitly defined as:
* Pivoting T81 from a heavy C++ VM/OS engine to a lightweight, universal **Accountable Evidence Standard** (specifications, JSON Schemas, verification algorithms, and SDKs).
* Exporting CanonFS identity and Axion policy concepts into runtime-agnostic specifications (e.g., RFC-00D4) adopted across standard WebAssembly, Linux, and Cloud infrastructure.
* Achieving adoption based on protocol interoperability rather than runtime lock-in.

---

## 5. Relationship to Existing Repository State

This research directive operates alongside existing repository documentation and operational guidelines without invalidating current stability guarantees:
* **[`AGENTS.md`](../../AGENTS.md) and [`docs/status/PROJECT_CONTROL_CENTER.md`](../status/PROJECT_CONTROL_CENTER.md):** The core C++ interpreter, CTest harness, and CanonFS CLI commands remain frozen and maintained for baseline stability.
* **Product Positioning:** While public product documentation continues to emphasize deterministic execution and policy-gated runtimes, internal architectural investment is strictly governed by this hypothesis-driven framework.
