# HanoiVM JIT Design: Trace-Based Deterministic Execution

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [HanoiVM JIT Design: Trace-Based Deterministic Execution](#hanoivm-jit-design-trace-based-deterministic-execution)
  - [1. Objectives](#1-objectives)
  - [2. Approach: Trace-Based Specialization](#2-approach-trace-based-specialization)
  - [3. Determinism & Safety](#3-determinism-&-safety)
  - [4. Prototype Scope](#4-prototype-scope)
  - [5. Implementation Roadmap](#5-implementation-roadmap)

<!-- T81-TOC:END -->


This document outlines the design for the HanoiVM Just-In-Time (JIT) compiler, focused on deterministic execution of compute-intensive TISC bytecode.

## 1. Objectives
- Improve performance for "hot" loops in TISC.
- Maintain bit-identical reproducibility compared to the interpreter.
- Avoid large external dependencies (no LLVM/AsmJit for the initial prototype).
- Ensure Axion policy enforcement remains intact.

## 2. Approach: Trace-Based Specialization
The JIT will use a trace-based approach:
1.  **Profiling**: The interpreter identifies "hot" jump targets.
2.  **Tracing**: When a hot target is hit, the VM enters "tracing mode," recording a linear sequence of opcodes (a "trace").
3.  **Compilation**: The trace is compiled into a specialized "threaded code" block or a simple machine code blob (platform-dependent).
4.  **Execution**: Future hits to the jump target execute the compiled trace directly.

## 3. Determinism & Safety
To ensure determinism:
- JIT will only be enabled for side-effect-free numeric and tensor opcodes in the prototype.
- Every compiled trace will include explicit Axion policy checks at its boundaries.
- Trace entry and exit will be logged to the Axion trace to maintain a consistent audit trail.

## 4. Prototype Scope
The prototype will target a subset of TISC:
- Arithmetic: `Add`, `Sub`, `Mul`.
- Control flow: Simple loops.
- Registers: `R0-R242`.

## 5. Implementation Roadmap
- `include/t81/jit/jit.hpp`: JIT interface.
- `runtime/jit/jit_compiler.cpp`: Trace recorder and basic emitter.
- `t81 debug --jit`: Debugger support for JIT-compiled regions.
