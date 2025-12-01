---
layout: page
title: C++ Tensor Guide
---

# T81 C++ Tensor Guide

This guide summarizes the lightweight `T729Tensor` implementation plus the supporting APIs shipped in `include/t81/tensor`. It is the practical reference for developers working with the current C++ tensor utilities.

**Companion Documents:**
- **Specification:** [`spec/t81-data-types.md`](../spec/t81-data-types.md)
- **API Reference:** [`include/t81/tensor.hpp`](../include/t81/tensor.hpp) and the headers under `include/t81/tensor/`
- **Tests:** `tests/cpp/tensor_*_test.cpp`
- **Examples:** `examples/tensor_ops.cpp`, `examples/demo.cpp`

______________________________________________________________________

## 1. Core Concepts

- **`t81::T729Tensor`:** A row-major tensor of `float` with a runtime `shape()` (`std::vector<int>`) and owned `data()` buffer. It exposes `rank()`, `size()`, broadcasting helpers, and basic serialization (`serialize`/`deserialize`).
- **Shape Utilities:** `include/t81/tensor/shape.hpp` provides row-major strides, size computation, `broadcast_shape`, `squeeze`, and reshape validation helpers that the ops headers reuse.
- **Ops Library:** The headers under `include/t81/tensor/` expose free functions in `t81::ops` such as `matmul`, `transpose`, `slice2d`, `reshape`, `broadcast_to`, elementwise/`unary` helpers, and reduction primitives. These have dedicated regression tests under `tests/cpp/`.
- **Status:** This is a partial implementation aligning with the current C++ migration roadmap—sufficient for basic numerics while more spec features (e.g., native ternary storage) are on the TODO list.

______________________________________________________________________

## 2. Creating a Tensor

Create a tensor by passing a shape (as initializer list or vector) and optionally providing the backing `float` data. The constructors validate positive dimensions, match the total size, and throw `std::invalid_argument` on mismatch.

```cpp
#include "t81/tensor.hpp"
#include <iostream>

int main() {
    t81::T729Tensor a({2, 3});       // zero-initialized data
    a.data() = {1.0f, 2.0f, 3.0f,
                4.0f, 5.0f, 6.0f};

    std::cout << "Rank: " << a.rank() << '\n';
    std::cout << "Shape: ";
    for (int dim : a.shape()) std::cout << dim << ' ';
    std::cout << '\n';
    std::cout << "Data: ";
    for (float v : a.data()) std::cout << v << ' ';
    std::cout << '\n';

    return 0;
}
```

The same header also exposes utility methods such as `T729Tensor::broadcast` (naive right-aligned repeat), `transpose2d`, and `contract_dot` (dot product between rank-1 tensors) that make small helpers possible without the full ops suite.

______________________________________________________________________

## 3. Core Operations

All tensor operations live in `t81::ops` and expect `T729Tensor` arguments. These utilities work well with the `examples/tensor_ops.cpp` demo and the matching regression tests.

### Matrix Multiplication (`matmul`)

`t81::ops::matmul` implements a standard 2D matrix product (`(m×k)·(k×n) = m×n`). It validates ranks and dimensions before constructing the result.

```cpp
t81::T729Tensor A({2, 3}, std::vector<float>{1, 2, 3, 4, 5, 6});
t81::T729Tensor B({3, 2}, std::vector<float>{7, 8, 9, 10, 11, 12});
auto C = t81::ops::matmul(A, B); // 2×2
```

**Test:** [`tests/cpp/tensor_matmul_test.cpp`](../tests/cpp/tensor_matmul_test.cpp)

### Transpose & Reshape

- `t81::ops::transpose` handles rank-2 tensors and flips `{rows, cols}` → `{cols, rows}`.
- `t81::ops::reshape` validates the new shape (with a single optional `-1` inference) and reuses the original data buffer, returning a new `T729Tensor` with the requested layout.

```cpp
auto transposed = t81::ops::transpose(A);
auto reshaped = t81::ops::reshape(A, {3, 2});
```

**Tests:** [`tests/cpp/tensor_transpose_test.cpp`](../tests/cpp/tensor_transpose_test.cpp), [`tests/cpp/tensor_reshape_test.cpp`](../tests/cpp/tensor_reshape_test.cpp)

### Slicing & Indexing

`t81::ops::slice2d` extracts a half-open row/column range from a rank-2 tensor. It checks that the requested row and column bounds are in range before copying the requested rectangle.

```cpp
auto patch = t81::ops::slice2d(A, 0, 1, 1, 3); // 1×2 block
```

**Test:** [`tests/cpp/tensor_slice_test.cpp`](../tests/cpp/tensor_slice_test.cpp)

### Reduction

`reduce_sum_2d` and `reduce_max_2d` collapse a rank-2 tensor along axis `0` (columns) or `1` (rows), producing a 1D tensor that summarizes sums or maxima.

```cpp
auto row_sums = t81::ops::reduce_sum_2d(A, 1);
auto col_max = t81::ops::reduce_max_2d(A, 0);
```

**Test:** [`tests/cpp/tensor_reduce_test.cpp`](../tests/cpp/tensor_reduce_test.cpp)

### Broadcasting & Elementwise Ops

`broadcast_to` uses the helpers in `t81::shape` to confirm right-aligned compatibility before materializing a larger tensor. `elemwise_binary` exposes add/sub/mul/div flavors that automatically broadcast mismatched shapes (div throws on divide-by-zero).

```cpp
t81::T729Tensor broadcasted = t81::ops::broadcast_to(A, {2, 3});
auto summed = t81::ops::add(A, broadcasted);
```

**Tests:** [`tests/cpp/tensor_broadcast_test.cpp`](../tests/cpp/tensor_broadcast_test.cpp), [`tests/cpp/tensor_elementwise_test.cpp`](../tests/cpp/tensor_elementwise_test.cpp)

### Unary Functions

`t81::ops::unary_map` supports applying any `float(float)` functor to every element. The convenience helpers `relu`, `tanh`, `exp`, and `log` demonstrate common activation-style transforms with safety guards (e.g., `log` throws on negative inputs).

**Test:** [`tests/cpp/tensor_unary_test.cpp`](../tests/cpp/tensor_unary_test.cpp)

### IO & Serialization

Use `t81::io::load_tensor_txt` / `save_tensor_txt` (and their `_file` helpers) to stream tensors in a simple text format. `T729Tensor` also exposes `serialize`/`deserialize` for binary persistence, keeping the header/int64 counts for shape and data.

**Test:** [`tests/cpp/tensor_loader_test.cpp`](../tests/cpp/tensor_loader_test.cpp)

______________________________________________________________________

## 4. Status & Next Steps

The current tensor API is **partial** but usable: base storage, reshape/slice/matmul/reduce, broadcasting, elementwise, unary transforms, and IO are all implemented and covered by tests.

- **Next Steps:** Expand the API to native ternary types (`T81Int`), add spec-aligned broadcasting rules, and upstream more operations (elementwise reductions, scatter/gather) once `T81Lang` reaches greater maturity.

For a full list of planned work, see [`TASKS.md`](../TASKS.md).
