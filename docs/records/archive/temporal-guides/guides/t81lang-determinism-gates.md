# T81Lang Determinism Gates

This guide defines the practical determinism checks used for T81Lang compile
and runtime behavior.

**Last Updated:** February 17, 2026

## Why these gates exist

A deterministic language runtime requires both of these to hold:

- identical source -> identical bytecode
- identical bytecode + inputs -> identical runtime-visible output

The current gates enforce both properties directly in tests.

## Current gate set

### 1) Compile-twice bytecode identity

Test: `tests/cpp/e2e_compile_determinism_test.cpp`

The gate compiles one source program twice using:

- `t81::cli::build_program_from_source(...)`

Then it verifies:

- `t81::tisc::encode(program_a) == t81::tisc::encode(program_b)`

### 2) Compile-twice hash identity

In the same test, bytecode from each compile pass is hashed with:

- `t81::crypto::sha3_512_hex(...)`

The two digests must be exactly equal.

### 3) Runtime printed output identity

The same program is executed twice in the interpreter VM, and both runs must
produce identical:

- `t81::vm::State::printed_output`

The fixture also asserts expected canonical output values.

### 4) Golden fixture pack

The compile/runtime gate is exercised over a fixture set in:

- `tests/fixtures/t81lang_determinism/*.t81`
- `tests/fixtures/t81lang_determinism/*.out`

Each fixture enforces:

- compile pass A == compile pass B (bytecode)
- SHA3-512 hash(pass A) == hash(pass B)
- run output(pass A) == run output(pass B) == golden `.out`

### 5) Binary serialization determinism regression

Test: `tests/cpp/tisc_binary_io_determinism_test.cpp`

This guard asserts that saving the same `tisc::Program` twice produces identical
`.tisc` bytes and identical SHA3-512 hashes, then round-trips through
`load_program(...)` to verify critical instruction fields (including
`literal_kind`) are preserved.

## Local verification commands

Run the full repository ritual:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run only determinism-focused tests:

```bash
ctest --test-dir build -R "e2e_compile_determinism_test|e2e_print_runtime_test|t81_vm_print_test" --output-on-failure
```

Run the CLI reproducibility script used in CI:

```bash
python3 scripts/ci/t81lang_repro_gate.py \
  --t81-bin build/t81 \
  --fixtures-dir tests/fixtures/t81lang_determinism \
  --workdir build/t81lang-repro \
  --hash-out build/t81lang-repro/hash.txt \
  --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
```

The checked-in hash file (`tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt`)
acts as a golden value. CI fails if fixture compile output drifts unexpectedly.

Run the equivalent CLI helper:

```bash
./build/t81 repro-hash
```

Optional fixture directory override:

```bash
./build/t81 repro-hash tests/fixtures/t81lang_determinism
```

Maintainers can refresh the tracked golden hash through:

- `.github/workflows/t81lang-repro-hash-refresh.yml`

## Interpretation

- Pass: compile + runtime determinism contract holds for this fixture.
- Fail at byte equality: compiler pipeline emitted non-canonical program data.
- Fail at hash equality with byte mismatch: same root cause as above.
- Fail at printed output equality only: runtime formatting or value-tag handling
  diverged while bytecode remained stable.

## Hash Mismatch Triage

Use this order to localize drift quickly:

1. CLI compile output:
   - run `t81 compile <fixture>.t81 -o pass_a.tisc` and repeat for `pass_b.tisc`
   - compare with `cmp -l pass_a.tisc pass_b.tisc`
2. In-process encode path:
   - run `e2e_compile_determinism_test`
   - if in-process is stable but CLI is unstable, inspect file serialization
3. Frontend pipeline:
   - lexer/parser diagnostics first
   - then semantic analysis differences
4. IR/binary lowering:
   - check `frontend_ir_generator_test` and `t81_isa_binary_emitter_test`
5. Program serialization:
   - run `t81_isa_binary_io_determinism_test`
   - focus on field-wise writes (avoid raw struct/padding writes)
6. Runtime output only mismatch:
   - run `e2e_print_runtime_test` and `t81_vm_print_test`
   - inspect tag-to-format mapping in VM print path

## Related docs

- `docs/guides/t81lang-print-runtime.md`
- `docs/proposals/t81lang-implementation-plan.md`
- `spec/t81lang-spec.md`
