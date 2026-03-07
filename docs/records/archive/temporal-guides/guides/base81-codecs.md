# Base-81 Codecs and SIMD Kernels

This guide covers the Base-81 representation layers in the T81 Foundation, including balanced digits, SIMD-accelerated arithmetic, and packed storage formats.

## 1. Overview

Base-81 is the symbolic and human-friendly bridge layer of the T81 stack. While PT-5 (5 trits per byte) remains the canonical machine baseline for hashing and storage, Base-81 (4 trits per digit) is used for symbolic I/O, vectorized arithmetic, and tryte-aligned instruction encoding.

## 2. Balanced Base-81

Balanced Base-81 represents 4 trits as a single digit in the range `[-40, 40]`.

### APIs
Defined in `include/t81/codec/base81_balanced.hpp`:
- `pack(trits, digits)`: Pack 4 trits into an `int8_t` digit.
- `unpack(digits, trits)`: Unpack an `int8_t` digit into 4 trits.
- `to_balanced(unbalanced, balanced)`: Convert `0..80` to `-40..40`.
- `to_unbalanced(balanced, unbalanced)`: Convert `-40..40` to `0..80`.

## 3. SIMD Kernels

T81 provides vectorized primitives for Base-81 digits (stored as `uint8_t` in `0..80`).

### Hot Path Primitives
Defined in `include/t81/simd/base81_digits.hpp`:
- `add`: Vectorized addition of digits.
- `sub`: Vectorized subtraction of digits.
- `negate`: Vectorized symbolic negation (`80 - d`).
- `mul_constant`: Vectorized multiplication by small constants.
- `normalize_and_carry`: Scalar pass to propagate carries and normalize results to `0..80`.

### Performance
Initial benchmarks show a **1.20x throughput advantage** for SIMD-accelerated Base-81 addition over pure scalar loops, even when accounting for the scalar carry-fix pass.

## 4. Packed Base-81 Blocks

For storage-focused compression, 5 Base-81 digits (20 trits) can be packed into a single 32-bit unsigned integer.

### APIs
Defined in `include/t81/codec/base81_packed.hpp`:
- `pack5(digits[5]) -> uint32_t`
- `unpack5(uint32_t, digits[5])`

This format achieves high density while remaining byte-aligned when stored in streams.

## 5. Canonical Metadata

All serialized T81 trit streams should be wrapped with a canonical metadata header to ensure portability and prevent ambiguity regarding padding and encoding.

### Header Layout (16 bytes)
| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | Magic | uint32 | 'T81C' (0x43313854) |
| 4 | Version | uint8 | Version (currently 1) |
| 5 | Encoding| uint8 | 1=PT5, 2=B81_DIGITS, 3=B81_TEXT |
| 6 | Flags | uint16| Reserved (0) |
| 8 | Trit Count| uint64| Authoritative number of trits |

### APIs
Defined in `include/t81/codec/metadata.hpp`:
- `wrap_encoded_buffer()`
- `unwrap_encoded_buffer()`
