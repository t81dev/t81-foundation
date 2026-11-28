# T81Lang Standard Library Plan

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Standard Library Plan](#t81lang-standard-library-plan)
  - [1. Design Philosophy](#1-design-philosophy)
  - [2. Library Structure](#2-library-structure)
    - [2.1 Core & Primitives (`std.core`)](#21-core-&-primitives-`stdcore`)
    - [2.2 Mathematics (`std.math`)](#22-mathematics-`stdmath`)
    - [2.3 Collections (`std.collections`)](#23-collections-`stdcollections`)
    - [2.4 Text & Data (`std.text`, `std.bytes`)](#24-text-&-data-`stdtext`-`stdbytes`)
    - [2.5 System & Runtime (`std.sys`)](#25-system-&-runtime-`stdsys`)
    - [2.6 Input/Output (`std.io`)](#26-inputoutput-`stdio`)
    - [2.7 Concurrency (`std.async`)](#27-concurrency-`stdasync`)
    - [2.8 AI & Tensor (`std.tensor`)](#28-ai-&-tensor-`stdtensor`)
    - [2.9 High-Level Agents (`std.agent`)](#29-high-level-agents-`stdagent`)
  - [3. Implementation Strategy](#3-implementation-strategy)
  - [4. Future Considerations](#4-future-considerations)
  - [5. Implementation Status (Current)](#5-implementation-status-current)
  - [6. Completed Milestone: `std.text.split` / `std.text.join`](#6-completed-milestone-`stdtextsplit`--`stdtextjoin`)
    - [6.1 Deterministic Semantics (Target)](#61-deterministic-semantics-target)
    - [6.2 Runtime/IR Work Completed](#62-runtimeir-work-completed)
    - [6.3 Acceptance Criteria](#63-acceptance-criteria)

<!-- T81-TOC:END -->


This document outlines the proposed standard libraries for T81Lang, designed to expose the powerful ternary-native types implemented in the `t81-foundation` C++ codebase.
For implementation continuity, see `docs/t81lang-standard-library-handoff.md`.

## 1. Design Philosophy

The T81Lang Standard Library adheres to the same core principles as the underlying C++ foundation:

1.  **Bounded Determinism**: Determinism claims apply to verified surfaces and deterministic profiles. Nondeterministic operations (e.g., system time, network IO) are wrapped in deterministic interfaces or explicitly flagged. **Exception:** Floating-point division and transcendentals currently rely on host precision.
2.  **Canonical Representation**: Data structures automatically maintain canonical forms (e.g., normalized fractions, sorted maps) to ensure consistent hashing and serialization.
3.  **Axion Integration**: All resource-intensive or side-effecting operations (IO, memory allocation, tensor ops) must emit Axion trace events and respect Axion policy limits.
4.  **Ternary Native**: Mathematical primitives prioritize balanced ternary arithmetic (`T81Int`, `T81BigInt`, `T81Float`) over binary approximations.

## 2. Library Structure

The standard library is organized into modules under the `std` namespace.

### 2.1 Core & Primitives (`std.core`)
Fundamental types that are likely built-in or implicitly available.

| T81Lang Type | C++ Implementation | Description |
| :--- | :--- | :--- |
| `Int` | `T81Int`, `T81Uint` | Standard balanced ternary integer (typically 27-trit). |
| `BigInt` | `T81BigInt` | Arbitrary-precision integer. |
| `Float` | `T81Float` | Floating-point type with host-dependent behavior for some operations (not globally bit-exact). |
| `Byte` | `T81Byte` | 5-trit byte representation. |
| `Bool` | `bool` (Trit) | Ternary logic values (False, Unknown, True). |
| `Result<T, E>` | `T81Result` | Error handling primitive (no exceptions). |
| `Option<T>` | `Option`, `T81Maybe` | Nullable value wrapper. |

### 2.2 Mathematics (`std.math`)
Extensive mathematical capabilities leveraging the `T81BigInt` and `T81Float` backends.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.math.fraction` | `T81Fraction` | Exact rational arithmetic. |
| `std.math.complex` | `T81Complex` | Complex numbers with ternary components. |
| `std.math.quat` | `T81Quaternion` | Quaternions for 3D rotation and physics. |
| `std.math.poly` | `T81Polynomial` | Polynomial arithmetic and evaluation. |
| `std.math.prob` | `T81Prob` | Probability handling, log-odds arithmetic. |
| `std.math.limb` | `T81Limb` | Low-level multiprecision limb access (advanced). |

### 2.3 Collections (`std.collections`)
Deterministic container types.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.collections.list` | `T81List` | Dynamic array/list implementation. |
| `std.collections.map` | `T81Map` | Ordered map (likely B-Tree or similar). |
| `std.collections.set` | `T81Set` | Ordered set. |
| `std.collections.tree` | `T81Tree` | Generic tree structure for hierarchical data. |
| `std.collections.graph` | `T81Graph` | Graph data structure with node/edge attributes. |

### 2.4 Text & Data (`std.text`, `std.bytes`)
String and binary data manipulation.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.text` | `T81String` | Unicode-aware string handling. |
| `std.bytes` | `T81Bytes` | Raw binary data manipulation. |
| `std.symbol` | `T81Symbol` | Interned strings/atoms for efficient comparison. |

### 2.5 System & Runtime (`std.sys`)
Interaction with the HanoiVM and Axion environment.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.sys.time` | `T81Time` | Deterministic logical time or synchronized real-time. |
| `std.sys.entropy` | `T81Entropy`, `T81Random` | CSPRNG seeded deterministically by the VM. |
| `std.sys.reflect` | `T81Reflection` | Introspection of code and state (Tier 4 capability). |
| `std.sys.proof` | `T81Proof` | Cryptographic proofs of execution/state. |

### 2.6 Input/Output (`std.io`)
Policy-gated I/O operations.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.io.stream` | `T81IOStream`, `T81Stream` | Buffered I/O streams. |
| `std.io.net` | `T81Network`, `T81Discovery` | Network sockets and peer discovery (Axion-gated). |

### 2.7 Concurrency (`std.async`)
Deterministic concurrency primitives.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.async.thread` | `T81Thread` | Green threads or deterministic fibers. |
| `std.async.promise` | `T81Promise` | Futures/Promises for asynchronous results. |

### 2.8 AI & Tensor (`std.tensor`)
High-performance tensor operations for AI workloads.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.tensor` | `T81Tensor` | N-dimensional arrays, autograd support. |
| `std.tensor.dist` | `DistributedTensor` | Tensors spanned across multiple nodes. |
| `std.tensor.matrix` | `T81Matrix` | Specialized 2D matrix operations. |
| `std.tensor.vector` | `T81Vector` | Specialized 1D vector operations. |

### 2.9 High-Level Agents (`std.agent`)
Building blocks for autonomous agents.

| Module | C++ Implementation | Features |
| :--- | :--- | :--- |
| `std.agent` | `T81Agent` | Base class for agents, handling identity and lifecycle. |

## 3. Implementation Strategy

1.  **Native Bindings**: Create a T81Lang binding layer that exposes the C++ `T81*` classes to the interpreter/VM.
2.  **Wrapper Modules**: Write `.t81` source files that provide idiomatic interfaces to these native bindings.
3.  **Axion Hooks**: Ensure every native call injects appropriate Axion trace events (`Axion::emit(...)`) before execution.
4.  **Unit Tests**: Port existing C++ unit tests to T81Lang to verify correctness of the bindings.

## 4. Future Considerations

-   **`std.crypto`**: Cryptographic primitives (hashing, signing) - potentially leveraging `T81Proof`.
-   **`std.physics`**: Engines using `T81Quaternion` and `T81Vector`.
-   **`std.ui`**: Abstract UI definitions for terminal or graphical output.

## 5. Implementation Status (Current)

- `std.math`: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`, `tanh`, `sqrt`, `exp`, `log`, `pow`, and `clamp` are wired as deterministic frontend/runtime aliases.
- `std.io`: `println`, `print_int`, and `print_float` aliases lower to `print`.
- `std.core`: `debug` now lowers as a deterministic frontend/runtime alias to `print` with existing scalar print-type constraints.
- `std.core`: `assert` now lowers as a deterministic frontend/runtime alias that traps on false conditions.
- `std.core`: `unwrap_or` now lowers as a deterministic frontend/runtime alias over `Option[T]` via `OPTION_IS_SOME` / `OPTION_UNWRAP`.
- `std.sys` / `std.async` / `std.agent`: `exit`, `time`, `entropy`, `proof`, `reflect`, `yield`, `sleep`, `thread`, `promise`, and `self_reflect` are now wired as deterministic frontend/runtime aliases (`exit -> TRAP`, `time -> 0.0`, `entropy -> 0`, `proof/thread/promise -> typed runtime handles with stable textual rendering`, `reflect/self_reflect -> META_REFLECT`, `yield/sleep -> no-op`) with conformance, IR, and CLI-check coverage.
- `std.sys` / `std.io` / `std.async`: fixture-driven CLI golden coverage now validates deterministic runtime output for typed-handle aliases (`proof`, `stream`, `net`, `thread`, `promise`) plus deterministic baseline behavior for `time`, `entropy`, `reflect`, `yield`, and `sleep`.
- `std.tensor`: `load`, `from_list`, `matmul`, and `vec_add` aliases are implemented and tested.
- `std.math` / `std.tensor` / `std.collections` / `std.sys` / `std.async` / `std.agent`: wrapper module files now expose the directly wrappable aliases via `lang/stdlib/std/{math,tensor,collections,sys,async,agent}.t81` (`std.tensor.load` remains direct-use because it currently requires a string literal argument at call sites).
- `std.tensor`: fixture-driven CLI golden coverage now validates deterministic observable output and runtime execution for `load`, `from_list`, `vec_add`, and `matmul` via `tests/fixtures/t81lang_std_tensor/*` and `tests/cpp/cli_std_tensor_fixtures_test.cpp`, using an in-memory deterministic fixture weights model for rank-2 `matmul`.
- `std.text`: `str_len`, `str_is_empty`, `concat`, `starts_with`, `ends_with`, `contains`, `index_of`, and `replace` are implemented end-to-end (semantic, IR, VM) and exposed via `lang/stdlib/std/text.t81`.
- `std.text`: `to_string` and `from_bytes` are exposed as deterministic aliases in frontend/IR/module wrappers (`T81String|T81Bytes -> T81String`).
- `std.text`: fixture-driven CLI golden coverage now validates deterministic runtime output via `tests/fixtures/t81lang_std_text/*` and `tests/cpp/cli_std_text_fixtures_test.cpp`.
- `std.text`: `split` / `join` are implemented end-to-end across semantic analysis, IR lowering, and VM execution, including deterministic handling of `Vector[T81String]` via runtime handles.
- `std.bytes`: `len`, `is_empty`, `concat`, `starts_with`, `ends_with`, `contains`, `index_of`, `replace`, `to_string`, and `from_string` are implemented for frontend-native `T81Bytes`, with explicit `T81Bytes(...)` conversion calls and fixture-driven CLI golden coverage in `tests/fixtures/t81lang_std_bytes/*` and `tests/cpp/cli_std_bytes_fixtures_test.cpp`.
- `std.bytes`: `split` / `join` now mirror `std.text` semantics for byte data (`split` preserves empty segments, `join` supports empty vectors) with deterministic semantic diagnostics for empty literal separators.
- `std.collections`: `len`, `is_empty`, `first`, `last`, `push`, and `pop` are wired as deterministic frontend/runtime aliases over frontend `Vector[T]` values, covering both interned tensor-backed vectors and string-vector runtime handles; `first`/`last`/`pop` reject empty literal vectors with deterministic semantic diagnostics.
- `std.collections`: roadmap module entrypoints `list`, `map`, `set`, `tree`, and `graph` are exposed; all five now return deterministic empty `Vector[T81String]` values as staged constructor placeholders.
- `std.collections`: staged flat-map helpers `map_size`, `map_has`, `map_put`, `map_get`, `map_remove`, and `map_keys` are now implemented deterministically over `Vector[T81String]` key/value-pair encodings (`[k0, v0, ...]`, odd tails ignored).
- `std.collections`: staged set helpers `set_size`, `set_has`, `set_add`, and `set_remove` are now implemented deterministically over `Vector[T81String]` set encodings (`set_add` is idempotent; `set_remove` removes all matches while preserving survivor order).
- `std.collections`: staged graph helpers `graph_edge_count`, `graph_has_edge`, `graph_add_edge`, `graph_remove_edge`, and `graph_neighbors` are now implemented deterministically over flat edge encodings (`[from0, to0, ...]`, odd tails ignored; `graph_add_edge` is idempotent, `graph_remove_edge` removes all matching edges while preserving survivor order, and `graph_neighbors` preserves encounter order by edge position).
- `std.collections`: fixture-driven CLI golden coverage now validates deterministic observable output via `tests/fixtures/t81lang_std_collections/*` and `tests/cpp/cli_std_collections_fixtures_test.cpp`.
- Generic user-defined functions: call-site explicit type arguments are now supported (`fn id[T](#x-t) -> T`, `id[i32](#7)`), including partial explicit binding with inference fallback (`fn first[T, U](#a-t-b-u) -> T`, `first[i32](#7-tail)`), with deterministic semantic diagnostics for explicit-type-arity and argument-type mismatches plus parser/semantic/conformance coverage.
- Generic user-defined functions: unresolved generic parameters now fail deterministically when inference cannot bind them, including single-parameter (`Cannot infer generic parameter 'T' ...`) and multi-parameter (`Cannot infer generic parameters 'T', 'U' ...`) diagnostics.
- `std.symbol`: `intern`, `to_string`, `eq`, and `ne` are exposed as deterministic frontend/IR aliases in `lang/stdlib/std/symbol.t81` (currently represented as interned `T81String` handles in T81Lang runtime state).
- `std.symbol`: fixture-driven CLI golden coverage now validates deterministic observable output via `tests/fixtures/t81lang_std_symbol/*` and `tests/cpp/cli_std_symbol_fixtures_test.cpp`.

## 6. Completed Milestone: `std.text.split` / `std.text.join`

### 6.1 Deterministic Semantics (Target)

- `split(s: T81String, sep: T81String) -> Vector[T81String]`
- `join(parts: Vector[T81String], sep: T81String) -> T81String`
- `split` keeps empty segments:
  - `split("a,,b", ",") == ["a", "", "b"]`
  - `split(",a,", ",") == ["", "a", ""]`
- `split` with empty separator is invalid and must fail deterministically in semantic analysis.
- `join([] , sep) == ""`
- `join(["a", "b"], ",") == "a,b"`

### 6.2 Runtime/IR Work Completed

1. Introduce a deterministic runtime representation for `Vector[T81String]` values that is usable by both frontend and VM.
2. Add lowering path for `std.text.split` and `std.text.join` in semantic analyzer + IR generator.
3. Implement VM execution path for split/join operations without host-dependent behavior.

### 6.3 Acceptance Criteria

1. Conformance tests validate type checking and deterministic diagnostics (bad arity, bad type, empty separator).
2. IR tests validate lowering for `STRSPLIT`, `STRJOIN`, and string-vector literal construction.
3. E2E tests validate observable behavior including empty-segment preservation.
4. CLI fixtures under `tests/fixtures/t81lang_std_text/` include golden outputs for split/join behavior.
