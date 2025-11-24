______________________________________________________________________

title: T81 Foundation Specification — T81VM
nav:

- [Overview](t81-overview.md)
- [Data Types](t81-data-types.md)
- [TISC Specification](tisc-spec.md)
- [T81 Virtual Machine](t81vm-spec.md)
- [T81Lang](t81lang-spec.md)
- [Axion Kernel](axion-kernel.md)
- [Cognitive Tiers](cognitive-tiers.md)

______________________________________________________________________

[← Back to Spec Index](index.md)

# T81 Virtual Machine Specification

Version 0.2 — Draft (Standards Track)

Status: Draft → Standards Track\
Applies to: TISC, T81Lang, Axion, Cognitive Tiers

The T81 Virtual Machine (T81VM) is the **deterministic execution environment** for TISC programs.\
This document defines the execution modes, determinism constraints, concurrency model, memory model, and Axion integration.

______________________________________________________________________

## 0. Scope

This specification defines:

- how TISC programs are loaded and executed
- the abstract machine state and its evolution
- the logical layout and semantics of VM memory
- rules for deterministic concurrency
- how Axion observes and governs execution

It is **normative** for all T81VM implementations.

______________________________________________________________________

## 1. Execution Modes

T81VM MUST support at least one of the following execution modes; both are recommended:

1. **Interpreter Mode**

   - Executes TISC instructions one at a time.
   - Directly applies the semantic rules in the TISC specification.
   - Serves as the reference implementation.

1. **Deterministic JIT Mode**

   - Translates blocks of TISC instructions into native code.
   - MUST preserve the exact observable semantics of the interpreter.
   - Any optimization MUST NOT change:
     - register and memory values
     - fault behavior
     - Axion-visible trace ordering

### 1.1 Mode Selection

- Mode selection (interpreter vs JIT) MAY be static or dynamic.
- The choice of mode MUST NOT change observable program behavior.
- Axion MUST be able to query which mode is active.

### 1.2 Execution Lifecycle

A typical program lifecycle is:

1. Load program and initial state.
1. Initialize memory segments and Axion metadata.
1. Execute until:
   - `HALT` instruction, or
   - deterministic fault, or
   - Axion veto / termination.

On termination, Axion MUST receive a final state snapshot.

______________________________________________________________________

## 2. Determinism Constraints

Determinism is a **hard requirement**.

Implementations MUST ensure:

1. **Same Inputs → Same Outputs**\
   Given:

   - identical program bytes
   - identical initial VM state
   - identical Axion policies and inputs\
     all executions MUST produce the same final state and traces.

1. **No Hidden Sources of Nondeterminism**

   - No direct access to wall-clock time, random devices, or system entropy sources.
   - Any pseudo-random behavior MUST be seeded deterministically and exposed via explicit APIs, not hidden in VM behavior.

1. **Stable Ordering**

   - Instruction execution order MUST be well-defined.
   - Concurrency scheduling (Section 3) MUST follow a deterministic policy.

1. **Canonical State Representation**

   - All values in registers and memory MUST adhere to the canonicalization rules in the Data Types spec.
   - Non-canonical values MUST either be normalized or rejected with a fault.

______________________________________________________________________

## 3. Concurrency Model

The T81VM concurrency model is **message-passing and deterministic**.

### 3.1 Threads and Contexts

T81VM MAY support multiple **execution contexts** (threads or lightweight processes) with:

- separate register sets (`R`, `PC`, `SP`, `FLAGS`)
- shared or partially shared memory segments
- Axion-supervised scheduling

### 3.2 Scheduling

- Scheduling MUST be deterministic given the same initial conditions.
- Round-robin, fixed-priority, or static schedule tables are RECOMMENDED.
- Scheduling decisions MUST be Axion-visible (e.g., recorded in META/trace).

### 3.3 Communication

- Contexts MUST communicate only via:

  - shared memory locations with well-defined ownership, or
  - explicit message queues / channels.

- Message operations MUST be:

  - blocking or non-blocking as defined by future extensions, but
  - always deterministic with respect to ordering and availability.

### 3.4 Race Freedom

To maintain determinism:

- T81VM MUST guarantee **data race freedom** at the semantic level.
- If an implementation allows low-level races internally, they MUST NOT affect the observable state:
  - results MUST match a well-defined serialization of operations.

If conflicting writes would occur in an implementation, the behavior MUST be defined as a deterministic fault or resolved via a deterministic tie-breaking rule.

______________________________________________________________________

## 4. Memory Model

The T81VM memory model defines how memory is structured, addressed, and manipulated.\
It is the core of deterministic execution and safe interoperability.

### 4.1 Segments

Memory is divided into logical segments:

1. **CODE**

   - Read-only executable instructions.
   - Writable only via privileged loader or Axion-verified transformations.
   - Normal TISC code MUST NOT modify CODE.

