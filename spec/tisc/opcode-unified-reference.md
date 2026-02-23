# TISC Opcode Listing (Unified Canonical + Semantic Reference)

Status: Canonical + Developer Semantic Companion (Unified v3)
Version: 3.0.0-draft

## 1. Purpose
- This document reconciles the canonical opcode inventory with developer-facing semantics/traps.

## 2. Encoding Overview
- **Instruction Word Format**: Fixed-width 13-byte encoding.
  - Byte 0: Opcode (`uint8_t`)
  - Bytes 1-4: Operand A (`int32_t`, little-endian)
  - Bytes 5-8: Operand B (`int32_t`, little-endian)
  - Bytes 9-12: Operand C (`int32_t`, little-endian)
- **Operands**: All instructions encode three 32-bit signed integer operands. Unused operands are encoded but ignored (typically zeroed).
- **Reserved Ranges**: `0xA7` through `0xFF` (167-255) reserved for future expansion.
- **Determinism Rule**: Implemented opcodes must exhibit bit-exact deterministic behavior across platforms.

## 3. Notation (Semantic Layer)
- `R[X]` denotes register `X`.
- Operand annotations such as `A: Dest, B: Src` indicate semantic roles for encoded operands.
- `Stack Effect` uses `-` when no stack delta is specified in the semantic reference.
- `Traps / Side Effects` may include fault classes (e.g., `BoundsFault`, `TypeFault`, `SecurityFault`) and/or Axion events.

## 4. Special Case: `NOP` (Present in semantic map, not categorized in v1 tables)

| Mnemonic | Dec | Hex | Category | Canonical Operands | Semantic Operands | Stack Effect | Description | Semantics | Traps / Side Effects | Deterministic | Status | Implementation |
| :--- | ---: | :---: | :--- | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| Nop | 0 | `0x00` | Reserved / Utility | A, B, C | - | - | No Operation | No Operation. Can carry a symbol handle in `B` for logging. | Axion log event | Yes | Implemented | src/vm/vm.cpp (dispatch audited in v1) |

## 5. Unified Opcode Reference by Canonical Category

### Control Flow

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| Halt | 1 | `0x01` | A, B, C | - | - | Halt Execution | Stops execution of the current context. | Sets `halted` flag | Yes | Implemented | src/vm/vm.cpp |
| Jump | 10 | `0x0A` | A, B, C | `A: Addr` | - | Unconditional Jump | Unconditional jump to address `A`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| JumpIfZero | 11 | `0x0B` | A, B, C | `A: Addr, B: Cond` | - | Jump if Zero | Jump to `A` if `R[B] == 0`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| JumpIfNotZero | 25 | `0x19` | A, B, C | `A: Addr, B: Cond` | - | Jump if Not Zero | Jump to `A` if `R[B] != 0`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Call | 26 | `0x1A` | A, B, C | `B: Addr` | -1 | Function Call | Push `PC` to stack and jump to `B`. Checks recursion depth/tier. | StackFault, SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| Ret | 27 | `0x1B` | A, B, C | - | +1 | Return from Call | Pop return address from stack and jump to it. | StackFault | Yes | Implemented | src/vm/vm.cpp |
| JumpIfNegative | 60 | `0x3C` | A, B, C | `A: Addr` | - | Jump if Negative | Jump to `A` if `Negative` flag is set. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| JumpIfPositive | 61 | `0x3D` | A, B, C | `A: Addr` | - | Jump if Positive | Jump to `A` if `Positive` flag is set. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| AxHalt | 131 | `0x83` | A, B, C | - | - | Axion Halt (Policy Deny) | Emergency halt triggered by Axion. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |

