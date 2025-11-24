______________________________________________________________________

title: T81 Foundation Specification
nav:

- [Overview](t81-overview.md)
- [Data Types](t81-data-types.md)
- [TISC Specification](tisc-spec.md)
- [T81 Virtual Machine](t81vm-spec.md)
- [T81Lang](t81lang-spec.md)
- [Axion Kernel](axion-kernel.md)
- [Cognitive Tiers](cognitive-tiers.md)

______________________________________________________________________

[← Back to Spec Index](t81-overview.md)

______________________________________________________________________

# **T81 Data Types Specification — Version 0.2 (Normative)**

**Status:** Draft → Standards Track
**Applies To:** T81Lang, TISC, T81VM, Axion, Cognitive Tiers
**Supersedes:** v0.1 Draft
**Purpose:** Define deterministic, canonical, base-81 type semantics for the T81 ecosystem.

______________________________________________________________________

# 0. Scope

This document defines:

- primitive numeric types
- composite structural types
- canonical representation rules
- deterministic arithmetic semantics
- Axion visibility invariants
- VM and ISA interoperability rules

It is **normative** for all runtime, compilation, and cognitive layers.

______________________________________________________________________

# 1. Design Goals (Normative)

All T81 data types MUST satisfy:

1. **Deterministic Semantics**

   - no nondeterministic outcomes
   - all arithmetic, comparisons, and structural operations MUST produce identical results across implementations

2. **Canonical Representation**

   - each representable value MUST have exactly one canonical encoding
   - no alternative or redundant forms permitted

3. **Base-81 Numeric Foundation**

   - all numeric types MUST use balanced-ternary or base-81 semantics
   - internal binary shortcuts are permitted but MUST NOT affect observable behavior

4. **Zero Undefined Behavior**

   - every operation MUST define behavior for all inputs
   - errors MUST resolve to deterministic fault states or `Result[T, E]` representations

5. **Axion Visibility**

   - all canonical forms must be introspectable by the Axion kernel
   - all normalization steps MUST emit metadata hooks

______________________________________________________________________

# 2. Primitive Numeric Types

Primitive types form the base of all T81 computation.

## 2.1 Trit

### Definition

A trit is the fundamental balanced-ternary digit:

```
−1 → T̄  
 0 → T0  
+1 → T1
```

### Representation

Canonical 2-bit balanced encoding (implementation detail, nondeterministic allowed internally).

### Operations

- unary negation
- comparison
- trit-wise logic (AND, OR, XOR, XNOR)

All MUST be deterministic.

______________________________________________________________________

## 2.2 T81BigInt

### Definition

An arbitrary-precision **base-81 integer** with the following constraints:

- digits are base-81 symbols: `0–80`
- sign is stored separately and canonically
- no leading zero digits unless value is exactly zero

### Canonical Form

A value MUST satisfy:

```
- Zero encoded as: [+] [0]
- Nonzero MUST NOT contain leading zeros
- Negative: sign bit set, magnitude canonical
```

### Arithmetic

All operations MUST be:

- deterministic
- exact
- overflow-free
- producing canonical normalized output

Operations:

- add
- sub
- mul
- div (trunc / floor modes)
- mod
- pow
- gcd
- compare

### Implementation

VM MAY implement long arithmetic, Karatsuba, FFT-based, or hardware-accelerated multiplication **as long as results remain identical**. Implementations MAY also spill large digit arrays to deterministic backing storage (e.g., mmap’d scratch files with fixed naming and allocation rules) when operands exceed in-memory thresholds. Such spill logic, as pioneered in the legacy `hvm-trit-util.cweb`, MUST remain transparent to observable behavior (no timing-dependent faults, no nondeterministic resource selection).

______________________________________________________________________

## 2.3 T81Float

### Definition

A reproducible floating-point format using:

- **base-81 mantissa**
- **base-81 exponent**
- **balanced rounding rules**

### Requirements (Normative)

1. Representations MUST round deterministically.

2. No NaN, no infinities.

3. All invalid states MUST map to a deterministic error code.

4. Round-trip encoding MUST be stable:

   ```
   encode(decode(x)) = x
   ```

### Components

- `mantissa`: T81BigInt
- `exponent`: T81BigInt
- `sign`: 1 trit

### Note

Floating points are **never silently lossy**; any precision loss MUST be made explicit via a `Result[T, E]`.

______________________________________________________________________

## 2.4 T81Fraction

### Definition

A rational number represented as:

```
numerator:   T81BigInt  
denominator: T81BigInt (non-zero)
```

### Canonicalization Rules

1. Fraction MUST always be in lowest terms.
2. Denominator MUST always be positive (+ sign).
3. Zero MUST be encoded as `0/1`.
4. GCD MUST be computed deterministically.

### Arithmetic

Exact and deterministic:

- add
- sub
- mul
- div
- invert

No floating approximations allowed.

______________________________________________________________________

# 3. Composite Types

## 3.1 Arrays

### Properties

- Fixed size or dynamically sized
- Deterministic iteration order
- Memory representation MUST follow:

```
[header][length][canonical elements...]
```

### Allowed element types

Any T81 type.

______________________________________________________________________

## 3.2 Vectors, Matrices, Tensors

### Canonical rules

- Shape MUST be immutable once constructed
- Dimensions MUST be ≥ 1
- All values MUST be normalized
- Out-of-bounds MUST be deterministic fault

### Operations

- reshape (dimensionally consistent only)
- transpose
- tensor contraction
- elementwise ops
- norm
- dot products

All MUST produce deterministic and canonical results.

