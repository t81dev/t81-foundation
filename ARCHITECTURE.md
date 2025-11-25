# T81 Foundation — Architecture Overview

_T81 is a ternary-native computation stack with a formal, spec-first constitution._

This document gives AI tools and humans a fast, high-level map of the system and where to read next.

______________________________________________________________________

## 0. Source of Truth

1. **Specifications (normative)**

   - `spec/` is the canonical definition of the T81 ecosystem.
   - Code must conform to the spec; the spec is not retrofitted to implementation.
   - Any normative change to behavior flows through an RFC → spec → implementation pipeline.

1. **Implementation (non-normative, but binding to spec)**

   - `include/t81/` — public C++ API surface.
   - `src/` — C++ implementations and C API bridge.
   - `legacy/` — historical CWEB / HanoiVM code, kept as reference and migration source.

If a conflict exists between spec and implementation, assume **spec is right** and propose a fix to the implementation.

______________________________________________________________________

## 1. Layered Stack

From bottom to top:

1. **T81 Arithmetic Layer**

   - Domain: base-81 arithmetic, balanced ternary representation, hashing/encoding.
   - Code: `include/t81/{bigint,fraction,hash,entropy}.hpp`, corresponding `src/` files.
   - Role: deterministic numeric substrate; all higher layers rely on this.

1. **T243 / T729 Symbolic & Tensor Layers**

   - Domain: symbolic logic trees (`T243`), tensor operations and AI-oriented math (`T729`).
   - Code: `include/t81/tensor/**.hpp`, `include/t81/entropy.hpp`, future `axion/` primitives.
   - Role: high-level algebra, tensor computations, AI-facing math.

1. **TISC / IR Layer (Instruction Set & Bytecode)**

   - Domain: Ternary Instruction Set (TISC), IR encoding & decoding, opcodes.
   - Code: `include/t81/ir/{opcodes,insn,encoding}.hpp`, IR helpers in `src/`.
   - Spec: `spec/tisc-spec.md`, `spec/t81vm-spec.md`.
   - Role: stable, spec-driven instruction format for the virtual machine.

1. **T81 Virtual Machine (HanoiVM)**

   - Domain: execution engine, loader, recursion tiers, promotion logic.
   - Code:
     - Modern C++ VM: lives under `src/` (ongoing migration).
     - Legacy CWEB VM: `legacy/hanoivm/src/hanoivm_core/`.
   - Spec: `spec/t81vm-spec.md`.
   - Role: execute TISC programs deterministically on a ternary model.

1. **T81Lang (Programming Language)**

   - Domain: language syntax, type system, compilation to TISC.
   - Code: `include/t81/ternary.hpp`, eventual `t81lang` compiler under `src/`.
   - Spec: `spec/t81lang-spec.md`.
   - Role: user-facing language for T81, mapping source → IR → VM.

1. **Axion Kernel & Cognitive Tiers**

   - Domain: safety supervisor, cognitive recursion, proto-AGI substrate.
   - Spec: `spec/axion-kernel.md`, `spec/cognitive-tiers.md`.
   - Code: `include/t81/axion/api.hpp` and related modules.
   - Role: enforce constitutional constraints, govern higher-order behaviors.

______________________________________________________________________

## 2. Directory Map for Agents

When working with this repo:

- **Start here (for understanding):**

  - `README.md`
  - `ARCHITECTURE.md` (this file)
  - `AGENTS.md`
  - `spec/index.md`

- **Specs (normative):**

  - `spec/t81-overview.md` — ecosystem overview.
  - `spec/t81-data-types.md` — numeric & structural types.
  - `spec/tisc-spec.md` — instruction set.
  - `spec/t81vm-spec.md` — VM behavior.
  - `spec/t81lang-spec.md` — language.
  - `spec/axion-kernel.md`, `spec/cognitive-tiers.md` — higher-tier behavior.

- **Implementation (C++):**

  - `include/t81/` — headers; treat this as the public API.
  - `src/` — implementations; safe place for refactors that respect the spec.
  - `tests/cpp/` — C++ test suite; always update tests when behavior changes.

- **Docs & site:**

  - `docs/` — static site, Jekyll/MkDocs-style docs.
  - `docs/search/` — search index builder (Node / Lunr).
  - `docs/_includes/sidebar.html` — generated sidebar.

- **Legacy + reference:**

  - `legacy/hanoivm/src/` — CWEB VM and libraries.
  - Use this for migration and historical understanding, not for new features.

______________________________________________________________________

## 3. How Changes Should Flow

1. **Idea → RFC (optional, for big changes)**
   - For substantial changes, write an RFC under `spec/rfcs/`.
1. **Spec update**
   - Update the relevant spec file in `spec/` first.
   - Keep prose and formal definitions consistent.
1. **Implementation update**
   - Update `include/t81/` and `src/` to conform to the new spec.
   - Keep the C API stable where possible.
1. **Tests + docs**
   - Add/extend tests in `tests/cpp/`.
   - Update `docs/` and any user-facing references.

______________________________________________________________________

## 4. Rules for AI Tools

1. **Always read** `AGENTS.md`, `ARCHITECTURE.md`, and `spec/index.md` before large changes.
1. **Do not** modify `spec/` files without:
   - Explaining how the change affects the rest of the system.
   - Updating the related implementation and tests in the same PR.
1. **Prefer** small, atomic PRs:
   - One behavior or subsystem per change.
1. **Never** silently change semantics:
   - If behavior changes, update the spec and tests explicitly.
