# T81Lang Match Demo

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Match Demo](#t81lang-match-demo)
  - [1. Source](#1-source)
  - [2. Compile](#2-compile)
  - [3. Run with the HanoiVM](#3-run-with-the-hanoivm)
  - [4. Implementation Notes](#4-implementation-notes)

<!-- T81-TOC:END -->








































































This guide walks through compiling and running a `match`-heavy T81Lang program using the `t81` CLI so you can see the full frontend → IR → VM pipeline in action.

## 1. Source

Save the following source as `match_demo.t81`:

```t81
fn compute() -> i32 {
    let maybe: Option[i32] = Some(3);
    let value: i32 = match (maybe) {
        Some(v) => v * 2,
        None => 0,
    };

    let artifact: Result[i32, T81String] = Ok(value);
    return match (artifact) {
        Ok(v) => v + 4,
        Err(_) => 0,
    };
}

fn main() -> i32 {
    return compute();
}
```

## 2. Compile

Use the CLI from the build directory to compile the program into TISC bytecode:

```bash
./build/t81 compile match_demo.t81 -o match_demo.tisc
```

Expected output:

```
Compilation successful → match_demo.tisc
```

If you deviate from this command, ensure the `t81` binary in `build/` is up to date and that the source file ends with `.t81`.

## 3. Run with the HanoiVM

Execute the compiled bytecode via the CLI:

```bash
./build/t81 run match_demo.tisc
```

The program should terminate normally, returning `10` because the option path `Some(3)` doubles to `6`, adds `4` in the result match, and exits.

## 4. Implementation Notes

- **Semantic Safeguards:** The analyzer ensures `match` arms cover both variants and produces a consistent result type before emitting IR.
- **IR Lowering:** `OPTION_IS_SOME`/`RESULT_IS_OK` plus `OPTION_UNWRAP`/`RESULT_UNWRAP_*` ensure the HanoiVM sees canonical branching semantics.
- **Binary Emission:** The helper opcodes now map to dedicated TISC instructions so execution stays deterministic across the stack.

Feel free to reuse this sample inside your own tests or onboarding tutorials to demonstrate pattern matching end-to-end.
