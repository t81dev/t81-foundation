---
layout: page
title: TISC and VM Guide
---

# TISC and VM Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [TISC and VM Guide](#tisc-and-vm-guide)
  - [1. Architecture](#1-architecture)
    - [Register and Memory Model](#register-and-memory-model)
  - [2. Implemented Opcodes](#2-implemented-opcodes)
    - [Stack/Heap Allocator Opcodes](#stackheap-allocator-opcodes)
    - [Axion Loop Metadata Example](#axion-loop-metadata-example)
  - [3. How to Add a New TISC Opcode](#3-how-to-add-a-new-tisc-opcode)
    - [Step 1: Define the New Opcode](#step-1-define-the-new-opcode)
    - [Step 2: Implement the Opcode in the VM](#step-2-implement-the-opcode-in-the-vm)
    - [Step 3: Write a Test](#step-3-write-a-test)

<!-- T81-TOC:END -->


































































































































This guide provides an overview of the TISC instruction set and the T81 Virtual Machine (VM) that executes it.

**Companion Documents:**
- **Specification:** [`spec/tisc-spec.md`](../../spec/tisc-spec.md), [`spec/t81vm-spec.md`](../../spec/t81vm-spec.md)
- **Key Source Files:**
    - [`include/t81/tisc/opcodes.hpp`](../../include/t81/tisc/opcodes.hpp): The `Opcode` enum.
    - [`src/vm/vm.cpp`](../../src/vm/vm.cpp): The VM implementation.
- **Tests:** `tests/cpp/t81_vm_*_test.cpp`, `tests/cpp/e2e_*_test.cpp`

______________________________________________________________________

## 1. Architecture

The TISC (Ternary Instruction Set Computer) is the low-level, stable assembly language of the T81 ecosystem. The T81 Virtual Machine (VM) is the interpreter that executes TISC bytecode.

- The **T81Lang Frontend** compiles `.t81` source files into TISC.
- The **`BinaryEmitter`** encodes TISC into a compact binary format.
- The **VM** loads and executes this binary format.

This separation ensures that the compiler and the VM are decoupled, connected only by the stable TISC specification.

### Register and Memory Model

- **Registers:** The VM has a flat bank of general-purpose registers. By convention, `r0` is often used for return values.
- **Memory:** The VM's memory model is currently a **simple linear address space**. The full stack and heap model described in the spec is not yet implemented.
- **Program Counter (PC):** A special register, `_pc`, tracks the address of the next instruction to be executed.

______________________________________________________________________

## 2. Implemented Opcodes

The following TISC opcodes are defined in `opcodes.hpp` and implemented in the VM.

| Category | Opcodes |
|---|---|
| **Control Flow** | `Halt`, `Nop`, `Jump`, `JumpIfZero`, `JumpIfNotZero`, `JumpIfNegative`, `JumpIfPositive`, `Call`, `Ret`, `Trap` |
| **Memory** | `LoadImm`, `Load`, `Store`, `Mov`, `Push`, `Pop` |
| **Integer Arithmetic** | `Add`, `Sub`, `Mul`, `Div`, `Mod`, `Inc`, `Dec`, `Cmp`, `Neg` |
| **Ternary Logic** | `TNot`, `TAnd`, `TOr`, `TXor` |
| **Float Arithmetic** | `FAdd`, `FSub`, `FMul`, `FDiv` |
| **Fraction Arithmetic** | `FracAdd`, `FracSub`, `FracMul`, `FracDiv` |
| **Type Conversion** | `I2F`, `F2I`, `I2Frac`, `Frac2I` |
| **Comparison Boolean** | `Less`, `LessEqual`, `Greater`, `GreaterEqual`, `Equal`, `NotEqual` |
| **Tensor Operations** | `TVecAdd`, `TMatMul`, `TTenDot`, `ChkShape` |
| **Option/Result Types** | `MakeOptionSome`, `MakeOptionNone`, `OptionIsSome`, `OptionUnwrap`, `MakeResultOk`, `MakeResultErr`, `ResultIsOk`, `ResultUnwrapOk`, `ResultUnwrapErr` |
| **Axion Interface** | `AxRead`, `AxSet`, `AxVerify` |
| **Allocator / Metadata** | `StackAlloc`, `StackFree`, `HeapAlloc`, `HeapFree`, `WeightsLoad` |

______________________________________________________________________

### Stack/Heap Allocator Opcodes

- `StackAlloc`, `StackFree`, `HeapAlloc`, `HeapFree` encode the deterministic allocator model described in [`spec/t81vm-spec.md`](../../spec/t81vm-spec.md#memory-model). Every stack allocation is bounded by the current stack limit (Axion policy layers may adjust the limit via hints) and must pair `StackAlloc` with a matching `StackFree` within the same lexical scope; missing frees raise deterministic traps rather than silently overflow. Heap allocations obey the same Axion-guided guards—VM state keeps per-program counters and faults when any request would exceed the configured heap cap.
- These opcodes are deterministic, so they avoid hidden nondeterminism: every allocation/deallocation is recorded in the Axion trace, and the Hanoi engine can replay or veto the operation if it would violate the spec's `+∞` / `-∞` invariants. The VM also uses them when lowering `loop`/`match` constructs that require temporary buffers or when the new `weights.load` builtin materializes handles.
- Use `StackAlloc`/`StackFree` for all short-lived temporaries so the Axion trace can track stack depth precisely. Heap paths (`HeapAlloc`/`HeapFree`) are reserved for long-lived state such as tensors stored across invocations or weights handles; the VM tracks these with the same deterministic telemetry used by Axion's policy text and loop metadata.

### Axion Loop Metadata Example

To demonstrate how allocator ops and Axion policy text interplay, consider a simple `loop` in T81Lang:

```
loop {
  let n = 0;
  if (n > 10) break;
  n = n + 1;
}
```

When the frontend lowers this loop, it emits `StackAlloc`/`StackFree` around any temporary `n` values and annotates the loop with a `loop hint` (file/line/column). The HanoiVM produces an Axion policy block such as:

```
(policy
  (tier 1)
  (loop
    (id 0)
    (file examples/demo.t81)
    (line 12)
    (column 5)
    (annotated true)
    (depth 0)
    (bound infinite)))
```

This policy text is emitted whenever `./build/t81 run` executes the TISC program, giving downstream consumers deterministic diagnostics (`file:line:column`) and exposing Axion's loop-tracking hooks. If the loop tries to grow the stack beyond the configured limit, the VM traps before Axion ever allows a `+∞` overflow; the Axion log entry and the policy text show the same metadata used by diagnostics, closing the trace from source to runtime. For a concrete CLI command/output pair you can copy into logs or release notes, see the **Axion CLI Trace Example** in the [demo gallery guide](./demo-gallery.md#axion-loop-trace).

The comparison boolean opcodes produce canonical `0/1` results (always stored as `ValueTag::Int`) instead of relying solely on flag-setting `Cmp`. The frontend IR generator now tags relational expressions with a `ComparisonRelation` so the binary emitter can lower `Less`, `Equal`, etc., directly to these TISC opcodes, keeping the emitter/VM behavior deterministic and easier to trace through tools like `tools/ir_inspector`.

The comparison boolean opcodes write canonical `0/1` values, so a simple relational expression such as:

```
let cmp = 1 < 2;
```

produces the following TISC sequence (after lowering):

```
  LOADI r1 1
  LOADI r2 2
  Less r0 r1 r2  ; emits 1 if r1 < r2, 0 otherwise
```

The frontend IR generator attaches a `ComparisonRelation` to the `CMP` instruction and marks it as a boolean result, allowing the binary emitter to lower it directly to the appropriate HanoiVM opcode instead of relying on flag-based `CMP` + conditional jumps.

## 3. How to Add a New TISC Opcode

This section provides a conceptual walkthrough for adding a new instruction to the TISC and the VM. We will use the example of adding a hypothetical `MOD` (modulo) instruction.

### Step 1: Define the New Opcode

1.  **Add to Enum:** Open `include/t81/tisc/opcodes.hpp` and add your new opcode to the `Opcode` enum.
2.  **Update Specification:** In a real contribution, you must update the formal TISC specification in `spec/tisc-spec.md`. This includes assigning a binary encoding and defining the precise semantics.

### Step 2: Implement the Opcode in the VM

In `src/vm/vm.cpp`, find the main `switch` statement in the `execute` or `run` method and add a new `case` for your opcode. The logic should:
1.  Fetch the operands (registers or immediate values).
2.  Perform the core operation.
3.  Store the result in the destination register.
4.  Handle any potential faults according to the spec (e.g., division by zero).

```cpp
// in VM::execute() in src/vm/vm.cpp
switch (instruction.opcode) {
    // ...
    case Opcode::MOD: {
        auto dest  = instruction.operands[0].as_register();
        auto src_a = instruction.operands[1].as_register();
        auto src_b = instruction.operands[2].as_register();

        auto val_a = _state.registers[src_a.index];
        auto val_b = _state.registers[src_b.index];

        if (val_b.is_zero()) {
            // This should trigger a spec-compliant VM fault.
            trap(TrapCode::DivisionByZero);
            break;
        }
        _state.registers[dest.index] = val_a % val_b;
        break;
    }
    // ...
}
```

### Step 3: Write a Test

1.  **Create a Test File:** Add a new file in `tests/cpp/`, such as `vm_my_opcode_test.cpp`.
2.  **Write the Test:** The test should manually construct a `tisc::Program` with your new instruction, set up the initial VM state (registers), execute the program, and assert that the final state is correct. See `tests/cpp/t81_vm_divmod_test.cpp` for a good example.
3.  **Add to CMake:** Add your new test file as an executable and test target in the root `CMakeLists.txt`.
