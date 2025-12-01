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
| **Tensor Operations** | `TVecAdd`, `TMatMul`, `TTenDot`, `ChkShape` |
| **Option/Result Types** | `MakeOptionSome`, `MakeOptionNone`, `OptionIsSome`, `OptionUnwrap`, `MakeResultOk`, `MakeResultErr`, `ResultIsOk`, `ResultUnwrapOk`, `ResultUnwrapErr` |
| **Axion Interface** | `AxRead`, `AxSet`, `AxVerify` |

______________________________________________________________________

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
