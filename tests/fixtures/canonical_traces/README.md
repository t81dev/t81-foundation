# Canonical VM Execution Traces

These files are golden VM execution traces used to verify that the T81 interpreter
produces a bit-identical instruction sequence on every run and platform.

## What a trace contains

Each line is one executed instruction in the format:

```
PC=<address> <OpcodeName> [trap=<TrapKind>]
```

Traces are captured with `t81 vm run <file.tisc> --trace -o <trace.txt>`.

## Verification

To verify that a trace matches on your machine:

```bash
# 1. Build the stack
cmake --preset default -DCMAKE_BUILD_TYPE=Release && cmake --build build

# 2. Compile the matching source fixture
./build/t81 code build tests/fixtures/t81lang_determinism/01_bigint_add.t81 \
  -o /tmp/01_bigint_add.tisc

# 3. Replay and verify
./build/t81 trace replay /tmp/01_bigint_add.tisc \
  tests/fixtures/canonical_traces/01_bigint_add.trace
```

A "Replay successful: traces are bit-identical" result confirms that the VM
execution on your machine matches the recorded golden trace exactly.

## Fixtures

| File | Source | Entries | Coverage |
| :--- | :--- | :--- | :--- |
| `01_bigint_add.trace` | `t81lang_determinism/01_bigint_add.t81` | 13 | T81BigInt arithmetic |
| `02_fraction_sub.trace` | `t81lang_determinism/02_fraction_sub.t81` | 21 | T81Fraction subtraction |
| `03_float_literal.trace` | `t81lang_determinism/03_float_literal.t81` | 11 | T81Float literal |
| `05_bool_and_string.trace` | `t81lang_determinism/05_bool_and_string.t81` | 13 | Bool and string types |
| `08_bounded_loop_print.trace` | `t81lang_determinism/08_bounded_loop_print.t81` | 51 | Bounded loop control flow |

## Automated verification

The `scripts/ci/trace_repro_gate.py` script verifies all fixtures in one pass
and produces an aggregate SHA-256 of the golden trace set:

```bash
python3 scripts/ci/trace_repro_gate.py \
  --t81-bin build/t81 \
  --fixtures-dir tests/fixtures/t81lang_determinism \
  --traces-dir   tests/fixtures/canonical_traces \
  --workdir      build/trace-repro \
  --hash-out     build/trace-repro/hash.txt
```

This gate runs in the `cross-platform-determinism` GitHub Actions workflow on
every push to `main`, comparing trace hashes across Linux x86_64 and macOS ARM64.

## Updating traces

If the VM execution sequence legitimately changes (e.g. new opcode semantics,
ISA version bump), regenerate the golden traces:

```bash
for f in 01_bigint_add 02_fraction_sub 03_float_literal 05_bool_and_string 08_bounded_loop_print; do
  ./build/t81 code build tests/fixtures/t81lang_determinism/${f}.t81 -o /tmp/${f}.tisc
  ./build/t81 vm run /tmp/${f}.tisc --trace -o tests/fixtures/canonical_traces/${f}.trace
done
```

Then commit the updated trace files. Any such update must be accompanied by a
change note explaining why the execution sequence changed.
