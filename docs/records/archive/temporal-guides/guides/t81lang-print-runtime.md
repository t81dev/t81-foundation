# T81Lang `print(...)` Runtime Behavior

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang `print(...)` Runtime Behavior](#t81lang-`print`-runtime-behavior)
  - [Scope](#scope)
  - [Contract](#contract)
  - [Execution Model](#execution-model)
  - [Canonical Output Forms](#canonical-output-forms)
  - [Test Coverage](#test-coverage)
  - [Determinism Gate](#determinism-gate)

<!-- T81-TOC:END -->


This guide documents the current deterministic runtime behavior of the T81Lang
`print(...)` builtin.

## Scope

- Frontend semantic contract: `lang/frontend/semantic_analyzer.cpp`
- Frontend IR lowering: `include/t81/frontend/ir_generator.hpp`
- TISC opcode mapping: `core/isa/binary_emitter.cpp`
- VM execution: `core/vm/vm.cpp`

## Contract

- `print` accepts exactly one argument.
- Accepted argument categories in semantic analysis:
  - scalar numeric
  - `bool`
  - `T81String`
- `print` returns `void`.

## Execution Model

The compiler lowers `print(x)` to a dedicated `PRINT` IR opcode and then to
`t81::tisc::Opcode::Print`.

At runtime, `Opcode::Print` does not write to stdout directly. Instead, it
appends a deterministic textual representation to:

- `t81::vm::State::printed_output`

This makes output verifiable in tests and reproducible across runs.

## Canonical Output Forms

Current VM formatting rules:

- Integer register value: decimal string (example: `42`)
- Boolean: `true` or `false`
- Float handle: locale-stable decimal with `t81` suffix (example: `1.25t81`)
- Fraction handle: `num/den` with `t81` suffix (example: `22/7t81`)
- Symbol/string handle: symbol text (example: `alpha`)
- Structured handles (tensor/shape/weights/reflection/enum): deterministic
  tagged placeholders (for debugging/traceability)

If a register/tag pair is invalid for formatting, the VM raises a deterministic
`TypeFault`.

## Test Coverage

- IR lowering check:
  - `tests/cpp/frontend_ir_generator_test.cpp`
- Binary emitter opcode mapping:
  - `tests/cpp/tisc_binary_emitter_test.cpp`
- VM print behavior and fault case:
  - `tests/cpp/vm_print_test.cpp`
- End-to-end frontend compile -> VM execution print capture:
  - `tests/cpp/e2e_print_runtime_test.cpp`

## Determinism Gate

`print(...)` output now participates in a compile determinism gate:

- Compile the same source twice via `t81::cli::build_program_from_source(...)`.
- Encode both `t81::tisc::Program` values and assert byte-for-byte equality.
- Hash both bytecode blobs with `SHA3-512` and assert equal digests.
- Run both programs and assert identical `State::printed_output`.

Primary test:

- `tests/cpp/e2e_compile_determinism_test.cpp`

This ensures runtime-visible output is deterministic together with canonical
bytecode generation, not just VM opcode behavior in isolation.
