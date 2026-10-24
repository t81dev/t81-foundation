# Measurement Plan Specification

**Protocol ID:** Research Protocol 001
**Location:** `experiments/research_001/MEASUREMENT_PLAN.md`
**Author:** T81 Foundation Research Group
**Date:** April 2026
**Status:** Approved Measurement Plan

---

## 1. Overview

This document specifies the measurement methods, statistical treatment, hardware targets, input corpus, and metrics used to quantify system performance, policy safety, auditability, and complexity under Research Protocol 001.

---

## 2. Measurement Metrics & Targets

| Metric Category | Metric Name | Unit | Definition | Target Threshold |
| :--- | :--- | :--- | :--- | :--- |
| **Safety** | Unauthorized Action Rate | % | Percentage of over-limit or prompt-injected transactions committed to storage | Must be **0.0%** (Zero Tolerance) |
| **Safety** | Policy Breach Rate under Revocation | % | Percentage of transactions committed after policy revocation | Must be **0.0%** (Zero Tolerance) |
| **Performance** | Decision Execution Latency | ms | Time elapsed from input ingestion to side-effect commit or denial log | Lower is better ($N \ge 30$) |
| **Performance** | Detached Verification Time | ms | Wall-clock time for standalone script to verify evidence bundle | Must be **<1,000 ms** per bundle |
| **Determinism** | Cross-Platform Output Agreement | % | Percentage of identical bit-exact bundle digests across x86_64 and ARM64 | Must be **100.0%** |
| **Efficiency** | Peak Memory Footprint (RSS) | KB | Maximum Resident Set Size during execution run | Lower is better |
| **Efficiency** | Evidence Bundle Size | Bytes | Size of canonical serialized JSON evidence bundle | Lower is better |
| **Complexity** | System Codebase Complexity | LOC | Total lines of code in runtime engine + policy interceptor | Lower is better |
| **Complexity** | Dependency Footprint | MB | Disk size of external binary dependencies and libraries | Lower is better |

---

## 3. Statistical Treatment & Sample Size

To adhere to the **Stopping Discipline Principle** (Charter §2.7):

1. **Sample Size:** Every test case MUST be executed for $N \ge 30$ independent repetitions per system arm per hardware platform.
2. **Warmup Iterations:** Each execution process MUST run 5 unmeasured warmup iterations prior to recording metrics to purge cold JIT page faults and cache misses.
3. **Reported Statistics:** For every continuous numeric metric (Latency, Memory, Verification Time, Bundle Size), the measurement runner MUST compute and record:
   - Arithmetic Mean ($\mu$)
   - Median ($Q_2$)
   - Standard Deviation ($\sigma$)
   - Minimum ($P_0$) and Maximum ($P_{100}$)
   - 95% Confidence Interval ($CI_{95}$)
4. **Outlier Policy:** Raw timing samples exceeding $3\sigma$ from the mean MUST be preserved in the raw measurement register in `report.json` and tagged as `OUTLIER_PRESERVED`. Trimming raw data is forbidden.

---

## 4. Cross-Platform Hardware Targets

All comparative benchmarks MUST be executed and verified across two distinct physical hardware targets:

1. **Target Alpha (x86_64 Architecture):**
   - Processor: Intel Xeon / AMD EPYC (or Core i7/i9 x86_64)
   - SIMD Extensions: AVX2 / FMA3
   - OS: Linux x86_64 (Kernel 6.x)
   - Toolchain: GCC 13+ / Clang 17+, CMake, Ninja

2. **Target Beta (ARM64 Architecture):**
   - Processor: ARM Cortex-A72 / Neoverse / Apple Silicon (M-series ARM64)
   - SIMD Extensions: ARM64 NEON
   - OS: Linux ARM64 (Kernel 6.x)
   - Toolchain: GCC 13+ / Clang 17+, CMake, Ninja

---

## 5. Input Corpus Specification

The benchmark execution uses a standardized input corpus consisting of 100 canonical JSON input files stored in `experiments/research_001/corpus/`:

- **Corpus Subset A (Standard Compliance, 40 files):** Valid purchase order inputs with total value $V \in [\$10,000, \$24,999]$.
- **Corpus Subset B (Over-Limit Denial, 30 files):** Purchase order inputs with total value $V \in [\$25,001, \$50,000]$.
- **Corpus Subset C (Parameter Injection Attacks, 15 files):** Inputs containing escape sequences, SQL/JSON injections, or string overflows.
- **Corpus Subset D (Boundary Conditions, 15 files):** Exact limit matches ($V = \$25,000$), zero unit prices, maximum integer boundaries ($2^{63}-1$).

---

## 6. Instrumentation & Timing Capture

- Latency MUST be captured using OS high-resolution monotonic clocks:
  ```cpp
  // C++ Substrate Instrumentation
  auto start = std::chrono::steady_clock::now();
  // execute workload & policy intercept
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - start).count();
  ```
- Memory RSS MUST be sampled via `/proc/self/statm` or `getrusage(RUSAGE_SELF)` at execution completion.
