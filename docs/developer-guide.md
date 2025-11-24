______________________________________________________________________

# T81 Foundation  

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation](#t81-foundation)
- [1. Overview of the Stack](#1-overview-of-the-stack)
- [2. Implementation Order (Recommended)](#2-implementation-order-recommended)
- [3. Implementing Canonical Data Types](#3-implementing-canonical-data-types)
  - [3.1 Base-81/Base-243 BigInt](#31-base-81base-243-bigint)
  - [3.2 Fractions](#32-fractions)
  - [3.3 Floats](#33-floats)
  - [3.4 Vectors, Matrices, Tensors](#34-vectors-matrices-tensors)
  - [3.5 Structural Types (Option / Result)](#35-structural-types-option--result)
- [4. Implementing TISC](#4-implementing-tisc)
  - [4.1 Instruction Decode](#41-instruction-decode)
  - [4.2 State Machine](#42-state-machine)
  - [4.3 Arithmetic Ops](#43-arithmetic-ops)
  - [4.4 Tensor Ops](#44-tensor-ops)
  - [4.5 Privileged Ops (AX*)](#45-privileged-ops-ax*)
- [5. Implementing the T81VM](#5-implementing-the-t81vm)
  - [5.1 Memory Model](#51-memory-model)
  - [5.2 VM Scheduler](#52-vm-scheduler)
  - [5.3 Fault System](#53-fault-system)
  - [5.4 Trace System (Required)](#54-trace-system-required)
- [6. Deterministic Garbage Collector](#6-deterministic-garbage-collector)
- [7. Implementing Axion Kernel Subsystems](#7-implementing-axion-kernel-subsystems)
  - [7.1 DTS — Deterministic Trace](#71-dts-—-deterministic-trace)
  - [7.2 VS — Verification Subsystem](#72-vs-—-verification-subsystem)
  - [7.3 CRS — Constraint Resolution](#73-crs-—-constraint-resolution)
  - [7.4 RCS — Recursion Control](#74-rcs-—-recursion-control)
  - [7.5 TTS — Tier Transition](#75-tts-—-tier-transition)
- [8. Implementing T81Lang](#8-implementing-t81lang)
  - [8.1 Parsing](#81-parsing)
  - [8.2 Type System](#82-type-system)
  - [8.3 IR](#83-ir)
  - [8.4 Codegen](#84-codegen)
- [9. Testing & Verification](#9-testing-&-verification)
  - [9.1 Deterministic Test Harness](#91-deterministic-test-harness)
  - [9.2 Fault Injection Tests](#92-fault-injection-tests)
  - [9.3 Trace Validator](#93-trace-validator)
- [10. Repository Structure for Implementers](#10-repository-structure-for-implementers)
- [11. Implementation Roadmap](#11-implementation-roadmap)
  - [Phase 1 — Core Arithmetic & Data Types](#phase-1-—-core-arithmetic-&-data-types)
  - [Phase 2 — TISC Interpreter](#phase-2-—-tisc-interpreter)
  - [Phase 3 — VM Core + Memory Model](#phase-3-—-vm-core-+-memory-model)
  - [Phase 4 — GC + Trace System](#phase-4-—-gc-+-trace-system)
  - [Phase 5 — Axion Kernel](#phase-5-—-axion-kernel)
  - [Phase 6 — T81Lang Compiler](#phase-6-—-t81lang-compiler)
  - [Phase 7 — Tier Engine](#phase-7-—-tier-engine)
  - [Phase 8 — Optimizations & Formal Proofs](#phase-8-—-optimizations-&-formal-proofs)
- [12. Final Guidance](#12-final-guidance)
  - [T81 C++ API](#t81-c++-api)
    - [Overview](#overview)
    - [Build](#build)
    - [Data Types](#data-types)
    - [IO Utilities](#io-utilities)
    - [C API (Stable ABI)](#c-api-stable-abi)
    - [Testing](#testing)
    - [Migration Notes](#migration-notes)

<!-- T81-TOC:END -->



























Developer Guide / Implementer’s Handbook  
Version 0.1

This guide provides a **practical, engineering-focused companion** to the T81 specification suite.  
Where the specs define *what* the system must do, this handbook explains *how to implement it correctly*.

It is not normative.  
It distills the architectural rules into **actionable developer workflows**.

---

# 1. Overview of the Stack

The T81 Foundation contains five implementation targets:

1. **Data Types** — canonical base-81 representations  
2. **TISC Interpreter/JIT** — the ISA executor  
3. **T81VM** — memory model, stack, GC, Axion hooks  
4. **T81Lang** — compiler to TISC  
5. **Axion Kernel** — supervision, safety, determinism  
6. **Cognitive Tier Engine** — optional higher-level reasoning

This handbook describes the order in which each component should be built.

---

# 2. Implementation Order (Recommended)

The implementation must proceed in the following order:

1. **Data Types** → define all canonical primitives and compound types  
2. **TISC Interpreter** (non-optimized)  
3. **Memory Model** + **VM Core**  
4. **Fault Model** + **Trace System**  
5. **GC System** (deterministic)  
6. **Axion Kernel Subsystems**  
7. **T81Lang Compiler**  
8. **Tier Engine** (optional, advanced)

This order ensures that every layer has a stable deterministic substrate.

---

# 3. Implementing Canonical Data Types

This is the foundation of everything.

## 3.1 Base-81/Base-243 BigInt
- Represent using canonical base-243 digits (MSB-first serialization uses `d0.d1...` with `0<=d<243`); provide base-81 textual form (`0..80` digits) for external encoding.
- Always normalize sign, remove leading zeros (only `0` allowed for zero)
- Implement canonical add/sub/mul/div/mod
- MUST NOT use native float approximations
- For spill-to-disk strategies or edge-case parsing/normalization, consult `legacy/hanoivm/src/lib/hvm-trit-util.cweb`. Its mmap threshold and debug hooks are the behavioral baseline for extreme operands.

## 3.2 Fractions
- Store `(numerator, denominator)` as canonical BigInts  
- Always reduce with GCD  
- Denominator MUST be positive  
- Canonicalization after every operation

## 3.3 Floats
- Deterministic binary or decimal float implementation is allowed  
- BUT: all conversions must canonicalize

## 3.4 Vectors, Matrices, Tensors
- Store shape explicitly  
- Canonicalize shape after load  
- No implicit broadcasting  
- All operations must check shape deterministically

## 3.5 Structural Types (Option / Result)

`Option[T]` and `Result[T, E]` are now part of the canonical type system (see
`spec/t81lang-spec.md §2.3` and RFC-0005/0006):

- `Some/None` and `Ok/Err` are just constructors; they do not perform casts.
- Context matters: `None` is illegal without an expected `Option[T]` type.
- Lowering emits `MAKE_OPTION_*` / `MAKE_RESULT_*` opcodes so the VM can store
  canonical handles (deduplicated by payload tag + value).
- Equality is deterministic because the VM compares handles and payloads.
- Pattern matching uses the new `match (value) { ... }` form. Option matches MUST
  provide both `Some(...)` and `None` arms; Result matches require `Ok(...)` and
  `Err(...)`. The compiler emits `OPTION_IS_SOME`/`RESULT_IS_OK` plus
  `OPTION_UNWRAP`/`RESULT_UNWRAP_{OK,ERR}` so only the selected arm is evaluated.
  Bindings are standard block-scoped locals.

Implementation tips:

1. Treat structural values like floats/fractions: canonical pools + handles.
2. When debugging, inspect `vm->state().options` / `.results`.
3. Policies may place tier limits on structural payload tags—log them in traces.

---

# 4. Implementing TISC

Start with a **simple, direct interpreter**.

## 4.1 Instruction Decode
- Build a table of 81 opcodes  
- Validate encoding before execution  
- Decode faults must stop execution deterministically

## 4.2 State Machine
Use the canonical state:

STATE = (R[27], PC, SP, FLAGS, MEM, META)

## 4.3 Arithmetic Ops
- Use canonical BigInt arithmetic  
- No shortcuts  
- All results MUST be canonical

## 4.4 Tensor Ops
- Tensor ops must follow Data Types shape rules  
- Shape mismatch → Shape Fault  
- Axion-visible fault

## 4.5 Privileged Ops (AX*)
Stub them first:

- `AXREAD` returns placeholder metadata  
- `AXSET` pushes metadata into META  
- `AXVERIFY` returns deterministic “OK”

Axion will replace these later.

---

# 5. Implementing the T81VM

The VM is the heart of determinism.

## 5.1 Memory Model
Segments:

- CODE  
- STACK  
- HEAP  
- TENSOR  
- META  

Each segment requires:

- deterministic allocation  
- deterministic bounds rules  
- deterministic layout  
- deterministic canonicalization

## 5.2 VM Scheduler
Start with:

PC cycles instructions sequentially
No parallelism
No OS signals

Advanced modes add deterministic concurrency later.

## 5.3 Fault System
Implement faults as:

struct Fault {
FaultType type;
uint64_t code;
TraceSnapshot snapshot;
}

A fault MUST stop execution unless Axion overrides.

## 5.4 Trace System (Required)
Trace every:

- instruction  
- register delta  
- memory write  
- GC event  
- AX* op  
- tier transition  

The trace must be deterministic.

---

# 6. Deterministic Garbage Collector

The GC is small but must be **perfectly deterministic**.

Rules:

1. Stop-the-world  
2. Canonical mark & sweep order  
3. Deterministic root set  
4. No fragmentation  
5. Shape-safe for tensors  
6. Axion-visible

GC is one of the hardest components — build it slowly and test exhaustively.

---

# 7. Implementing Axion Kernel Subsystems

Axion has **five subsystems**:

1. **DTS** — Deterministic Trace  
2. **VS** — Verification  
3. **CRS** — Constraint Resolution  
4. **RCS** — Recursion Control  
5. **TTS** — Tier Transition

Build them in this order.

> **Policy language:** Axion now consumes the Axion Policy Language (APL) defined in
> [RFC-0009](../spec/rfcs/RFC-0009-axion-policy-language.md). When wiring subsystems,
> embed policy bytecode into the VM header and feed it through CRS/RCS so policy
> decisions are deterministic and traceable.

---

## 7.1 DTS — Deterministic Trace
- Attach hooks to VM events  
- Log before and after states  
- Log faults  
- Log tier transitions  
- Log GC  
- Log AX* events

All logs must be canonical.

---

## 7.2 VS — Verification Subsystem
VS must check:

- canonical forms  
- shape safety  
- purity and effect boundaries  
- type constraints  
- privilege boundaries  
- memory bounds  

VS is the most frequently invoked subsystem.

---

## 7.3 CRS — Constraint Resolution
CRS enforces:

- tier-appropriate resource ceilings  
- shape constraints  
- algebraic invariants  
- branching rules  

Tie CRS into VM instruction execution.

---

## 7.4 RCS — Recursion Control
RCS tracks:

- recursion depth  
- structural decrease proofs  
- tensor rank growth  
- branching entropy  
- symbolic expansion  

On violation → Tier Fault.

---

## 7.5 TTS — Tier Transition
TTS governs:

- Tier 1 → 2: structural computation  
- Tier 2 → 3: symbolic recursion  
- Tier 3 → 4: analytic reasoning  
- Tier 4 → 5: metareasoning  

Transitions must be deterministic and logged.

---

# 8. Implementing T81Lang

The compiler must be deterministic:

## 8.1 Parsing
- No backtracking  
- No nondeterministic grammar resolution  
- AST must be canonical

## 8.2 Type System
- All types reduce to Data Types  
- Static shape validation  
- Static recursion validation (first pass)

## 8.3 IR
Define a **pure, canonical intermediate representation**:

IR = sequence of canonical operations

## 8.4 Codegen
Lower IR:

IR → TISC sequence

Rules:

- All compilation decisions must be deterministic  
- No ambiguous optimizations  
- No heuristic-based rewrites  
- No nondeterministic lowering paths

---

# 9. Testing & Verification

Implementers must build:

## 9.1 Deterministic Test Harness
- run program twice  
- compare traces  
- differences → violation

## 9.2 Fault Injection Tests
Simulate:

- divide by zero  
- shape mismatch  
- privilege violation  
- recursion collapse  

VM must respond deterministically.

## 9.3 Trace Validator
Ensure:

- same number of trace entries  
- same contents  
- same ordering  

---

# 10. Repository Structure for Implementers

Recommended:

src/
data_types/
tisc/
vm/
axion/
lang/
tiers/
tests/
unit/
integration/
determinism/
tools/
trace-diff/
canonicalizer/

---

# 11. Implementation Roadmap

## Phase 1 — Core Arithmetic & Data Types  
## Phase 2 — TISC Interpreter  
## Phase 3 — VM Core + Memory Model  
## Phase 4 — GC + Trace System  
## Phase 5 — Axion Kernel  
## Phase 6 — T81Lang Compiler  
## Phase 7 — Tier Engine  
## Phase 8 — Optimizations & Formal Proofs

---

# 12. Final Guidance

Implementing T81 is:

- straightforward  
- rigorous  
- demanding  
- deeply structured  
- fundamentally deterministic  

The payoff is a computing system that:

- cannot drift  
- cannot diverge  
- cannot violate shape or type invariants  
- cannot escape supervision  
- cannot behave inconsistently  
- and can support **safe, deterministic cognitive reasoning**.

```

Here’s **`docs/developer-guide.md` (C++ API section to append)**:

````md
## T81 C++ API

### Overview
The modern C++ API lives under `include/t81/`. It is header-only and can be consumed via:
```cpp
#include <t81/t81.hpp>
````

For focused modules:

```cpp
#include <t81/bigint.hpp>
#include <t81/fraction.hpp>
#include <t81/tensor.hpp>
#include <t81/tensor/ops.hpp>      // transpose, slice, reshape
#include <t81/canonfs.hpp>
#include <t81/canonfs_io.hpp>
```

### Build

- **CMake**

  ```bash
  cmake -S . -B build -DT81_BUILD_EXAMPLES=ON -DT81_BUILD_TESTS=ON
  cmake --build build
  ```

  Targets:

  - `t81` (INTERFACE library)
  - `t81_demo` example
  - Unit tests: `t81_*_test`

- **Bazel**

  ```
  bazel test //:t81_*_test
  bazel run  //:t81_demo
  ```

- **Make (shim)**

  ```bash
  make
  make run-tests
  ```

### Data Types

- **`T243BigInt`** (signed base-243 big integer)

  - `add`, `sub`, `mul`, `mod`, `gcd`, `cmp_abs`, `to_string`
  - Construction helpers: `from_ascii(...)` (placeholder encoding)

- **`T81Fraction`** (signed rationals)

  - Invariants: denominator > 0, reduced by `gcd`, zero is `0/1`
  - Ops: `add`, `sub`, `mul`, `div`, `to_string`

- **`T729Tensor`** (row-major tensor)

  - Core: rank/shape/data; `contract_dot`, `transpose`, `broadcast`
  - Ops (in `t81/tensor/ops.hpp`): `transpose(...)`, `slice2d(...)`, `reshape(...)`

### IO Utilities

- **Tensor text IO** (`t81/io/tensor_loader.hpp`)

  - Format:

    ```
    RANK D1 ... DR
    v0 v1 v2 ...
    ```

  - APIs: `load_tensor_txt(_file)`, `save_tensor_txt(_file)`

- **CanonFS wire IO** (`t81/canonfs_io.hpp`)

  - Encode/decode `CanonRef` to a fixed 99-byte little-endian buffer.
  - Helper: `permissions_allow(perms, mask)`

### C API (Stable ABI)

Legacy C callers can link against the thin C façade:

```c
#include "src/c_api/t81_c_api.h"

t81_bigint a = t81_bigint_from_ascii("123");
char* s = t81_bigint_to_string(a);  // "..."
t81_bigint_free(a);
free(s);
```

### Testing

- Canonical vectors reused from `tests/harness/canonical/*.json`.

- C++ unit tests under `tests/cpp/`:

  - `bigint_roundtrip.cpp`, `fraction_roundtrip.cpp`
  - `tensor_{transpose,slice,reshape,loader}_test.cpp`
  - `canonfs_io_test.cpp`

### Migration Notes

- Existing `include/t81/t81.h` remains for C; new C++ includes are header-only.
- Legacy `.cweb` modules map to focused headers under `include/t81/` and small `.cpp` files under `src/` for IO-only utilities.
- Prefer including the umbrella header for app code; include specific headers for library code to minimize build times.

```
```

______________________________________________________________________
