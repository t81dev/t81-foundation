# Migration Guide: RFC 0040 SWAR Formalization

**Target Audience:** C++ integrators, VM authors, assembly/tooling authors
**Date:** 2026-03-18

## 1. Overview

RFC 0040 promotes SWAR ternary-vector operations from the experimental
`PackedTritVector` surface into the stable `t81::swar` API and makes the SWAR
path directly addressable from the VM via explicit TISC opcodes.

The promoted surface is intentionally narrow:

- `t81::swar::t_not`
- `t81::swar::t_and`
- `t81::swar::t_or`
- `t81::swar::t_not_swar`
- `t81::swar::t_and_swar`
- `t81::swar::t_or_swar`

The old experimental methods remain available as deprecated compatibility shims
for the current release cycle.

## 2. C++ API Migration

Prefer the stable header:

```cpp
#include "t81/swar/swar.hpp"
```

Instead of calling deprecated instance methods on
`t81::experimental::ComputeTritVector`, call the stable namespace functions.

Before:

```cpp
auto out = vec.t_not_swar();
```

After:

```cpp
auto out = t81::swar::t_not_swar(vec);
```

Binary operations migrate the same way.

Before:

```cpp
auto out = lhs.t_and_swar(rhs);
```

After:

```cpp
auto out = t81::swar::t_and_swar(lhs, rhs);
```

## 3. VM / TISC Migration

RFC 0040 adds explicit VM opcodes for exact-trit tensor handles:

| Opcode | Hex | Semantics |
| :--- | :--- | :--- |
| `TNOT_SWAR` | `0xD5` | Unary ternary negation over an `ExactTrit` tensor handle |
| `TAND_SWAR` | `0xD6` | Elementwise ternary minimum over two `ExactTrit` tensor handles |
| `TOR_SWAR` | `0xD7` | Elementwise ternary maximum over two `ExactTrit` tensor handles |

These opcodes are not integer bitwise operations and they are not generic tensor
ops for arbitrary numeric classes. They require tensor handles whose numeric
class is `ExactTrit`.

Failure behavior is deterministic:

- Non-tensor operands trap as `TypeFault`
- Non-`ExactTrit` tensors trap as `TypeFault`
- Shape mismatch on binary ops traps as `ShapeFault`
- Invalid packed-trit decoding traps as `DecodeFault`

## 4. Setun Authoring Migration

Setun text assembly now supports direct mnemonics:

```text
TNOT_SWAR R3 R1
TAND_SWAR R4 R1 R2
TOR_SWAR  R5 R1 R2
```

Prefer these mnemonics over hand-encoding numeric opcodes.

## 5. Compatibility Expectations

The compatibility model is:

- Stable code should include `t81/swar/swar.hpp`
- Existing experimental call sites may continue to compile during the
  deprecation window
- New tooling should target the explicit RFC 0040 opcodes instead of ad hoc
  lowering through scalar ternary loops

## 6. Non-Goals

RFC 0040 does not promote:

- General SIMD policy or AVX2/NEON-specific contracts
- New ternary arithmetic beyond `TNot`, `TAnd`, and `TOr`
- Arbitrary tensor numeric classes on the SWAR VM opcodes
