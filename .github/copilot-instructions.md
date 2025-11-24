This file is an adapter; the canonical rules live in AGENTS.md and ARCHITECTURE.md

# GitHub Copilot Instructions for T81 Foundation

You are assisting with a spec-first, ternary-native computing stack called **T81 Foundation**.

When generating code or suggestions in this repository:

1. **Respect the spec hierarchy**

   - Treat files in `spec/` as normative. Do not introduce behavior that contradicts them.
   - If behavior is unclear, recommend clarifying the spec before assuming semantics.

2. **Follow the architecture**

   - Public APIs live in `include/t81/*.hpp`.
   - Implementations live in `src/`.
   - Tests live in `tests/cpp/`.
   - Legacy reference code lives in `legacy/` and should not be extended for new features.

3. **Coding style**

   - Use modern C++20/23 features judiciously.
   - Prefer clear, deterministic code over clever tricks.
   - Add or update tests when changing behavior.

4. **Safety and Axion**

   - Any change touching Axion, cognitive tiers, or safety-critical paths should be conservative and well-commented.
   - Point humans back to `spec/axion-kernel.md` and `spec/cognitive-tiers.md` for major changes.

5. **Documentation**

   - When you create new types or behaviors, keep `spec/` and `docs/` in sync with the implementation.
   - Use `AGENTS.md`, `ARCHITECTURE.md`, and `spec/index.md` as your starting context for planning larger changes.
