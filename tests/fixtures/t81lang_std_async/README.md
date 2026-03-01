# `tests/fixtures/t81lang_std_async`

Fixture pack for CLI end-to-end coverage of `std.async` builtins.

## Purpose
- Validate deterministic observable output for `std.async` operations when executed via `t81 run`.
- Current fixture set covers `thread`, `promise`, `yield`, and `sleep`.

## Expectations
- Keep fixtures small and behavior-focused.
- Every `.t81` fixture has a matching `.out` golden file.
- `thread()` and `promise()` return deterministic handle alias strings (`std.async.thread`, `std.async.promise`).
- `yield()` and `sleep()` are deterministic no-ops; they produce no output.
