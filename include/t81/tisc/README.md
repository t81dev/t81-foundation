# TISC Toolchain Headers

This directory contains the public header files for the TISC (Ternary Instruction Set Computer) toolchain. These headers define the core data structures and interfaces for working with the TISC Intermediate Representation (IR) and its binary format.

## Key Headers

-   `ir.hpp`: This is the central header for the TISC IR. It defines the `IntermediateProgram` class, which is a container for a sequence of TISC instructions, as well as the structures for representing instructions, operands, labels, and registers.

-   `opcodes.hpp`: Defines the TISC opcode enumeration, providing a symbolic name for each instruction in the instruction set.

-   `binary_emitter.hpp`: Defines the `BinaryEmitter` class, which is responsible for serializing an `IntermediateProgram` into the TISC binary format.

-   `pretty_printer.hpp`: Declares the `pretty_print` function, a utility for generating a human-readable, textual representation of a TISC `IntermediateProgram`.

-   `program.hpp`: Defines the `Program` struct, which is the in-memory representation of a compiled and loaded TISC binary file. This is the data structure that the T81 Virtual Machine executes.

-   `encoding.hpp`: Provides functions and constants related to the binary encoding of TISC instructions and operands.

-   `binary_io.hpp`: Declares functions for reading and writing TISC binary files.
