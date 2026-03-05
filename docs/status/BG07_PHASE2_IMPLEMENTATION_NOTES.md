# BG-07 Phase 2 Implementation Notes

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

## Acceptance Signal

- No >64-bit integer literal truncation on IR→binary→VM path.
- Deterministic round-trip preserved.
- Existing test suite remains green.
