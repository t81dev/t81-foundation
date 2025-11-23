# T81 C++ Migration Roadmap

This document outlines the staged plan for migrating legacy `.cweb` code to the new header-first C++ API.

## Phase 0 — Compatibility (DONE)
- Keep C ABI stable via `src/c_api/` wrappers over modern C++ headers.
- New code consumes `<t81/t81.hpp>`; legacy code continues to include `include/t81/t81.h`.

## Phase 1 — Core Data Types (IN PROGRESS)
- ✅ `T243BigInt` → `include/t81/bigint.hpp` (signed, gcd)
- ✅ `T81Fraction` → `include/t81/fraction.hpp` (reduced, denom>0)
- ✅ `T729Tensor` → `include/t81/tensor.hpp` + ops in `include/t81/tensor/*.hpp`
- ✅ Entropy utilities → `include/t81/entropy.hpp`
- ✅ CanonFS surface + wire IO → `include/t81/canonfs*.hpp`

**Tests** under `tests/cpp`: `bigint_*`, `fraction_*`, `tensor_*`, `canonfs_io_*`.

## Phase 2 — IO & Codec Surfaces
- ✅ Tensor text IO → `include/t81/io/tensor_loader.hpp` + `src/io/tensor_loader.cpp`
- ✅ Base-81 & CanonHash stubs → `include/t81/hash/*`
- ✅ Base-243 codec surface (stub) → `include/t81/codec/base243.hpp`

## Phase 3 — IR Surface
- ✅ Minimal opcodes/insns/encoding → `include/t81/ir/*`
- 🔜 Map legacy instruction tables (`.td` / `.cweb`) into `Opcode` expansions.

## Phase 4 — Axion Façade
- ✅ Stub context → `include/t81/axion/api.hpp`
- 🔜 Replace with real backend bindings; keep API stable.

## Phase 5 — De-risked Deprecation
- For each migrated legacy module, leave a stub that forwards to C++ or emits a migration note.
- Provide Bazel wrapper targets in `legacy/` that reference the new headers/sources.

## Build Matrix
- **CMake**: `t81` (headers), `t81_io` (IO .cpp), examples/tests behind toggles.
- **Bazel**: `//:t81` library + `cc_test` and `cc_binary` targets.
- **Make**: shim for local dev (`make`, `make run-tests`).

## Canonical Test Vectors
- Reuse `tests/harness/canonical/*.json` for `bigint`, `fraction`, `tensor`.
- Add new vectors as we implement more ops; prefer JSON for language-agnostic reuse.

## Open Items
- BigInt division and normalization improvements (replace naive paths).
- Real Base-243 and Base-81 codecs; wire into BigInt and CanonHash seams.
- Tensor broadcasting semantics (expand tests beyond naive repeat).
