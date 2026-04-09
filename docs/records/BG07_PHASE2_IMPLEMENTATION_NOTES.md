# BG-07 Phase 2 Implementation Notes

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [BG-07 Phase 2 Implementation Notes](#bg-07-phase-2-implementation-notes)
  - [Scope](#scope)
  - [Current Constraint](#current-constraint)
  - [Phase 2 Work Items](#phase-2-work-items)
  - [Progress (2026-03-05)](#progress-2026-03-05)
  - [Acceptance Signal](#acceptance-signal)

<!-- T81-TOC:END -->


Last Updated: 2026-03-05
Owner: @t81dev

## Scope

Implement true arbitrary-precision integer literal transport from frontend IR to VM runtime without 64-bit truncation.

## Current Constraint

- `include/t81/frontend/ir_generator.hpp` currently parses `Base81Integer` literals through a 64-bit immediate path (`std::stoll`), with overflow diagnostics only.
- `core/isa/binary_emitter.cpp` only materializes `text_literal` for `FloatHandle` and `SymbolHandle`.
- `include/t81/isa/program.hpp` has no dedicated BigInt literal pool.

## Phase 2 Work Items

1. Add a BigInt literal transport representation in ISA/program layer.
2. Extend IR generation to emit non-64-bit integer literals through that representation.
3. Extend binary emitter/IO to serialize and deserialize BigInt literal payloads deterministically.
4. Update VM `LOADI` handling to materialize BigInt-backed values without lossy conversion.
5. Add targeted tests:
   - frontend literal lowering for >64-bit integer constants
   - binary round-trip for BigInt literal payloads
   - VM execution conformance for >64-bit literal paths

## Progress (2026-03-05)

- ✅ Item 1 landed: `LiteralKind::BigIntHandle` + `Program::bigint_pool`.
- ✅ Item 3 landed: binary emitter and binary I/O wiring for BigInt literal pool transport.
- ✅ Item 4 landed (incremental): VM `LOADI` materializes `BigIntHandle` as canonical `FractionHandle` (`BigInt/1`) to avoid lossy 64-bit conversion.
- ✅ Item 2 landed for oversized decimal/base81 integers: frontend IR lowering now emits `BigIntHandle` on 64-bit overflow instead of hard failing.
- ✅ Partial item 5 landed: VM conformance coverage added in `tests/cpp/test_vm_literal_pool_extension.cpp`.
- ✅ Partial item 5 landed: frontend lowering coverage added in `tests/cpp/frontend_ir_generator_test.cpp`.
- ✅ Item 5 landed (binary conformance slice): explicit BigInt literal pool round-trip determinism coverage added in `tests/cpp/tisc_binary_io_determinism_test.cpp` and `tests/cpp/tisc_binary_metadata_roundtrip_property_test.cpp`.
- ✅ Runtime hardening slice landed: `Frac2I` now fails closed on non-`int64` BigInt numerators (deterministic `DecodeFault`) instead of relying on uncaught overflow behavior (`core/vm/vm.cpp`, `tests/cpp/test_vm_literal_pool_extension.cpp`).
- ✅ JIT safety slice landed: JIT `LoadImm` now explicitly deopts on `LiteralKind::BigIntHandle`, preserving interpreter arbitrary-precision materialization path and avoiding lossy JIT literal handling (`runtime/jit/jit_compiler.cpp`).

## Acceptance Signal

- No >64-bit integer literal truncation on IR→binary→VM path.
- Deterministic round-trip preserved.
- Existing test suite remains green.
