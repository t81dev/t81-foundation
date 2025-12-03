# Guide: T81 Data Types Overview

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Guide: T81 Data Types Overview](#guide-t81-data-types-overview)
  - [1. Categories Covered](#1-categories-covered)
  - [2. Example: `examples/data_types.t81`](#2-example-`examplesdata_typest81`)
  - [3. Run the Demos](#3-run-the-demos)
  - [4. Highlights: High-Rank Tensor and Graph Demos](#4-highlights-high-rank-tensor-and-graph-demos)
  - [5. Next Steps](#5-next-steps)

<!-- T81-TOC:END -->

This guide introduces a curated set of T81 data types through a runnable example (`examples/data_types.t81`). The goal is to show how primitives, strings, and structural types behave once compiled with the `t81` CLI.

## 1. Categories Covered

| Category | Types | Highlights |
|----------|-------|------------|
| Primitives | `T81Int`, `T81Float`, `T81String` | Basic arithmetic, floating-point adjustments, and string literals |
| Structural | `Option[T]`, `Result[T, E]` | Pattern-match both constructors and inspect the canonical TISC branches |

## 2. Example: `examples/data_types.t81`

```t81
fn demo_primitives() -> T81Int { ... }
fn demo_structural() -> T81Int { ... }
fn main() -> T81Int { ... }
```

The example showcases:

- Balanced ternary integers and floats combined in `demo_primitives()`.
- A string literal (`"primitives"`) assigned to `T81String`.
- `Option[T81Int]` and `Result[T81Int, T81String]` usages with exhaustive `match` expressions.

## 3. Run the Demos

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

## 4. Highlights: High-Rank Tensor and Graph Demos

- `examples/high_rank_tensor_demo.t81` declares a `Tensor[T81Int, 2, 2, 2]` literal, performs nested indexing to reach opposite corners of the block, and returns the balanced ternary sum so you can see deterministic reductions across three dimensions.
- `examples/graph_demo.t81` treats an adjacency matrix as a `Matrix[T81Int, 3, 3]`, reads pairs of matrix entries that represent edges, and totals them to demonstrate how graphs can be encoded as tensors in T81Lang.

## 5. Next Steps

- Add more vector/matrix demos that perform shape-aware arithmetic (broadcasting, reshaping).
- Experiment with advanced `T81Fraction` operations (normalization, comparisons) by modifying `examples/fraction_demo.t81`.
- Build demos for other core data types to expand the example suite.
