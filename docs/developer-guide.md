
---

```markdown
# T81 Foundation  
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

## 3.1 Base-81 BigInt
- Represent using a vector of balanced trits  
- Always normalize sign, remove leading zeros  
- Implement canonical add/sub/mul/div/mod  
- MUST NOT use native float approximations

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

* **CMake**

  ```bash
  cmake -S . -B build -DT81_BUILD_EXAMPLES=ON -DT81_BUILD_TESTS=ON
  cmake --build build
  ```

  Targets:

  * `t81` (INTERFACE library)
  * `t81_demo` example
  * Unit tests: `t81_*_test`

* **Bazel**

  ```
  bazel test //:t81_*_test
  bazel run  //:t81_demo
  ```

* **Make (shim)**

  ```bash
  make
  make run-tests
  ```

### Data Types

* **`T243BigInt`** (signed base-243 big integer)

  * `add`, `sub`, `mul`, `mod`, `gcd`, `cmp_abs`, `to_string`
  * Construction helpers: `from_ascii(...)` (placeholder encoding)
* **`T81Fraction`** (signed rationals)

  * Invariants: denominator > 0, reduced by `gcd`, zero is `0/1`
  * Ops: `add`, `sub`, `mul`, `div`, `to_string`
* **`T729Tensor`** (row-major tensor)

  * Core: rank/shape/data; `contract_dot`, `transpose`, `broadcast`
  * Ops (in `t81/tensor/ops.hpp`): `transpose(...)`, `slice2d(...)`, `reshape(...)`

### IO Utilities

* **Tensor text IO** (`t81/io/tensor_loader.hpp`)

  * Format:

    ```
    RANK D1 ... DR
    v0 v1 v2 ...
    ```
  * APIs: `load_tensor_txt(_file)`, `save_tensor_txt(_file)`
* **CanonFS wire IO** (`t81/canonfs_io.hpp`)

  * Encode/decode `CanonRef` to a fixed 99-byte little-endian buffer.
  * Helper: `permissions_allow(perms, mask)`

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

* Canonical vectors reused from `tests/harness/canonical/*.json`.
* C++ unit tests under `tests/cpp/`:

  * `bigint_roundtrip.cpp`, `fraction_roundtrip.cpp`
  * `tensor_{transpose,slice,reshape,loader}_test.cpp`
  * `canonfs_io_test.cpp`

### Migration Notes

* Existing `include/t81/t81.h` remains for C; new C++ includes are header-only.
* Legacy `.cweb` modules map to focused headers under `include/t81/` and small `.cpp` files under `src/` for IO-only utilities.
* Prefer including the umbrella header for app code; include specific headers for library code to minimize build times.

```
```

---
