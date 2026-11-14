# Repository Restructure Masterplan

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Repository Restructure Masterplan](#repository-restructure-masterplan)
  - [0.1 Target Directory Tree (Canonical)](#01-target-directory-tree-canonical)
  - [0.2 Mapping Table: Old → New](#02-mapping-table-old-→-new)
  - [0.3 Public vs Internal API Policy](#03-public-vs-internal-api-policy)
  - [0.4 Build System Strategy](#04-build-system-strategy)
  - [0.5 Migration Rules](#05-migration-rules)
  - [0.6 Rollout Phases](#06-rollout-phases)
    - [Phase 1: Core Types Migration (The Pilot)](#phase-1-core-types-migration-the-pilot)
    - [Phase 2: ISA (TISC) Module](#phase-2-isa-tisc-module)
    - [Phase 3: VM Core](#phase-3-vm-core)
    - [Phase 4: Axion Kernel](#phase-4-axion-kernel)
    - [Phase 5: Language & Tooling](#phase-5-language-&-tooling)
    - [Phase 6: Runtime & Serialization](#phase-6-runtime-&-serialization)
    - [Phase 7: Experimental Containment](#phase-7-experimental-containment)
    - [Phase 8: Public API Cleanup & Final Polish](#phase-8-public-api-cleanup-&-final-polish)
    - [Phase 9: Documentation Updates](#phase-9-documentation-updates)

<!-- T81-TOC:END -->


## 0.1 Target Directory Tree (Canonical)

This layout reflects the conceptual architecture of the T81 Foundation stack.

```
/core
  /types            # ternary primitives, base-81, dmath, canonical numerics (T81BigInt etc)
  /isa              # TISC ISA, opcode tables, encoding/decoding, disasm helpers
  /vm               # interpreter core, state model, memory model
  /serialization    # canonical serialization formats (CanonFS, Codec)
  /api              # C API implementation (formerly src/c_api)
  /crypto           # Cryptographic primitives (Hash, etc)
/kernel
  /axion            # policy engine, enforcement hooks, trace policy infra
/runtime
  /jit              # trace JIT (experimental unless verified)
  /tracing          # trace formats, trace capture, trace hashing
/lang
  /frontend         # lexer/parser/semantic/IR/emitter
  /stdlib           # language library (std/)
/tooling
  /cli              # t81 CLI implementation
  /model            # safetensors/gguf/t81w tooling
  /python           # Python bindings/tooling
/experimental
  /tiers            # cognitive tiers
  /hanoi            # hanoi vm/kernel
  /setun            # setun legacy/experimental
/tests
  (unchanged structure, but updated paths)
/include
  /t81              # public headers only (stable API)
    /types          # core types public headers
    /isa            # ISA public headers
    /vm             # VM public headers
    ...
/internal           # private headers (not exposed to users)
/docs
/spec
/book
/examples
/scripts
/tools
/contracts
```

## 0.2 Mapping Table: Old → New

| Current                         | New                     | Notes |
| ------------------------------- | ----------------------- | ----- |
| `src/data_types/**`             | `core/types/**`         | Mostly docs/tests currently |
| `src/core/**`                   | `core/types/**`         | Core numerics implementation (BigInt, Fraction) |
| `include/t81/core/**`           | `include/t81/types/**`  | Public headers for core types |
| `core/isa/**`                   | `core/isa/**`           | |
| `src/vm/**`                     | `core/vm/**`            | Interpreter core |
| `src/axion/**`                  | `kernel/axion/**`       | |
| `src/canonfs/**`                | `core/serialization/**` | CanonFS drivers |
| `src/codec/**`                  | `core/serialization/codec/**` | |
| `src/io/**`                     | `core/io/**`            | |
| `src/frontend/**`               | `lang/frontend/**`      | |
| `src/lang/**`                   | `lang/stdlib/**`        | Contains `std/` |
| `src/tiers/**`                  | `experimental/tiers/**` | |
| `src/hanoi/**`                  | `experimental/hanoi/**` | |
| `src/setun/**`                  | `experimental/setun/**` | |
| `src/cli/**`                    | `tooling/cli/**`        | |
| `src/tools/**`                  | `tooling/model/**`      | Weights tooling |
| `src/python/**`                 | `tooling/python/**`     | Python bindings |
| `src/c_api/**`                  | `core/api/**`           | C API implementation |
| `src/crypto/**`                 | `core/crypto/**`        | |
| `src/hash/**`                   | `runtime/tracing/**`    | |
| `src/simd/**`                   | `core/types/simd/**`    | Optimized primitives |
| `src/tensor/**`                 | `core/types/tensor/**`  | Tensor implementation |
| `include/t81/**`                | `include/t81/**`        | Stays, but subdirs reorganized (e.g. `core` -> `types`) |

## 0.3 Public vs Internal API Policy

* **Public**: `include/t81/**`. These headers define the stable API.
* **Internal**: `internal/**` (new top-level). Headers required for build but not exposed to users.
* **Private**: Headers inside `src` (now `core`, `kernel`, etc) remain private to that module.

## 0.4 Build System Strategy

* **CMake**: Primary build system. `CMakeLists.txt` at root will be updated to include new subdirectories.
* **Bazel**: Must be kept in sync with module boundaries if present.

## 0.5 Migration Rules

* Use `git mv` for all moves to preserve history.
* Update `#include` paths mechanically (scripted).
* Update `CMakeLists.txt` targets.
* Verify build and tests after each phase.
* **No semantic changes**.

## 0.6 Rollout Phases

### Phase 1: Core Types Migration (The Pilot)
* **Goal**: Move `src/core`, `src/data_types`, and `include/t81/core` to `core/types` and `include/t81/types`.
* **Rationale**: Foundational dependency, good candidate to validate tooling.
* **Steps**:
  1. Create `core/types`, `include/t81/types`.
  2. `git mv src/core/* core/types/`
  3. `git mv src/data_types/* core/types/`
  4. `git mv include/t81/core/* include/t81/types/`
  5. Update includes (`t81/core/...` -> `t81/types/...`).
  6. Update CMake targets.

### Phase 2: ISA (TISC) Module
* **Goal**: Move `src/tisc` to `core/isa`.
* **Steps**:
  1. `git mv src/tisc core/isa`
  2. Update includes.

### Phase 3: VM Core
* **Goal**: Move `src/vm` to `core/vm`.
* **Steps**:
  1. `git mv src/vm core/vm`
  2. Update includes.

### Phase 4: Axion Kernel
* **Goal**: Move `src/axion` to `kernel/axion`.

### Phase 5: Language & Tooling
* **Goal**: Split `src/frontend`, `src/lang`, `src/cli`, `src/tools`, `src/python`.
* **Moves**:
  * `src/frontend` -> `lang/frontend`
  * `src/lang` -> `lang/stdlib`
  * `src/cli` -> `tooling/cli`
  * `src/tools` -> `tooling/model`
  * `src/python` -> `tooling/python`

### Phase 6: Runtime & Serialization
* **Goal**: Move `src/canonfs`, `src/codec`, `src/io`, `src/crypto`, `src/hash`.
* **Moves**:
  * `src/canonfs` -> `core/serialization`
  * `src/codec` -> `core/serialization/codec`
  * `src/io` -> `core/io`
  * `src/crypto` -> `core/crypto`
  * `src/hash` -> `core/crypto/hash`

### Phase 7: Experimental Containment
* **Goal**: Move `src/tiers`, `src/hanoi`, `src/setun`.
* **Moves**:
  * `src/tiers` -> `experimental/tiers`
  * `src/hanoi` -> `experimental/hanoi`
  * `src/setun` -> `experimental/setun`

### Phase 8: Public API Cleanup & Final Polish
* **Goal**: Audit `include/t81`, move internal headers to `internal/`.
* **Moves**:
  * `src/c_api` -> `core/api` (or keep in src/core/api?) -> `core/api`.

### Phase 9: Documentation Updates
* Update paths in `docs/` and `README.md`.
