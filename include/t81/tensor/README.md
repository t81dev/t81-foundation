# t81/tensor — T729Tensor and Ops

Lightweight tensor type and a small set of dependency-free ops intended for
CPU-side manipulation and testing.

## Types

- **`T729Tensor`**
  - Storage: `std::vector<float>` (row-major)
  - Shape: `std::vector<int>`
  - Rank: `int`
  - API:
    - `rank()`, `shape()`, `data()`, `size()`
    - `contract_dot(vec, vec)` → `{1}`
    - `transpose(2D)` → `{cols, rows}`
    - `broadcast(new_shape)` (naïve repeat semantics)

## Ops (header-only)

Aggregate:

```cpp
#include <t81/tensor/ops.hpp>
```

Or pick à la carte:

- `transpose.hpp` → `t81::ops::transpose(m)`
- `slice.hpp` → `t81::ops::slice2d(m, r0, r1, c0, c1)`
- `reshape.hpp` → `t81::ops::reshape(m, new_shape /* -1 allowed once */)`

## Examples

```cpp
#include <t81/tensor.hpp>
#include <t81/tensor/ops.hpp>

t81::T729Tensor m({2,3});
m.data() = {1,2,3, 4,5,6};

auto mt = t81::ops::transpose(m);      // 3x2
auto s  = t81::ops::slice2d(m,1,2,0,2); // 1x2 -> {4,5}
auto r  = t81::ops::reshape(m,{3,2});   // 3x2 (view-copy)
```

## Notes

- This module intentionally avoids external deps (BLAS, Eigen, etc.).
- For GPU/accelerated paths, keep the same interface and provide a separate
  backend library; do not change the headers’ public API.

```
```
