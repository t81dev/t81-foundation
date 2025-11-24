# T81 Foundation — Good First Tasks (for Humans & AI Agents)

This file highlights safe, high-leverage tasks for contributors and AI tools.

______________________________________________________________________

## 1. Documentation & Spec

- Improve wording and clarity in:
  - `spec/t81-overview.md`
  - `spec/t81-data-types.md`
  - `spec/t81vm-spec.md`
- Add cross-links between related sections (data types ↔ VM ↔ language).
- Add small examples illustrating key concepts (e.g., TISC instruction sequences).

______________________________________________________________________

## 2. Tests

- Increase coverage in `tests/cpp/` for:
  - Big integer operations and edge cases.
  - Fraction normalization and arithmetic.
  - Tensor shape/broadcast rules.
- Add regression tests for any bugs discovered in the VM or language.

______________________________________________________________________

## 3. Implementation Cleanups

- Refactor C++ code in `src/` to:
  - Use modern C++ constructs consistently.
  - Reduce duplication between similar modules.
- Improve error messages and diagnostics for users of the library.

______________________________________________________________________

## 4. Tooling & Dev Experience

- Enhance developer docs in `docs/`:
  - “How to build and run tests”
  - “How to add a new opcode”
  - “How to extend T81Lang”

These tasks are designed to be safe for AI assistance: they improve correctness, clarity, and ergonomics without altering the core architecture or safety guarantees.
