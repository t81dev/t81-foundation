---
layout: page
title: C++ Tensor Guide
---

# T81 C++ Tensor Guide

This guide provides a practical introduction to the T81 C++ `Tensor` library, a core component of the `t81_core` library.

**Companion Documents:**
- **Specification:** [`spec/t81-data-types.md`](../spec/t81-data-types.md)
- **API Reference:** `t81/core/tensor.hpp` (generate with Doxygen)
- **Tests:** `tests/cpp/t81_tensor_*_test.cpp`

______________________________________________________________________

## 1. Core Concepts

The T81 `Tensor` is a multi-dimensional array designed for high-performance numerical computing with ternary data.

- **Storage:** Tensors are stored in a contiguous, row-major flat array in memory.
- **Shape:** The dimensions of the tensor are tracked in a `TensorShape` object, which is a simple vector of integers.
- **Data Type:** Tensors are currently templated and typically hold `int64_t` for demonstration purposes, but are designed to eventually hold native ternary types like `T81Int`.
- **API:** The tensor API is provided as a set of free functions (e.g., `matmul`, `transpose`) that operate on `Tensor` objects, rather than class methods.

______________________________________________________________________

## 2. Creating a Tensor

You can create a tensor by specifying its shape and providing a C++ initializer list with the data.

```cpp
#include <t81/core/tensor.hpp>
#include <iostream>

int main() {
    using t81::core::Tensor;

    // Create a 2x3 tensor (2 rows, 3 columns)
    Tensor<int64_t> a({2, 3}, {
        1, 2, 3,
        4, 5, 6
    });

    std::cout << "Shape: " << a.shape.to_string() << std::endl;
    std::cout << "Data: ";
    for (const auto& val : a.data) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

**Key Files:**
- **Header:** [`include/t81/core/tensor.hpp`](../include/t81/core/tensor.hpp)
- **Test:** [`tests/cpp/tensor_shape_test.cpp`](../tests/cpp/tensor_shape_test.cpp)

______________________________________________________________________

## 3. Core Operations

The tensor library supports a range of common operations, demonstrated in `examples/tensor_ops.cpp`.

### Matrix Multiplication (`matmul`)

Performs standard matrix multiplication on two 2D tensors.

```cpp
Tensor<int64_t> a({2, 3}, {1, 2, 3, 4, 5, 6});
Tensor<int64_t> b({3, 2}, {7, 8, 9, 10, 11, 12});

// c = a * b
auto c = matmul(a, b); // Result is a 2x2 tensor
```
**Test:** [`tests/cpp/tensor_matmul_test.cpp`](../tests/cpp/tensor_matmul_test.cpp)

### Transpose

Swaps the axes of a tensor. For a 2D tensor, this flips rows and columns.

```cpp
Tensor<int64_t> a({2, 3}, {1, 2, 3, 4, 5, 6});

auto at = transpose(a); // Result is a 3x2 tensor
```
**Test:** [`tests/cpp/tensor_transpose_test.cpp`](../tests/cpp/tensor_transpose_test.cpp)

### Reduction (`reduce_sum`)

Reduces a tensor's dimension by summing its elements along a given axis.

```cpp
Tensor<int64_t> a({2, 3}, {1, 2, 3, 4, 5, 6});

// Sum along columns (axis 1)
auto row_sums = reduce_sum(a, 1); // Result is a {2} tensor: {6, 15}
```
**Test:** [`tests/cpp/tensor_reduce_test.cpp`](../tests/cpp/tensor_reduce_test.cpp)

### Broadcasting

Extends a tensor to a larger shape by repeating its data along new or expanded dimensions.

```cpp
Tensor<int64_t> a({2, 3}, {1, 2, 3, 4, 5, 6});
Tensor<int64_t> b({3}, {10, 20, 30}); // A row vector to broadcast

// Add b to each row of a
auto c = broadcast_add(a, b);
```
**Test:** [`tests/cpp/tensor_broadcast_test.cpp`](../tests/cpp/tensor_broadcast_test.cpp)

______________________________________________________________________

## 4. Current Status & Next Steps

The tensor library is currently **`Partial`**. The core infrastructure is in place, but many features from the specification are not yet implemented.

- **Next Steps:**
    - Implement the full suite of elementwise operations.
    - Integrate native ternary types (`T81Int`) as the tensor's data type.
    - Expand broadcasting rules to cover all cases in the specification.

For a full list of planned work, see [`TASKS.md`](../TASKS.md).
