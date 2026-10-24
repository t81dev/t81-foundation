# Competing Hypotheses Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/HYPOTHESES.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Status:** Active Hypothesis Register

---

## 1. Overview

Research Protocol 001 evaluates three mutually exclusive, falsifiable competing hypotheses regarding deterministic artifact execution, policy enforcement, and accountable computation.

```
+-----------------------------------------------------------------------------------+
|                            COMPETING HYPOTHESES                                   |
|                                                                                   |
|  [ H1: T81 Advantage ]         [ H2: Conventional Equivalence ]  [ H3: Environmental ]|
|  Vertically integrated         Decoupled off-the-shelf           Observed delta     |
|  VM kernel is required         pipeline (Wasmtime + OPA +        stems from serial- |
|  for non-bypassable            RFC-00D4) provides equivalent     ization or platform|
|  policy interception.          guarantees with lower complexity. configuration.   |
+-----------------------------------------------------------------------------------+
```

---

## 2. Hypothesis H1: T81 Advantage

> **H1 Statement:** T81's vertically integrated execution pipeline (T81VM + Axion Kernel + CanonFS) provides a reproducible technical advantage in deterministic artifact execution, pre-side-effect policy enforcement, and auditability that cannot be replicated by conventional decoupled pipelines without incurring equivalent or greater engineering overhead.

### 2.1 Supporting Observations
- Pre-dispatch policy evaluation (`eval_axion_call` inside `AgentInvoke`/`WLoad` execution loop) guarantees fail-closed interception before any external IO or state mutation occurs.
- Integrated execution traces directly link instruction PC, registers, input arguments, policy AST verdicts, and content-addressed payload hashes in a single canonical structure.
- Bit-exact interpreter determinism eliminates cross-platform float/integer drift at the hardware boundary without requiring container sandboxing or OS hypervisors.

### 2.2 Falsifying Observations
- A conventional off-the-shelf runtime (e.g., Wasmtime with restricted capability FFI host imports) prevents unauthorized side effects with identical 0.0% breach rates under mid-flight policy revocation.
- Detached evidence verification (e.g., RFC-00D4 bundles verified via offline CLI tools) matches T81's replay accuracy and hash verification rate across x86_64 and ARM64.
- T81 incurs $\ge 2\times$ execution latency tax or $\ge 5\times$ maintenance LOC tax compared to the conventional baseline.

### 2.3 Required Measurements
- Unauthorized Action Rate (%) under mid-flight policy revocation scenarios.
- Execution Latency (ms per decision, $N \ge 30$).
- Detached Verification Time (ms per evidence bundle).
- Cross-Platform Reproduction Agreement Rate (%) across x86_64 and ARM64.
- Codebase & Dependency Complexity (LOC, binary size in MB).

### 2.4 Confounding Variables
- Differences in compiler optimization levels (`-O3` vs. `-O2`).
- Host process scheduling jitter and CPU frequency scaling.
- Memory allocation overhead differences between C++ runtime allocator and Rust/Wasm memory allocator.

### 2.5 Explicit Limitations
- Finding an advantage for H1 on integer decision arithmetic DOES NOT extend to floating-point or non-deterministic neural inference.
- An advantage in local single-host policy enforcement DOES NOT establish advantage in multi-party distributed consensus.

---

## 3. Hypothesis H2: Conventional Equivalence

> **H2 Statement:** A conventional content-addressed execution pipeline (utilizing standard WebAssembly runtimes such as Wasmtime, Open Policy Agent / host FFI capability boundaries, standard SHA256/BLAKE3 hashing, and RFC-00D4 evidence schemas) provides equivalent guarantees of determinism, policy enforcement, and auditability with equal or lower engineering complexity and lower execution latency.

### 3.1 Supporting Observations
- Host FFI capability restrictions in WebAssembly/Wasmtime isolate execution modules, making un-sandboxed side effects structurally impossible.
- Detached RFC-00D4 Accountable Decision Bundles allow 100% offline auditability without needing to run or maintain the T81 C++ virtual machine.
- Standard tooling (Wasm, OPA, JSON-Schema) benefits from decades of compiler optimization, reducing decision execution latency by $5\times$ to $10\times$.

### 3.2 Falsifying Observations
- Decoupled OPA sidecar middleware fails to intercept a mid-flight policy revocation due to network race conditions or async buffer lags, resulting in unauthorized purchase order side effects ($>0.0\%$ breach rate).
- Cross-platform Wasm execution exhibits bit-level output divergence across x86_64 and ARM64 architectures under edge arithmetic conditions.
- Constructing an equivalent detached audit trail in conventional stacks requires equal or greater LOC and dependency complexity than T81VM.

### 3.3 Required Measurements
- Side-Effect Interception Breach Rate (%) under race conditions.
- Cold-start and warm-execution decision latency (ms).
- Evidence bundle verification throughput (bundles/second).
- Total system LOC and third-party dependency footprint (MB).

### 3.4 Confounding Variables
- Asynchronous vs. synchronous IPC overhead in OPA sidecar communication.
- Wasmtime JIT compilation time vs. T81VM interpreter loop dispatch.

### 3.5 Explicit Limitations
- Equivalence on local decision workloads does not evaluate host kernel-level fault isolation or hardware-level interrupts.

---

## 4. Hypothesis H3: Environmental Dependence

> **H3 Statement:** Observed performance, determinism, or policy enforcement differences between T81 and conventional reference pipelines are primarily attributable to platform, serialization format, configuration, or implementation choices rather than intrinsic architectural properties of the virtual machine or kernel coupling.

### 4.1 Supporting Observations
- Discrepancies in verification time disappear when both systems use identical canonical JSON serialization (RFC 8785) instead of Base81 tryte encoding.
- Determinism failures in conventional pipelines vanish when explicit IEEE 754 float rounding modes or fixed-point integer modes are strictly configured.
- Execution latency variations are dominated by disk IO or process launch overhead rather than in-kernel policy dispatch logic.

### 4.2 Falsifying Observations
- Differences in side-effect breach rates persist even when serialization, IPC transport, compiler flags, and host thread pin settings are strictly normalized across both systems.
- T81 exhibits structural architectural advantages (e.g. single-pass instruction-level policy trace capture) that cannot be replicated in conventional pipelines regardless of configuration tuning.

### 4.3 Required Measurements
- Microbenchmark breakdown isolating serialization overhead vs. dispatch latency vs. policy evaluation time.
- Impact of OS kernel scheduler settings, memory allocators (jemalloc vs. glibc malloc), and CPU pinning on execution time variance ($R^2$ correlation).

### 4.4 Confounding Variables
- OS page cache behavior and filesystem sync latencies (`fsync`).
- Differences in CPU SIMD extensions (AVX2 vs. ARM64 NEON).

### 4.5 Explicit Limitations
- Normalizing environmental variables completely may require synthetic configurations that differ from standard production deployments.
