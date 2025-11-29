# T81 Virtual Machine (HanoiVM)

This directory contains the source code for the T81 Virtual Machine, also known as the HanoiVM. The VM is a bytecode interpreter that executes programs compiled into the TISC (Ternary Instruction Set Computer) binary format.

## Core Responsibilities

The primary responsibilities of the VM are:

-   **Loading:** Reading a TISC binary file into an in-memory `Program` representation.
-   **Execution:** Interpreting the bytecode instructions one by one and modifying the VM state accordingly.
-   **State Management:** Maintaining the complete state of the executed program, including the program counter, registers, and memory.
-   **Traps & Faults:** Handling exceptional conditions, such as illegal instructions or memory access violations, in a deterministic manner.

## Components

-   `vm.cpp`: The main implementation of the `IVirtualMachine` interface. It contains the primary execution loop (`step()`) that fetches, decodes, and executes a single TISC instruction.

-   `state.hpp` (in `include/t81/vm`): Defines the `State` struct, which encapsulates the entire state of the virtual machine, including its registers and a pointer to the loaded program.

-   `traps.hpp` (in `include/t81/vm`): Defines the various trap and fault conditions that the VM can encounter during execution.

## The Execution Cycle

The VM operates on a simple fetch-decode-execute cycle, which is implemented in the `step()` method:

1.  **Fetch:** The instruction at the current program counter (`pc`) is read from the loaded `Program`.
2.  **Decode:** The opcode is identified, and any associated operands are decoded.
3.  **Execute:** The corresponding operation is performed on the VM's `State` (e.g., an arithmetic operation is performed on the registers). The `pc` is then advanced to the next instruction.
4.  **Repeat:** The cycle repeats until a `HALT` instruction is encountered or a fault occurs.
