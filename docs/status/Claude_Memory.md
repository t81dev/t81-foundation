# T81 Foundation - Claude Memory

## Project Identity
- **Name:** T81 Foundation
- **Purpose:** Deterministic ternary virtual machine + governance stack for high-stakes AI, cryptography, and scientific computing
- **Core concept:** Balanced ternary logic {-1, 0, +1}, bit-exact reproducibility, frozen immutable ISA
- **Repo:** /Users/t81dev/Code/t81-foundation

## Tech Stack
- **Language:** C++20/23 (primary), Python bindings, JS (doc indexing only)
- **Build:** CMake 3.16+, presets in CMakePresets.json; also has BUILD.bazel
- **Compilers:** AppleClang 17+, Clang 18+, GCC 14+, MSVC
- **Optional:** ASAN/UBSAN, llama.cpp adapter (T81_ENABLE_LLAMA_CPP), LLVM interface

## Key Directory Map
- `core/` — Frozen foundation: types (T81BigInt, T81Float), ISA (TISC), VM interpreter
- `kernel/axion/` — Axion Policy Kernel: governance enforcement, audit trails, CanonFS hooks
- `runtime/` — Tracing (CanonHash81), experimental Trace-JIT
- `lang/frontend/` — T81Lang: lexer → parser → semantic analyzer → TISC bytecode
- `src/` — codec (Base81/Base243), canonfs, c_api, crypto, simd (AVX2), tensor, io
- `include/t81/` — Public C++ API headers
- `spec/` — FROZEN normative specs (tisc-spec.md, t81lang-spec.md, t81vm-spec.md, axion-kernel.md)
- `docs/architecture/OVERVIEW.md` — Architecture authority doc ("North Star")
- `tooling/cli/main.cpp` — Main CLI entry point → `./build/t81` executable
- `tests/cpp/` — 300+ C++ unit/integration tests
- `experimental/` — Cognitive tiers (Tier1-5), distributed, hanoi (archived)
- `examples/` — Demos: axion, llama32, canonfs, tensor_ops, ir_roundtrip

## Architecture Pattern
```
T81Lang (source)
    ↓ lang/frontend (lexer/parser/semantic)
TISC Bytecode
    ↓ kernel/axion (policy enforcement intercept)
T81VM (core/vm) - deterministic interpreter
    ↓
Foundation: TISC ISA + ternary data types (FROZEN per major version)
```

## CMake Libraries (12 core)
t81_core → t81_axion, t81_io, t81_c_api, t81_lang_frontend, t81_isa, t81_vm, t81_jit, t81_llvm, t81_tool_model, t81_tool_cli
- Main executable: `t81` (tooling/cli/main.cpp)

## Key CMake Flags
- T81_BUILD_TESTS, T81_BUILD_EXAMPLES, T81_BUILD_BENCHMARKS
- T81_USE_CXX23, T81_STRICT_DETERMINISTIC_FLOAT, T81_TRITWISE_PROFILE
- T81_ENABLE_ASAN, T81_ENABLE_UBSAN, T81_ENABLE_LLAMA_CPP

## Important Conventions
- **Spec authority:** /spec > docs/architecture > /docs > /book (never modify spec without major version bump)
- **Determinism Gates:** CI reproducibility checks; CanonHash81 for bit-exact tracing
- **Cognitive Tiers:** Tier1 (symbolic) → Tier5 (infinite) — computation abstraction levels
- **T81W format:** Custom weight format for model serialization
- **CanonFS:** Canonical filesystem for immutable audit trails
- **Policy as Code:** Axion YAML/JSON policies intercepting TISC opcodes at runtime

## Key Files
- `CMakeLists.txt` — Main build config (~82KB, complex linkage tree)
- `spec/tisc-spec.md` — Frozen ISA spec
- `core/vm/vm.cpp` — VM interpreter
- `kernel/axion/policy_engine.cpp` — Governance enforcement
- `tooling/cli/main.cpp` — `t81` CLI entry point
- `tests/determinism/` — Reproducibility verification

## Recent Work (from git log)
- fa343e1: Merge from main
- c16dcd69: Remove test_from_chars
- dd90a21b: Merge PR #424 (dtype-closure-loop-graph)
- c93ed484: Lower T81Graph to VM native opcodes + tests

## Conformance Suite Status (2026-03-02)
- 21/24 → 22/24 passing: Fixed `policy-enforcement-allow-deny.t81` (Result.unwrap_ok TypeFault)
  - Fix: SA `visit(CallExpr)` caches object type in `_expr_type_cache` for method dispatch
- 22/24 → pending 23rd: `tier-annotation-enforcement.t81` still DecodeFaults
- 24th still pending: `type-kind-completeness.t81` (byte-string, Set/Map literals)

## TShape Opcode (Added this session)
- `include/t81/isa/opcodes.hpp` — Added `TShape` after `SetSize`
- `include/t81/isa/ir.hpp` — Added `TSHAPE` after `TSET`
- `core/isa/binary_emitter.cpp` — Added `TSHAPE` → `TShape` mapping
- `core/vm/vm.cpp` — Added TShape VM handler: `A = shape[R[C]] of tensor R[B]`

## Matrix 2D Indexing (Added this session, partially working)
- SA: `visit(VectorLiteralExpr)` handles Matrix context → returns Matrix type
- SA: `visit(IndexExpr)` handles `Matrix[T][i] → Vector[T]` and `Map[K,V][K] → V`
- SA: `visit(VariableExpr)` now caches to `_expr_type_cache` (line ~4845 sa.cpp)
- IR gen: `visit(VectorLiteralExpr)` — Matrix literal → 2D tensor pool entry + LOADI
- IR gen: `visit(IndexExpr)` — double-index `m[row][col]` → TSHAPE+MUL+ADD+TGET+F2I
- **KNOWN ISSUE**: DecodeFault in conformance program when multiple functions present
  - `canonical_types` + `tensor_rank` + `recursion` combo fails; individual pairs work
  - SUSPECTED: register clobbering across recursive calls (flat 243-register VM file, no save/restore)
  - Test programs at /tmp/test_tier_*.t81 for debugging

## Key SA/IR Architecture Notes
- `_expr_type_cache`: `unordered_map<const Expr*, Type>` populated by `evaluate_expression()`
- `evaluate_expression()` auto-caches types for all visited expressions
- `visit(VariableExpr)` now also caches explicitly (defensive fix)
- IR generator registers: global counter `_register_count` (starts at 1, skips 75-80)
- VM register file: flat 243 registers (R0..R242) shared across all function calls
- `add_tensor()` returns 1-based handle; `tensor_ptr(h)` does `h-1` for 0-based index
