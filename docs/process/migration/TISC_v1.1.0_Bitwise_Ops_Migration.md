# Migration Guide: TISC v1.1.0 Bitwise Operations

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Migration Guide: TISC v1.1.0 Bitwise Operations](#migration-guide-tisc-v110-bitwise-operations)
  - [1. Overview](#1-overview)
  - [2. New Opcodes](#2-new-opcodes)
  - [3. Shift Behavior & Masking](#3-shift-behavior-&-masking)
    - [Example](#example)
  - [4. Arithmetic vs. Logical Right Shift](#4-arithmetic-vs-logical-right-shift)
  - [5. Migration from Ternary Logic](#5-migration-from-ternary-logic)
  - [6. Polyfilling Missing Opcodes](#6-polyfilling-missing-opcodes)

<!-- T81-TOC:END -->


**Target Audience:** T81 Assembly Programmers, Compiler Writers
**Date:** 2025-02-15

## 1. Overview

TISC v1.1.0 introduces native integer bitwise operations to the Core ISA. Previously, these operations had to be emulated or were unavailable. This guide explains how to use the new opcodes and avoid common pitfalls.

## 2. New Opcodes

| Opcode | Mnemonic | Operands | Description |
| :--- | :--- | :--- | :--- |
| `0xA7` | `BitAnd` | `Dest, Src1, Src2` | `Dest = Src1 & Src2` |
| `0xA8` | `BitOr` | `Dest, Src1, Src2` | `Dest = Src1 \| Src2` |
| `0xA9` | `BitXor` | `Dest, Src1, Src2` | `Dest = Src1 ^ Src2` |
| `0xAA` | `BitNot` | `Dest, Src` | `Dest = ~Src` |
| `0xAB` | `BitShl` | `Dest, Val, Amt` | `Dest = Val << Amt` |
| `0xAC` | `BitShr` | `Dest, Val, Amt` | `Dest = Val >> Amt` (Arithmetic) |
| `0xAD` | `BitUShr`| `Dest, Val, Amt` | `Dest = Val >>> Amt` (Logical) |

## 3. Shift Behavior & Masking

To ensure **deterministic execution** across all hardware platforms (including those where out-of-range shifts are undefined behavior), the TISC VM enforces strict masking of the shift amount.

**Rule:** `Amount = OperandC & 0x3F` (63)

### Example
```tisc
; Shift R1 left by 70 bits
LoadImm R2, 70
BitShl R3, R1, R2
```
*   `70` in binary is `1000110`.
*   `70 & 63` is `6` (`0000110`).
*   Result: `R3 = R1 << 6`.

## 4. Arithmetic vs. Logical Right Shift

*   **`BitShr` (Arithmetic)**: Preserves the sign bit. Use this for signed integers.
    *   `-2 >> 1` becomes `-1`.
*   **`BitUShr` (Logical)**: Fills with zeros. Use this for unsigned integers, bitmaps, or processing binary data.
    *   `-2 >>> 1` becomes a large positive integer (sign bit shifted out).

## 5. Migration from Ternary Logic

Do **NOT** confuse the new integer bitwise ops with the existing Ternary Logic ops:

*   **Ternary Ops** (`TAnd`, `TOr`, `TXor`, `TNot`): Operate on trits (-1, 0, 1). They perform min/max logic.
*   **Bitwise Ops** (`BitAnd`, ...): Operate on 64-bit integers.

**Wrong:**
```tisc
; Trying to mask bits with TAnd
TAnd R1, R2, R3 ; This does NOT do bitwise AND!
```

**Right:**
```tisc
BitAnd R1, R2, R3
```

## 6. Polyfilling Missing Opcodes

TISC v1.1.0 does **not** include Rotate (`RotL`, `RotR`). You must implement them using shifts and OR:

**Rotate Left (x, n):**
```tisc
; R1 = x, R2 = n, R3 = 64-n
BitShl R4, R1, R2  ; x << n
BitUShr R5, R1, R3 ; x >>> (64-n)
BitOr R1, R4, R5   ; Combine
```
