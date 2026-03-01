# `tests/fixtures/t81lang_std_agent`

Fixture pack for CLI end-to-end coverage of `std.agent` builtins.

## Purpose
- Validate deterministic observable output for `std.agent` operations when executed via `t81 run`.
- Current fixture set covers `self_reflect`.

## Expectations
- Keep fixtures small and behavior-focused.
- Every `.t81` fixture has a matching `.out` golden file.
- `self_reflect()` is a deterministic no-op opcode alias; it produces no output.
- Agent capability semantics remain bounded/experimental pending governance promotion.
