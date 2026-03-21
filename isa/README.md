# `core/isa`

Implementation of TISC IR serialization, parsing, and presentation utilities.

## Scope
- Binary emit/read/write for `.tisc`
- Encoding helpers
- Human-readable disassembly/pretty-print support

## Key Files
- `binary_emitter.cpp`: lowers IR to canonical binary form.
- `binary_io.cpp`: read/write helpers for binary artifacts.
- `encoding.cpp`: instruction and operand encoding helpers.
- `pretty_printer.cpp`: readable text form of IR/programs.

## Related Interfaces
- `include/t81/isa/ir.hpp`
- `include/t81/isa/program.hpp`
- `include/t81/isa/binary_emitter.hpp`

## Notes
- Output ordering and encoding must stay reproducible.
- Avoid introducing host-dependent metadata into emitted artifacts.
