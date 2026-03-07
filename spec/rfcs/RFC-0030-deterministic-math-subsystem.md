# RFC-0030: Deterministic Math Subsystem

Version 0.1 — Standards Track
Status: Draft
Author: T81 Foundation Architecture Team
Applies to: T81 Data Types, T81Float, TISC

______________________________________________________________________

# Summary

This RFC outlines the architectural integration of a verified, software-level transcendental math library tailored for `T81Float`. By defining a deterministic math subsystem with correctly rounded operations, T81 can support advanced AI inference and cryptographic operations across architectures without relying on host-dependent `cmath` implementations.

# Motivation

To maintain cross-architecture bit-exact determinism, T81 currently disables host-dependent `cmath` transcendental functions (like `sin`, `cos`, `exp`) in deterministic builds, causing them to throw domain errors. This severely limits the platform's ability to handle the complex mathematical models required for advanced workloads. A reliable, architecture-independent soft-float expansion is necessary to provide deterministic transcendental capabilities within the Deterministic Core Profile (DCP).

# Proposal

### 1. Architectural Integration of a Software-Level Transcendental Library
A new module, `include/t81/math/t81_soft_math/`, will house a clean-room, C++23 `constexpr`-compatible implementation of trigonometric and exponential functions.
*   **Integer-Backed Implementation:** The library will unpack the IEEE 754 `T81Float` into a high-precision deterministic integer format (using `__int128` or `std::uint64_t`).
*   **Hybrid Evaluation Strategy:** A correctly rounded, bit-identical library must be built utilizing deterministic range reduction, polynomial/minimax evaluation (`minimax.hpp`), and CORDIC iterations (`cordic.hpp`) executed purely via deterministic integer arithmetic before explicit packing and truncation back to `T81Float`.
*   **Deterministic Lookup Tables:** Precomputed tables for functions like `sin`, `cos`, `exp`, and `log` will be stored as canonical constants, enabling fast evaluation and enforcing deterministic behavior with a lower runtime cost.

### 2. Deterministic Rounding Rules
The math subsystem must explicitly define rounding behaviors in the specification, avoiding reliance on hardware floating-point rounding modes. Standard rounding constants include:
*   `round_toward_zero`
*   `round_nearest_even`
*   `round_down`

### 3. Integration into `include/t81/types/T81Float.hpp`
*   **Macro Replacement:** The existing `#ifndef T81_DETERMINISTIC` blocks in `T81Float` that currently throw `std::domain_error` for transcendentals (e.g., `expi`, `sqrt`, `sin`, `cos`) will be removed.
*   **Method Redirection:** The `T81Float` host-native methods will bind directly to the new soft-float implementations (e.g., `constexpr T81Float T81Float::sin() const noexcept`).
*   **Dependency Firewall Compliance:** The soft-float library will reside strictly within the types/math layer, ensuring `core/isa` and `core/vm` can natively consume it without establishing upward dependencies.

### 4. Verification and CI Gating
*   **Test Suite Extension:** Extend `tests/cpp/test_float.cpp` to assert the output of all transcendentals against pre-computed, known-good hexadecimal floating-point literals across the full operational domain, including edge cases.
*   **Canonical Repro Hash:** A script in `tests/fixtures/t81lang_determinism/` will compute transcendentals to verify the resulting AST/IR repro hash (`t81lang_repro_hash.txt`) remains perfectly identical across architecture targets via `scripts/ci/t81lang_repro_gate.py`.

# Impact

## Backward Compatibility
Removes a significant runtime domain error for valid math expressions under deterministic builds. No breaking syntax changes.

## Performance
Introduces minor performance overhead compared to hardware FPUs, but this is an acceptable tradeoff to guarantee absolute determinism. JIT mechanisms (RFC-0028) may optimize sequences of operations.

## Security
Provides mathematically provable, reproducible paths for cryptographic and high-assurance models that require transcendental functions.

# Alternatives Considered

*   **Host Libm with strict flags:** Compiling host math libraries with strict rounding flags does not guarantee bit-exact results across differing microarchitectures or compiler toolchains.
*   **External soft-float library:** Relying on existing third-party soft-float libraries introduces supply-chain risk and potential integration issues with T81's unique base-81 ternary mapping layer.

# References
*   RFC-0001: Architecture Principles
*   RFC-0002: Deterministic Execution Contract
*   T81 Data Types Specification (`t81-data-types.md`)