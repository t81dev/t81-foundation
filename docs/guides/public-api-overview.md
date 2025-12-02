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
  - [3. Frontend (`include/t81/frontend`) & Language Interface (`include/t81/lang`)](#3-frontend-`includet81frontend`-&-language-interface-`includet81lang`)
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

## 3. Frontend (`include/t81/frontend`) & Language Interface (`include/t81/lang`)

- **Purpose:** exposes lexer, parser, semantic analyzer, IR generator, and the high-level `t81::lang` compiler API.  
- **Public entrypoints:** `frontend::Lexer`, `frontend::Parser`, `frontend::SemanticAnalyzer`, `lang::Compiler`, `lang::Builder` (if present).  
- **Thread Safety:** the parser and semantic analyzer instance state is not thread-safe; instantiate per thread or guard with `std::mutex`.  
- **Error Handling:** lexing/parsing errors return `t81::expected<...>` values containing diagnostics; the CLI surfaces human-friendly diagnostics with file/line numbers.  
- **Notes:** `include/t81/lang/types.hpp` defines the AST type system and `lang::Compiler` exposes hooks to compile or run programs in the HanoiVM. All semantic invariants trace back to `spec/t81lang-spec.md`.

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
