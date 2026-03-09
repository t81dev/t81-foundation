# tests/cpp

Main C++ test suite for language, runtime, determinism, and policy behavior.

## Coverage areas
- Frontend: lexer/parser/semantic analyzer conformance
- TISC: encoding, metadata, determinism, binary IO
- VM: interpreter behavior, traps, memory/load-store, tensor ops
- Axion: policy, recursion/loop/match guards, segment/trace behavior
- CLI/e2e: compile/run/disasm/check/internal repro-hash workflows
- Numerics: bigint/fraction/tensor correctness and perf guardrails

## Running targeted slices
```bash
ctest --test-dir build --output-on-failure -R "frontend|semantic|tisc"
ctest --test-dir build --output-on-failure -R "axion|vm|jit"
ctest --test-dir build --output-on-failure -R "cli|e2e|determinism"
```

## Authoring notes
- Keep failure diagnostics actionable.
- For standalone tests, return non-zero with clear messages.
- Add fixtures under `tests/fixtures/` when test data needs long-term stability.
