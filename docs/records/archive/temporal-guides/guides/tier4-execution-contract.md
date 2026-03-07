# Tier 4 Execution Contract

This document specifies the normative behavior of Tier 4 Cognition within the T81 stack. It defines the semantics of reflection, refinement, and the associated safety boundaries.

## 1. Reflection Semantics

### 1.1 Cognitive Snapshots
The `METAREFLECT` opcode captures a `ReflectionSnapshot`, which includes:
- **Program Counter (PC)**: The current execution point.
- **Register File**: All 243 general-purpose and special registers.
- **Flags**: Zero, Negative, and Positive status.
- **Recent Trace**: A ring buffer of the last 81 `TraceEntry` objects, capturing recent opcodes and traps.
- **CODE Hash**: A deterministic hash of the current instructions in the `CODE` segment.

### 1.2 Reflection Handles
- **Opacity**: Handles are 1-based integers that reference internal snapshot storage.
- **Lifetime**: Handles are valid only within the current execution epoch. They are invalidated if the program is reloaded or the VM is reset.
- **Single-Use**: It is recommended that refinement logic treats handles as single-use to maintain a clear causality chain.

## 2. Refinement Semantics

### 2.1 Refinement Commands
The `METAREFINE` opcode applies a sequence of commands to modify the VM state. Each command is encoded in memory as four 64-bit words:
1. `op`: The operation to perform (0=No-op, 1=WriteCode, 2=WriteReg, 3=WriteMem).
2. `target`: The address or register index.
3. `value`: The new value to write.
4. `tag`: The `ValueTag` associated with the value.

### 2.2 Atomicity
Refinement is **all-or-nothing**. If any command in a refinement block is invalid or denied by policy, none of the commands are applied, and the VM state remains unchanged.

### 2.3 Self-Patching
- `WriteCode` allows the program to modify its own instructions.
- Modifications must be within the `CODE` segment bounds.
- All code writes invalidate the JIT cache for the affected regions.

## 3. Safety and Determinism

### 3.1 Cognitive Budgets
Axion policies enforce limits on:
- `max-reflections`: Maximum number of `METAREFLECT` calls per epoch.
- `max-meta-writes`: Maximum number of `CODE` segment modifications per epoch.

### 3.2 Determinism Guarantees
- All reflection events are recorded in the Axion trace with canonical reason strings.
- Snapshot capture and command application are bit-identical across runs given the same initial state and policy.
- Structured events (`StructuredEvent`) provide machine-readable metadata for each reflection operation.

## 4. Canonical Reason Strings

- `METAREFLECT`: Triggered when capturing a snapshot.
- `METAREFINE`: Triggered when applying refinement commands.
- `cog:tier4:reflect`: High-level cognitive event logged by Tier 4 loops.
- `meta slot axion event`: Logged whenever an Axion event is recorded in the `META` segment.
