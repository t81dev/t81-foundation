# Demo Gallery

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Demo Gallery](#demo-gallery)
  - [1. CLI Demos](#1-cli-demos)
  - [2. Automation Script](#2-automation-script)
  - [3. IR/VM Introspection](#3-irvm-introspection)
  - [Axion Loop Trace](#axion-loop-trace)
    - [Axion CLI Trace Example](#axion-cli-trace-example)
  - [4. What’s Next?](#4-what’s-next?)

<!-- T81-TOC:END -->




























































































































This page lists the runnable demos and utilities available in the repository, giving you quick pointers to the content mentioned elsewhere.

## 1. CLI Demos

- **Match Demo** (`examples/match_demo.t81`): Covers `Option`/`Result` `match` expressions. See [docs/guides/match-example.md](./match-example.md).
- **Data-Type Demo** (`examples/data_types.t81`): Exercises primitives, strings, and structural matches. See [docs/guides/data-types-overview.md](./data-types-overview.md).
- **Fraction Demo** (`examples/fraction_demo.t81`) + **Tensor Demo** (`examples/tensor_demo.t81`): Focused examples for rational numbers and typed tensors; expand the guide to explore them further under “Next Steps.”
- **BigInt, Float, String Demos** (`examples/bigint_demo.t81`, `examples/float_demo.t81`, `examples/string_demo.t81`): Show canonical usage of `T81BigInt`, `T81Float`, and `T81String` values respectively; link back to the overview guide for instructions.
- **Vector, Matrix, Cell, Quaternion Demos** (`examples/vector_demo.t81`, `examples/matrix_demo.t81`, `examples/cell_demo.t81`, `examples/quaternion_demo.t81`): Focus on the next wave of numerics—vectors/matrices for shape-aware operations and handles like cells/quaternions for the balanced ternary core.
- **High-Rank Tensor Demo** (`examples/high_rank_tensor_demo.t81`): Maps a 3D tensor literal and computes a scalar sum via deterministic indexing; run it to see how higher-rank `Tensor` types behave.
- **Graph Demo** (`examples/graph_demo.t81`): Models a simple adjacency matrix and reports total edge counts, demonstrating how graph structures arise as tensor/matrix handles.
- **Weights Load Demo** (`examples/weights_load_demo.t81`): Shows the new `weights.load("<tensor>")` builtin. Compile the `.t81` alongside a `.t81w` model (`t81 weights load my-model.t81w`). Run `./build/t81 compile examples/weights_load_demo.t81 -o examples/weights_load_demo.tisc` and `./build/t81 run examples/weights_load_demo.tisc` to verify the handles are reused without copying.
- **Handle Blueprints** (`T81IOStream`, `T81Promise`, `T81Agent`): The data-type guide contains pseudo-code snippets describing the expected syntax and operations for streams, promises, and agents once the frontend/IR supports them. These blueprints document the entropy fuel parameters and coroutine semantics we’ll need before runnable demos exist.

Compile and run both manually using `./build/t81` or with the helper below.

## 2. Automation Script

`./scripts/run-demos.sh` compiles both examples and runs the resulting TISC programs sequentially, printing the CLI output for each.

```bash
./scripts/run-demos.sh
```

Set `WEIGHTS_MODEL=/path/to/model.t81w` before running the script if you want it to include `examples/weights_load_demo.t81`. The script passes the flag through `t81 compile ... --weights-model` when the file exists.

## 3. IR/VM Introspection

- **IR Inspector** (`tools/ir_inspector`): A lightweight CLI tool that prints the IR instructions emitted for a T81 source file, letting you inspect the generated match lowering before the binary emitter runs. Example usage:

```bash
./build/ir_inspector examples/match_demo.t81
```

The tool runs the usual frontend pipeline (lexer → parser → semantic analyzer → IR generator) and emits each `tisc::ir::Instruction` with operand values, making it easy to see how `match` lowers before the HanoiVM executes anything.

This tool walks the usual frontend pipeline (lexer → parser → semantic analyzer → IR generator) and dumps the resulting `tisc::ir::Instruction` list with operand types and primitive annotations. Comparisons now carry `ComparisonRelation` metadata so the inspector makes it easy to verify when the emitter will produce the new `Less`, `Equal`, etc., boolean opcodes instead of flag-only `CMP` sequences.

- **Benchmark Suite** (`./build/t81 benchmark`): Executes `build/benchmarks/benchmark_runner`, comparing throughput and latency for the T81 cell vs. binary variants, and auto-updates `docs/benchmarks.md` with the execution highlights table.

## Axion Loop Trace

Follow the **Axion Loop Metadata Example** in [`docs/guides/vm-opcodes.md`](./vm-opcodes.md) to see how allocator ops (`StackAlloc`/`StackFree`) and loop hints end up in the Hanoi policy text. Compile and run any loop-bearing demo (e.g., `examples/high_rank_tensor_demo.t81`) with `./build/t81 compile …` and `./build/t81 run …`; the terminal will print a `(policy … (loop … (file …) (line …) (column …)))` block that matches the diagnostics shipped by `t81`. Together, the doc plus this CLI trace show the deterministic path from source loop → Axion policy output.

### Axion CLI Trace Example

As a concrete Axion syscall/loop metadata example, run the weights demo with diagnostics enabled:

```bash
./build/t81 compile examples/weights_load_demo.t81 -o examples/weights_load_demo.tisc
./build/t81 run examples/weights_load_demo.tisc --weights-model model.t81w
```

The HanoiVM prints the loop metadata right before execution:

```
axion loop metadata: file=examples/weights_load_demo.t81 line=12 column=5 bound=infinite annotated=true
(policy (tier 1) (loop (id 0) (file examples/weights_load_demo.t81) (line 12) (column 5) (annotated true) (depth 0) (bound infinite)))
```

Copy this `(policy …)` block into logs or release notes to prove the deterministic trace from the T81 source to Axion’s policy engine; the reported `file:line:column` matches the diagnostics `t81` already prints when a semantic error occurs.

## 4. What’s Next?

Future gallery additions could include:

- Structural type demos for tensors (`Tensor[T]`) or handles like `T81Float` arrays.
- Axion-focused samples that exercise `AXREAD`/`AXSET`.
- Visual helpers for VM traces/flags.

Please file an issue or PR if you’d like to expand the gallery with new samples or tooling.
