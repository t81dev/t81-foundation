# CLAUDE.md — Project Memory for T81 Foundation

This file provides guidance to Claude Code when working with this repository.

---

## 1. What This Project Is

- **T81 Foundation** is a spec-first, ternary-native computation stack.
- It includes:
  - A base-81 / balanced-ternary arithmetic layer,
  - A deterministic virtual machine (T81VM / HanoiVM),
  - A language (T81Lang),
  - An Axion kernel and cognitive tiers focused on safe AI / proto-AGI substrates.

**Specs are the source of truth.** Implementation must conform to the documents in `spec/`.

---

## 2. Where to Start Reading

When analyzing or modifying code, first read:

1. `ARCHITECTURE.md`
2. `AGENTS.md`
3. `spec/index.md`
4. The specific spec file for the subsystem you’re touching (e.g., `spec/tisc-spec.md`).

For C++ implementation details, then consult:

- `include/t81/` for public APIs,
- `src/` for implementations,
- `tests/cpp/` for expected behavior.

---

## 3. Coding & Change Guidelines

When generating or editing code:

1. **Stay aligned with the spec**
   - Before proposing new behavior, check `spec/` for existing definitions.
   - If the spec is silent, suggest either:
     - Extending the spec (via RFC), or
     - Keeping behavior minimal and well-documented.

2. **Prefer small, self-contained changes**
   - Keep PRs and patches focused.
   - Update tests under `tests/cpp/` whenever semantics change.

3. **Respect the separation of concerns**
   - Do not mix spec edits, VM internals, and Axion semantics in a single change unless strictly necessary.
   - Leave legacy CWEB code in `legacy/` as a reference; migrate incrementally to modern C++.

4. **Determinism and safety**
   - Avoid hidden non-determinism (e.g., time, random, global state).
   - Any safety-relevant behavior must be consistent with `spec/axion-kernel.md` and `spec/cognitive-tiers.md`.

---

## 4. Tasks That Are Safe to Automate

Good targets for automation:

- Adding or improving tests in `tests/cpp/`.
- Refactoring C++ code in `src/` to better match `include/t81/*.hpp`.
- Improving documentation in `spec/` and `docs/` without changing semantics.
- Building tooling around search, docs, and examples.

Changes that require extra care:

- Modifying Axion or cognitive tier semantics.
- Changing VM or instruction set behavior.
- Adjusting data type definitions in `t81-data-types.md`.

---

## 5. What Not to Do Without Explicit Instruction

- Do not rewrite the entire architecture in another language.
- Do not remove or radically alter legacy CWEB code; it is a reference.
- Do not silently weaken safety constraints in Axion or cognitive tiers.
