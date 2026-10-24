# Competing Hypotheses Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/HYPOTHESES.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Version:** v1.1.0 (Corrected Specification)
**Status:** Active Hypothesis Register

---

## 1. Overview

Research Protocol 001 evaluates three nonexclusive, falsifiable explanatory models regarding deterministic artifact execution, policy enforcement, and accountable computation. Hypotheses H1, H2, and H3 are nonexclusive: experimental outcomes may support elements of multiple hypotheses simultaneously or yield inconclusive results.

```
+-----------------------------------------------------------------------------------+
|                        NONEXCLUSIVE RESEARCH EXPLANATIONS                         |
|                                                                                   |
|  [ H1: Integrated VM Kernel ]  [ H2: Decoupled Architecture ]  [ H3: Environmental ] |
|  In-kernel policy dispatch      Decoupled off-the-shelf        Observed deltas    |
|  provides non-bypassable        pipeline (Wasmtime + OPA +     stem from IPC,     |
|  pre-side-effect enforcement.   RFC-00D4 draft) provides       serialization, or  |
|                                 equivalent guarantees.          host OS tuning.    |
+-----------------------------------------------------------------------------------+
```

---

## 2. Hypothesis H1: Integrated VM Kernel Advantage

> **H1 Statement:** T81's vertically integrated execution pipeline (T81VM + Axion Kernel + CanonFS) provides a reproducible technical advantage in deterministic artifact execution, pre-side-effect policy enforcement, and auditability that cannot be replicated by conventional decoupled pipelines without incurring equivalent or greater host runtime complexity.

### 2.1 Supporting Observations
- Pre-dispatch policy evaluation (`eval_axion_call` inside `AgentInvoke`/`WLoad` execution loop) guarantees fail-closed interception before any external IO or state mutation occurs.
- Integrated execution traces directly link instruction PC, registers, input arguments, policy AST verdicts, and content-addressed payload hashes in a single canonical structure.
- Bit-exact interpreter determinism eliminates cross-platform integer drift at the hardware boundary without requiring container sandboxing or OS hypervisors.

### 2.2 Falsifying Observations
- A conventional off-the-shelf runtime (e.g., Wasmtime with restricted capability FFI host imports and synchronous pre-commit hooks) prevents unauthorized side effects with identical 0.0% breach rates under mid-flight policy revocation.
- Detached evidence verification using internal RFC-00D4 draft schemas matches T81's replay accuracy and hash verification rate across x86_64 and ARM64.
- T81 incurs higher execution latency or $\ge 5\times$ higher host runtime C++ code complexity (LOC) compared to the conventional baseline.

### 2.3 Required Measurements
- Unauthorized Action Rate (%) under mid-flight policy revocation scenarios.
- Decision Execution Latency (ms per decision, $N \ge 30$).
- Detached Verification Time (ms per evidence bundle).
- Cross-Platform Reproduction Agreement Rate (%) across x86_64 and ARM64.
- Host Runtime Codebase Complexity (LOC) and dependency binary footprint (MB).

---

## 3. Hypothesis H2: Decoupled Architecture Equivalence

> **H2 Statement:** A conventional content-addressed execution pipeline (utilizing standard WebAssembly runtimes such as Wasmtime, Open Policy Agent / in-process host FFI capability boundaries, standard SHA256 hashing, and the internal RFC-00D4 experimental draft evidence schema) provides equivalent guarantees of determinism, policy enforcement, and auditability with equal or lower execution latency and $\ge 5\times$ lower host runtime LOC footprint.

### 3.1 Supporting Observations
- Host FFI capability restrictions in WebAssembly/Wasmtime isolate execution modules, making un-sandboxed side effects structurally impossible.
- Detached internal RFC-00D4 draft Accountable Decision Bundles allow 100% offline auditability without needing to run or maintain the T81 C++ virtual machine.
- Standard tooling (Wasm, OPA, JCS JSON-Schema) benefits from optimized compiler engines, achieving decision execution latency equal to or lower than the unoptimized T81VM C++ interpreter.

### 3.2 Falsifying Observations
- Decoupled policy interception fails to intercept a mid-flight policy revocation due to execution state race conditions or async buffer lags, resulting in unauthorized purchase order side effects ($>0.0\%$ breach rate).
- Cross-platform Wasm execution exhibits output divergence across x86_64 and ARM64 architectures under edge arithmetic conditions.
- Constructing an equivalent detached audit trail in conventional stacks requires greater host runtime LOC complexity than T81VM.

---

## 4. Hypothesis H3: Environmental & Transport Dependence

> **H3 Statement:** Observed performance, determinism, or policy enforcement differences between T81 and conventional reference pipelines are primarily attributable to transport mechanisms (in-process FFI vs asynchronous socket/sidecar IPC), serialization formats (RFC 8785 JCS JSON vs custom tryte encoding), or OS host process configuration rather than intrinsic architectural properties of the virtual machine or kernel coupling.

### 4.1 Supporting Observations
- Discrepancies in verification time disappear when both systems use identical RFC 8785 JSON Canonicalization Scheme (JCS) instead of custom serialization.
- Execution latency variations between Arm (i) and Arm (iii) are dominated by asynchronous HTTP/gRPC IPC socket overhead when OPA is run as an external sidecar process, and vanish when OPA is linked in-process via C/Rust FFI bindings.
- Determinism and performance variances correlate strongly ($R^2 > 0.85$) with OS page cache state, memory allocators, or CPU governor pinning.

### 4.2 Falsifying Observations
- Differences in side-effect breach rates or decision latency persist even when serialization, IPC transport, compiler flags, and host thread pin settings are strictly normalized across both systems.
- T81 exhibits structural architectural advantages (e.g. single-pass instruction-level policy trace capture) that cannot be replicated in conventional pipelines regardless of transport or configuration tuning.

---

## 5. Nonexclusive Research Explanations & Reporting Rules

Hypotheses H1, H2, and H3 are nonexclusive. Experimental outcomes may combine elements of multiple hypotheses or prove inconclusive. The evaluation runner MUST report results according to the following rules:

1. **Combined / Mixed Outcomes:** If Arm (iii) achieves 0.0% breach rates (supporting H2) but demonstrates that performance deltas are driven entirely by IPC transport choice (supporting H3), the report MUST attribute the security finding to H2 and the performance finding to H3.
2. **Inconclusive Outcomes:** If performance or timing differences between arms fall within the overlapping 95% confidence intervals ($\frac{| \mu_A - \mu_B |}{CI_{95\_max}} < 1.0$) or fail non-parametric significance tests ($p > 0.05$ on Mann-Whitney U test), the performance comparison MUST be formally classified as `INCONCLUSIVE_INSUFFICIENT_DELTA`.
3. **Unsupported / Disproven Explanations:** If an outcome contradicts a hypothesis's supporting observations (e.g., if H1 is disproven by 0.0% breach rates on Wasmtime under race conditions), the hypothesis MUST be marked `UNSUPPORTED_BY_EVIDENCE` in the experimental summary report.
