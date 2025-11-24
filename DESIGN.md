# T81 Foundation — Design Principles

This document captures the design constraints and invariants that should guide all changes.

---

## 1. Core Philosophy

1. **Spec-first**  
   Behavior is defined by the spec (`spec/`), not by historical implementation quirks.

2. **Determinism by default**  
   - VM and language behavior must be deterministic given the same inputs.
   - Any introduction of nondeterminism (e.g., time, entropy, network) must be explicitly spec’d and gated by Axion.

3. **Ternary realism**  
   - Even when running on binary hardware, the model is balanced ternary.
   - Implementation should preserve ternary semantics (e.g., rounding, overflow, encoding) even when optimized.

4. **Safety and introspection**  
   - Axion and cognitive tiers are not “afterthought” modules; they are integral to the architecture.
   - Every subsystem should be introspectable: metrics, invariants, and failure modes should be inspectable by higher layers.

---

## 2. Invariants

When editing code or specs, preserve these invariants:

1. **Data Types**
   - `T243BigInt` (or equivalent bigint types) must be:
     - Correct for addition/subtraction/multiplication/division/modulo across the supported range.
     - Stable under serialization/deserialization.
   - Fractions are always reduced with positive denominators.
   - Tensor shapes and broadcasting rules follow `spec/t81-data-types.md`.

2. **IR / TISC**
   - Opcodes are stable and versioned; new opcodes must be added without breaking existing ones.
   - Encoding/decoding must be reversible and round-trip cleanly.

3. **VM**
   - Execution must be reproducible across environments.
   - Errors should be explicit (no silent UB); prefer well-defined traps.

4. **Axion**
   - All externally visible behaviors that could impact safety or alignment must route through Axion’s decision surface.
   - Axion code changes require spec review.

---

## 3. Coding Conventions (C++)

1. **Headers first**
   - Public API definitions live in `include/t81/*.hpp`.
   - Implementation lives in `src/` and should not expose internal headers to users.

2. **Modern C++**
   - Prefer C++20/23 features where they clarify intent (ranges, `std::span`, `constexpr` where practical).
   - No raw new/delete in high-level API; use RAII and smart pointers where ownership is non-trivial.

3. **Error handling**
   - For library API: prefer exceptions or error types over bare `assert` for user errors.
   - Use `assert` only for internal invariants that should never fail in valid usage.

4. **Testing**
   - Every non-trivial function should have tests in `tests/cpp/`.
   - Bug fixes should add regression tests.

---

## 4. What AI Tools Should Do

When assisting on this repo, AI tools should:

- **When in doubt, read the spec** before inventing new behavior.
- Prefer changes that:
  - Reduce complexity,
  - Increase determinism,
  - Improve test coverage,
  - Or clarify the spec.

Avoid:

- Introducing hidden global state,
- Adding implicit I/O in core math or VM loops,
- Modifying `spec/` or `AGENTS.md` without explicit justification.
