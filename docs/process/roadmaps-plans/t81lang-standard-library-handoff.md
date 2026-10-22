# T81Lang Standard Library Handoff

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Standard Library Handoff](#t81lang-standard-library-handoff)
  - [1. Current State](#1-current-state)
  - [2. Files Most Recently Touched](#2-files-most-recently-touched)
  - [3. Required Validation Gate](#3-required-validation-gate)
  - [4. What Is Next (Priority Order)](#4-what-is-next-priority-order)
  - [5. Determinism/Quality Rules To Preserve](#5-determinismquality-rules-to-preserve)
  - [6. Suggested Commit Chunking](#6-suggested-commit-chunking)
  - [7. Fast Resume Checklist For New AI](#7-fast-resume-checklist-for-new-ai)

<!-- T81-TOC:END -->


This document is the execution handoff for continuing the standard library plan with another AI agent.

## 1. Current State

Implemented and validated end-to-end (semantic + IR + VM + CLI coverage):
- `std.core`: `assert`, `debug`, `unwrap_or`
- `std.math`: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `sqrt`, `exp`, `log`, `pow`, `clamp`
- `std.io`: `println`, `print_int`, `print_float`, `stream`, `net` (`stream/net` currently materialize typed runtime handles with stable textual rendering)
- `std.text`: `str_len`, `str_is_empty`, `concat`, `starts_with`, `ends_with`, `contains`, `index_of`, `replace`, `to_string`, `from_bytes`, `split`, `join`
- `std.bytes`: `len`, `is_empty`, `concat`, `starts_with`, `ends_with`, `contains`, `index_of`, `replace`, `to_string`, `from_string`, `split`, `join`, `T81Bytes(...)`
- `std.collections`: `len`, `is_empty`, `first`, `last`, `push`, `pop`, `list`, `map`, `set`, `tree`, `graph` (all five constructors currently return deterministic empty runtime-backed vector values)
- `std.collections`: staged map helpers `map_size`, `map_has`, `map_put`, `map_get`, `map_remove`, and `map_keys` are implemented over flat `Vector[T81String]` key/value encodings.
- `std.collections`: staged set helpers `set_size`, `set_has`, `set_add`, and `set_remove` are implemented over `Vector[T81String]` set encodings (`set_add` idempotent, `set_remove` removes all matches preserving survivor order).
- `std.collections`: staged graph helpers `graph_edge_count`, `graph_has_edge`, `graph_add_edge`, `graph_remove_edge`, and `graph_neighbors` are implemented over flat `Vector[T81String]` edge encodings (`[from0, to0, ...]`, odd tails ignored; `graph_add_edge` idempotent, `graph_remove_edge` removes all matching edges while preserving survivor order, and `graph_neighbors` preserves encounter order by edge position).
- `std.symbol`: `intern`, `to_string`, `eq`, `ne`
- `std.sys`: `exit`, `time`, `entropy`, `proof`, `reflect` (`proof` currently materializes a typed runtime handle with stable textual rendering; `reflect` lowers to `META_REFLECT`)
- `std.async`: `yield`, `sleep`, `thread`, `promise` (`thread/promise` currently materialize typed runtime handles with stable textual rendering)
- `std.tensor`: `load`, `from_list`, `matmul`, `vec_add`
- `std.agent`: `self_reflect`
- `std.sys` / `std.io` / `std.async`: fixture-driven CLI goldens for runtime observable behavior are now present under `tests/fixtures/t81lang_std_runtime/*` and `tests/cpp/cli_std_runtime_fixtures_test.cpp` (including deterministic `std.sys.reflect` execution coverage)

Generic function work now supported:
- Generic declarations: `fn id[T](#x-t) -> T`
- Inferred calls: `id(7)`
- Explicit calls: `id[i32](#7)`
- Partial explicit calls with inference fallback: `first[i32](#7-tail)`
- Deterministic unresolved inference diagnostics for unbound generics:
  - Example: `fn none_of[T]() -> Option[T] { return None; }` then `none_of()`
  - Diagnostic: `Cannot infer generic parameter 'T' for function 'none_of'.`
  - Example: `fn none_pair[T, U]() -> Option[Result[T, U]] { return None; }` then `none_pair()`
  - Diagnostic: `Cannot infer generic parameters 'T', 'U' for function 'none_pair'.`

## 2. Files Most Recently Touched

Core implementation:
- `core/vm/vm.cpp`
- `runtime/jit/jit_compiler.cpp`
- `include/t81/vm/state.hpp`

Coverage:
- `tests/cpp/vm_extended_ops_test.cpp`
- `tests/fixtures/t81lang_std_runtime/01_tokens.t81`
- `tests/fixtures/t81lang_std_runtime/01_tokens.out`
- `tests/fixtures/t81lang_std_runtime/README.md`

Tracking/docs:
- `docs/standard-library.md`
- `docs/t81lang-standard-library-plan.md`
- `TASKS.md`

## 3. Required Validation Gate

Always run before proposing commit/PR:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Current known good baseline: full suite passing (`212/212`).

## 4. What Is Next (Priority Order)

1. Replace vector-placeholder constructor semantics with full runtime semantics for:
   - `std.collections.list/map/set/tree/graph`
   - keyed/tree/graph-specific deterministic data models and operations
2. Keep fixture-driven CLI goldens for each new module before marking complete.

## 5. Determinism/Quality Rules To Preserve

- No hidden nondeterminism.
- Deterministic diagnostics for type/arity/semantic failures.
- Do not weaken existing tests; add coverage for every behavior change.
- Keep frontend/IR/VM behavior aligned (no frontend-only aliases without runtime story).

## 6. Suggested Commit Chunking

For cleaner handoff and review, keep future changes in small chunks:
1. Semantics change
2. IR/runtime lowering
3. CLI fixture + golden
4. Docs + TASKS status update

## 7. Fast Resume Checklist For New AI

1. Read:
   - `docs/t81lang-standard-library-plan.md`
   - `docs/standard-library.md`
   - `TASKS.md`
2. Run full tests once to confirm baseline.
3. Pick next unimplemented module and define:
   - semantic contract
   - IR opcode mapping
   - VM behavior
   - conformance + fixture tests
4. Implement in vertical slice and re-run full gate.
