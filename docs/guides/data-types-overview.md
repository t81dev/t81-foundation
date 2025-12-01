# T81 Data Types Overview

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Data Types Overview](#t81-data-types-overview)
  - [Categories Covered](#categories-covered)
  - [Example: `examples/data_types.t81`](#example-`examplesdata_typest81`)
  - [Run the Demos](#run-the-demos)
  - [Highlights: High-Rank Tensor and Graph Demos](#highlights-high-rank-tensor-and-graph-demos)
  - [Next steps](#next-steps)
  - [Blueprint: Handle-heavy Types (Streams / Promises / Agents)](#blueprint-handle-heavy-types-streams--promises--agents)
  - [Expand the Playground](#expand-the-playground)

<!-- T81-TOC:END -->






































This guide introduces a curated set of T81 data types through a runnable example (`examples/data_types.t81`). The goal is to show how primitives, strings, and structural types behave once compiled with the `t81` CLI.

## Categories Covered

| Category | Types | Highlights |
|----------|-------|------------|
| Primitives | `i32`, `T81Float`, `T81String` | Basic arithmetic, floating-point adjustments, and string literals |
| Structural | `Option[T]`, `Result[T, E]` | Pattern-match both constructors and inspect the canonical TISC branches |

## Example: `examples/data_types.t81`

```t81
fn demo_primitives() -> i32 { ... }
fn demo_structural() -> i32 { ... }
fn main() -> i32 { ... }
```

The example showcases:

- Balanced ternary integers and floats combined in `demo_primitives()`.
- A string literal (`"primitives"`) assigned to `T81String`.
- `Option[i32]` and `Result[i32, T81String]` usages with exhaustive `match` expressions.

## Run the Demos

```bash
./build/t81 compile examples/data_types.t81 -o /tmp/data_types.tisc
./build/t81 run /tmp/data_types.tisc

./build/t81 compile examples/fraction_demo.t81 -o /tmp/fraction_demo.tisc
./build/t81 run /tmp/fraction_demo.tisc

./build/t81 compile examples/tensor_demo.t81 -o /tmp/tensor_demo.tisc
./build/t81 run /tmp/tensor_demo.tisc

./build/t81 compile examples/bigint_demo.t81 -o /tmp/bigint_demo.tisc
./build/t81 run /tmp/bigint_demo.tisc

./build/t81 compile examples/float_demo.t81 -o /tmp/float_demo.tisc
./build/t81 run /tmp/float_demo.tisc

./build/t81 compile examples/string_demo.t81 -o /tmp/string_demo.tisc
./build/t81 run /tmp/string_demo.tisc

./build/t81 compile examples/vector_demo.t81 -o /tmp/vector_demo.tisc
./build/t81 run /tmp/vector_demo.tisc

./build/t81 compile examples/matrix_demo.t81 -o /tmp/matrix_demo.tisc
./build/t81 run /tmp/matrix_demo.tisc

./build/t81 compile examples/cell_demo.t81 -o /tmp/cell_demo.tisc
./build/t81 run /tmp/cell_demo.tisc

./build/t81 compile examples/quaternion_demo.t81 -o /tmp/quaternion_demo.tisc
./build/t81 run /tmp/quaternion_demo.tisc

./build/t81 compile examples/high_rank_tensor_demo.t81 -o /tmp/high_rank_tensor_demo.tisc
./build/t81 run /tmp/high_rank_tensor_demo.tisc

./build/t81 compile examples/graph_demo.t81 -o /tmp/graph_demo.tisc
./build/t81 run /tmp/graph_demo.tisc
```

Use `scripts/run-demos.sh` (which now runs match, primitive, fraction, tensor, bigint, float, string, vector, matrix, cell, quaternion, high-rank tensor, and graph demos) to execute the entire suite in sequence.

## Highlights: High-Rank Tensor and Graph Demos

- `examples/high_rank_tensor_demo.t81` declares a `Tensor[i32, 2, 2, 2]` literal, performs nested indexing to reach opposite corners of the block, and returns the balanced ternary sum so you can see deterministic reductions across three dimensions.
- `examples/graph_demo.t81` treats an adjacency matrix as a `Matrix[i32, 3, 3]`, reads pairs of matrix entries that represent edges, and totals them to demonstrate how graphs can be encoded as tensors in T81Lang.

## Next steps

- Add more vector/matrix demos that perform shape-aware arithmetic (broadcasting, reshaping) and hook them into this guide.
- Experiment with advanced `T81Fraction` operations (normalization, comparisons) by modifying `examples/fraction_demo.t81`.
- Build demos for other core handles (`T81Graph`, `T81Stream`, `T81Promise`, etc.) using this page and the scripts as templates so the evolution stays systematic.

## Blueprint: Handle-heavy Types (Streams / Promises / Agents)

The core handles in `include/t81/core` (notably `T81IOStream`, `T81Promise<T>`, and `T81Agent`) require future language bindings. Here is what a T81Lang demo *could* look like once these constructs are available:

```t81
fn stream_log() {
    let log = stream::stdout();
    log << "Starting deterministic log\n";
    log << current_time();
}

fn await_prediction() -> T81String {
    let promise: T81Promise[T81String] = dream::compute_meaning_of_life();
    return promise.await(T81Entropy::acquire_batch(64));
}

fn agent_loop(agent: T81Agent) {
    agent.observe(symbols::SELF_PRESERVATION);
    agent.act();
    let reflection = agent.reflect();
    log << "Agent intent: " << reflection.intent();
}
```

Enable these demos by:

1. Extending the parser/lexer with keywords like `stream::stdout`, `dream::compute_meaning_of_life`, and coroutine syntax for `await`.
2. Lowering `T81Promise`/`T81Agent` semantics to the IR with explicit entropy/fuel operands and serialization hooks so the HanoiVM can enforce deterministic accounting.
3. Wiring the CLI demos (`scripts/run-demos.sh`) to compile/run these sources once the language and IR support the constructs.

For now, treat these snippets as a roadmap: they explain the control flow and thermodynamic accounting we want to expose when the remaining handle APIs are fully implemented.

You should see the CLI report “Compilation successful” followed by “Program terminated normally,” and the HanoiVM registers will contain the computed sum.

## Expand the Playground

- Reuse this example as a template to try other native types (e.g., `T81Fraction`, vectors) by introducing new functions and accumulating their results in `main`.
- Use `match` to deconstruct handles like `Option`, `Result`, or future sequencers such as `Tensor` handles once the language supports them.
