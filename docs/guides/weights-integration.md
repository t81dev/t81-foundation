# Weights Integration Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Weights Integration Guide](#weights-integration-guide)
  - [Preparing a `.t81w`](#preparing-a-`t81w`)
  - [Calling `weights.load` from T81Lang](#calling-`weightsload`-from-t81lang)
  - [Run the `weights-load` demo](#run-the-`weights-load`-demo)
  - [Hanoi VM & IR support](#hanoi-vm-&-ir-support)
  - [Keeping CLI diagnostics sharp](#keeping-cli-diagnostics-sharp)

<!-- T81-TOC:END -->










This guide explains how to get `.t81w` models through the CLI, consume their tensors inside T81Lang, and how the VM keeps everything zero-copy via `weights.load`.

## Preparing a `.t81w`

1. Use the CLI import helpers to convert the upstream model into the canonical T81 format:
   ```bash
   t81 weights import my-model.gguf -o my-model.t81w
   ```
2. Inspect the resulting file to ensure the tensor names, shapes, trit counts, and checksum match expectations:
   ```bash
   t81 weights info my-model.t81w
   ```
3. During compilation you can pass the `.t81w` straight into the `t81 compile` pipeline; the CLI driver now preserves the accompanying `ModelFile` so `weights.load` can reach those tensors at runtime.

## Calling `weights.load` from T81Lang

After compiling a module that references `weights.load`, any Hanoi VM execution will keep a shared `ModelFile` in `tisc::Program::weights_model`. Inside your code you simply ask Hanoi for a named tensor handle:

```rust
fn main() -> i32 {
    let embed: i32 = weights.load("encoder.value_proj");
    // The handle is reused—calling weights.load twice returns the same handle.
    let embed_again: i32 = weights.load("encoder.value_proj");
    assert(embed == embed_again);
    return 0;
}
```

The returned handle tags the register as a `ValueTag::WeightsTensorHandle` so downstream arithmetic instructions (matrix multiplications, tensor reshapes, etc.) know when they reference the shared model data. No copy is made—the handle points at `State::weights_tensor_refs` bound to the active `ModelFile`.

## Run the `weights-load` demo

The repository now includes `examples/weights_load_demo.t81`, a minimal program that calls `weights.load` for two tensor names and returns the handle. Compile it alongside your `.t81w` model to see the handles in action:

```bash
t81 weights load my-model.gguf -o my-model.t81w
t81 compile examples/weights_load_demo.t81 -o examples/weights_load_demo.tisc \
    --source-name weights_load_demo --weights-model my-model.t81w
t81 run examples/weights_load_demo.tisc
```

Because the CLI driver attaches the annotated `ModelFile` to the emitted `tisc::Program`, the interpreter sees the tensor names and returns cached handles without copying the underlying `NativeTensor` data. The new tests and this demo both ensure repeated `weights.load("encoder.value_proj")` returns the same handle.

## Hanoi VM & IR support

- The IR generator emits a new `WEIGHTS_LOAD` pseudo-opcode for `weights.load`.  
- `BinaryEmitter` interns the string literal into `Program::symbol_pool` and emits the `WeightsLoad` opcode.  
- At runtime, `WeightsLoad` looks up the symbol index, calls `Interpreter::intern_weights_tensor`, and sets the destination register’s tag to `ValueTag::WeightsTensorHandle`.

Reusing handles avoids duplicating large tensors in `State::tensors`. The new `tests/cpp/weights_load_test.cpp` verifies the handle is cached and that the lookup reads from the shared `ModelFile::native` map without copies.

## Keeping CLI diagnostics sharp

- Compilation errors now include `file:line:column` and work the same for generated modules coming from `.t81w`.  
- Before shipping a release, run the mandated checks (build + `ctest`) and exercise `./build/t81 compile path/to/invalid.t81` to ensure diagnostics report the expected activity and the new handle logic is covered.

See `docs/benchmarks.md` and `docs/guides/vm-opcodes.md` for corresponding opcode details and runtime path information.