### Memory / Stack / Heap

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| LoadImm | 2 | `0x02` | A, B, C | `A: Dest, B: Imm/Handle` | - | Load Immediate | Loads immediate `B` into `R[A]`. Type determined by literal kind (Int, Float, Symbol, etc.). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Load | 3 | `0x03` | A, B, C | `A: Dest, B: Addr` | - | Load from Memory | Loads value from memory address `B` into `R[A]`. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| Store | 4 | `0x04` | A, B, C | `A: Addr, B: Src` | - | Store to Memory | Stores value in `R[B]` to memory address `A`. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| Mov | 12 | `0x0C` | A, B, C | `A: Dest, B: Src` | - | Move Register | Copies `R[B]` to `R[A]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Push | 16 | `0x10` | A, B, C | `A: Src` | -1 | Push to Stack | Pushes `R[A]` onto the stack. | StackFault | Yes | Implemented | src/vm/vm.cpp |
| Pop | 17 | `0x11` | A, B, C | `A: Dest` | +1 | Pop from Stack | Pops top of stack into `R[A]`. | StackFault | Yes | Implemented | src/vm/vm.cpp |
| SetF | 44 | `0x2C` | A, B, C | Semantic Detail Pending | - | Set Flag Register | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StackAlloc | 68 | `0x44` | A, B, C | `A: Dest, B: Size` | - | Allocate Stack Frame | Allocates stack frame of `B` bytes. | StackFault | Yes | Implemented | src/vm/vm.cpp |
| StackFree | 69 | `0x45` | A, B, C | `A: Addr, B: Size` | - | Free Stack Frame | Frees stack frame. | StackFault | Yes | Implemented | src/vm/vm.cpp |
| HeapAlloc | 70 | `0x46` | A, B, C | `A: Dest, B: Size` | - | Allocate Heap Block | Allocates heap block. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| HeapFree | 71 | `0x47` | A, B, C | `A: Addr, B: Size` | - | Free Heap Block | Frees heap block. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| Canon | 128 | `0x80` | A, B, C | `A: Addr` | - | Canonicalize Memory | Canonicalizes memory/register at `R[A]`. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| MemZero | 129 | `0x81` | A, B, C | `A: Addr, B: Size` | - | Zero Memory Region | Zeroes out memory region. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| Copy | 130 | `0x82` | A, B, C | `A: Dst, B: Src, C: Len` | - | Copy Memory Region | Copies memory region. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |

### Arithmetic

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| Add | 5 | `0x05` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Integer Addition | `R[A] = R[B] + R[C]` (Integer). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Sub | 6 | `0x06` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Integer Subtraction | `R[A] = R[B] - R[C]` (Integer). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Mul | 7 | `0x07` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Integer Multiplication | `R[A] = R[B] * R[C]` (Integer). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Div | 8 | `0x08` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Integer Division | `R[A] = R[B] / R[C]` (Integer). | DivisionFault | Yes | Implemented | src/vm/vm.cpp |
| Mod | 9 | `0x09` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Integer Modulo | `R[A] = R[B] % R[C]` (Integer). | DivisionFault | Yes | Implemented | src/vm/vm.cpp |
| Inc | 13 | `0x0D` | A, B, C | `A: Reg` | - | Integer Increment | Increments `R[A]` by 1. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Dec | 14 | `0x0E` | A, B, C | `A: Reg` | - | Integer Decrement | Decrements `R[A]` by 1. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TVecAdd | 33 | `0x21` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Vector Addition | Tensor element-wise addition. | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| TMatMul | 34 | `0x22` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Matrix Multiplication | Tensor Matrix Multiplication. | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| TTenDot | 35 | `0x23` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Dot Product | Tensor Dot Product (Contraction). | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| FAdd | 36 | `0x24` | A, B, C | Semantic Detail Pending | - | Float Addition | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FSub | 37 | `0x25` | A, B, C | Semantic Detail Pending | - | Float Subtraction | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FMul | 38 | `0x26` | A, B, C | Semantic Detail Pending | - | Float Multiplication | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FDiv | 39 | `0x27` | A, B, C | Semantic Detail Pending | - | Float Division | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FracAdd | 40 | `0x28` | A, B, C | Semantic Detail Pending | - | Fraction Addition | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FracSub | 41 | `0x29` | A, B, C | Semantic Detail Pending | - | Fraction Subtraction | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FracMul | 42 | `0x2A` | A, B, C | Semantic Detail Pending | - | Fraction Multiplication | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FracDiv | 43 | `0x2B` | A, B, C | Semantic Detail Pending | - | Fraction Division | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| Neg | 59 | `0x3B` | A, B, C | `A: Dest, B: Src` | - | Integer Negation | `R[A] = -R[B]` (Integer). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TVecMul | 79 | `0x4F` | A, B, C | `A: Dest, B: T1, C: T2` | - | Tensor Vector Multiplication | Tensor element-wise multiplication. | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| TNorm | 127 | `0x7F` | A, B, C | `A: Dest, B: Src` | - | Ternary Normalization | Normalizes/Clamps trit value. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |

### Comparison

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| Cmp | 15 | `0x0F` | A, B, C | `A: Src1, B: Src2` | - | Compare | Compares `R[A]` and `R[B]`, sets flags (Zero, Positive, Negative). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Less | 62 | `0x3E` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Less Than | `R[A] = (R[B] < R[C])`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| LessEqual | 63 | `0x3F` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Less Than or Equal | `R[A] = (R[B] <= R[C])`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| Greater | 64 | `0x40` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Greater Than | `R[A] = (R[B] > R[C])`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| GreaterEqual | 65 | `0x41` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Greater Than or Equal | `R[A] = (R[B] >= R[C])`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| Equal | 66 | `0x42` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Equal | `R[A] = (R[B] == R[C])`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| NotEqual | 67 | `0x43` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Not Equal | `R[A] = (R[B] != R[C])`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |

### Logic

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| TNot | 18 | `0x12` | A, B, C | `A: Dest, B: Src` | - | Ternary Not | Ternary NOT: `-1 -> 1`, `0 -> 0`, `1 -> -1`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TAnd | 19 | `0x13` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Ternary And | Ternary AND (Min function). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TOr | 20 | `0x14` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Ternary Or | Ternary OR (Max function). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TXor | 21 | `0x15` | A, B, C | `A: Dest, B: Src1, C: Src2` | - | Ternary Xor | Ternary XOR / Sum. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |

### Axion / Policy

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| AxRead | 22 | `0x16` | A, B, C | `A: Dest, B: GuardAddr` | - | Read Axion Guard | Reads guarded memory with Axion verification. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| AxSet | 23 | `0x17` | A, B, C | `A: GuardAddr, B: Val` | - | Set Axion Guard | Sets guarded memory with Axion verification. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| AxVerify | 24 | `0x18` | A, B, C | `A: Dest` | - | Verify Axion State | Returns Axion policy verification status (Allow/Deny). | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| Trap | 28 | `0x1C` | A, B, C | - | - | Raise Trap | Explicitly triggers a trap. | TrapInstruction | Yes | Implemented | src/vm/vm.cpp |
| Assert | 132 | `0x84` | A, B, C | `A: Cond` | - | Runtime Assertion | Traps if `R[A]` is 0. | AssertionFailed | Yes | Implemented | src/vm/vm.cpp |
| AxCheck | 158 | `0x9E` | A, B, C | - | - | Axion Check (Stub) | Stub: Explicit Axion Policy Check. | Axion Event | Yes | Stub | src/vm/vm.cpp |
| AxSign | 159 | `0x9F` | A, B, C | - | - | Axion Sign (Stub) | Stub: Signs execution state. | Axion Event | Yes | Stub | src/vm/vm.cpp |
| AxLineage | 160 | `0xA0` | A, B, C | - | - | Axion Lineage (Stub) | Stub: Records lineage event. | Axion Event | Yes | Stub | src/vm/vm.cpp |
| AxCanon | 161 | `0xA1` | A, B, C | - | - | Axion Canon (Stub) | Stub: Enforces canonicalization policy. | Axion Event | Yes | Stub | src/vm/vm.cpp |
| AxReport | 162 | `0xA2` | A, B, C | - | - | Axion Report (Stub) | Stub: Generates safety report. | Axion Event | Yes | Stub | src/vm/vm.cpp |

### Type Conversion

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| I2F | 29 | `0x1D` | A, B, C | `A: Dest, B: Int` | - | Int to Float | Converts Integer `R[B]` to Float Handle `R[A]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| F2I | 30 | `0x1E` | A, B, C | `A: Dest, B: Float` | - | Float to Int | Converts Float Handle `R[B]` to Integer `R[A]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| I2Frac | 31 | `0x1F` | A, B, C | `A: Dest, B: Int` | - | Int to Fraction | Converts Integer `R[B]` to Fraction Handle `R[A]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Frac2I | 32 | `0x20` | A, B, C | `A: Dest, B: Frac` | - | Fraction to Int | Converts Fraction Handle `R[B]` to Integer `R[A]` (if denominator is 1). | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| F2Frac | 163 | `0xA3` | A, B, C | `A: Dest, B: Float` | - | Float to Fraction | Converts Float `R[B]` to Fraction `R[A]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| Frac2F | 164 | `0xA4` | A, B, C | `A: Dest, B: Frac` | - | Fraction to Float | Converts Fraction `R[B]` to Float `R[A]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |

