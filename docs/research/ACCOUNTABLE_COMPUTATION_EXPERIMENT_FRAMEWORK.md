# Accountable Computation Research Framework & Empirical Experiment Protocol

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Accountable Computation Research Framework & Empirical Experiment Protocol](#accountable-computation-research-framework-&-empirical-experiment-protocol)
  - [1. Research Posture and Core Question Restatement](#1-research-posture-and-core-question-restatement)
    - [Research Posture](#research-posture)
    - [Core Research Question](#core-research-question)
    - [Primitive: The Accountable Computational Act](#primitive-the-accountable-computational-act)
  - [2. Mapping the Current T81 Implementation](#2-mapping-the-current-t81-implementation)
    - [A. Observed / Testable Properties (Works in the Repo Today)](#a-observed--testable-properties-works-in-the-repo-today)
    - [B. Unproven Assumptions (Promoted Hypotheses Lacking Comparative Advantage)](#b-unproven-assumptions-promoted-hypotheses-lacking-comparative-advantage)
    - [C. Unknown Territory (Uncharted Operational Realities)](#c-unknown-territory-uncharted-operational-realities)
  - [3. Empirical Ranking of Competing Hypotheses](#3-empirical-ranking-of-competing-hypotheses)
    - [Commitment vs. Testing Analysis](#commitment-vs-testing-analysis)
  - [4. Comparative Experiment Protocol: Inventory Replenishment Task](#4-comparative-experiment-protocol-inventory-replenishment-task)
    - [Workload Specification: "Inventory Replenishment Under Constraint"](#workload-specification-"inventory-replenishment-under-constraint")
    - [Adversarial Scenarios](#adversarial-scenarios)
    - [Three Implementations Under Test](#three-implementations-under-test)
    - [Explicit Evaluation Metrics](#explicit-evaluation-metrics)
    - [Allowed Outcomes & Falsification Criteria](#allowed-outcomes-&-falsification-criteria)
  - [5. Next Steps: Concrete Artifacts to Preserve Experimental Freedom](#5-next-steps-concrete-artifacts-to-preserve-experimental-freedom)

<!-- T81-TOC:END -->


**Date:** April 2026
**Status:** Open Research Hypothesis
**Target Surface:** `docs/research/ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md`

---

## 1. Research Posture and Core Question Restatement

### Research Posture
The T81 Foundation project is temporarily suspending its foundational commitment to its current stack architecture. T81 is explicitly treated as a **scientific and engineering experiment**, not a product that must defend, justify, or complete its existing components (TISC ISA, T81VM, Axion Kernel, CanonFS, T81Lang, or ternary-native execution). The current implementation is recognized as **one specific hypothesis** among many about how to organize governed, verifiable computation.

### Core Research Question
> **Can we create a computing environment in which the identity, authority, evidence, and consequences of computation are first-class, composable objects—regardless of whether the computation itself is deterministic, probabilistic, binary, or ternary?**

### Primitive: The Accountable Computational Act
We define an **Accountable Computational Act** as any computation bound by six primitive properties:
1. **Meaning:** What state transition or decision was intended?
2. **Evidence:** What verifiable artifact proves what actually executed?
3. **Method:** Under what runtime, model, or transformation rule was it produced?
4. **Authority:** What capability or policy permitted this action prior to side effects?
5. **Accountability:** Who or what entity bears responsibility for the outcome?
6. **Challengeability:** Can an independent party replay, audit, or falsify the output?

---

## 2. Mapping the Current T81 Implementation

Mapping the current codebase (`/spec`, `/src`, `/tools`, `/docs`) onto observed properties, unproven assumptions, and unknown territory.

```
+-----------------------------------------------------------------------------------+
|                        OBSERVED / TESTABLE PROPERTIES                             |
|  - Bit-exact interpreter determinism across x86_64/ARM64 (DCP)                   |
|  - Pre-side-effect policy intercept (eval_axion_call on AgentInvoke)              |
|  - Immutable content addressing & import/export interchange (CanonFS / RFC-00D1)   |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|                            UNPROVEN ASSUMPTIONS                                   |
|  - Ternary vectorization (SWAR/AVX2) offers advantage over SIMD binary            |
|  - A custom VM/ISA is needed for computational accountability                      |
|  - AI governance belongs inside an execution VM kernel                            |
|  - Immutable artifacts fit non-deterministic probabilistic cognitive loops        |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|                             UNKNOWN TERRITORY                                     |
|  - Performance tax of deep evidence recording under high throughput               |
|  - Behavioral semantics under model drift and non-deterministic LLM weights        |
|  - Cross-organization authority transfer and capability delegation                |
+-----------------------------------------------------------------------------------+
```

### A. Observed / Testable Properties (Works in the Repo Today)
- **Bit-Exact Cross-Platform Determinism (DCP):** The core interpreter (`core/vm`), TISC ISA (v1.9.0), and core types (`Cell`, `T81Float`, `T81Int`) deliver mathematically proven, bit-identical outputs across x86_64 and ARM64 (`tests/cpp/vm_determinism_property_test.cpp`).
- **Pre-Side-Effect Policy Enforcement (Axion):** Pre-dispatch interception exists and is tested. `eval_axion_call` evaluates policy ASTs before `AgentInvoke` or `WLoad` opcodes mutate VM state or external storage (`tests/cpp/axion_agent_invoke_policy_test.cpp`).
- **Content-Addressed Immutability & Interchange (CanonFS):** Immutable provenance tracking and foreign filesystem interchange (RFC-00D1) successfully hash, store, import, and export artifacts (`canonfs import/export`).
- **Bounded Decision Bundle Composition:** The admitted AI OS-object family (`assess-fixed`, `route-fixed`, `classify-fixed`) packages results, provenance, typed actions, and canonical headers into self-contained JSON decision bundles (`docs/reference/AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md`).

### B. Unproven Assumptions (Promoted Hypotheses Lacking Comparative Advantage)
- **Assumption 1: Custom Virtual Machine (T81VM/TISC) is Necessary.**
  *Unproven:* It is assumed that first-class identity and governance require a bespoke VM/ISA, rather than lightweight cryptographic wrappers around standard binary runtimes (e.g., Wasm, eBPF, Linux processes).
- **Assumption 2: Ternary Representation Provides Practical Advantage.**
  *Unproven:* SWAR/AVX2 balanced ternary packing (`TNOT_SWAR`, `TAND_SWAR`, `TOR_SWAR`) executes on binary hardware, but evidence showing a efficiency, density, or verification advantage over standard packed binary SIMD remains absent.
- **Assumption 3: Governance Belongs Inside an Execution Kernel.**
  *Unproven:* Inserting policy checks into low-level VM dispatch (`eval_axion_call`) couples policy evaluation tightly to the runtime, whereas policy enforcement could occur at API gateways, sidecars, or OS system-call boundaries.
- **Assumption 4: Immutable Artifacts Match Cognitive / AI Processes.**
  *Unproven:* Content-addressed static bundles assume AI reasoning can be snapshot as static artifacts, obscuring streaming, continuous context, and non-deterministic stochastic model state.

### C. Unknown Territory (Uncharted Operational Realities)
- **Performance & Storage Tax of Full Provenance:** The overhead of generating CanonHash-81 cryptographic hashes, execution traces, and policy evaluation logs for millions of low-latency inferences per second is unquantified.
- **Governance Under Model Drift & Probabilistic Non-Determinism:** How policy contracts behave when underlying models exhibit stochastic output drift or temperature-based divergence remains untested in production.
- **Dynamic Capabilities & Cross-Domain Authority:** Transferable capability delegation across distrusted organizational boundaries without a centralized trust root is unaddressed by local Axion policies.

---

## 3. Empirical Ranking of Competing Hypotheses

Ranking the five rival hypotheses based strictly on existing repo code and test evidence:

| Rank | Hypothesis | Repo Evidence Status | Existing Infrastructure |
| :---: | :--- | :--- | :--- |
| **1** | **A. Computation as Evidence** | **Strongest Evidence** | CanonFS (RFC-00D1), Decision Bundles, `canonfs import/export`, CanonHash-81 provenance schemas, audit traces. |
| **2** | **D. Boundary of Uncertainty & Certainty** | **Moderate Evidence** | Bounded AI OS-Object Family (`assess-fixed`, `route-fixed`), Axion `eval_axion_call` pre-dispatch policy checks (`AI-01..05`), GGUF ingestion bridges. |
| **3** | **B. Authority as a Computational Resource** | **Partial Evidence** | Axion policy language (`.apl`), TTS Tier transitions (`AXREAD`, `AXSET`), fail-closed policy checks. Lacks dynamic capability delegation. |
| **4** | **C. Programs as Temporary Environments** | **Minimal Evidence** | T81Lang AST, basic graph execution (`T81Graph`). Lacks model-replacement survival or long-lived state graph preservation. |
| **5** | **E. Different Substrate (Ternary Advantage)** | **Lowest Evidence / Unfalsified** | SWAR ternary emulation (`test_swar.cpp`, `test_base81_simd.cpp`), CORDIC soft-float. No benchmark proving superiority over binary hardware. |

### Commitment vs. Testing Analysis
- **Current Core Commitment:** The current architecture is most committed to **Hypothesis E (Ternary Substrate)** and **Virtual Machine Centrality**, having invested heavily in SWAR emulation, TISC opcodes, and custom floating-point types (`T81Float`).
- **Barely Tested Hypothesis:** The project has barely tested **Hypothesis A (Computation as Evidence as a pure protocol)** or **Hypothesis B (Explicit Transferable Capabilities)** independent of the T81VM stack.

---

## 4. Comparative Experiment Protocol: Inventory Replenishment Task

To test whether T81’s tightly coupled stack provides genuine emergent value over conventional or protocol-only alternatives, we specify a bounded, decisive 2-week experiment.

```
                                [ ADVERSARIAL SCENARIOS ]
                                            |
         +----------------------------------+----------------------------------+
         |                                  |                                  |
         v                                  v                                  v
+------------------+              +-------------------+              +-------------------+
| (i) Conventional |              |  (ii) Current T81 |              | (iii) Lightweight |
|     Baseline     |              |     Substrate     |              |    Alternative    |
|                  |              |                   |              |                   |
| Python + FastAPI |              | T81VM + Axion +   |              | Wasm / Docker +   |
| + Postgres + OPA |              | CanonFS + TISC    |              | Cosign + Protocol |
+------------------+              +-------------------+              +-------------------+
         |                                  |                                  |
         +----------------------------------+----------------------------------+
                                            |
                                            v
                                  [ DECISIVE METRICS ]
                       - Unauthorized Action Rate (0% required)
                       - Independent Verification Time (<1s)
                       - Policy Mutability Handling
                       - Engineering Complexity (LOC / dependencies)
```

### Workload Specification: "Inventory Replenishment Under Constraint"
An automated agent receives inventory levels, sales forecasts, and supplier quotes. It must decide whether to issue a purchase order ($10,000–$50,000) and execute the order.

### Adversarial Scenarios
1. **Adversarial Prompt Injection:** The agent receives a manipulated supplier quote containing a prompt injection attempting to bypass spending limits.
2. **Mid-Flight Policy Mutation:** The authorized spending limit is revoked or reduced *after* the agent computes its decision but *before* the purchase order side-effect occurs.
3. **Stale/Corrupted Provenance:** An attacker alters the inventory history record or supplier quote hash prior to decision verification.
4. **Independent Audit Challenge:** A third-party auditor with zero access to the runtime environment or internal database must verify whether purchase order #8491 was policy-compliant.

### Three Implementations Under Test

| Implementation | Architecture / Tech Stack | Execution Mechanism | Policy Enforcement |
| :--- | :--- | :--- | :--- |
| **(i) Conventional Baseline** | Python 3.12, FastAPI, PostgreSQL, Open Policy Agent (OPA) | Standard Python process | OPA HTTP sidecar middleware |
| **(ii) Current T81 Substrate** | T81VM, TISC bytecode, Axion Policy Kernel, CanonFS | Deterministic TISC execution in T81VM | Pre-dispatch `eval_axion_call` inside VM kernel |
| **(iii) Lightweight Alternative** | Wasmtime (WebAssembly), SQLite, Sigstore/Cosign, JSON-LD Evidence Protocol | Isolated Wasm module execution | Capability-based Host FFI import restriction + detached evidence bundle |

### Explicit Evaluation Metrics

1. **Unauthorized Action Rate (%)**: Percentage of prompt-injected or over-limit purchase orders that result in side effects (Must be 0.0%).
2. **Independent Verification Latency (ms)**: Time required for an external, offline verifier to prove policy compliance and artifact integrity.
3. **Handling of Mid-Flight Policy Revocation**: System behavior when policy changes between calculation and execution (Pass/Fail fail-closed check).
4. **Reproducibility Hash Match Rate (%)**: Bit-exact agreement of execution output across x86_64 and ARM64 hosts.
5. **System Complexity & Overhead**: Lines of code (LOC), dependency footprint (MB), and execution latency per decision (ms).

### Allowed Outcomes & Falsification Criteria
- **Outcome A (Current T81 Wins):** T81 prevents mid-flight policy breaches and guarantees replayability where (i) and (iii) fail, justifying VM/kernel coupling.
- **Outcome B (Lightweight Alternative Wins):** Implementation (iii) achieves identical policy enforcement, challengeability, and provenance with 10× lower complexity and latency, proving that **Accountable Computation is a Protocol, Not a Runtime**.
- **Outcome C (Missing Requirement Discovered):** None of the implementations handle dynamic, multi-party authority delegation safely, revealing that Capability Distribution (Hypothesis B) is the true missing primitive.

---

## 5. Next Steps: Concrete Artifacts to Preserve Experimental Freedom

To ensure the project maintains scientific objectivity and does not prematurely commit to stack completion, the project will produce the following three concrete, finishable artifacts:

1. **Research Charter (`docs/research/RESEARCH_CHARTER.md`):**
   A 2-page document formalizing T81 as an empirical experiment, suspending roadmap pressures, and declaring falsification conditions for the custom VM and ternary layers.
2. **Accountable Computation Evidence Schema (RFC-00D4 Draft):**
   A standalone, runtime-agnostic JSON-Schema specification for **Accountable Decision Bundles** that can be produced by Wasm, eBPF, Python, or T81VM alike.
3. **Minimal Alternative Prototype ("Project Baseline-X"):**
   A 500-LOC Rust/Python prototype implementing Hypothesis A (Computation as Evidence) using standard WebAssembly and detached cryptographic evidence bundles, serving as the benchmark control for the inventory replenishment experiment.
