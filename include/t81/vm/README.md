# `include/t81/vm`

Public VM interfaces for T81VM.

## Key Headers
- `vm.hpp`: VM interface and constructors.
- `state.hpp`: machine state model.
- `traps.hpp`: trap/fault enums and helpers.

## Consumers
- Runtime implementation in `core/vm`
- CLI/run flows that embed VM execution
- Unit/integration tests validating opcode behavior

## Compatibility
- Treat these headers as stable API surfaces.
- Prefer additive changes; document breaking changes in `../../../docs/reference/CHANGELOG.md`.
