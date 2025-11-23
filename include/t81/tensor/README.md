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
