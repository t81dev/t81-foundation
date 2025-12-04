---
layout: page
title: C++ Quickstart Guide
---

# T81 C++ Quickstart Guide

This page provides a hands-on guide to building the C++ project, running tests, and writing your first piece of ternary-native code.

______________________________________________________________________

## 1. Prerequisites

- A C++20 compliant compiler (e.g., GCC 10+, Clang 12+)
- CMake 3.16+
- Ninja (recommended) or Make

______________________________________________________________________

## 2. Build and Test

The project uses a standard CMake workflow.

```bash
# 1. Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# 2. Configure the build (add -G Ninja if you prefer)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build libraries, examples, tests, and docs
cmake --build build --parallel

# 4. Run the CTest test suite
ctest --test-dir build --output-on-failure
```
A successful run will show all unit tests passing.

______________________________________________________________________

## 3. "Hello Ternary" Example

The core of the numeric system is the `T81Int` class. Here is a minimal example of how to use it.

```cpp
#include <t81/core/T81Int.hpp>
#include <iostream>

int main() {
    using t81::core::T81Int;

    // Create two 8-trit integers from decimal values
    T81Int<8> a{5};
    T81Int<8> b{-3};

    // Arithmetic works as expected
    auto sum = a + b; // 5 + (-3) = 2

    // Convert back to a standard C++ integer for printing
    std::cout << "Sum (decimal): " << sum.to_binary<int64_t>() << std::endl;

    // Print the native balanced ternary representation
    std::cout << "Sum (ternary): " << sum.str() << std::endl;

    return 0;
}
```

To compile and run this, you would link against the `t81_core` library.

______________________________________________________________________

## 4. Where to Go Next

- **To understand the code's structure:** Read the [`ARCHITECTURE.md`](../ARCHITECTURE.md) to see how the different libraries (`t81_core`, `t81_frontend`, etc.) fit together.
- **To find a task to work on:** See the prioritized list of needed contributions in [`TASKS.md`](../TASKS.md).
- **To understand the onboarding flow:** See [`docs/onboarding.md`](./onboarding.md).
- **To explore the API:** Generate the Doxygen documentation by running `cmake --build build --target docs` and opening `docs/api/html/index.html`.

______________________________________________________________________

## 5. Notes & Caveats

- **`T81BigInt` is Partial:** The `T81BigInt` class provides arbitrary-precision storage, but its arithmetic functions are still incomplete.
- **`Tensor` is Partial:** The `Tensor` class has a solid foundation but does not yet implement the full set of operations defined in the spec.
- **Axion & CanonFS are Stubs:** The Axion Kernel and CanonFS are non-functional placeholders.

For a detailed status of all components, see the [`System Status Report`](./system-status.md).
