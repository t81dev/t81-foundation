# C Frontend MLIR Proof of Concept

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [C Frontend MLIR Proof of Concept](#c-frontend-mlir-proof-of-concept)
  - [1. Goal](#1-goal)
  - [2. Why C First](#2-why-c-first)
  - [3. Non-Goals](#3-non-goals)
  - [4. Proposed User Surface](#4-proposed-user-surface)
  - [5. Supported C Subset v0](#5-supported-c-subset-v0)
  - [6. Determinism Contract](#6-determinism-contract)
  - [7. Integration Strategy](#7-integration-strategy)
  - [8. IR Boundary Recommendation](#8-ir-boundary-recommendation)
  - [9. Recommended Milestones](#9-recommended-milestones)
    - [Milestone A: Parse and Reject Correctly](#milestone-a-parse-and-reject-correctly)
    - [Milestone B: Integer Function Lowering](#milestone-b-integer-function-lowering)
    - [Milestone C: Multi-Function Subset](#milestone-c-multi-function-subset)
    - [Milestone D: Memory Discipline](#milestone-d-memory-discipline)
  - [9.1 Current Status (2026-03-09)](#91-current-status-2026-03-09)
  - [10. File/Module Sketch](#10-filemodule-sketch)
  - [11. Key Design Rules](#11-key-design-rules)
  - [12. Recommendation](#12-recommendation)

<!-- T81-TOC:END -->


This proposal defines the next external-ecosystem milestone after the LLVM/MLIR
translator work: accept a tightly restricted subset of C and lower it into the
existing T81 MLIR/TISC pipeline.

## 1. Goal

Demonstrate that mainstream source languages can target the TISC/T81 execution
surface without rewriting logic in T81Lang.

The first milestone is intentionally narrow:

- one source language: C
- one small deterministic-safe subset
- one compile-only pipeline
- explicit rejection for unsupported constructs

This is a proof of viability, not a general-purpose C compiler.

## 2. Why C First

C is the best first ingress path because:

- Clang tooling is already adjacent to the LLVM/MLIR stack we now ship.
- The subset can be defined precisely in terms of syntax and semantics.
- A C proof of concept establishes the external-frontend architecture without
  prematurely solving Rust borrow/lifetime rules or Python dynamism.
- The resulting adapter can later inform Rust and Python subset boundaries.

## 3. Non-Goals

The first proof of concept does not aim to support:

- full ISO C
- pointers or arbitrary memory aliasing
- structs, unions, or bitfields
- preprocessor-heavy builds
- host ABI compatibility
- external linkage or libc integration
- floating-point by default
- undefined-behavior-dependent code

## 4. Proposed User Surface

Add a new experimental CLI family:

```text
t81 c compile <input.c> [-o <output.mlir|output.ll|output.tisc>] [--emit <mlir|llvm|tisc>] [--mode <compat|dcp>] [--dialect <standard|t81>]
```

Initial implementation should only guarantee `--emit mlir`.

Later stages can reuse the current MLIR/LLVM/TISC pipeline to emit LLVM IR or
TISC if the front half is stable.

## 5. Supported C Subset v0

The first subset should be intentionally austere.

Allowed:

- one translation unit
- `int32_t`-like integer semantics only
- local variables
- function parameters and returns
- arithmetic: `+`, `-`, `*`, `/`, `%`
- comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`
- boolean operators lowered as integer predicates
- `if`, `while`, `for`
- simple function calls within the same translation unit
- fixed-size local arrays lowered to explicit T81 memory slots, with compile-time constant indexing in v0

Rejected:

- pointers and address-of/dereference
- `goto`, `switch`, `break` across unsupported control surfaces
- recursion in v0
- varargs
- globals with mutable storage
- floating-point types in v0
- casts other than narrow, explicit integer-safe casts
- inline assembly

## 6. Determinism Contract

The C frontend must be fail-closed.

That means:

- unsupported syntax is rejected with an explicit diagnostic
- supported-but-nondeterministic semantics are rejected
- undefined-behavior-prone constructs are rejected rather than approximated
- no fallback to host C compilation is allowed

The frontend must treat determinism as a type boundary, not as a best-effort
optimization.

## 7. Integration Strategy

Use a dedicated adapter layer rather than trying to force C through the T81Lang
frontend.

Preferred architecture:

1. Parse restricted C via Clang tooling.
2. Normalize accepted constructs into a small internal C-subset IR.
3. Lower that IR into the existing T81 MLIR translation surface.
4. Reuse current MLIR lowering to LLVM IR and TISC-adjacent outputs.

This is preferable to lowering directly from Clang AST to TISC because:

- it keeps unsupported-C diagnostics isolated
- it gives us a reusable frontend-neutral seam for future Rust/Python adapters
- it preserves room for deterministic normalization before codegen

## 8. IR Boundary Recommendation

Do not lower from full C straight into raw TISC bytecode.

Instead, introduce a minimal adapter IR with:

- typed locals
- explicit basic blocks
- explicit integer ops
- structured control flow normalized to branches
- function-local symbol tables
- no implicit conversions

This adapter IR can then lower either to:

- the current TISC `Program` surface, or
- the current MLIR translator directly

For the first implementation, targeting MLIR is the better path because it
reuses the new compiler investment and gives us better inspection artifacts.

## 9. Recommended Milestones

### Milestone A: Parse and Reject Correctly

Deliverables:

- `t81 c compile foo.c --emit mlir`
- Clang-based parser hookup
- deterministic-safe subset checker
- user diagnostics for rejected constructs

Acceptance:

- supported arithmetic/control-flow examples parse and validate
- unsupported pointer/float/global examples fail with deterministic diagnostics

### Milestone B: Integer Function Lowering

Deliverables:

- single-function lowering
- locals, parameters, return values
- arithmetic and branch lowering

Acceptance:

- representative integer functions emit valid MLIR
- MLIR lowers through the existing pipeline to LLVM IR

### Milestone C: Multi-Function Subset

Deliverables:

- internal calls between supported functions
- basic call graph validation
- no recursion in v0

Acceptance:

- small multi-function C examples compile end to end

### Milestone D: Memory Discipline

Deliverables:

- explicit model for fixed local arrays if included
- deterministic mapping to T81 memory / stack semantics

Acceptance:

- array examples compile only when they fit the constrained model

## 9.1 Current Status (2026-03-09)

Implemented and verified:

- `t81 c compile` CLI wired through the existing MLIR path
- entry-point model: `int main()`
- helper `int` functions with named `int` parameters
- local initialized `int` variables
- fixed local `int[N]` arrays with compile-time integer initializers/indexing lowered to T81 memory
- compile-time constant array expressions accepted for fixed local indexing/initialization
- statement-only `++`/`--`, arithmetic/bitwise/comparisons/logical integer expressions, loop-local `break`/`continue`
- assignment statements
- structured control flow: `if`, `while`, `for`
- same-translation-unit direct calls
- explicit recursion rejection
- explicit rejection diagnostics for pointers, pointer parameters, globals,
  float returns, conditionless `for` loops, prototypes/`extern` declarations,
  variadic helpers, non-`int main()` signatures, `switch`, `do-while`, `goto`,
  labels, address-of/dereference, casts, runtime array indices, ternary,
  member access, `sizeof`, and compound assignment

Verified in targeted builds:

- `t81_c_frontend_mlir_smoke_test`
- `t81_cli_contract_test ./build_c_frontend_next/t81`
- `t81 help c`

Latest implementation commits on `main`:

- `78d92d84` `Add experimental C-subset frontend PoC`
- `ccf8b311` `Expand C frontend subset with structured control flow`
- `eb9743d1` `Add C frontend parameters and helper calls`
- `3c8034c8` `Add for-loop support to C frontend subset`
- `ae533560` `Harden C frontend rejection diagnostics`
- `eee73c34` `Reject unsupported C control and cast syntax`
- `cdf5d857` `Clarify unsupported C expression diagnostics`
- `671aea46` `Add bitwise ops to C frontend subset`
- `7da0fa7e` `Add logical operators to C frontend subset`
- `d3c6bf6c` `Add increment and decrement to C frontend subset`
- `1a8c4602` `Add loop break and continue to C frontend subset`
- `95b32961` `Add fixed arrays to C frontend subset`
- `904d1ae5` `Harden C frontend declaration surfaces`
- `05f45fce` `Add Rust frontend PoC proposal`
- local follow-up (uncommitted at proposal refresh time): compile-time constant array expression support
- `4b54c254` `Add Rust frontend CLI scaffold`

Recommended resume point:

1. decide whether compile-time constant array expressions are the intended end-state for C v0
   or whether array indexing should grow into a broader T81 memory/addressing surface
2. if C is considered sufficient at compile-time-constant indexing, the next major
   resume point is the Rust ingress path using the already-landed CLI/build
   scaffold (`t81 rust compile`, `T81_ENABLE_RUST_FRONTEND`)
3. implement real Rust lowering only once a Rust toolchain is available in the
   build environment

## 10. File/Module Sketch

Suggested initial layout:

```text
include/t81/c_frontend/
  compile.hpp
  diagnostics.hpp
  subset_ir.hpp

src/c_frontend/
  compile.cpp
  clang_bridge.cpp
  subset_checker.cpp
  subset_to_mlir.cpp
```

Optional CLI wiring:

```text
tooling/cli/
  driver.cpp
  main.cpp
```

Tests:

```text
tests/cpp/
  c_frontend_subset_test.cpp
  c_frontend_mlir_smoke_test.cpp
  c_frontend_rejection_test.cpp
```

## 11. Key Design Rules

- The accepted subset must be documented before it expands.
- Rejection quality matters as much as successful compilation.
- The adapter must not silently introduce semantics not present in the subset.
- If a construct cannot be made deterministic, reject it.
- Do not hide unsupported C behind partial lowering.

## 12. Recommendation

The next implementation step should be Milestone A:

- create `include/t81/c_frontend` / `src/c_frontend`
- add a compile-only CLI stub for `t81 c compile`
- wire a restricted Clang parse path
- implement explicit rejection tests before successful lowering tests

That is the smallest real step that proves the ecosystem direction without
overcommitting to a full language frontend.
