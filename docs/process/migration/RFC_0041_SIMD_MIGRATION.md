# RFC 0041 SIMD Migration Guide

RFC-0041 formalizes the packed-trit SIMD surface that already existed in
`experimental/packed_trit_vector.hpp`.

## Preferred Surface

Use the stable header:

```cpp
#include "t81/simd/simd.hpp"
```

Use the stable namespace:

```cpp
t81::simd::ComputeTritVector
t81::simd::t_not(...)
t81::simd::t_and(...)
t81::simd::t_or(...)
t81::simd::t_not_inplace(...)
t81::simd::t_and_inplace(...)
t81::simd::t_or_inplace(...)
```

## Compatibility

The current stable API is a promotion wrapper over the existing implementation.
`t81::simd::ComputeTritVector` remains an alias of
`t81::experimental::ComputeTritVector` for this release cycle.

This means:

- existing packed 2-bit trit storage is unchanged
- AVX2/NEON threshold dispatch is unchanged
- SWAR tail handling is unchanged
- existing `t81::tritwise` callers remain valid

## Migration Examples

Before:

```cpp
#include "t81/experimental/packed_trit_vector.hpp"

using t81::experimental::ComputeTritVector;

auto a = ComputeTritVector::from_trits(lhs).value();
auto b = ComputeTritVector::from_trits(rhs).value();
auto c = a.t_and(b).value();
```

After:

```cpp
#include "t81/simd/simd.hpp"

using t81::simd::ComputeTritVector;

auto a = ComputeTritVector::from_trits(lhs).value();
auto b = ComputeTritVector::from_trits(rhs).value();
auto c = t81::simd::t_and(a, b).value();
```

## What Did Not Change

- `TXor` still routes to the LUT-safe path, not SIMD
- there are still no SIMD-specific TISC opcodes
- VM/JIT tritwise integration remains the SWAR-driven RFC-0040 path

## Follow-On Work

- deprecate direct external inclusion of `experimental/packed_trit_vector.hpp`
- record refreshed x86_64 and ARM64 benchmark evidence under RFC-0041
- decide whether SIMD should stay a library/runtime substrate only, or grow a VM-visible surface later
