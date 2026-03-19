______________________________________________________________________

# RFC-0007 — T81Lang Standard Library

Version 0.2 — Standards Track\
Status: Accepted\
Updated: 2026-03-15\
Author: T81Lang Working Group\
Applies to: T81Lang, TISC, Axion, Spec Tooling

______________________________________________________________________

# 0. Summary

This RFC introduces the **deterministic, pure-by-default standard library**
promised in `spec/t81lang-spec.md` but not yet formalized. It specifies:

1. Core modules (`arith`, `tensor`, `option`, `result`, `io.axsafe`).
2. Deterministic semantics for every exported function.
3. Compilation + versioning rules so programs can rely on library stability.

______________________________________________________________________

# 1. Motivation

Without a standard library, every program reimplements the same canonical
helpers (option combinators, tensor utilities, canonical hashing). This causes:

- duplicated code
- inconsistent Axion annotations
- difficulty auditing determinism across repositories

The standard library provides blessed implementations vetted by Axion and
the spec maintainers.

______________________________________________________________________

# 2. Design / Specification

### 2.1 Module Layout

```
std/
  arith.t81      // deterministic numerics
  tensor.t81     // shape-safe helpers, referencing RFC-0004
  option.t81     // map/flat_map/zip for Option[T]
  result.t81     // combinators for Result[T,E]
  axsafe_io.t81  // Axion-supervised logging, no external side effects
```

### 2.2 Determinism Rules

- All functions are pure unless annotated `@effect`.
- Module initialization is forbidden; no global mutable state.
- Functions must document their Axion tier requirements.

### 2.3 Versioning

- Each module declares `@version(major, minor)` metadata.
- Breaking changes require bumping the module version and updating the
  top-level RFC index.
- Programs import via `use std::tensor@1.0 as tensor`, locking the version.

### 2.4 Compilation

- Standard modules compile alongside user code; the compiler embeds them into
  the same TISC binary to avoid runtime linking.
- `t81mod` tooling verifies hashes to ensure the standard library was not
  altered locally.

______________________________________________________________________

# 3. Rationale

- Providing canonical combinators reduces user mistakes and ensures Option /
  Result semantics match the spec.
- Embedding Axion-aware IO helpers keeps logging deterministic and traceable.
- Version locking via annotations prevents “works on my machine” library drift.

______________________________________________________________________

# 4. Backwards Compatibility

- Existing programs continue to build; importing `std::*` is optional.
- Once modules reach `@version(1,0)`, updates follow semantic versioning.

______________________________________________________________________

# 5. Security Considerations

- Axion audits the standard modules, lowering the review burden for downstream
  teams.
- `axsafe_io` ensures logging can occur without providing ambient host IO
  access—Axion mediates every call.

______________________________________________________________________

# 6. Open Questions

1. Should tensor helpers include in-place variants, or remain purely functional?
   **Resolved:** Purely functional for v1.0. In-place variants, if introduced,
   require a new `@mutates` annotation and a separate RFC.
2. How do we distribute precompiled hashes so air‑gapped systems can verify the
   standard library?
   **Resolved:** The standard library is compiled into the binary as TISC
   intrinsics, not as distributable source files. Hash verification is covered
   by the existing CanonHash81 determinism registry on the `t81` binary itself.
3. Should Axion enforce maximum module version skew across a deployment?
   **Resolved:** N/A under the builtin-registry model — all builtins are
   frozen with the ISA version and cannot skew independently.

______________________________________________________________________

# 7. Implementation Note

**The standard library is implemented as compiler intrinsics, not as `.t81`
source files.**

The original design proposed `std/arith.t81`, `std/tensor.t81`, etc. compiled
alongside user code. The actual implementation instead embeds all standard
functions directly into the compiler as entries in `kBuiltinTable`
(`include/t81/frontend/builtin_registry.hpp`). This achieves the same goals:

- canonical, deterministic implementations vetted by spec maintainers
- Axion tier gates (`min_tier`) and effect-surface flags on every call
- single source of truth — adding a builtin requires one row in `kBuiltinTable`
- module stability guaranteed by ISA version freeze, not per-module `@version`

The version-locked import syntax (`use std::tensor@1.0 as tensor`) is
superseded by the ISA freeze guarantee: all builtins are frozen at the ISA
major version boundary and cannot change without a major version bump.

The full feature matrix is documented in `spec/t81lang_features.md`.

______________________________________________________________________

# 8. Acceptance Note

**Status advanced `Draft → Accepted` on 2026-03-15.**

| Criterion | Evidence |
| :--- | :--- |
| [A-0007-01] Core module: `arith` | Arithmetic ops (`+`, `-`, `*`, `/`, `%`, `neg`) are compiler-native; `std.core.assert` in `kBuiltinTable` |
| [A-0007-02] Core module: `tensor` | `std.tensor.load`, `std.tensor.matmul`, `std.tensor.dot_product` in `kBuiltinTable`; `ChkShape` opcode in TISC |
| [A-0007-03] Core module: `option` | `std.option.is_some/is_none/unwrap` + `std.core.unwrap_or` in `kBuiltinTable`; `MakeOptionSome/None` opcodes in TISC |
| [A-0007-04] Core module: `result` | `std.result.is_ok/is_err/unwrap/unwrap_err` in `kBuiltinTable`; `MakeResultOk/Err` opcodes in TISC |
| [A-0007-05] Core module: `axsafe_io` | `std.io.println` / `std.core.debug` (`PRINT` opcode); Axion audit via policy hooks |
| [A-0007-06] Determinism rules | All builtins are pure unless `is_effect_surface = true`; no global mutable state; Axion tier requirements enforced by SA |
| [A-0007-07] Versioning / stability | ISA major-version freeze replaces per-module `@version`; builtin assignments are frozen in `opcodes.hpp` |

______________________________________________________________________