1. **STACK**

   - Call frames, local variables, return addresses.
   - Grows/shrinks in a single, deterministic direction.
   - Each context has its own STACK region.

1. **HEAP**

   - Dynamically allocated objects and structures.
   - Managed by the VM’s allocation and GC subsystem.
   - Shared among contexts unless otherwise specified.

1. **TENSOR**

   - Dedicated region for tensor/matrix data.
   - Optimized layout for high-throughput numeric operations.
   - Subject to strict shape and alignment rules.

1. **META**

   - Debug, trace, and Axion metadata.
   - Readable by privileged tools and Axion.
   - Write access restricted and deterministic.

The actual physical layout is implementation-defined, but the **logical semantics** MUST match this specification.

______________________________________________________________________

### 4.2 Addressing

Addresses are logical, base-81 aligned integers interpreted as:

```text
(address_space, offset)
```

Implementations MAY encode this as:

- a flat integer with ranges reserved per segment, or
- a tagged representation.

Requirements:

1. It MUST be possible to determine, for any address, which segment it belongs to (or if it is invalid).
1. Out-of-segment accesses MUST cause a **Bounds Fault**, not wrap-around or undefined behavior.
1. Alignment rules for composite types (vectors, matrices, tensors) MUST follow the Data Types spec.

______________________________________________________________________

### 4.3 Object Model

Objects in HEAP and TENSOR segments MUST follow:

- **Header**:

  - type tag or descriptor
  - size and/or shape
  - optional Axion metadata pointer

- **Body**:

  - canonical values as defined in the Data Types spec

Objects MUST:

- be immutable in type and shape once constructed
- maintain valid, canonical contents at all times
- be subject to GC (Section 4.5)

______________________________________________________________________

### 4.4 Stack Semantics

Each context maintains a stack with:

- frames pushed by `CALL` and removed by `RET`
- local variables and temporaries
- deterministic layout that matches the ABI (to be defined separately)

Requirements:

1. Stack overflow/underflow MUST produce **Stack Faults**.
1. Return addresses MUST always point into CODE; violations MUST fault.
1. Stack contents MUST NOT be accessed outside their valid frame, except by explicit debugging or Axion tools in privileged modes.

______________________________________________________________________

### 4.5 Garbage Collection (GC)

GC MUST be:

- **deterministic** in its effects
- **identity-preserving**: any live object keeps the same semantic identity
- **safe**: no dangling references, no double frees, no leaks that affect determinism

### 4.5.1 Triggering

GC MAY be:

- incremental
- stop-the-world
- concurrent internally

But visible behavior MUST be identical to some deterministic sequence of collection points.

### 4.5.2 Roots

Roots include:

- registers
- stack slots
- well-defined global references in CODE/HEAP/META
- Axion-tracked handles

### 4.5.3 Compaction and Movement

Objects MAY be moved during GC **if and only if**:

- all references are updated atomically and deterministically
- Axion receives a mapping from old to new locations (for debugging and trace consistency)

______________________________________________________________________

### 4.6 Canonicalization in Memory

All values in memory MUST be stored in canonical form:

- non-canonical values decoded from external sources MUST be:

  - normalized before being committed, or
  - rejected with a deterministic fault / error value

Memory writes that would violate canonicalization MUST be intercepted and handled by the VM (potentially via Axion).

______________________________________________________________________

### 4.7 Serialization and Snapshots

T81VM MAY support:

- **snapshots** (full state capture)
- **checkpoints** (partial state capture)

Requirements:

- Serialized form MUST be deterministic given the same state.

- Deserialization MUST recreate state exactly, including:

  - registers
  - memory
  - Axion state
  - META/trace information (up to a defined point)

______________________________________________________________________

### 4.8 Literal Pools and Handle Semantics

T81VM programs include **float**, **fraction**, and **symbol** literal pools
alongside CODE. Loading a program MUST:

1. Copy these pools into the VM state (`state.floats`, `state.fractions`,
   `state.symbols`) before any instruction executes.
1. Initialize all registers that reference non-integer types with **handles**,
   defined as 1-based indices into the corresponding pool.

Requirements:

- Dereferencing a handle that is zero, negative, or beyond the current pool size
  MUST raise `Trap::IllegalInstruction`.
- Arithmetic opcodes (`FADD`…`FRACDIV`) MUST resolve handles, perform canonical
  arithmetic (per Data Types), and append canonical results to the pool unless a
  deterministically equal entry already exists. Deduplication, if implemented,
  MUST follow a stable ordering so observers cannot infer hidden randomness.
- Conversions (`I2F`, `I2FRAC`, etc.) MUST also produce handles and therefore
  extend pools deterministically.
- Axion-visible traces MUST record both the opcode and the resulting handle so
  observers can map back to canonical literals if required.

