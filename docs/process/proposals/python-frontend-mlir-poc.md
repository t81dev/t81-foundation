# Python Frontend MLIR Proof of Concept

This proposal records the Python ingress milestone after the C and Rust subset
frontends: accept a tightly restricted subset of Python and lower it through
the existing T81 MLIR/TISC pipeline.

## Current State

Already landed on `main`:

- `T81_ENABLE_PYTHON_FRONTEND` CMake option
- `t81_python_frontend` target, gated on `python3` availability plus the
  experimental C frontend adapter path
- `t81 python compile` CLI/help/completion surface
- fail-closed scalar-subset lowering through a Python `ast` normalizer
  script and the existing C-subset adapter
- CLI contract coverage for `help python`, fish completion, and
  `t81 python compile`

Latest implementation commit:

- current follow-up in workspace: Python scalar subset lowering

## Supported Python Subset v0

Allowed:

- one source file
- one entry function: `def main() -> int`
- helper functions with explicit `int` parameters and `int` return type
- annotated local bindings like `x: int = 2`
- simple assignment
- integer and boolean literals
- arithmetic: `+`, `-`, `*`, `/`, `%`
- bitwise ops: `&`, `|`, `^`, `<<`, `>>`
- comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`
- boolean operators: `not`, `and`, `or`
- `if` / `else`
- same-file helper calls
- reachable `return`

Rejected:

- loops
- lists, indexing, attributes
- objects/classes/modules/imports
- floats, strings, containers
- keyword arguments and decorators
- unsupported statements or expressions outside the scalar subset

## Integration Strategy

The current Python path does not invent a third lowering backend.

It works as:

1. parse accepted Python with a dedicated `ast`-based subset normalizer
2. emit equivalent restricted C-subset source
3. reuse the existing C-subset adapter into TISC / MLIR
4. reuse the current MLIR lowering pipeline downstream

That keeps the Python ingress fail-closed and lets the repo share one practical
adapter seam across C, Rust, and Python instead of three separate backends.

## Last Verified Locally

- `t81_python_frontend_mlir_smoke_test`
- `t81_cli_contract_test ./build_python_frontend_subset/t81`
- direct smoke compile of a helper-call Python example to MLIR with `--dialect=t81`

## Recommended Resume Point

1. decide whether to deepen Python first with loops / constrained list-style
   memory or stop here and consolidate the shared adapter boundary
2. if Python deepens next, keep the subset fail-closed and deterministic
3. if the three frontends start to drift, introduce a more explicit shared IR
   seam instead of extending C-as-adapter indefinitely
