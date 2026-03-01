# `tests/fixtures/t81lang_std_io`

Fixture pack for CLI end-to-end coverage of `std.io` builtins.

## Purpose
- Validate deterministic observable output for `std.io` operations when executed via `t81 run`.
- Current fixture set covers `println`, `print_int`, `print_float`, `stream`, and `net`.

## Expectations
- Keep fixtures small and behavior-focused.
- Every `.t81` fixture has a matching `.out` golden file.
- `stream()` and `net()` return deterministic handle alias strings (`std.io.stream`, `std.io.net`).
