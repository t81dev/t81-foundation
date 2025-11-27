# Developer Guide: Adding a TISC Opcode to the VM

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Developer Guide: Adding a TISC Opcode to the VM](#developer-guide-adding-a-tisc-opcode-to-the-vm)
    - [Understanding the TISC Architecture](#understanding-the-tisc-architecture)
    - [Step 1: Define the New Opcode](#step-1-define-the-new-opcode)
      - [1.1 Add the Opcode to the Enum](#11-add-the-opcode-to-the-enum)
      - [1.2 Update Specification (Crucial!)](#12-update-specification-crucial!)
    - [Step 2: Implement the Opcode in the VM](#step-2-implement-the-opcode-in-the-vm)
      - [2.1 Find the Dispatch Loop](#21-find-the-dispatch-loop)
      - [2.2 Add the Implementation Case](#22-add-the-implementation-case)
    - [Step 3: Write a Test](#step-3-write-a-test)
      - [3.1 Create a Test File](#31-create-a-test-file)
      - [3.2 Write the Test Code](#32-write-the-test-code)
      - [3.3 Add the Test to CMake](#33-add-the-test-to-cmake)
    - [Conclusion](#conclusion)

<!-- T81-TOC:END -->























This guide provides a conceptual walkthrough for adding a new instruction (opcode) to the TISC (Ternary Instruction Set Computer) and its reference implementation, the HanoiVM.

We will use the example of adding a hypothetical `MOD` (modulo) instruction, which builds upon the example in `adding-a-language-feature.md`.

______________________________________________________________________

### Understanding the TISC Architecture

The TISC is the low-level assembly language that all high-level T81Lang code is compiled into. The virtual machine (VM) executes this instruction set. The core components you will interact with are:

- **`include/t81/tisc/opcodes.hpp`**: Defines the `Opcode` enum, which lists all available instructions.
- **`src/vm/vm.cpp`**: The main implementation of the VM, containing the execution loop (often called the "dispatch loop") that interprets the TISC bytecode.
- **TISC IR (`tisc::Instruction`)**: The in-memory representation of an instruction, consisting of an opcode and its operands.

______________________________________________________________________

### Step 1: Define the New Opcode

The first step is to formally define the new opcode.

#### 1.1 Add the Opcode to the Enum

Open `include/t81/tisc/opcodes.hpp` and add `MOD` to the `Opcode` enum. It's crucial to place it correctly, typically grouped with other arithmetic operations.

```cpp
// in enum class Opcode
// ...
ADD,  // Addition
SUB,  // Subtraction
MUL,  // Multiplication
DIV,  // Division
MOD,  // Modulo <-- Add this line
// ...
```

#### 1.2 Update Specification (Crucial!)

In a real-world scenario, you would now be required to update the formal TISC specification in `spec/tisc-spec.md`. This involves:

1. Assigning a unique binary encoding for the new opcode.
2. Defining its precise semantics (e.g., how it handles division by zero, negative numbers).
3. Specifying its operand types (e.g., registers, immediate values).

For this guide, we will skip the formal spec update, but it is a non-negotiable step in the project's development process.

______________________________________________________________________

### Step 2: Implement the Opcode in the VM

Now, we will implement the logic for the `MOD` instruction within the VM's execution loop.

#### 2.1 Find the Dispatch Loop

Open `src/vm/vm.cpp`. Find the main execution loop, which is typically a large `switch` statement that dispatches based on the current instruction's opcode.

#### 2.2 Add the Implementation Case

Add a new `case` to the `switch` statement for `Opcode::MOD`. The logic inside this case will:

1. Fetch the operands (source registers `a` and `b`, and the destination register `dest`).
2. Perform the modulo operation.
3. Store the result in the destination register.
4. Advance the program counter (PC).

```cpp
// in VM::run() or similar method in src/vm/vm.cpp

// ...
switch (instruction.opcode) {
    // ...
    case Opcode::MUL: {
        // ... implementation for multiplication ...
        break;
    }
    case Opcode::MOD: { // <-- Add this new case
        // Assuming the instruction format is MOD dest, src_a, src_b
        auto dest  = instruction.operands[0].as_register();
        auto src_a = instruction.operands[1].as_register();
        auto src_b = instruction.operands[2].as_register();

        // Fetch values from registers
        auto val_a = _registers[src_a.index];
        auto val_b = _registers[src_b.index];

        // --- Core Semantic Logic ---
        // A real implementation must handle division by zero and other
        // edge cases according to the spec.
        if (val_b.is_zero()) {
            // Trigger a VM trap/fault as per the spec.
            return VMStatus::TrapDivisionByZero;
        }
        _registers[dest.index] = val_a % val_b; // Placeholder logic
        // --- End Core Semantic Logic ---

        _pc++; // Advance the program counter
        break;
    }
    // ...
}
// ...
```

______________________________________________________________________

### Step 3: Write a Test

As with any new feature, a dedicated test is essential. For a new opcode, you would typically write a low-level VM test.

#### 3.1 Create a Test File

Create a new file in `tests/cpp/` named `vm_mod_test.cpp`.

#### 3.2 Write the Test Code

The test should:

1. Manually construct a `tisc::Program` containing the new `MOD` instruction.
2. Initialize the source registers with known values.
3. Create a VM instance and execute the program.
4. Assert that the destination register contains the correct result.

```cpp
// Example test code for tests/cpp/vm_mod_test.cpp
#include "t81/vm/vm.hpp"
#include "t81/tisc/program.hpp"
#include <cassert>

int main() {
    t81::tisc::Program program;
    // Corresponds to: MOD r0, r1, r2
    program.instructions.push_back({
        t81::tisc::Opcode::MOD,
        {t81::tisc::Register{0}, t81::tisc::Register{1}, t81::tisc::Register{2}}
    });

    t81::vm::VM vm;
    // Set up initial state: r1 = 10, r2 = 3
    vm.set_register(t81::tisc::Register{1}, t81::core::BigInt(10));
    vm.set_register(t81::tisc::Register{2}, t81::core::BigInt(3));

    // Execute the program
    vm.execute(program);

    // Check the result: r0 should be 1
    auto result = vm.get_register(t81::tisc::Register{0});
    assert(result.value() == 1 && "Modulo operation failed.");

    return 0;
}
```

#### 3.3 Add the Test to CMake

Finally, open the root `CMakeLists.txt` and add a new test target for the VM test, similar to the other `t81_vm_*_test` targets.

______________________________________________________________________

### Conclusion

You have successfully added a new opcode to the TISC instruction set and implemented its behavior in the VM. This low-level development requires careful attention to the formal specification and rigorous testing to ensure the entire T81 stack remains deterministic and correct.
