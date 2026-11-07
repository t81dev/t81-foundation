# HanoiVM JIT Research Document

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [HanoiVM JIT Research Document](#hanoivm-jit-research-document)
  - [Overview](#overview)
  - [HanoiVM Bytecode Analysis](#hanoivm-bytecode-analysis)
  - [Target Architectures](#target-architectures)
  - [Library Selection](#library-selection)
  - [Proposed Architecture](#proposed-architecture)
  - [Initial Performance Estimates](#initial-performance-estimates)
  - [Future Work](#future-work)
    - [1. Incremental Deterministic Trace-JIT Hardening](#1-incremental-deterministic-trace-jit-hardening)
    - [2. Native Trace-JIT Backend Prototype (x86_64/ARM64)](#2-native-trace-jit-backend-prototype-x86_64arm64)
    - [3. Distributed Tensor Orchestration](#3-distributed-tensor-orchestration)

<!-- T81-TOC:END -->


## Overview
This document explores the feasibility and architectural design for a Just-In-Time (JIT) compiler for HanoiVM, focusing on strict determinism and high performance for balanced ternary workloads.

## HanoiVM Bytecode Analysis
HanoiVM executes TISC opcodes. The current interpreter is efficient but limited by the overhead of the fetch-decode-execute loop.

## Target Architectures
- **x86_64:** Utilizing AVX2 and AVX-512.
- **AArch64:** Leveraging NEON and SVE.

## Library Selection
- **AsmJit:** Lightweight C++ library for machine code generation.
- **LLVM:** Powerful optimization pipeline.

## Proposed Architecture
1. **Hot-Spot Detection.**
2. **Translation Layer.**
3. **Trace JIT.**
4. **Axion Integration.**

## Initial Performance Estimates
JIT compilation could yield a 5x-10x speedup for pure ternary arithmetic loops.

## Future Work

### 1. Incremental Deterministic Trace-JIT Hardening
We aim to harden the Trace JIT specifically for numeric and tensor hot paths. This involves:
-   Identifying repetitive arithmetic or tensor sequences that can be aggressively fused.
-   Ensuring that any JIT-compiled trace produces *identical* Axion trace events as the interpreted path.
-   Using profile-guided optimization (PGO) data from previous runs to warm the JIT cache.

### 2. Native Trace-JIT Backend Prototype (x86_64/ARM64)
A prototype backend targeting x86_64 and ARM64 directly is planned. Key requirements:
-   **Deterministic mmap**: Memory mapping for executable code pages must be deterministic across runs to support replay debugging.
-   **Constant-Time Execution**: Ensuring that branch prediction and cache behavior do not leak information or affect the observable execution trace.
-   **Cross-Platform Parity**: Validating that the generated machine code on x86_64 produces results bit-identical to the ARM64 output for all supported operations.

### 3. Distributed Tensor Orchestration
Evaluating patterns for distributed tensor operations that preserve replay guarantees. This includes:
-   Sharding tensors deterministically across nodes.
-   Handling network nondeterminism by synchronizing only at deterministic checkpoints (Axion trace boundaries).
