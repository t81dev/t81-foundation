# T81 Data Types Overview

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

```

Use `scripts/run-demos.sh` (which now runs match, primitive, fraction, tensor, bigint, float, string, vector, matrix, cell, and quaternion demos) to execute the entire suite in sequence.

## Next steps

- Add more vector/matrix demos that perform shape-aware arithmetic (broadcasting, reshaping) and hook them into this guide.
- Experiment with advanced `T81Fraction` operations (normalization, comparisons) by modifying `examples/fraction_demo.t81`.
- Build demos for other core handles (`T81Graph`, `T81Stream`, `T81Promise`, etc.) using this page and the scripts as templates so the evolution stays systematic.

You should see the CLI report “Compilation successful” followed by “Program terminated normally,” and the HanoiVM registers will contain the computed sum.

## Expand the Playground

- Reuse this example as a template to try other native types (e.g., `T81Fraction`, vectors) by introducing new functions and accumulating their results in `main`.
- Use `match` to deconstruct handles like `Option`, `Result`, or future sequencers such as `Tensor` handles once the language supports them.
