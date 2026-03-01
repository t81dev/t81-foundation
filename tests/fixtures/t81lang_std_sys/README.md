# `tests/fixtures/t81lang_std_sys`

Fixture pack for CLI end-to-end coverage of `std.sys` builtins.

## Purpose
- Validate deterministic observable output for `std.sys` operations when executed via `t81 run`.
- Current fixture set covers `proof`, `entropy`, `time`, and `reflect`.

## Expectations
- Keep fixtures small and behavior-focused.
- Every `.t81` fixture has a matching `.out` golden file.
- `time()` returns `0t81` and `entropy()` returns `0` under the deterministic placeholder alias profile.
- `reflect()` is a void no-op; it produces no output.