Symbol pools follow the same handle rules but are primarily used by language
constructs; opcodes that accept symbol handles MUST enforce the same validation
behavior.

______________________________________________________________________

## 5. Axion Interface

Axion supervises the T81VM and MUST be able to:

- observe state transitions
- inspect memory and registers in canonical form
- veto or modify execution under well-defined rules

### 5.1 Observation

T81VM MUST emit a **trace stream** containing:

- executed instruction addresses
- changes to `R`, `PC`, `SP`, `FLAGS`
- memory writes (at least at an abstract, structured level)
- fault events and their categories
- GC events and object movements (if compaction is used)
- context switching and scheduling decisions

This trace is written into META (or a side channel pointed to by META) and is Axion-visible.

### 5.2 Policy Hooks

Before or after:

- executing privileged instructions (AXREAD / AXSET / AXVERIFY)
- performing potentially unsafe memory operations
- executing control flow instructions that cross privilege or tier boundaries

T81VM MUST call Axion-defined hooks which MAY:

- allow
- reject (fault)
- alter the operation within the deterministic rules

### 5.3 Recursion and Tier Control

Axion MUST be able to:

- inspect call depth and recursion patterns
- enforce limits on recursion depth or structural complexity
- promote or demote a computation to/from different cognitive tiers

T81VM MUST provide sufficient metadata (stack structure, call graph hints, etc.) for Axion to make these decisions.

______________________________________________________________________

## 6. Error and Fault Handling

Fault categories are defined in the TISC spec.
T81VM is responsible for:

- detecting faults
- halting normal instruction execution
- preserving state consistency
- informing Axion

### 6.1 State on Fault

On any fault:

1. No partial state updates may remain unaccounted for:

   - Either the instruction has no effect, or
   - It is fully applied and the fault is associated with the resulting state.

1. Fault details MUST be recorded in META / Axion metadata:

   - fault type
   - instruction address
   - operand snapshot
   - relevant memory addresses

1. Axion decides:

   - terminate execution
   - transition to diagnostic mode
   - trigger a cognitive-tier analysis

### 6.2 Interaction With T81Lang

When executing compiled T81Lang programs, T81VM MAY:

- map low-level faults to `Result[T, E]` values at language boundaries
- enforce language-level contracts (e.g., bounds checks) as VM-level faults
- provide structured error information to the language runtime

______________________________________________________________________

## 7. Interoperability Summary

T81VM MUST:

- enforce [Data Types](t81-data-types.md) canonicalization in memory
- execute [TISC](tisc-spec.md) instructions deterministically
- serve as the target of [T81Lang](t81lang-spec.md) compilation
- honor [Axion](axion-kernel.md) policies for safety and optimization
- provide stable substrates for [Cognitive Tiers](cognitive-tiers.md) to reason over execution

______________________________________________________________________

# Cross-References

## Overview

- **Execution Layer Position** → [`t81-overview.md`](t81-overview.md#2-architectural-layers)
- **Determinism Principles** → [`t81-overview.md`](t81-overview.md#3-determinism-requirements)

## Data Types

- **Runtime Representation Requirements** → [`t81-data-types.md`](t81-data-types.md#2-primitive-types)
- **Composite Structure Execution Rules** → [`t81-data-types.md`](t81-data-types.md#3-composite-types)
- **Canonical Forms at Runtime** → [`t81-data-types.md`](t81-data-types.md#5-canonicalization-rules-critical-normative-section)

## TISC

- **Instruction Execution Semantics** → [`tisc-spec.md`](tisc-spec.md#5-opcode-classes)
- **State Machine Model** → [`tisc-spec.md`](tisc-spec.md#1-machine-model)
- **Fault Semantics → VM Handling** → [`tisc-spec.md`](tisc-spec.md#6-fault-semantics)

## T81Lang

- **Compiler → VM Pipeline** → [`t81lang-spec.md`](t81lang-spec.md#5-compilation-pipeline)
- **Type Behavior in Execution** → [`t81lang-spec.md`](t81lang-spec.md#3-type-system)
- **Purity / Effects at Runtime** → [`t81lang-spec.md`](t81lang-spec.md#1-language-properties)

## Axion

- **Axion Trace Hooks** → [`axion-kernel.md`](axion-kernel.md#2-subsystems)
- **Recursion and Safety Vetoes** → [`axion-kernel.md`](axion-kernel.md#3-recursion-controls)
- **Optimization and Verification of Execution** → [`axion-kernel.md`](axion-kernel.md#1-responsibilities)

## Cognitive Tiers

- **VM State as Cognitive Input** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)
- **Deterministic Execution as Tier Prerequisite** → [`cognitive-tiers.md`](cognitive-tiers.md#2-constraints)

______________________________________________________________________
