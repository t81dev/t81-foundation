# T81 C++ Migration Roadmap

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 C++ Migration Roadmap](#t81-c++-migration-roadmap)
  - [Phase 0 — Compatibility (**DONE**)](#phase-0-—-compatibility-**done**)
  - [Phase 1 — Core Data Types (**DONE**)](#phase-1-—-core-data-types-**done**)
  - [Phase 2 — IO & Codec Surfaces (**DONE**)](#phase-2-—-io-&-codec-surfaces-**done**)
  - [Phase 3 — IR Surface (**DONE, EXPANDABLE**)](#phase-3-—-ir-surface-**done-expandable**)
  - [Phase 4 — Axion Façade (**IN PROGRESS**)](#phase-4-—-axion-façade-**in-progress**)
  - [Phase 5 — De-risked Deprecation (**ONGOING**)](#phase-5-—-de-risked-deprecation-**ongoing**)
  - [Phase 6 — Build & CI Matrix (**ONGOING**)](#phase-6-—-build-&-ci-matrix-**ongoing**)
  - [Canonical Test Vectors](#canonical-test-vectors)
  - [Open Items (Migration-Focused)](#open-items-migration-focused)
    - [BigInt / Numerics](#bigint--numerics)
    - [Codecs & Hash](#codecs-&-hash)
    - [Tensor](#tensor)
    - [IR](#ir)
    - [Axion](#axion)
  - [“How to Resume Work” Checklist](#“how-to-resume-work”-checklist)
  - [What’s New Since Last Revision](#what’s-new-since-last-revision)

<!-- T81-TOC:END -->













This document tracks migration of legacy `.cweb` code to the new header-first C++ API.

For long-horizon tracking of the entire ecosystem (VM, language, Axion, CanonFS, CI), see `TODO.md`.\
This file is for the **C++ migration path** specifically.

______________________________________________________________________

## Phase 0 — Compatibility (**DONE**)

- Keep C ABI stable via `src/c_api/` wrappers over modern C++ headers.
- New code consumes `<t81/t81.hpp>`; legacy code may continue to include `include/t81/t81.h`.
- CMake, Bazel, and Make shims build the same headers.

**Status:**\
Shims exist and build; new C++ headers are the preferred entrypoint, C API remains available for legacy and interop callers.

______________________________________________________________________

## Phase 1 — Core Data Types (**DONE**)

- ✅ `T243BigInt` → `include/t81/bigint.hpp` (signed; add/sub/mul/mod/gcd/exact-div)
- ✅ `T81Fraction` → `include/t81/fraction.hpp` (reduced, `den > 0`, canonical zero)
- ✅ `T729Tensor` → `include/t81/tensor.hpp`
- ✅ Tensor ops → `include/t81/tensor/{transpose,slice,reshape,matmul,reduce,broadcast}.hpp`
- ✅ Shape helpers → `include/t81/tensor/shape.hpp`
- ✅ Entropy utilities → `include/t81/entropy.hpp`
- ✅ CanonFS surface + wire IO → `include/t81/{canonfs,canonfs_io}.hpp`

**Tests** (`tests/cpp`):

- BigInt: `bigint_roundtrip.cpp`
- Fraction: `fraction_roundtrip.cpp`
- Tensor: `tensor_{transpose,slice,reshape,matmul,reduce,broadcast}_test.cpp`, `tensor_shape_test.cpp`
- Entropy: `entropy_test.cpp`
- CanonFS/Hash: `canonfs_io_test.cpp`, `hash_stub_test.cpp`

**Next moves tied to Phase 1:**

- Tighten BigInt division/GCD behavior and performance.
- Expand fraction and ternary tests using canonical JSON vectors.

______________________________________________________________________

## Phase 2 — IO & Codec Surfaces (**DONE**)

- ✅ Tensor text IO → `include/t81/io/tensor_loader.hpp` + `src/io/tensor_loader.cpp` + `tensor_loader_test.cpp`
- ✅ Base-81 & CanonHash stubs → `include/t81/hash/{base81,canonhash}.hpp`
- ✅ Base-243 codec surface (stub) → `include/t81/codec/base243.hpp` + `codec_base243_test.cpp`

> Note: Base-81 and Base-243 are deterministic **stubs**. Swap with canonical codecs without changing call sites.

**Next moves tied to Phase 2:**

- Implement real Base-81/Base-243 codecs and CanonHash81, then flip tests from “stub” to “canonical”.

______________________________________________________________________

## Phase 3 — IR Surface (**DONE, EXPANDABLE**)

- ✅ Minimal IR: `include/t81/ir/{opcodes,insn,encoding}.hpp` with 32-byte LE encoding.
- ✅ Round-trip tests covered by `ir_encoding_test.cpp` (C++ side).

**Next: opcode import and enrichment**

1. Define a full `Opcode` + metadata table in `opcodes.hpp`:
   - `Opcode`, `OperandKind`, `OpcodeDesc` (or equivalent).
   - Flags for branching, privileged, memory, etc.
1. Import opcodes from legacy sources:
   - `legacy/hanoivm/src/t81lang_compiler/T81InstrFormats.td`
   - `legacy/hanoivm/src/t81lang_compiler/T81InstrInfo.td`
   - `spec/tisc-spec.md` (spec tables).
1. Extend encoding/decoding in `encoding.hpp` to respect those formats.
1. Grow `ir_encoding_test.cpp` to:
   - Roundtrip all opcodes.
   - Assert spec ↔ C++ table alignment (CI guard).

______________________________________________________________________

## Phase 4 — Axion Façade (**IN PROGRESS**)

- ✅ Stub context/API → `include/t81/axion/api.hpp`
- ✅ Example & test → `examples/axion_demo.cpp`, `tests/cpp/axion_stub_test.cpp`
- 🔜 Wire real backend bindings behind the same façade (keep API stable).

**Target for Phase 4 completion:**

- Minimal but real policy engine:
  - Capability checks for privileged instructions / syscalls.
  - Deterministic decision API that can be called from the VM.
- Expanded tests covering:
  - Allowed/denied operations.
  - Telemetry/error-code behavior.

______________________________________________________________________

## Phase 5 — De-risked Deprecation (**ONGOING**)

- Leave thin shims in `legacy/` that forward or emit migration notes.
- Provide Bazel/CMake wrapper targets pointing to new headers/impls.
- Track per-module parity and remove `.cweb` once tests pass.
- Mark `legacy/hanoivm/` as an immutable reference snapshot, not an active target.

**Parity checklist (per subsystem):**

- [ ] BigInt / numerics
- [ ] Tensor / T729 behavior
- [ ] TISC opcode behavior
- [ ] VM execution semantics
- [ ] T81Lang front-end semantics
- [ ] Axion integration hooks

______________________________________________________________________

## Phase 6 — Build & CI Matrix (**ONGOING**)

- **CMake**
  - Library targets: `t81` (headers), `t81_io` (IO .cpp), plus examples/tests behind options.
- **Bazel**
  - `//:t81` library + `cc_binary`/`cc_test` targets for all examples/tests.
- **Make**
  - Local dev shim (`make`, `make run-tests`).

**Planned CI:**

- Matrix over {CMake, Bazel, Make} × {Linux, macOS (if desired)}.
- Jobs:
  - Configure + build.
  - Run `ctest` and `tests/harness/run_all.sh`.
- Goal: green parity for every push/PR to main.

______________________________________________________________________

## Canonical Test Vectors

- Reuse `tests/harness/canonical/*.json` for `bigint`, `fraction`, `tensor`.
- Expand vectors as new ops are implemented:
  - Prefer JSON for language-agnostic reuse.
  - Keep a 1:1 mapping between spec examples and canonical vectors where possible.

______________________________________________________________________

## Open Items (Migration-Focused)

These are the “live” edges of the migration. Long-horizon items live in `TODO.md`.

### BigInt / Numerics

- Improve division/normalization performance (replace naive paths).
- Clarify and document signed division semantics (negative inputs, zero edge cases).
- Add fuzz tests and large-number stress tests.

### Codecs & Hash

- Replace Base-81 and Base-243 stubs with canonical implementations.
- Re-wire:
  - `T243BigInt` serialization to `codec/base243.hpp`.
  - `CanonHash81` to a real digest + Base-81 encoding.
- Update tests from stub expectations to canonical outputs.

### Tensor

- Broaden broadcasting semantics and ensure they match spec.
- Add more op coverage:
  - Elementwise unary/binary ops (+ tests).
  - Reductions over multiple axes.
  - Stats (variance, stddev, argmax/argmin).
- Confirm memory layout and document it in both code and spec.

### IR

- Import full opcode table (Phase 3 “Next”).
- Add validation helpers:
  - “Is this encoding legal for this opcode?”
  - “Does this instruction require Axion/privileged context?”
- Enrich tests with cross-checks against legacy opcode tables and `spec/tisc-spec.md`.

### Axion

- Integrate real backend logic behind `axion/api.hpp`.
- Add capability descriptors and device enumeration hooks.
- Define stable error codes, telemetry paths, and basic observability.

______________________________________________________________________

## “How to Resume Work” Checklist

When picking this up after a gap:

1. **Build & tests**

   - `cmake -S . -B build && cmake --build build -j`
   - `ctest --test-dir build -R "t81_"`
   - Or the equivalent Bazel/Make targets.

1. **Choose one active edge from Open Items:**

   - IR opcode table import (Phase 3).
   - BigInt division/GCD polish.
   - Codecs/hash canonicalization.
   - Tensor unary/reduce extensions.
   - Axion façade backend.

1. **Update both:**

   - This roadmap (phase status, “What’s New”).
   - `TODO.md` (for cross-cutting, long-term tasks).

______________________________________________________________________

## What’s New Since Last Revision

- Added `broadcast.hpp`, `matmul.hpp`, `reduce.hpp`, and `tensor/shape.hpp` (+ tests).
- Introduced `ternary` primitives & arithmetic utilities/tests.
- Added `codec/base243.hpp` and `codec_base243_test.cpp`.
- Added entropy utilities and tests.
- Introduced C API bridge for BigInt: `src/c_api/t81_c_api.{h,cpp}`.
- Expanded examples: `examples/{demo.cpp,tensor_ops.cpp,axion_demo.cpp}`.
- Updated build files (CMake, Bazel, Make) to include all new targets.
- Created `TODO.md` as the long-horizon tracker complementing this migration roadmap.
