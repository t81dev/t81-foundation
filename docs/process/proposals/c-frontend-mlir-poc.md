# C Frontend MLIR Proof of Concept

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
- fixed-size local arrays only if lowered to explicit T81 memory slots

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
