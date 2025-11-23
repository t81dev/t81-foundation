# t81/ternary — Balanced Ternary Primitives

Core types and helpers for working in balanced ternary (−1, 0, +1).

## Files
- `../ternary.hpp` — defines `enum class Trit { Neg=-1, Zero=0, Pos=1 }` and `uint81_t`.
- `arith.hpp` — balanced-ternary helpers:
  - `encode_i64(int64_t) -> std::vector<Trit>` (LSB-first)
  - `decode_i64(const std::vector<Trit>&) -> int64_t`
  - `add(const std::vector<Trit>&, const std::vector<Trit>&) -> std::vector<Trit>`
  - `normalize(std::vector<Trit>&)`

## Notes
- Digits are **LSB-first** (index 0 is 3^0 place).
- `encode_i64`/`decode_i64` are safe for signed 64-bit integers.
- `add` is a pure-ternary vector adder using a balanced half-adder; it trims leading zero trits.