______________________________________________________________________

## 3.3 Graphs

### Structure

A deterministic graph consists of:

```
nodes: array of canonical nodes  
edges: array of (nodeA, nodeB, metadata)
```

### Requirements

- node ordering MUST be preserved
- edge ordering MUST be preserved
- adjacency queries MUST be deterministic
- metadata MUST be Axion-visible

______________________________________________________________________

# 4. Structural Types

## 4.1 Records (Structs)

### Requirements

- fields MUST have a fixed global ordering
- no implicit reordering
- field names MUST be unique
- all fields MUST be canonical and Axion-visible

______________________________________________________________________

## 4.2 Enums

### Requirements

- variants MUST be globally distinct
- variant ordering MUST be preserved
- payloads MUST be canonicalized

______________________________________________________________________

## 4.3 Optional and Result Types

### Option[T]

- MUST be either `Some(value)` or `None`
- MUST NOT allow null references

### Result[T, E]

- deterministic error propagation
- MUST NOT support exceptions
- MUST encode domain errors as canonical `E`

______________________________________________________________________

# 5. Canonicalization Rules (Critical Normative Section)

This is the most important part of the entire spec.

Canonicalization MUST occur after:

- creation
- arithmetic operations
- parsing
- loading from VM memory
- serialization
- Axion inspection

## 5.1 Invariants for All Types

1. **No redundant forms**

   - fractions reduce
   - integers strip leading zeros
   - floats normalize mantissa/exponent

2. **Deterministic ordering**

   - arrays, structs, enums all follow strict order rules

3. **Deterministic hashing**

   - MUST depend only on canonical form

4. **Axion visibility**

   - Axion MUST be able to inspect normalized representation

______________________________________________________________________

# 6. Interoperability Rules

## 6.1 With TISC (Instruction Set)

- TISC immediates MUST encode canonical forms
- registers MUST contain normalized values only
- decoding MUST fail deterministically for non-canonical inputs

## 6.2 With T81VM

- GC MUST preserve canonical representations
- runtime MUST reject malformed structural types
- VM serialization MUST preserve canonical form exactly

## 6.3 With T81Lang

- static type checking MUST enforce canonical invariants
- semantic analyzer MUST normalize literals
- IR must mark all values with canonical metadata

## 6.4 With Axion

- Axion MUST receive metadata on:

  - normalization
  - overflow attempts
  - uncanonical construction
  - drift in recursive structures

## 6.5 With Cognitive Tiers

Higher tiers depend on deterministic and canonical types for:

- symbolic graphs
- tensor recursion
- cognitive state transitions

______________________________________________________________________

# 7. Error Model (Normative)

All errors MUST be represented via:

```
Result[T, E]
```

### Errors include:

- division by zero
- non-canonical input
- malformed tensor shape
- overflow in intermediate BigInt
- invalid Fractions (0 denominator)
- invalid enumeration variants
- recursion depth failures

No exceptions or traps allowed.

______________________________________________________________________

# 8. Serialization

All serialized forms MUST:

- encode canonical representations only
- be round-trip stable
- be fully deterministic
- be Axion-inspectable
- never encode invalid states

Binary and textual variants allowed; semantics identical.

______________________________________________________________________

# 9. Future Extensions (Non-Normative)

Future type extensions MAY include:

- symbolic algebra types
- holotensor types for high-tier cognition
- ternary complex numbers
- probabilistic bounded distributions
- canonical semantic graphs

All MUST follow determinism and canonicalization invariants.

______________________________________________________________________

# 10. Status

Pending review, discussion, and approval as v0.2 of the T81 Data Types Standard.

______________________________________________________________________

# Cross-References

## Data Types

- **Primitive Types** → [`t81-data-types.md`](t81-data-types.md#2-primitive-types)
- **Composite Types** → [`t81-data-types.md`](t81-data-types.md#3-composite-types)
- **Normalization Rules** → [`t81-data-types.md`](t81-data-types.md#5-normalization-rules)

## TISC (Ternary Instruction Set)

- **Machine Model** → [`tisc-spec.md`](tisc-spec.md#1-machine-model)
- **Instruction Encoding** → [`tisc-spec.md`](tisc-spec.md#4-instruction-encoding)
- **Opcode Classes** → [`tisc-spec.md`](tisc-spec.md#5-opcode-classes)

## T81 Virtual Machine

- **Execution Modes** → [`t81vm-spec.md`](t81vm-spec.md#1-execution-modes)
- **Deterministic Concurrency** → [`t81vm-spec.md`](t81vm-spec.md#3-concurrency-model)
- **Axion Interface Hooks** → [`t81vm-spec.md`](t81vm-spec.md#5-axion-interface)

## T81Lang

- **Language Properties** → [`t81lang-spec.md`](t81lang-spec.md#1-language-properties)
- **Grammar** → [`t81lang-spec.md`](t81lang-spec.md#2-grammar)
- **Type System** → [`t81lang-spec.md`](t81lang-spec.md#3-type-system)

## Axion Kernel

- **Responsibilities** → [`axion-kernel.md`](axion-kernel.md#1-responsibilities)
- **Subsystems** → [`axion-kernel.md`](axion-kernel.md#2-subsystems)
- **Recursion Controls** → [`axion-kernel.md`](axion-kernel.md#3-recursion-controls)

## Cognitive Tiers

- **Tier Structure** → [`cognitive-tiers.md`](cognitive-tiers.md#1-tier-structure)
- **Constraints** → [`cognitive-tiers.md`](cognitive-tiers.md#2-constraints)

______________________________________________________________________
