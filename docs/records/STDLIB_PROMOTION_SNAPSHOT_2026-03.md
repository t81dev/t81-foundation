# T81Lang Standard Library Promotion Snapshot

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Standard Library Promotion Snapshot](#t81lang-standard-library-promotion-snapshot)
  - [1. Module Status Matrix](#1-module-status-matrix)
  - [2. Governance Notes](#2-governance-notes)

<!-- T81-TOC:END -->


Date: 2026-02-26  
Status: Active Snapshot  
Baseline: `c5c4aa59`

## 1. Module Status Matrix

| Module | Status | Determinism Posture | Evidence |
| :--- | :--- | :--- | :--- |
| `std.core` | bounded | deterministic aliases; behavior locked by fixtures | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: core) |
| `std.math` | bounded | host-math dependency documented; bounded deterministic profile only | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: math), `spec/tisc-spec.md` |
| `std.io` | bounded | deterministic handle aliases and print paths | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: runtime) |
| `std.collections` | stable | deterministic staged semantics and fixture coverage | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: collections) |
| `std.text` | stable | deterministic text semantics and fixture coverage | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: text) |
| `std.bytes` | stable | deterministic byte semantics and fixture coverage | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: bytes) |
| `std.symbol` | stable | deterministic alias semantics and fixture coverage | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: symbol) |
| `std.sys` | bounded | deterministic placeholder aliases (`time=0`, `entropy=0`) | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: runtime) |
| `std.async` | bounded | deterministic placeholder aliases (`yield/sleep` no-op) | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: runtime) |
| `std.tensor` | bounded | deterministic alias behavior within current runtime profile | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: tensor) |
| `std.agent` | experimental | deterministic opcode alias exists; capability semantics remain bounded | `tests/cpp/cli_stdlib_fixtures_test.cpp` (module: runtime) |

## 2. Governance Notes

1. This snapshot is descriptive, not a global determinism claim expansion.
2. Any status promotion/demotion requires:
   - snapshot update,
   - evidence update,
   - release/status synchronization.
3. `std.math` remains bounded until host-dependent transcendental surfaces are
   fully contract-resolved for cross-platform bit-identity claims.
4. Usage reference:
   - `docs/reference/T81LANG_STDLIB_REFERENCE.md`
