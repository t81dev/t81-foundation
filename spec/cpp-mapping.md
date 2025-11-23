# T81 C++ Mapping (Reference)

This document maps legacy T81 data types and ops to the new C++ API.

## Scalars & Primitives

| Legacy | C++ |
|---|---|
| `trit` | `t81::Trit` (`Neg, Zero, Pos`) |
| `uint81` | `t81::uint81_t` (128-bit carrier; platform hashing seam) |

## Big Integers (Base-243)

- **Type**: `t81::T243BigInt`
- **Sign**: `enum class Sign { Neg, Zero, Pos }`
- **Digits**: LSB-first, each in `[0..242]`.
- **Key APIs**:
  - Construction: `from_ascii(std::string_view)` — canonical base-243 digits, MSB-first, `.` separated, optional sign
  - Also: `from_base81_string(std::string_view)` / `to_base81_string()` for canonical base-81 digit strings
  - Arithmetic: `add`, `sub`, `mul`, `mod`, `gcd`
  - Compare: `cmp_abs(a,b)`
  - Format: `to_string()` *(MSB-first base-243 digits with '.', optional leading `-`)*

## Fractions

- **Type**: `t81::T81Fraction`
- **Invariants**:
  - Denominator always positive
  - Reduced by `gcd(|num|,|den|)`
  - Zero canonicalized as `0/1`
- **Ops**: `add`, `sub`, `mul`, `div`
- **Format**: `NUM/DEN` via `to_string()`

## Tensors

- **Type**: `t81::T729Tensor`
- **Layout**: Row-major
- **Shape**: `std::vector<int>`; `rank()` and `size()`
- **Core ops**:
  - `contract_dot(vec, vec) -> {1}`
  - `transpose(2D)`
  - `broadcast(new_shape)`
- **Extended ops** (`t81/tensor/ops.hpp`):
  - `ops::transpose(m)`
  - `ops::slice2d(m, r0, r1, c0, c1)`
  - `ops::reshape(m, new_shape /* -1 allowed once */)`

## CanonFS

- **Types**: `t81::CanonHash81`, `t81::CanonRef`
- **Hash seam**: `canonhash81_of_bytes(const void*, size_t)` *(platform-specific)*
- **Wire IO**: `t81::canonfs_io::{encode_ref, decode_ref, permissions_allow}`

## IO Utilities

- **Tensor text IO** (`t81/io/tensor_loader.hpp`):

```

RANK D1 ... DR
v0 v1 v2 ...

```

Functions: `load_tensor_txt(_file)`, `save_tensor_txt(_file)`

## Umbrella Header

```cpp
#include <t81/t81.hpp>          // pulls in bigint, fraction, tensor, canonfs, entropy
#include <t81/tensor/ops.hpp>   // extra tensor ops (transpose, slice2d, reshape)
```

## C API Bridge (Stable ABI)

```c
#include "src/c_api/t81_c_api.h"

t81_bigint h = t81_bigint_from_ascii("123");
char* s = t81_bigint_to_string(h);
t81_bigint_free(h);
free(s);
```

## Testing

- Canonical vectors: `tests/harness/canonical/*.json`

- C++ tests under `tests/cpp/`:

  - `bigint_roundtrip.cpp`, `fraction_roundtrip.cpp`
  - `tensor_{transpose,slice,reshape,loader}_test.cpp`
  - `canonfs_io_test.cpp`
