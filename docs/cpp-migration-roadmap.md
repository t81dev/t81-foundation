# T81 C++ Migration Roadmap

This document tracks migration of legacy `.cweb` code to the new header-first C++ API.

## Phase 0 — Compatibility (DONE)
- Keep C ABI stable via `src/c_api/` wrappers over modern C++ headers.
- New code consumes `<t81/t81.hpp>`; legacy code may continue to include `include/t81/t81.h`.
- CMake, Bazel, and Make shims build the same headers.

## Phase 1 — Core Data Types (**DONE**)
- ✅ `T243BigInt` → `include/t81/bigint.hpp` (signed; add/sub/mul/mod/gcd/exact-div)
- ✅ `T81Fraction` → `include/t81/fraction.hpp` (reduced, `den>0`, canonical zero)
- ✅ `T729Tensor` → `include/t81/tensor.hpp`
- ✅ Tensor ops → `include/t81/tensor/{transpose,slice,reshape,matmul,reduce,broadcast}.hpp`
- ✅ Shape helpers → `include/t81/tensor/shape.hpp`
- ✅ Entropy utilities → `include/t81/entropy.hpp`
- ✅ CanonFS surface + wire IO → `include/t81/{canonfs,canonfs_io}.hpp`
  
**Tests** (`tests/cpp`)
- BigInt: `bigint_roundtrip.cpp`
- Fraction: `fraction_roundtrip.cpp`
- Tensor: `tensor_{transpose,slice,reshape,matmul,reduce,broadcast}_test.cpp`, `tensor_shape_test.cpp`
- Entropy: `entropy_test.cpp`
- CanonFS/Hash: `canonfs_io_test.cpp`, `hash_stub_test.cpp`

## Phase 2 — IO & Codec Surfaces (**DONE**)
- ✅ Tensor text IO → `include/t81/io/tensor_loader.hpp` + `src/io/tensor_loader.cpp` + `tensor_loader_test.cpp`
- ✅ Base-81 & CanonHash stubs → `include/t81/hash/{base81,canonhash}.hpp`
- ✅ Base-243 codec surface (stub) → `include/t81/codec/base243.hpp` + `codec_base243_test.cpp`

> Note: Base-81 and Base-243 are deterministic **stubs**. Swap with canonical codecs without changing call sites.

## Phase 3 — IR Surface (**DONE**)
- ✅ Minimal IR: `include/t81/ir/{opcodes,insn,encoding}.hpp` with 32-byte LE encoding
- ✅ Round-trip tests covered by `t81_ir_encoding_test` (see test list)

**Next:** Map/expand opcodes from legacy `.td`/`.cweb` tables into `Opcode` (non-breaking additive extensions).

## Phase 4 — Axion Façade (**IN PROGRESS**) 
- ✅ Stub context/API → `include/t81/axion/api.hpp`
- ✅ Example & test → `examples/axion_demo.cpp`, `tests/cpp/axion_stub_test.cpp`
- 🔜 Wire real backend bindings behind the same façade (keep API stable)

## Phase 5 — De-risked Deprecation (**ONGOING**)
- Leave thin shims in `legacy/` that forward or emit migration notes.
- Provide Bazel/CMake wrapper targets pointing to new headers/impls.
- Track per-module parity and remove `.cweb` once tests pass.

## Phase 6 — Build & CI Matrix (**ONGOING**)
- **CMake**: `t81` (headers), `t81_io` (IO .cpp), examples/tests behind toggles.
- **Bazel**: `//:t81` library + `cc_test`/`cc_binary` targets for all examples/tests.
- **Make**: local dev shim (`make`, `make run-tests`).
- Add CI jobs for all three to keep parity green.

## Canonical Test Vectors
- Reuse `tests/harness/canonical/*.json` for `bigint`, `fraction`, `tensor`.
- Expand vectors as we implement more ops; prefer JSON for language-agnostic reuse.

## Open Items
- BigInt: improve division/normalization performance (replace naive paths).
- Codecs: replace Base-243 and Base-81 stubs with canonical implementations; then:
  - Re-wire `T243BigInt` serialization to `codec/base243.hpp`.
  - Re-wire `CanonHash81` to real digest + Base-81.
- Tensor: broaden broadcasting semantics and add more op coverage (e.g., elementwise ops, reductions with axes>1).
- IR: import full opcode table; add validation helpers and richer flag semantics.
- Axion: backend integration; device enumeration; error codes/telemetry.

## What’s New Since Last Revision
- Added `broadcast.hpp`, `matmul.hpp`, `reduce.hpp`, and `tensor/shape.hpp` (+ tests).
- Introduced `ternary` primitives & arithmetic utilities/tests.
- Added `codec/base243.hpp` and tests.
- Added entropy utilities and tests.
- Introduced C API bridge for BigInt: `src/c_api/t81_c_api.{h,cpp}`.
- Expanded examples: `examples/{demo.cpp,tensor_ops.cpp,axion_demo.cpp}`.
- Updated build files (CMake, Bazel, Make) to include all new targets.
