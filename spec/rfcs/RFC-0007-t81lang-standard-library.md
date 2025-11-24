______________________________________________________________________

# RFC-0007 — T81Lang Standard Library

Version 0.1 — Draft (Standards Track)\
Status: Draft\
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
2. How do we distribute precompiled hashes so air‑gapped systems can verify the
   standard library?
3. Should Axion enforce maximum module version skew across a deployment?

______________________________________________________________________
