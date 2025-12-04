---
layout: page
title: Public API Overview
---

# T81 Foundation: Public API Overview

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation: Public API Overview](#t81-foundation-public-api-overview)
  - [1. Where to Look](#1-where-to-look)
  - [2. Core Numerics (`include/t81/core`, `include/t81/tensor`, `include/t81/ternary`)](#2-core-numerics-`includet81core`-`includet81tensor`-`includet81ternary`)
  - [3. Frontend (`include/t81/frontend`)](#3-frontend-`includet81frontend`)
  - [4. TISC & VM (`include/t81/tisc`, `include/t81/vm`)](#4-tisc-&-vm-`includet81tisc`-`includet81vm`)
  - [5. Weights & Tooling (`include/t81/weights`, `src/tools`)](#5-weights-&-tooling-`includet81weights`-`srctools`)
  - [6. CanonFS & Axion Entrypoints (`include/t81/canonfs`, `include/t81/axion`)](#6-canonfs-&-axion-entrypoints-`includet81canonfs`-`includet81axion`)
  - [7. Targeted Coverage Notes](#7-targeted-coverage-notes)

<!-- T81-TOC:END -->






























This guide catalogs the public C++ APIs under `include/t81/`, highlights threading/error-handling expectations, and points readers toward the generated Doxygen docs.

______________________________________________________________________

## 1. Where to Look

- **Header bundle:** all public symbols live under `include/t81/` (organized per subsystem). Prefer the generated API reference at `docs/api/html/index.html` after running `cmake --build build --target docs`.  
- **Namespace promise:** we expose `t81::...` (and nested namespaces such as `t81::core`, `t81::frontend`, `t81::lang`, `t81::vm`, `t81::weights`). Respect the `t81::v1` inline namespace alias for future ABI compatibility.
- **Versioning:** the public API snapshot may evolve only when the semantic version increments; breaking exports require RFC sign-off.

## 2. Core Numerics (`include/t81/core`, `include/t81/tensor`, `include/t81/ternary`)

- **Purpose:** exact balanced-ternary arithmetic, tensors, containers, and the binary-to-ternary helpers.  
- **Key headers:** `T81Int.hpp`, `T81Float.hpp`, `T81Tensor.hpp`, `T81Matrix.hpp`, `T81List.hpp`, `T81Set.hpp`, `T81Map.hpp`, `T81Result.hpp`, `T81Maybe.hpp`.  
- **Thread Safety:** these headers are *not* thread-safe by default; clients should synchronize around shared mutable containers or use `t81::support::expected` (which is copy-on-write safe) when sharing results across threads.  
- **Error Handling:** arithmetic functions either return `t81::support::expected`/`T81Result` wrappers or emit controlled Axion traps for overflow/entropy violations. Rare unrecoverable faults throw `std::domain_error`/`std::runtime_error` (documented in `DESIGN.md`). Prefer the `noexcept` helpers in `include/t81/detail/assert.hpp` for invariants.

## 3. Frontend (`include/t81/frontend`)

- **Purpose:** Exposes the public components of the T81Lang compiler, including the lexer, parser, semantic analyzer, and IR generator.
- **Public Entrypoints:** `frontend::Lexer`, `frontend::Parser`, `frontend::SemanticAnalyzer`, `frontend::IRGenerator`.
- **Thread Safety:** The state of the frontend components is not thread-safe. Each thread should have its own instances of these classes.
- **Error Handling:** Errors from the frontend are collected as diagnostic messages, which can be printed to the console with source context. The `t81` CLI tool provides the canonical implementation of this error reporting.
- **Notes:** The full compiler pipeline is orchestrated by the `t81` CLI driver, which serves as the primary public interface for compiling and running T81Lang code.

### 3.1 Command-Line Frontend Helpers

- The CLI driver (`include/t81/cli/driver.hpp`) exposes `build_program_from_source`, `compile`, `check_syntax`, `run_tisc`, and `repl`. `build_program_from_source` is the canonical entry point for lex/parse/semantic/IR/TISC generation and now accepts the original source string so diagnostics can print the exact line and caret for semantic errors.
- `build_program_from_source` returns `t81::tisc::Program` plus Axion loop metadata and optional weights attachments. Use it to integrate the frontend pipeline into custom tooling or tests (see `tests/cpp/cli_diagnostic_context_test.cpp` for an example that verifies the Option/Result/loop/match diagnostics).
- The CLI driver reuses this helper for `compile`/`check`/`repl`, ensuring all commands share a single diagnostics path and deterministic semantic context.

## 4. TISC & VM (`include/t81/tisc`, `include/t81/vm`)

- **Purpose:** emitter for binary TISC programs (`BinaryEmitter`, `Program`), runtime state/VM (`vm::IVirtualMachine`, `vm::State`, `vm::OptionValue`).  
- **Thread Safety:** the VM is single-threaded; use per-`t81::vm::State` instances or guard access via your scheduler to prevent concurrent mutation.  
- **Error Handling:** traps (Axion events) are communicated via `vm::AxionEvent`; invalid instructions throw `std::runtime_error`/`AxionTrap`. Memory faults wrap the `t81::vm::AxionEvent::type` enum.

## 5. Weights & Tooling (`include/t81/weights`, `src/tools`)

- **Purpose:** import/export quantized weights, metadata, canonical blobs. The CLI uses these helpers for `t81 weights ...` commands.  
- **Thread Safety / Error Handling:** operations are I/O-bound and return `t81::support::expected` results; disk concurrency control is handled by the caller (e.g., the CLI). Unexpected formats result in descriptive diagnostics rather than crashes.

## 6. CanonFS & Axion Entrypoints (`include/t81/canonfs`, `include/t81/axion`)

- **Status:** stubs today but considered public contracts. See spec files `spec/canonfs-spec.md` and `spec/axion-kernel.md` for normative definitions.  
- **Thread Safety:** these APIs delegate to in-memory drivers (`src/canonfs/in_memory_driver.cpp`) and are not synchronized; wrap them before sharing across threads.  
- **Error Handling:** the Axion engine reports verdicts through `t81::axion::Verdict`. Implementations always return `t81::axion::AllowAllEngine` or `InstructionCountingEngine` for testing until the kernel is fully functional.

## 7. Targeted Coverage Notes

- Annotate each header with a brief Doxygen summary (existing files already have `namespace t81 { ... }` and `///` comments; keep them updated when adding APIs).  
- When exposing new APIs, add tests under `tests/cpp/` and a short entry in `docs/guides/public-api-overview.md`.  
- Nothing in `include/t81/` should introduce raw `new`/`delete`, global mutable state, or hidden nondeterminism; refer to `AGENTS.md`/`DESIGN.md` when in doubt.
