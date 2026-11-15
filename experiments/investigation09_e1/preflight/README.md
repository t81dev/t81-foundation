# Preflight fixture correction

Before the exhaustive experiment and any footprint comparison, the initial
boolean adapter used `(boolean_expression) as i32` as its integer result.
The existing semantic analyzer rejected the fixture with:

```text
7:13: Return type mismatch: expected 'i32' but got 'bool'.
8:1: Return type mismatch: expected 'i32' but got 'bool'.
i09: semantic analysis failed
```

The exact rejected fixture is retained here. The experiment source generator
uses ordinary `if` statements returning integer literals instead. This fixes an
experiment-adapter typing mistake; it does not change boolean short-circuit
semantics, compiler behavior, the preregistered kernels, or the threshold.
No executable artifact or footprint result was produced from this fixture.
