# T81 C++ Quickstart

This guide provides a concise, hands-on guide to building the C++ project, running tests, and generating documentation.

**For a comprehensive overview of the project, please start with the [T81 Foundation Documentation Hub](./index.md).**

______________________________________________________________________

### 1. Build & Test with CMake

The project uses CMake as its sole build system.

```bash
# Configure the project (run once)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build all libraries, examples, and tests
cmake --build build --parallel

# Run the CTest test suite
ctest --test-dir build --output-on-failure
```

______________________________________________________________________

### 2. Run Examples

The examples are built by default and can be found in the `build/` directory.

```bash
./build/t81_demo
./build/t81_tensor_ops
./build/t81_ir_roundtrip
./build/axion_demo
```

______________________________________________________________________

### "Hello Ternary" Example

Here is a simple example of how to use the `T81Int` class:

```cpp
#include <t81/core/T81Int.hpp>
#include <iostream>

int main() {
    using namespace t81::core;
    T81Int<8> a{5};  // Decimal 5 in ternary
    T81Int<8> b{-3}; // Decimal -3
    auto sum = a + b; // Should be 2
    std::cout << "Sum (decimal): " << sum.to_binary<int64_t>() << std::endl;
    std::cout << "Sum (ternary): " << sum.str() << std::endl; // e.g., "+0+--"
    return 0;
}
```

______________________________________________________________________

### 3. Generate API Documentation

The project is fully documented with Doxygen. To generate a browsable HTML API reference, run the `docs` target.

```bash
cmake --build build --target docs
```

The output will be generated in `docs/api/html/`. Open `index.html` in your browser to view the documentation.

______________________________________________________________________

### 4. Key Headers

While the umbrella header (`t81/t81.hpp`) is convenient, these are the most common headers for direct inclusion:

- `t81/core/T81Int.hpp` — `T81Int` class
- `t81/core/bigint.hpp` — `BigInt` class
- `t81/core/fraction.hpp` — `Fraction` class
- `t81/core/tensor.hpp` — `Tensor` struct
- `t81/frontend/lexer.hpp`, `parser.hpp`, `ir_generator.hpp` — T81Lang compiler components
- `t81/vm/vm.hpp` — The T81 Virtual Machine
- `t81/axion/api.hpp` — The Axion Kernel API (stub)

______________________________________________________________________

### 5. Notes & Caveats

- **Base-243/81 Codecs & Hashes:** The implementations for codecs and hashing are currently deterministic stubs suitable for testing, not for production use.
- **Core Types:** The `BigInt` and `Tensor` types are minimal implementations and do not yet match the full semantics of the T81 specification. See the [System Status Report](./system-status.md) for details.
