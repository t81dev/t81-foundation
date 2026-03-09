# Rust Frontend MLIR Proof of Concept

This proposal defines the next ingress milestone after the C-subset frontend:
accept a tightly restricted subset of Rust and lower it into the existing
T81 MLIR/TISC pipeline.

## 1. Goal

Demonstrate that a mainstream safe-systems language can target the T81 execution
surface without rewriting logic in T81Lang and without relaxing determinism.

The first Rust milestone should stay deliberately small:

- one source language: Rust
- one compile-only path
- one deterministic-safe subset
- explicit rejection for unsupported features

This is not a general Rust compiler. It is an ingress proof of concept.

## 2. Why Rust Next

Rust is the right next frontend after C because:

- it strengthens the ecosystem claim beyond “legacy systems language only”
- it offers a naturally tighter baseline around memory safety and explicit types
- it is a realistic target language for deterministic infrastructure and runtime
  components
- the C frontend already established the ingress pattern, CLI surface, and
  fail-closed philosophy

## 3. Current State

Already landed on `main`:

- `T81_ENABLE_RUST_FRONTEND` CMake option
- `t81_rust_frontend` target, gated on `rustc` availability
- `t81 rust compile` CLI/help/completion surface
- fail-closed contract when the Rust frontend is not built
- fail-closed scalar-subset lowering through the existing C frontend adapter
- CLI contract coverage for `help rust`, fish completion, and `t81 rust compile`

Latest implementation commits:

- `4b54c254` `Add Rust frontend CLI scaffold`
- `6053c17b` `Expand constant-expression array support in C frontend`
- current follow-up: real scalar Rust lowering using the C-subset adapter

Important environment note:

- real Rust lowering depends on a Rust toolchain plus the experimental C frontend
  adapter path in the build environment

## 4. Non-Goals

The first Rust proof of concept should not try to support:

- general Cargo workspaces
- crates.io dependency resolution
- traits beyond the small subset needed for parsing/typing accepted programs
- generics in v0
- borrowing-heavy APIs that require a real ownership model beyond simple locals
- heap allocation, `Vec`, `String`, slices, or references
- floating-point by default
- `unsafe`
- macros beyond what `rustc` must minimally parse

## 5. Proposed User Surface

```text
t81 rust compile <input.rs> [-o <output.mlir>] [--emit mlir] [--mode <compat|dcp>] [--dialect <standard|t81>]
```

Initial guarantee:

- `--emit mlir` only

Like the C frontend, later stages can reuse the existing MLIR/LLVM/TISC path
once the frontend half is stable.

## 6. Supported Rust Subset v0

The first accepted subset should be intentionally austere.

Allowed:

- one source file
- one entry function: `fn main() -> i32`
- additional helper functions with explicit `i32` parameters and `i32` return
  types
- local `let mut` bindings of `i32`
- integer literals
- arithmetic: `+`, `-`, `*`, `/`, `%`
- bitwise ops: `&`, `|`, `^`, `<<`, `>>`
- comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`
- boolean operators lowered as integer predicates: `!`, `&&`, `||`
- `if`, `while`, and simple `loop` only if it can be normalized to `while`
- bounded `for` only if it can be reduced into an integer loop surface
- same-file helper calls without recursion
- fixed local arrays `[i32; N]` only if lowered to the same constrained T81
  memory surface used by the C frontend

Rejected:

- references, borrowing, and dereference
- slices, `Vec`, `String`, tuples, structs, enums
- pattern matching in v0
- generics and traits
- closures
- methods and impl blocks
- `unsafe`
- macros beyond a minimal accepted parse surface
- external crates and module trees

## 7. Determinism Contract

The Rust frontend must follow the same fail-closed rule as the C frontend:

- unsupported syntax is rejected explicitly
- supported-but-nondeterministic semantics are rejected
- no fallback to host compilation or native code generation is allowed
- subset boundaries must be part of the user-facing contract, not hidden

Rust’s richer syntax should not tempt us into silent partial lowering.

## 8. Integration Strategy

Recommended architecture:

1. invoke `rustc` in a parser/analysis-friendly mode
2. normalize accepted Rust constructs into a small internal subset IR
3. lower that IR into the existing TISC/MLIR surface
4. reuse current MLIR lowering to LLVM IR and downstream outputs

This should mirror the architectural lesson from the C frontend: do not lower
directly from a full host-language AST into raw TISC instructions without an
adapter seam.

## 9. Milestones

Milestone A: scaffold and diagnostics

- already done
- build flag, target, CLI, help, completion, fail-closed error path

Milestone B: minimal scalar Rust subset

- `fn main() -> i32`
- local `i32` variables
- arithmetic/comparisons
- `if`
- `return`
- MLIR output only

Milestone C: structured control flow and helpers

- helper functions
- loops
- boolean/bitwise operators
- recursion rejection

Milestone D: constrained memory

- fixed local `[i32; N]` arrays
- indexing contract aligned with the C frontend

## 10. Recommended Resume Point

1. decide whether to deepen Rust first with loops and constrained arrays, or
   keep Rust at scalar subset v0 and start Python ingress work
2. if Rust deepens next, keep using the current adapter seam unless a real
   `rustc` AST/HIR integration becomes necessary
3. align any future Rust array work with the existing constrained C memory model
4. keep the subset and diagnostics at least as explicit as the current C
   frontend

## 11. Success Criteria

This Rust PoC is successful if:

- `t81 rust compile hello.rs -o hello.mlir` works for a very small accepted
  subset
- unsupported Rust features fail with direct diagnostics
- the emitted MLIR reuses the current T81/standard MLIR pipeline
- the feature set is documented precisely enough that later Rust work is
  extension, not reinterpretation
