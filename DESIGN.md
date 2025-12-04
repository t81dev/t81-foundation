# T81 Foundation: Design Principles

This document captures the design principles, architectural invariants, and implementation strategies that guide development on the T81 Foundation project.

______________________________________________________________________

## 1. Core Philosophy

- **Specification First:** The specifications in `/spec` are the source of truth. The implementation must conform to them. Any deviation in the code is considered a bug.
- **Determinism by Default:** All components, especially the VM and T81Lang, must be deterministic. Any introduction of non-determinism (e.g., from I/O or entropy sources) must be explicit and managed.
- **Ternary Native:** While the system runs on binary hardware, the *logical model* is balanced ternary. The implementation must preserve ternary semantics for arithmetic, rounding, and overflow, even when optimized.
- **Safety and Introspection:** The Axion Kernel is a core part of the architecture, not an afterthought. All subsystems are designed to be introspectable, allowing higher-level components to monitor their invariants and behavior.

______________________________________________________________________

## 2. Architectural Principles

- **Decoupled Components:** The system is built from libraries with distinct responsibilities (e.g., `t81_core`, `t81_frontend`, `t81_vm`). This separation is enforced by the CMake build system and allows for independent development and testing.
- **Stable TISC Interface:** The Ternary Instruction Set Computer (TISC) serves as a stable, well-defined interface between the T81Lang compiler frontend and the T81VM execution backend. This allows different frontends or backends to be developed independently.
- **Immutable Historical Reference:** The original CWEB implementation in `/legacy/hanoivm` is preserved as an immutable reference. It is a source of historical context and semantic clarification, but it is not a target for new development.

______________________________________________________________________

## 3. C++ Implementation Strategy

### Data Types
- **`T81Int<N>`:** A header-only, fixed-size, templated class for balanced ternary integers. Its internal storage is a `std::array` of packed trytes.
- **`T81BigInt`:** An arbitrary-precision integer class that uses a vector of trytes for its internal storage. While the core storage is implemented for arbitrary size, some of the arithmetic operations are still under development.
- **`Fraction`:** A rational number type that is always maintained in canonical form (reduced, with a positive denominator).
- **`T81Float`:** A production-quality, IEEE-754-inspired balanced ternary floating-point type.
- **Additional arithmetic primitives:** `T81Fixed`, `T81UInt`, and `T81Prob` extend the foundation with ternary fixed-point, unsigned integer, and probability-specialized numerics respectively; `T81Float` also has specializations for different mantissa/exponent budgets and interoperability with `T81Int` through constexpr bridges.
- **Algebraic and geometric structures:** `T81Complex`, `T81Quaternion`, and `T81Polynomial` capture higher-order algebra, while `T81Vector`, `T81Matrix`, and `T81Tensor` expose fixed- and variable-dimensional linear spaces that compose cleanly with the numeric types.
- **Containers and helpers:** `T81List`, `T81Map`, `T81Set`, `T81Tree`, `T81Graph`, along with `T81String`, `T81Bytes`, `T81Symbol`, and `T81Result`/`T81Maybe`/`T81Promise`, offer deterministic collection semantics, canonical string/byte handling, and rich error/result signaling within the ternary stack.
- **Systems and observability primitives:** `T81Stream`/`T81IOStream`, `T81Time`, `T81Entropy`, `T81Agent`, `T81Thread`, `T81Network`, `T81Discovery`, and `T81Reflection` round out the platform, providing deterministic I/O, timing, entropy sourcing, metadata, and runtime/topology introspection.
- **Error Handling:**
    - For recoverable errors, functions return a `t81::support::expected`-like object.
    - For unrecoverable mathematical or logical errors (e.g., division by zero), functions throw standard C++ exceptions like `std::domain_error`. The "no exceptions" rule in the spec applies to value-level semantics, not fundamental domain errors.

### Match Pattern Semantics
- Nested pattern matching uses `MatchPattern::Kind::Variant` and the CLI now formats nested arms in Axion trace output. The parser accepts forms such as `Nested(Data(v))` or `Wrap({ inner: Data(v) })`, making it possible to bind into inner enums or record fields without needing multiple locals.
- The semantic analyzer treats nested variants as enums with their own payload metadata, reusing the same binding helpers for identifiers, tuple tuples, and records so deeply nested arms stay deterministic and self-contained.

### Coding Conventions
- **C++20 Standard:** The project uses C++20. Features should be used where they enhance clarity and safety.
- **API in `/include`:** All public APIs are defined in the `/include/t81/` directory. Implementation details reside in `/src/`.
- **RAII and Smart Pointers:** Dynamic memory is managed using RAII. Raw `new` and `delete` are forbidden in high-level code.
- **Test-Driven:** Non-trivial functionality requires corresponding unit tests in `/tests/cpp/`. Bug fixes must be accompanied by a regression test.
- **Minimal Dependencies:** The core libraries have zero external dependencies. External libraries (like `google/benchmark`) are used only for non-essential targets like benchmarks.
