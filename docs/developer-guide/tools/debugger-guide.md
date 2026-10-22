# HanoiVM Debugger Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [HanoiVM Debugger Guide](#hanoivm-debugger-guide)
  - [Usage](#usage)
  - [Commands](#commands)
    - [Execution Control](#execution-control)
    - [Breakpoints & Watchpoints](#breakpoints-&-watchpoints)
    - [Inspection](#inspection)
    - [Help](#help)
  - [Advanced Features](#advanced-features)
    - [Policy Breakpoints](#policy-breakpoints)
    - [Watchpoints](#watchpoints)
  - [Example Session](#example-session)

<!-- T81-TOC:END -->


The `t81 debug` command provides a built-in interactive debugger for HanoiVM, allowing you to step through instructions, inspect memory and registers, and set various types of breakpoints.

## Usage

```bash
t81 debug <program_file>
```

The debugger supports both source files (`.t81`) and compiled bytecode (`.tisc`).

## Commands

### Execution Control

- `s`: **Step** one instruction. Executes the current instruction and stops at the next one.
- `c`: **Continue** execution until the next breakpoint, watchpoint, or program halt.
- `q`: **Quit** the debugger.

### Breakpoints & Watchpoints

- `b <pc>`: Set a **breakpoint** at the specified program counter (PC). Execution will pause when the PC reaches this value.
- `w <addr>`: Set a **watchpoint** at the specified memory address. Execution will pause when the value at this address changes.
- `p <pattern>`: Set a **policy breakpoint**. Execution will pause when an Axion policy log event matches the given pattern string.

### Inspection

- `r`: Print **registers**. Displays:
    - Standard registers R0-R8.
    - Any other non-zero registers (up to R242).
    - Program Counter (PC).
    - Stack Pointer (SP).
    - Flags (Z: Zero, N: Negative, P: Positive).
- `k`: Print the **stack**. Displays the top 10 elements on the stack.
- `m <addr>`: Print **memory** at the specified address.
- `l`: **List** surrounding instructions. Shows the current instruction and a few instructions before and after it.

### Help

- `h`: Show **help**. Displays the list of available commands.

## Advanced Features

### Policy Breakpoints

Policy breakpoints are unique to the HanoiVM debugger. They allow you to pause execution based on Axion policy events. When a policy decision matches the pattern you provided, the debugger will stop, allowing you to inspect the state at that critical moment.

### Watchpoints

Watchpoints are powerful for tracking down memory corruption or unexpected state changes. By setting a watchpoint on a specific memory address, you can catch exactly when a value is modified.

## Example Session

```text
$ t81 debug my_program.t81
HanoiVM Debugger active. Type 'h' for help.
[   0] LOAD 1, 0, 0
dbg> b 10
Breakpoint set at PC=10
dbg> w 42
Watchpoint set at address 42
dbg> c
Watchpoint hit at address 42: 0 -> 1
dbg> r
Registers:
  R 0: 0
  R 1: 5
  PC: 5
  SP: 1
  Flags: P
dbg> l
   [   4] ADD 1, 2, 3
-> [   5] STORE 1, 42, 0
   [   6] JMP 10, 0, 0
dbg> s
[   6] JMP 10, 0, 0
dbg> c
Breakpoint hit at PC=10
dbg> q
```