### Tensor / Neural

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| ChkShape | 45 | `0x2D` | A, B, C | Semantic Detail Pending | - | Check Tensor Shape | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| WeightsLoad | 72 | `0x48` | A, B, C | `A: Dest, B: NameHandle` | - | Load Weights Tensor | Loads pre-trained weights by name. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TExp | 73 | `0x49` | A, B, C | `A: Dest, B: Tens` | - | Tensor Exp | Element-wise Exponential. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TSqrt | 74 | `0x4A` | A, B, C | `A: Dest, B: Tens` | - | Tensor Sqrt | Element-wise Square Root. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TSiLU | 75 | `0x4B` | A, B, C | `A: Dest, B: Tens` | - | Tensor SiLU | Applies SiLU activation. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| TSoftmax | 76 | `0x4C` | A, B, C | `A: Dest, B: Tens` | - | Tensor Softmax | Applies Softmax activation. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| TRMSNorm | 77 | `0x4D` | A, B, C | `A: Dest, B: Tens, C: W` | - | Tensor RMSNorm | Applies RMSNorm with weights `R[C]`. | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| TRoPE | 78 | `0x4E` | A, B, C | `A: Dest, B: Tens, C: Pos` | - | Tensor RoPE | Applies Rotary Positional Embedding. | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| TTranspose | 80 | `0x50` | A, B, C | `A: Dest, B: Tens` | - | Tensor Transpose | Transposes tensor (2D). | ShapeFault | Yes | Implemented | src/vm/vm.cpp |
| TGet | 107 | `0x6B` | A, B, C | `A: Dest, B: Tens, C:Idx` | - | Tensor Get Element | Gets element at `R[C]` from tensor `R[B]`. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| TNew | 108 | `0x6C` | A, B, C | `A: Dest, B: Size` | - | Tensor New | Allocates new tensor of size `R[B]`. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| TSet | 109 | `0x6D` | A, B, C | `A: Tens, B: Idx, C:Val` | - | Tensor Set Element | Sets element at `R[B]` in tensor `R[A]`. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| TLoadHash | 121 | `0x79` | A, B, C | `A: Dest, B: HashStr` | - | Load Tensor by Hash | Loads tensor from CanonFS by SHA3-256 hash. Verifies integrity. | SecurityFault, BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| TID | 122 | `0x7A` | A, B, C | `A: Dest, B: Tens` | - | Tensor Identity | Creates identity/copy of tensor `R[B]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| TNeuralFwd | 165 | `0xA5` | A, B, C | `A: Dest, B: Input` | - | Neural Forward (Stub) | Performs neural forward pass (Identity/Stub). | Axion Event | Yes | Stub | src/vm/vm.cpp |
| TNeuralBwd | 166 | `0xA6` | A, B, C | `A: Model` | - | Neural Backward (Stub) | Performs neural backward pass (Stub). | Axion Event | Yes | Stub | src/vm/vm.cpp |

### Options / Results / Enums

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| MakeOptionSome | 46 | `0x2E` | A, B, C | `A: Dest, B: Val` | - | Create Option Some | Creates `Option::Some(R[B])`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| MakeOptionNone | 47 | `0x2F` | A, B, C | `A: Dest` | - | Create Option None | Creates `Option::None`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| MakeResultOk | 48 | `0x30` | A, B, C | `A: Dest, B: Val` | - | Create Result Ok | Creates `Result::Ok(R[B])`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| MakeResultErr | 49 | `0x31` | A, B, C | `A: Dest, B: Val` | - | Create Result Err | Creates `Result::Err(R[B])`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| OptionIsSome | 50 | `0x32` | A, B, C | `A: Dest, B: Opt` | - | Check Option Some | Checks if Option `R[B]` is Some. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| OptionUnwrap | 51 | `0x33` | A, B, C | `A: Dest, B: Opt` | - | Unwrap Option | Unwraps Option `R[B]` or traps if None. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| ResultIsOk | 52 | `0x34` | A, B, C | Semantic Detail Pending | - | Check Result Ok | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| ResultUnwrapOk | 53 | `0x35` | A, B, C | Semantic Detail Pending | - | Unwrap Result Ok | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| ResultUnwrapErr | 54 | `0x36` | A, B, C | Semantic Detail Pending | - | Unwrap Result Err | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| MakeEnumVariant | 55 | `0x37` | A, B, C | Semantic Detail Pending | - | Create Enum Variant | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| MakeEnumVariantPayload | 56 | `0x38` | A, B, C | Semantic Detail Pending | - | Create Enum Payload | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| EnumIsVariant | 57 | `0x39` | A, B, C | Semantic Detail Pending | - | Check Enum Variant | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| EnumUnwrapPayload | 58 | `0x3A` | A, B, C | Semantic Detail Pending | - | Unwrap Enum Payload | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |

### Math (Float)

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| FSin | 81 | `0x51` | A, B, C | Semantic Detail Pending | - | Float Sin | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FCos | 82 | `0x52` | A, B, C | Semantic Detail Pending | - | Float Cos | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FTan | 83 | `0x53` | A, B, C | Semantic Detail Pending | - | Float Tan | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FAsin | 84 | `0x54` | A, B, C | Semantic Detail Pending | - | Float Asin | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FAcos | 85 | `0x55` | A, B, C | Semantic Detail Pending | - | Float Acos | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FAtan | 86 | `0x56` | A, B, C | Semantic Detail Pending | - | Float Atan | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FSinh | 87 | `0x57` | A, B, C | Semantic Detail Pending | - | Float Sinh | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FCosh | 88 | `0x58` | A, B, C | Semantic Detail Pending | - | Float Cosh | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FTanh | 89 | `0x59` | A, B, C | Semantic Detail Pending | - | Float Tanh | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FSqrt | 90 | `0x5A` | A, B, C | Semantic Detail Pending | - | Float Sqrt | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FExp | 91 | `0x5B` | A, B, C | Semantic Detail Pending | - | Float Exp | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FLog | 92 | `0x5C` | A, B, C | Semantic Detail Pending | - | Float Log | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| FPow | 93 | `0x5D` | A, B, C | Semantic Detail Pending | - | Float Pow | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |

### Meta / System

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| MetaRead | 94 | `0x5E` | A, B, C | `A:Dst, B:Seg, C:Addr` | - | Read Meta Memory | Reflective read from memory segment `B` at `R[C]`. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| MetaWrite | 95 | `0x5F` | A, B, C | `A:Val, B:Seg, C:Addr` | - | Write Meta Memory | Reflective write to memory segment `B`. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| MetaReflect | 96 | `0x60` | A, B, C | `A: Dest` | - | Reflection Capture | Captures full VM state snapshot. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| MetaRefine | 97 | `0x61` | A, B, C | `A:Res, B:Cmds, C:Cnt` | - | Meta Refinement | Applies atomic meta-refinement commands. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| Print | 98 | `0x62` | A, B, C | `A: Reg` | - | System Print | Prints value in `R[A]` to output. | TypeFault | Yes | Implemented | src/vm/vm.cpp |

### String / Vector

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| StrLen | 99 | `0x63` | A, B, C | `A: Dest, B: StrHandle` | - | String Length | Gets length of string at `R[B]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| StrEmpty | 100 | `0x64` | A, B, C | Semantic Detail Pending | - | String Is Empty | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| VecLen | 101 | `0x65` | A, B, C | Semantic Detail Pending | - | Vector Length | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| VecEmpty | 102 | `0x66` | A, B, C | Semantic Detail Pending | - | Vector Is Empty | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| VecFirst | 103 | `0x67` | A, B, C | Semantic Detail Pending | - | Vector First Element | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| VecLast | 104 | `0x68` | A, B, C | Semantic Detail Pending | - | Vector Last Element | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| VecPush | 105 | `0x69` | A, B, C | Semantic Detail Pending | - | Vector Push | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| VecPop | 106 | `0x6A` | A, B, C | Semantic Detail Pending | - | Vector Pop | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrConcat | 110 | `0x6E` | A, B, C | `A: Dest, B: Str1, C: Str2` | - | String Concat | Concatenates strings. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| StrStartsWith | 111 | `0x6F` | A, B, C | Semantic Detail Pending | - | String Starts With | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrEndsWith | 112 | `0x70` | A, B, C | Semantic Detail Pending | - | String Ends With | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrContains | 113 | `0x71` | A, B, C | Semantic Detail Pending | - | String Contains | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrIndexOf | 114 | `0x72` | A, B, C | Semantic Detail Pending | - | String Index Of | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrReplace | 115 | `0x73` | A, B, C | Semantic Detail Pending | - | String Replace | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrVecNew | 116 | `0x74` | A, B, C | Semantic Detail Pending | - | New String Vector | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrVecPush | 117 | `0x75` | A, B, C | Semantic Detail Pending | - | String Vector Push | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrSplit | 118 | `0x76` | A, B, C | Semantic Detail Pending | - | String Split | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |
| StrJoin | 119 | `0x77` | A, B, C | Semantic Detail Pending | - | String Join | Semantic detail pending in v2 companion. | Pending | Yes | Implemented | src/vm/vm.cpp |

