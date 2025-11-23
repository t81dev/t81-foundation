Here’s **`include/t81/README.md`**:

````md
# t81 C++ Headers

Modular, header-only C++ API for T81 data types and utilities.

## Modules

- `config.hpp` — feature toggles, version macros, portability attrs.
- `ternary.hpp` — `Trit` (−1/0/+1), `uint81_t` carrier.
- `bigint.hpp` — signed base-243 big integer (`T243BigInt`) with `add/sub/mul/mod/gcd`.
- `fraction.hpp` — signed rationals (`T81Fraction`), always reduced; denom > 0.
- `tensor.hpp` — row-major tensor (`T729Tensor`) with basic ops.
- `tensor/ops.hpp` — aggregator for extra tensor ops:
  - `tensor/transpose.hpp` — `ops::transpose(m)`
  - `tensor/slice.hpp` — `ops::slice2d(m, r0, r1, c0, c1)`
  - `tensor/reshape.hpp` — `ops::reshape(m, new_shape)` (one `-1` inference)
- `canonfs.hpp` — `CanonHash81`, `CanonRef`, hashing seam.
- `canonfs_io.hpp` — fixed 99-byte wire encode/decode helpers.
- `ir/opcodes.hpp` — minimal opcode enum.
- `ir/insn.hpp` — POD instruction format.
- `ir/encoding.hpp` — 32-byte binary encoding for `Insn`.

## Usage

```cpp
#include <t81/t81.hpp>
#include <t81/tensor/ops.hpp>  // if you need extra tensor ops

using namespace t81;
T243BigInt a = T243BigInt::from_ascii("1.42.7"); // base-243 digits (MSB-first, '.'-separated)
T243BigInt b = T243BigInt::from_base81_string("1.80.5");
auto s = T243BigInt::add(a,b);

T729Tensor m({2,3}); m.data() = {1,2,3,4,5,6};
auto mt = t81::ops::transpose(m);
````

## Notes

* `from_ascii(...)` expects canonical base-243 digits (`[+-]?d(.d)*` where `0<=d<243`), MSB-first. `from_base81_string(...)` handles base-81 digit strings (`0..80`).
* IO helpers are minimal and non-cryptographic; validate per CanonFS spec in production.

```
```
