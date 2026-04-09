# T81Lang Standard Library Reference

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Standard Library Reference](#t81lang-standard-library-reference)
  - [1. Read This First](#1-read-this-first)
  - [2. How to Use Modules](#2-how-to-use-modules)
  - [3. Module Index](#3-module-index)
  - [4. Determinism Notes](#4-determinism-notes)
  - [5. Fixture-Driven Usage References](#5-fixture-driven-usage-references)
  - [6. Upgrade Discipline](#6-upgrade-discipline)

<!-- T81-TOC:END -->


Date: 2026-02-26  
Status: Active  
Scope: Current `lang/stdlib/std/*.t81` module surface

## 1. Read This First

This reference describes currently available `std.*` modules and usage patterns.

- Promotion status and determinism posture are tracked in:
  - `docs/status/STDLIB_PROMOTION_SNAPSHOT_2026-03.md`
- Change governance is defined in:
  - `docs/governance/STDLIB_CHANGE_POLICY.md`
- Canonical module descriptions remain in:
  - `docs/standards/standard-library.md`

## 2. How to Use Modules

Example import and use:

```t81
fn main() -> i32 {
    let txt: T81String = "hello";
    let n: i32 = std.text.str_len(txt);
    print(n);
    return 0;
}
```

Guidance:

1. Prefer explicit `std.<module>.<function>` calls.
2. Treat `bounded` and `experimental` modules as non-frozen surfaces.
3. For deterministic behavior checks, mirror fixture patterns under
   `tests/fixtures/t81lang_std_*`.

## 3. Module Index

| Module | Status | Core Usage | Evidence Fixture/Test |
| :--- | :--- | :--- | :--- |
| `std.core` | bounded | `assert`, `debug`, `unwrap_or` | `tests/cpp/cli_std_core_fixtures_test.cpp` |
| `std.math` | bounded | `sqrt`, `exp`, `pow`, `clamp` and related math aliases | `tests/cpp/cli_std_math_fixtures_test.cpp` |
| `std.io` | bounded | `println`, `print_int`, `print_float`, `stream`, `net` | `tests/cpp/cli_std_runtime_fixtures_test.cpp` |
| `std.collections` | stable | vector/list/map/set/tree/graph staged deterministic helpers | `tests/cpp/cli_std_collections_fixtures_test.cpp` |
| `std.text` | stable | `str_len`, `concat`, `split`, `join`, `replace` | `tests/cpp/cli_std_text_fixtures_test.cpp` |
| `std.bytes` | stable | bytes `len`, `concat`, `split`, `join`, conversions | `tests/cpp/cli_std_bytes_fixtures_test.cpp` |
| `std.symbol` | stable | `intern`, `to_string`, `eq`, `ne` | `tests/cpp/cli_std_symbol_fixtures_test.cpp` |
| `std.sys` | bounded | `exit`, `time`, `entropy`, `proof`, `reflect` | `tests/cpp/cli_std_runtime_fixtures_test.cpp` |
| `std.async` | bounded | `yield`, `sleep`, `thread`, `promise` | `tests/cpp/cli_std_runtime_fixtures_test.cpp` |
| `std.tensor` | bounded | `load`, `from_list`, `vec_add`, `matmul` | `tests/cpp/cli_std_tensor_fixtures_test.cpp` |
| `std.agent` | experimental | `self_reflect` | `tests/cpp/cli_std_runtime_fixtures_test.cpp` |

## 4. Determinism Notes

1. Determinism guarantees are bounded by DCP/registry-verified surfaces.
2. `std.math` is `bounded` due to host-math dependence for transcendental paths.
3. `std.sys` and `std.async` are deterministic placeholder aliases on current profile.
4. `std.agent` remains experimental and must not be treated as a frozen contract.

## 5. Fixture-Driven Usage References

Use these as executable examples:

- `tests/fixtures/t81lang_std_core/`
- `tests/fixtures/t81lang_std_math/`
- `tests/fixtures/t81lang_std_text/`
- `tests/fixtures/t81lang_std_bytes/`
- `tests/fixtures/t81lang_std_collections/`
- `tests/fixtures/t81lang_std_tensor/`
- `tests/fixtures/t81lang_std_runtime/`
- `tests/fixtures/t81lang_std_symbol/`

## 6. Upgrade Discipline

Before relying on a symbol as stable:

1. Check its module status in `STDLIB_PROMOTION_SNAPSHOT_2026-03.md`.
2. Confirm fixture and test evidence exists.
3. Confirm no conflicting change-class record in `STDLIB_CHANGE_POLICY.md`.