### Complex

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| MakeComplex | 120 | `0x78` | A, B, C | `A: Dest, B: Re, C: Im` | - | Create Complex Num | Creates Complex number from Integers `R[B]`, `R[C]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |

### Network / Async

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| NSend | 123 | `0x7B` | A, B, C | `B: Handle` | - | Network Send (Stub) | Sends handle/data to network (Placeholder). | Axion Event | Yes | Stub | src/vm/vm.cpp |
| NRecv | 124 | `0x7C` | A, B, C | `A: Dest` | - | Network Recv (Stub) | Receives data from network (Placeholder). | Axion Event | Yes | Stub | src/vm/vm.cpp |
| VWait | 125 | `0x7D` | A, B, C | `A: Dest, B: Promise` | - | Async Wait (Stub) | Waits on a promise/handle. | Axion Event | Yes | Stub | src/vm/vm.cpp |
| VYield | 126 | `0x7E` | A, B, C | `B: Handle` | - | Async Yield (Stub) | Yields execution result. | Axion Event | Yes | Stub | src/vm/vm.cpp |

### Cognitive Tier / Reflection

| Mnemonic | Dec | Hex | Canonical Operands | Semantic Operands | Stack | Canonical Description | Semantic Notes | Traps / Side Effects | Deterministic | Status | Impl |
| :--- | ---: | :---: | :--- | :--- | :---: | :--- | :--- | :--- | :---: | :--- | :--- |
| SymLoad | 133 | `0x85` | A, B, C | `A: Dest, B: Sym` | - | Symbol Load | Loads a symbolic graph from symbol `R[B]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| SymRewrite | 134 | `0x86` | A, B, C | `A: Graph, B: Match` | - | Symbol Rewrite | Rewrites graph `R[A]` using rule `R[B]->R[C]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| SymConfluence | 135 | `0x87` | A, B, C | `A: Dest, B: Graph` | - | Symbol Confluence | Checks confluence of symbolic graph. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| SymCanon | 136 | `0x88` | A, B, C | `A: Graph` | - | Symbol Canonicalize | Canonicalizes symbolic graph `R[A]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| SymBind | 137 | `0x89` | A, B, C | - | - | Symbol Bind | Binds symbol (Placeholder). | - | Yes | Placeholder | src/vm/vm.cpp |
| ReflCap | 138 | `0x8A` | A, B, C | `A: Dest, B: Desc` | - | Reflection Capture | Captures current state (PC, Regs) into a Reflective Frame. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| ReflJustify | 139 | `0x8B` | A, B, C | `A: Frame, B: Text` | - | Reflection Justify | Adds a justification step (text) to the frame. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| ReflCheck | 140 | `0x8C` | A, B, C | `A: Dest, B: Frame, C: Crit` | - | Reflection Check | Verifies frame against criteria `R[C]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| ReflTrace | 141 | `0x8D` | A, B, C | `A: Frame` | - | Reflection Trace | Appends current register state to frame's trace. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| ReflSeal | 142 | `0x8E` | A, B, C | `A: Frame` | - | Reflection Seal | Seals the reflective frame (hashing content). | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| Recurse | 143 | `0x8F` | A, B, C | - | - | Recursive Descent | Increases recursion depth; verifies Tier 3 limits. | SecurityFault | Yes | Implemented | src/vm/vm.cpp |
| Contract | 144 | `0x90` | A, B, C | `B: Entropy` | - | Recursive Contract | Verifies entropy contraction against previous frame. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Entropy | 145 | `0x91` | A, B, C | `A: Dest` | - | Get Entropy | Calculates current system entropy (stack + heap + depth). | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Depth | 146 | `0x92` | A, B, C | `A: Dest` | - | Get Depth | returns current recursion depth to `R[A]`. | DecodeFault | Yes | Implemented | src/vm/vm.cpp |
| Terminate | 147 | `0x93` | A, B, C | - | - | Terminate Recursion | Decreases recursion depth (pops recursion frame). | - | Yes | Implemented | src/vm/vm.cpp |
| Merge | 148 | `0x94` | A, B, C | `A: Dest` | - | Merge State | Merges distributed state from inbox. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| Gossip | 149 | `0x95` | A, B, C | `B: Val` | - | Gossip State | Broadcasts value `R[B]` to distributed network (Tier 4). | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| TickSync | 150 | `0x96` | A, B, C | `A: RemoteTick` | - | Tick Synchronization | Synchronizes local tick with remote tick `R[A]`. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| Coherence | 151 | `0x97` | A, B, C | `A: Dest` | - | Coherence Check | Checks coherence (tick drift) and stores in `R[A]`. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| DistSeal | 152 | `0x98` | A, B, C | `A: Dest` | - | Distributed Seal | Seals distributed state and returns seal hash. | Axion Event | Yes | Implemented | src/vm/vm.cpp |
| InfSeed | 153 | `0x99` | A, B, C | `A: Dest, B: Val` | - | Infinite Seed | Seeds an Infinite Canonical Form with value `R[B]`. | TypeFault | Yes | Implemented | src/vm/vm.cpp |
| InfExpand | 154 | `0x9A` | A, B, C | `A: Form, B: Ratio` | - | Infinite Expand | Expands infinite form with ratio `R[B]`. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| InfCollapse | 155 | `0x9B` | A, B, C | `A: Form` | - | Infinite Collapse | Collapses infinite form to its limit/sum. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| InfConverge | 156 | `0x9C` | A, B, C | `A: Dest, B: Form` | - | Infinite Converge | Checks if infinite form converges. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |
| InfSignature | 157 | `0x9D` | A, B, C | `A: Dest, B: Form` | - | Infinite Signature | Generates signature hash for the infinite form. | BoundsFault | Yes | Implemented | src/vm/vm.cpp |

## 6. Reconciliation Summary
- **Canonical opcode rows merged from v1 categories**: 166 (opcodes `0x01`-`0xA6` plus `0x00` NOP handled separately)
- **Semantic rows available from v2**: 121
- **Canonical rows with v2 semantic coverage**: 120/166
- **Rows marked Stub/Placeholder**: 12

### 6.1 Canonical opcodes missing detailed semantic rows in v2

- **Arithmetic**: `FAdd` (0x24), `FSub` (0x25), `FMul` (0x26), `FDiv` (0x27), `FracAdd` (0x28), `FracSub` (0x29), `FracMul` (0x2A), `FracDiv` (0x2B)
- **Memory / Stack / Heap**: `SetF` (0x2C)
- **Tensor / Neural**: `ChkShape` (0x2D)
- **Options / Results / Enums**: `ResultIsOk` (0x34), `ResultUnwrapOk` (0x35), `ResultUnwrapErr` (0x36), `MakeEnumVariant` (0x37), `MakeEnumVariantPayload` (0x38), `EnumIsVariant` (0x39), `EnumUnwrapPayload` (0x3A)
- **Math (Float)**: `FSin` (0x51), `FCos` (0x52), `FTan` (0x53), `FAsin` (0x54), `FAcos` (0x55), `FAtan` (0x56), `FSinh` (0x57), `FCosh` (0x58), `FTanh` (0x59), `FSqrt` (0x5A), `FExp` (0x5B), `FLog` (0x5C), `FPow` (0x5D)
- **String / Vector**: `StrEmpty` (0x64), `VecLen` (0x65), `VecEmpty` (0x66), `VecFirst` (0x67), `VecLast` (0x68), `VecPush` (0x69), `VecPop` (0x6A), `StrStartsWith` (0x6F), `StrEndsWith` (0x70), `StrContains` (0x71), `StrIndexOf` (0x72), `StrReplace` (0x73), `StrVecNew` (0x74), `StrVecPush` (0x75), `StrSplit` (0x76), `StrJoin` (0x77)

### 6.2 Reserved / Unused
- `0x00` = `NOP` (semantically documented; not listed in v1 category tables).
- `0xA7` through `0xFF` reserved for future standardization.

## 7. Implementation Consistency Audit (carried from v1)
- **VM Opcode Count**: 167 defined opcodes (`0x00`-`0xA6`).
- **Header Enum Count**: 167 entries.
- **Coverage**: All opcodes defined in `include/t81/tisc/opcodes.hpp` are present in `src/vm/vm.cpp` dispatch switch.
- **Discrepancies**: None found (per v1 audit).
