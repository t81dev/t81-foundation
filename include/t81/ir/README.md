# t81/ir — Minimal IR Surface

Lightweight instruction representation used by tools/tests while the full
compiler/VM is still in `legacy/`.

## Files

- `opcodes.hpp`
  Minimal `enum class Opcode : uint16_t` with buckets for arithmetic, BigInt,
  tensor, control, memory, and capability ops.

- `insn.hpp`
  POD `struct Insn` with:

  - `Opcode op`
  - `std::array<uint32_t,3> ops` (generic operands)
  - `uint64_t imm` (immediate/addr)
  - `uint32_t flags`, `_reserved`

  Helpers: `make0/make1/make2/make3/make_imm`.

- `encoding.hpp`
  Fixed **32-byte** little-endian binary encoding for `Insn`.
  Functions: `encode`, `decode`, `encode_many`, `decode_many`.

## Stability

- Keep layout stable; extend via new opcodes and flags.
- Backwards compatibility: unknown opcodes should be treated as NOP by consumers.

## Example

```cpp
#include <t81/ir/opcodes.hpp>
#include <t81/ir/insn.hpp>
#include <t81/ir/encoding.hpp>

using namespace t81::ir;
Insn i = make3(Opcode::BigMul, 1, 2, 3);
uint8_t buf[32]; encode(i, buf);
Insn j = decode(buf); // roundtrip
```
