---
layout: page
title: C++ Quickstart Guide
---

# T81 C++ Quickstart Guide

This page is the C++ portal: it describes how to bootstrap the deterministic ledger (build, tests, code, docs) so every artifact remains reproducible.

______________________________________________________________________

## 1. Prerequisites

- A C++23-capable compiler (temporary fallback lane: `-DT81_USE_CXX23=OFF`)
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

To validate the temporary compatibility lane explicitly:

```bash
cmake -S . -B build-cxx20 -DCMAKE_BUILD_TYPE=Release -DT81_USE_CXX23=OFF
cmake --build build-cxx20 --parallel
ctest --test-dir build-cxx20 --output-on-failure
```

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

- **To understand the code's structure:** Read the [`../../explanation/ARCHITECTURE.md`](../../explanation/ARCHITECTURE.md) to see how the different libraries (`t81_core`, `t81_lang_frontend`, etc.) fit together in the deterministic ledger.
- **To find a task to work on:** See the prioritized list of needed contributions in [`TASKS.md`](../../status/TASKS.md).
- **To understand the onboarding flow:** See [`../../records/archive/temporal-guides/tutorials/onboarding.md`](../../records/archive/temporal-guides/tutorials/onboarding.md).
- **To explore the API:** Generate the Doxygen documentation by running `cmake --build build --target docs` and opening `build/api/html/index.html`.

______________________________________________________________________

## 5. Notes & Caveats

- **Status evolves quickly:** prefer `../../reference/system-status.md` and `../explanation/ANALYSIS.md` for current implementation parity against spec.
- **Use the deterministic ritual:** run configure/build/test exactly as documented above before trusting local behavior.
- **Compatibility lane:** C++23 is default; run the optional C++20 lane (`-DT81_USE_CXX23=OFF`) when validating cross-toolchain compatibility.

For a detailed status of all components, see the [`System Status Report`](../../reference/system-status.md).
