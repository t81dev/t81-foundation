# Restructure Completion Checklist

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Restructure Completion Checklist](#restructure-completion-checklist)
  - [Final Canonical Directory Tree](#final-canonical-directory-tree)
  - [Phase Completion Status](#phase-completion-status)
  - [Invariant Checks](#invariant-checks)
  - [Validation Commands](#validation-commands)
  - [Future Move Protocol](#future-move-protocol)

<!-- T81-TOC:END -->


This document is the closeout artifact for repository restructure Phases 1-9.

## Final Canonical Directory Tree

```
/core
  /types
  /isa
  /vm
/kernel
  /axion
/runtime
  /tracing
  /jit
/lang
  /frontend
  /stdlib
/tooling
  /cli
  /model
/experimental
  /tiers
  /hanoi
/include
  /t81
/internal
/docs
/spec
/book
/examples
/scripts
/tools
/contracts
```

## Phase Completion Status

- [x] Phase 1: Types migrated to `core/types` and public types headers under `include/t81/types`.
- [x] Phase 2: ISA migrated to `core/isa` with public ISA headers under `include/t81/isa`.
- [x] Phase 3: VM migrated to `core/vm` with public VM headers under `include/t81/vm`.
- [x] Phase 4: Axion migrated to `kernel/axion` with public Axion headers under `include/t81/axion`.
- [x] Phase 5: Language/tooling split into `lang/*` and `tooling/*`.
- [x] Phase 6: Runtime tracing/JIT split into `runtime/tracing` and `runtime/jit`.
- [x] Phase 7: Experimental subsystems contained under `experimental/*`.
- [x] Phase 8: Public/internal header boundaries enforced (`include/t81/**` vs `internal/**`).

## Invariant Checks

- [x] Legacy module paths are removed from active code/docs/scripts scope.
- [x] Legacy include patterns are removed from active source/include scope.
- [x] Core modules do not depend on experimental modules.
- [x] Public C++ API surface is `include/t81/**` only.
- [x] Historical snapshots that intentionally preserve legacy paths are explicitly marked historical.

## Validation Commands

Run from repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
scripts/restructure/verify_restructure_clean.sh
```

## Future Move Protocol

1. Always use `git mv` for physical moves.
2. Update CMake/Bazel targets and include paths in the same change.
3. Sweep docs/scripts/examples for stale paths.
4. Run build + tests + restructure verification before merge.
