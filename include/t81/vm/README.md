# T81 Virtual Machine Headers

This directory contains the public header files for the T81 Virtual Machine (HanoiVM). These headers define the primary interfaces and data structures for interacting with and controlling the VM.

## Key Headers

-   `vm.hpp`: This is the most important header in this directory. It defines the `IVirtualMachine` interface, which is the primary abstraction for the virtual machine. It provides methods for loading programs, executing them (`step()` and `run()`), and inspecting the VM's state. It also contains the `make_interpreter_vm()` factory function, which is the standard way to create a new VM instance.

-   `state.hpp`: Defines the `State` struct, which encapsulates the entire state of the virtual machine at a given moment. This includes the program counter, the register file, and a pointer to the currently loaded `Program`.

-   `traps.hpp`: Defines the enumerations and constants related to the various trap and fault conditions that the VM can encounter. These are used for deterministic error handling and debugging.
