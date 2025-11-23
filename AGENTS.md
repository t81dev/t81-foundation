# AGENTS.md — Guidance for Coding Agents in the T81 Foundation

This file is for AI coding agents (Codex, Copilot, Cursor, Gemini, etc.) working on the **T81 Foundation** repository.

Your job: help extend and harden a **ternary-native, base-81 computational stack** without breaking determinism, specs, or public APIs.

______________________________________________________________________

## 1. Repository Map (What Matters to You)

Treat these as the primary zones:

- `spec/`

  - Normative Markdown specifications for the whole T81 ecosystem (TISC, T81VM, T81Lang, Axion kernel, data types, etc.).
  - These documents define semantics. **Do not silently change semantics here.**
  - Preserve section headings, anchors, and RFC-style language (`MUST`, `SHOULD`, `MAY`).

- `include/t81/`

  - Public **header-first C++ API** surface.
  - New C++ types and functions MUST live here first, under the `t81::` namespace hierarchy.
  - Keep headers self-contained and suitable for installation.

- `src/`

  - C++ implementations backing `include/t81/`.
  - C API bridge code that preserves a stable C ABI for external consumers.
  - Internal helpers that are not meant to be exposed as public API.

- `legacy/hanoivm/src/`

  - Historical C/CWEB implementation of HanoiVM and related libraries.
  - **Reference implementation only.** Do not delete or rewrite in place.
  - When migrating, **mirror semantics in modern C++** and keep legacy files as documentation/oracles.

- `docs/` (if present)

  - Book-style narrative documentation, diagrams, and higher-level overviews.
  - Non-normative, but should remain consistent with `spec/`.

If you need to modify files outside these areas, prefer asking for human guidance in a comment first.

______________________________________________________________________

## 2. Build & Test Commands

Before proposing changes, ensure the tree builds and tests pass.

### Preferred build (CMake)

From repo root:

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build -R "t81_"
```

### Other entrypoints (only if present)

If these targets exist, you MAY also use:

```bash
# Make-based
make run-tests

# Bazel-based
bazel test //:all
```

**Rules:**

- After touching any C++ in `include/` or `src/`, **run the tests** before suggesting the change is “ready”.
- If you add a new feature, add or extend tests under the existing test conventions (match local structure).

______________________________________________________________________

## 3. Coding Conventions

### 3.1 C++ (primary implementation language)

- Use **modern C++ (C++20 or whatever the project already uses)**.

- Public APIs live in `include/t81/` under the `t81::` namespace (and nested namespaces as appropriate).

- Prefer:

  - Value semantics and RAII.
  - `std::string_view` over `const std::string&` when feasible.
  - `std::optional`/result-style types instead of “magic” sentinel values.

- Avoid:

  - Raw `new`/`delete` in new code (prefer smart pointers or value types).
  - Global mutable state unless clearly required and documented.
  - Silent changes to existing function signatures in headers; treat public headers as a stable contract.

Match the existing file’s style:

- Follow local brace style, indentation, and naming.
- Do not auto-reformat whole files just to change style.
- Minimize diffs: **only change what you need.**

### 3.2 C & Legacy CWEB

- Treat `legacy/hanoivm/src/` as a **behavior oracle**, not a playground.

- Do NOT introduce new features in legacy C/CWEB; implement them in C++ instead.

- When creating modern replacements:

  - Read the relevant CWEB file(s) to understand semantics.
  - Write C++ in `include/t81/` + `src/` that replicates behavior.
  - Add tests to lock in equivalent behavior.

### 3.3 Markdown (specs & docs)

- Maintain existing front-matter (`title`, `nav`, etc.).

- Keep anchors and section headings stable; many documents may link to them.

- Use RFC-style normative language consistently:

  - `MUST`, `SHOULD`, `MAY`, `MUST NOT`, etc.

- When updating behavior in code that affects semantics:

  - Update the corresponding spec in `spec/`.
  - Call out the change in a clear “Version / Change Log” section if one exists.

______________________________________________________________________

## 4. Semantic Invariants You MUST Preserve

The T81 stack has specific invariants. Do not violate them without explicit human instruction.

- **Determinism**

  - No hidden randomness in core execution paths.
  - Any randomness (if present at all) MUST be explicitly parameterized and documented.

- **Balanced ternary / base-81 semantics**

  - Big integer and fraction types must preserve canonical forms.

  - Base-81 encodings must remain round-trip safe:

    - `encode()` followed by `decode()` MUST return the original value for valid input.

  - Do not introduce alternate alphabets or encodings without updating specs and tests.

- **No silent behavioral drift**

  - If you change observable behavior of an existing public function, you MUST:

    1. Update tests to reflect the new behavior.
    1. Update the relevant spec(s) in `spec/`.
    1. Document the change in comments or changelog as appropriate.

- **Safety before cleverness**

  - Prefer clear, simple implementations that match the spec over clever micro-optimizations.
  - Only add low-level optimizations (SIMD, bit tricks, etc.) when tests fully cover the semantics.

______________________________________________________________________

## 5. How to Approach Common Tasks

### 5.1 Migrating a legacy component to C++

1. Identify the relevant CWEB file(s) under `legacy/hanoivm/src/`.

1. Locate the corresponding spec sections in `spec/` (TISC, T81VM, data types, etc.).

1. Design an appropriate C++ API in `include/t81/`:

   - Keep it small, composable, and consistent with existing types.

1. Implement the API in `src/`, reusing logic from the CWEB implementation where helpful.

1. Add or extend tests to prove equivalence with the legacy behavior.

1. Leave the legacy CWEB code intact; add comments noting the existence of a modern replacement.

### 5.2 Adding a new feature

- First, check `spec/` to see if the feature is already described.

- If it is:

  - Implement according to the spec.
  - Add tests to cover edge cases mentioned in the spec.

- If it is not:

  - Propose minimal spec wording (e.g., a new subsection) and update the relevant `spec/` file.
  - Then implement and test.

### 5.3 Refactoring

- Do not perform repo-wide refactors without explicit instruction.

- Local refactors are acceptable when:

  - They clearly simplify logic.
  - They are fully covered by existing or updated tests.
  - They do not expand the public API surface in surprising ways.

______________________________________________________________________

## 6. Security, Safety, and Scope

- Do NOT add:

  - Network access, filesystem access, or external dependencies to core libraries unless explicitly requested.
  - Experimental cryptography or ad-hoc schemes; if encryption/sealing is involved, follow the spec and existing code paths.

- Keep changes **self-contained**:

  - For new external dependencies, leave a clear comment and update any build configs.
  - Avoid adding heavy libraries to core paths.

- Keep this repository focused on:

  - Ternary/base-81 computation.
  - TISC / T81VM / T81Lang / Axion kernel.
  - Supporting tools and tests.

Do not introduce unrelated experimentation without a clear link to these goals.

______________________________________________________________________

## 7. When in Doubt

If a task could:

- Change normative behavior,
- Affect determinism,
- Or alter public APIs,

then:

1. Prefer to implement the minimal, obviously correct version.
1. Add comments explaining assumptions.
1. Surface any uncertainties in a short note or TODO rather than “guessing” a new design.

This `AGENTS.md` is the contract between you (the coding agent) and the T81 Foundation. Stay within it unless explicitly told otherwise in a future update to this file.
