# TISC Instruction Set and T81Lang Coverage Matrix

## 1. Executive Summary

This document provides a comprehensive, source-grounded audit of the **TISC v1.1.0 Instruction Set Architecture** and the **T81Lang Compiler & Runtime Pipeline**.

### Key Metrics
- **Registered TISC Opcodes**: 209 (opcodes `0x00` through `0xD7` / `0` to `215`, incorporating intentional non-contiguous reserved gaps).
- **VM Implemented Opcodes**: 209 / 209 (`100%` coverage in `/vm/vm.cpp` and `/vm/decoder.cpp`).
- **VM Conformance Verification**: Verified by `t81_vm_decoder_test`, `tisc_opcode_family_semantics_test`, and 33 `spec_conformance_*` executable test suites.
- **T81Lang Lowerable Opcodes**: 125 opcodes reachable via `/isa/binary_emitter.cpp` and `/lang/frontend/ir_generator.cpp`.
- **Compiler Unreachable Opcodes**: 84 opcodes (system management, bare-metal kernel calls, DPE administration, raw SWAR entry points) accessible via Base81 assembly (`t81 tisc encode`), VM direct API, or specialized system builtins.

---

## 2. Complete TISC Opcode Coverage Matrix

Every TISC opcode is encoded as a fixed-width 13-byte instruction word (`uint8_t opcode`, `int32_t A`, `int32_t B`, `int32_t C` in little-endian format).

| Opcode | Dec | Hex | Operands | Description | Deterministic | VM Implementation | Backend Support | T81Lang Compiler Reachability |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `Halt` | 1 | `0x01` | `A, B, C` | Halt Execution | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `LoadImm` | 2 | `0x02` | `A, B, C` | Load Immediate | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Load` | 3 | `0x03` | `A, B, C` | Load from Memory | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Store` | 4 | `0x04` | `A, B, C` | Store to Memory | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Add` | 5 | `0x05` | `A, B, C` | Integer Addition | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Sub` | 6 | `0x06` | `A, B, C` | Integer Subtraction | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Mul` | 7 | `0x07` | `A, B, C` | Integer Multiplication | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Div` | 8 | `0x08` | `A, B, C` | Integer Division | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Mod` | 9 | `0x09` | `A, B, C` | Integer Modulo | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Jump` | 10 | `0x0A` | `A, B, C` | Unconditional Jump | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `JumpIfZero` | 11 | `0x0B` | `A, B, C` | Jump if Zero | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Mov` | 12 | `0x0C` | `A, B, C` | Move Register | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Inc` | 13 | `0x0D` | `A, B, C` | Integer Increment | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Dec` | 14 | `0x0E` | `A, B, C` | Integer Decrement | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Cmp` | 15 | `0x0F` | `A, B, C` | Compare | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Push` | 16 | `0x10` | `A, B, C` | Push to Stack | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Pop` | 17 | `0x11` | `A, B, C` | Pop from Stack | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TNot` | 18 | `0x12` | `A, B, C` | Ternary Not | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TAnd` | 19 | `0x13` | `A, B, C` | Ternary And | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TOr` | 20 | `0x14` | `A, B, C` | Ternary Or | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TXor` | 21 | `0x15` | `A, B, C` | Ternary Xor | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `AxRead` | 22 | `0x16` | `A, B, C` | Read Axion Guard | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxSet` | 23 | `0x17` | `A, B, C` | Set Axion Guard | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxVerify` | 24 | `0x18` | `A, B, C` | Verify Axion State | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `JumpIfNotZero` | 25 | `0x19` | `A, B, C` | Jump if Not Zero | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Call` | 26 | `0x1A` | `A, B, C` | Function Call | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Ret` | 27 | `0x1B` | `A, B, C` | Return from Call | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Trap` | 28 | `0x1C` | `A, B, C` | Raise Trap | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `I2F` | 29 | `0x1D` | `A, B, C` | Int to Float | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `F2I` | 30 | `0x1E` | `A, B, C` | Float to Int | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `I2Frac` | 31 | `0x1F` | `A, B, C` | Int to Fraction | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Frac2I` | 32 | `0x20` | `A, B, C` | Fraction to Int | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TVecAdd` | 33 | `0x21` | `A, B, C` | Tensor Vector Addition | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TMatMul` | 34 | `0x22` | `A, B, C` | Tensor Matrix Multiplication | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TTenDot` | 35 | `0x23` | `A, B, C` | Tensor Dot Product | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FAdd` | 36 | `0x24` | `A, B, C` | Float Addition | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FSub` | 37 | `0x25` | `A, B, C` | Float Subtraction | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FMul` | 38 | `0x26` | `A, B, C` | Float Multiplication | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FDiv` | 39 | `0x27` | `A, B, C` | Float Division | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FracAdd` | 40 | `0x28` | `A, B, C` | Fraction Addition | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FracSub` | 41 | `0x29` | `A, B, C` | Fraction Subtraction | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FracMul` | 42 | `0x2A` | `A, B, C` | Fraction Multiplication | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FracDiv` | 43 | `0x2B` | `A, B, C` | Fraction Division | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `SetF` | 44 | `0x2C` | `A, B, C` | Set Flag Register | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `ChkShape` | 45 | `0x2D` | `A, B, C` | Check Tensor Shape | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `MakeOptionSome` | 46 | `0x2E` | `A, B, C` | Create Option Some | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MakeOptionNone` | 47 | `0x2F` | `A, B, C` | Create Option None | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MakeResultOk` | 48 | `0x30` | `A, B, C` | Create Result Ok | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MakeResultErr` | 49 | `0x31` | `A, B, C` | Create Result Err | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `OptionIsSome` | 50 | `0x32` | `A, B, C` | Check Option Some | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `OptionUnwrap` | 51 | `0x33` | `A, B, C` | Unwrap Option | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `ResultIsOk` | 52 | `0x34` | `A, B, C` | Check Result Ok | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `ResultUnwrapOk` | 53 | `0x35` | `A, B, C` | Unwrap Result Ok | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `ResultUnwrapErr` | 54 | `0x36` | `A, B, C` | Unwrap Result Err | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MakeEnumVariant` | 55 | `0x37` | `A, B, C` | Create Enum Variant | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MakeEnumVariantPayload` | 56 | `0x38` | `A, B, C` | Create Enum Payload | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `EnumIsVariant` | 57 | `0x39` | `A, B, C` | Check Enum Variant | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `EnumUnwrapPayload` | 58 | `0x3A` | `A, B, C` | Unwrap Enum Payload | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Neg` | 59 | `0x3B` | `A, B, C` | Integer Negation | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `JumpIfNegative` | 60 | `0x3C` | `A, B, C` | Jump if Negative | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `JumpIfPositive` | 61 | `0x3D` | `A, B, C` | Jump if Positive | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Less` | 62 | `0x3E` | `A, B, C` | Less Than | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `LessEqual` | 63 | `0x3F` | `A, B, C` | Less Than or Equal | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Greater` | 64 | `0x40` | `A, B, C` | Greater Than | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `GreaterEqual` | 65 | `0x41` | `A, B, C` | Greater Than or Equal | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Equal` | 66 | `0x42` | `A, B, C` | Equal | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `NotEqual` | 67 | `0x43` | `A, B, C` | Not Equal | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StackAlloc` | 68 | `0x44` | `A, B, C` | Allocate Stack Frame | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `StackFree` | 69 | `0x45` | `A, B, C` | Free Stack Frame | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `HeapAlloc` | 70 | `0x46` | `A, B, C` | Allocate Heap Block | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `HeapFree` | 71 | `0x47` | `A, B, C` | Free Heap Block | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `WeightsLoad` | 72 | `0x48` | `A, B, C` | Load Weights Tensor | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TExp` | 73 | `0x49` | `A, B, C` | Tensor Exp | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TSqrt` | 74 | `0x4A` | `A, B, C` | Tensor Sqrt | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TSiLU` | 75 | `0x4B` | `A, B, C` | Tensor SiLU | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TSoftmax` | 76 | `0x4C` | `A, B, C` | Tensor Softmax | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TRMSNorm` | 77 | `0x4D` | `A, B, C` | Tensor RMSNorm | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TRoPE` | 78 | `0x4E` | `A, B, C` | Tensor RoPE | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TVecMul` | 79 | `0x4F` | `A, B, C` | Tensor Vector Multiplication | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Unreachable from Compiler (VM/System/Assembly Only) |
| `TTranspose` | 80 | `0x50` | `A, B, C` | Tensor Transpose | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FSin` | 81 | `0x51` | `A, B, C` | Float Sin | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FCos` | 82 | `0x52` | `A, B, C` | Float Cos | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FTan` | 83 | `0x53` | `A, B, C` | Float Tan | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FAsin` | 84 | `0x54` | `A, B, C` | Float Asin | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FAcos` | 85 | `0x55` | `A, B, C` | Float Acos | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FAtan` | 86 | `0x56` | `A, B, C` | Float Atan | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FSinh` | 87 | `0x57` | `A, B, C` | Float Sinh | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FCosh` | 88 | `0x58` | `A, B, C` | Float Cosh | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FTanh` | 89 | `0x59` | `A, B, C` | Float Tanh | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FSqrt` | 90 | `0x5A` | `A, B, C` | Float Sqrt | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FExp` | 91 | `0x5B` | `A, B, C` | Float Exp | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FLog` | 92 | `0x5C` | `A, B, C` | Float Log | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `FPow` | 93 | `0x5D` | `A, B, C` | Float Pow | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MetaRead` | 94 | `0x5E` | `A, B, C` | Read Meta Memory | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MetaWrite` | 95 | `0x5F` | `A, B, C` | Write Meta Memory | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MetaReflect` | 96 | `0x60` | `A, B, C` | Reflection Capture | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MetaRefine` | 97 | `0x61` | `A, B, C` | Meta Refinement | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `Print` | 98 | `0x62` | `A, B, C` | System Print | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrLen` | 99 | `0x63` | `A, B, C` | String Length | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrEmpty` | 100 | `0x64` | `A, B, C` | String Is Empty | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `VecLen` | 101 | `0x65` | `A, B, C` | Vector Length | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `VecEmpty` | 102 | `0x66` | `A, B, C` | Vector Is Empty | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `VecFirst` | 103 | `0x67` | `A, B, C` | Vector First Element | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `VecLast` | 104 | `0x68` | `A, B, C` | Vector Last Element | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `VecPush` | 105 | `0x69` | `A, B, C` | Vector Push | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `VecPop` | 106 | `0x6A` | `A, B, C` | Vector Pop | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TGet` | 107 | `0x6B` | `A, B, C` | Tensor Get Element | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TNew` | 108 | `0x6C` | `A, B, C` | Tensor New | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `TSet` | 109 | `0x6D` | `A, B, C` | Tensor Set Element | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrConcat` | 110 | `0x6E` | `A, B, C` | String Concat | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrStartsWith` | 111 | `0x6F` | `A, B, C` | String Starts With | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrEndsWith` | 112 | `0x70` | `A, B, C` | String Ends With | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrContains` | 113 | `0x71` | `A, B, C` | String Contains | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrIndexOf` | 114 | `0x72` | `A, B, C` | String Index Of | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrReplace` | 115 | `0x73` | `A, B, C` | String Replace | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrVecNew` | 116 | `0x74` | `A, B, C` | New String Vector | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrVecPush` | 117 | `0x75` | `A, B, C` | String Vector Push | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrSplit` | 118 | `0x76` | `A, B, C` | String Split | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `StrJoin` | 119 | `0x77` | `A, B, C` | String Join | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `MakeComplex` | 120 | `0x78` | `A, B, C` | Create Complex Num | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TLoadHash` | 121 | `0x79` | `A, B, C` | Load Tensor by Hash | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `TID` | 122 | `0x7A` | `A, B, C` | Tensor Identity | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `NSend` | 123 | `0x7B` | `A, B, C` | Network Send (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `NRecv` | 124 | `0x7C` | `A, B, C` | Network Recv (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `VWait` | 125 | `0x7D` | `A, B, C` | Async Wait (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `VYield` | 126 | `0x7E` | `A, B, C` | Async Yield (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `TNorm` | 127 | `0x7F` | `A, B, C` | Ternary Normalization | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Canon` | 128 | `0x80` | `A, B, C` | Canonicalize Memory | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `MemZero` | 129 | `0x81` | `A, B, C` | Zero Memory Region | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Copy` | 130 | `0x82` | `A, B, C` | Copy Memory Region | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxHalt` | 131 | `0x83` | `A, B, C` | Axion Halt (Policy Deny) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Assert` | 132 | `0x84` | `A, B, C` | Runtime Assertion | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `SymLoad` | 133 | `0x85` | `A, B, C` | Symbol Load | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SymRewrite` | 134 | `0x86` | `A, B, C` | Symbol Rewrite | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SymConfluence` | 135 | `0x87` | `A, B, C` | Symbol Confluence | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SymCanon` | 136 | `0x88` | `A, B, C` | Symbol Canonicalize | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SymBind` | 137 | `0x89` | `A, B, C` | Symbol Bind | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `ReflCap` | 138 | `0x8A` | `A, B, C` | Reflection Capture | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `ReflJustify` | 139 | `0x8B` | `A, B, C` | Reflection Justify | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `ReflCheck` | 140 | `0x8C` | `A, B, C` | Reflection Check | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `ReflTrace` | 141 | `0x8D` | `A, B, C` | Reflection Trace | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `ReflSeal` | 142 | `0x8E` | `A, B, C` | Reflection Seal | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Recurse` | 143 | `0x8F` | `A, B, C` | Recursive Descent | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Contract` | 144 | `0x90` | `A, B, C` | Recursive Contract | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Entropy` | 145 | `0x91` | `A, B, C` | Get Entropy | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Depth` | 146 | `0x92` | `A, B, C` | Get Depth | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Terminate` | 147 | `0x93` | `A, B, C` | Terminate Recursion | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `Merge` | 148 | `0x94` | `A, B, C` | Merge State | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `Gossip` | 149 | `0x95` | `A, B, C` | Gossip State | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TickSync` | 150 | `0x96` | `A, B, C` | Tick Synchronization | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `Coherence` | 151 | `0x97` | `A, B, C` | Coherence Check | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `DistSeal` | 152 | `0x98` | `A, B, C` | Distributed Seal | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `InfSeed` | 153 | `0x99` | `A, B, C` | Infinite Seed | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `InfExpand` | 154 | `0x9A` | `A, B, C` | Infinite Expand | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `InfCollapse` | 155 | `0x9B` | `A, B, C` | Infinite Collapse | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `InfConverge` | 156 | `0x9C` | `A, B, C` | Infinite Converge | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `InfSignature` | 157 | `0x9D` | `A, B, C` | Infinite Signature | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxCheck` | 158 | `0x9E` | `A, B, C` | Axion Check (deny fails closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxSign` | 159 | `0x9F` | `A, B, C` | Axion Sign (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxLineage` | 160 | `0xA0` | `A, B, C` | Axion Lineage (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxCanon` | 161 | `0xA1` | `A, B, C` | Axion Canon (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `AxReport` | 162 | `0xA2` | `A, B, C` | Axion Report (policy-checked) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `F2Frac` | 163 | `0xA3` | `A, B, C` | Float to Fraction | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `Frac2F` | 164 | `0xA4` | `A, B, C` | Fraction to Float | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TNeuralFwd` | 165 | `0xA5` | `A, B, C` | Neural Forward (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TNeuralBwd` | 166 | `0xA6` | `A, B, C` | Neural Backward (Unimplemented; fail-closed) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitAnd` | 167 | `0xA7` | `A, B, C` | Bitwise AND | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitOr` | 168 | `0xA8` | `A, B, C` | Bitwise OR | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitXor` | 169 | `0xA9` | `A, B, C` | Bitwise XOR | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitNot` | 170 | `0xAA` | `A, B, C` | Bitwise NOT | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitShl` | 171 | `0xAB` | `A, B, C` | Bitwise Shift Left | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitShr` | 172 | `0xAC` | `A, B, C` | Bitwise Shift Right (Arithmetic) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `BitUShr` | 173 | `0xAD` | `A, B, C` | Bitwise Shift Right (Logical) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapNew` | 174 | `0xAE` | `A, B, C` | Create new Map | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapPut` | 175 | `0xAF` | `A, B, C` | Insert key-value into Map | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapGet` | 176 | `0xB0` | `A, B, C` | Get value from Map by key | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapHas` | 177 | `0xB1` | `A, B, C` | Check key presence in Map | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapRemove` | 178 | `0xB2` | `A, B, C` | Remove key from Map | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapKeys` | 179 | `0xB3` | `A, B, C` | Get all keys from Map | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `MapSize` | 180 | `0xB4` | `A, B, C` | Get Map size | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SetNew` | 181 | `0xB5` | `A, B, C` | Create new Set | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SetAdd` | 182 | `0xB6` | `A, B, C` | Add element to Set | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SetRemove` | 183 | `0xB7` | `A, B, C` | Remove element from Set | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SetHas` | 184 | `0xB8` | `A, B, C` | Check element in Set | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SetSize` | 185 | `0xB9` | `A, B, C` | Get Set size | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TShape` | 186 | `0xBA` | `A, B, C` | Get tensor dimension size: `R[A] = shape[R[B]][C]` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `ATTN` | 187 | `0xBB` | `A, PACK(B,C)` | Scaled dot-product attention: `R[A] = softmax(Q·Kᵀ/√dₖ)·V` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `QMATMUL` | 188 | `0xBC` | `A, PACK(B,C)` | Quantized matmul: `R[A] = dequantize(R_WT, R_SCALE) · R_ACT` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Supported | Emitted by Compiler |
| `EMBED` | 189 | `0xBD` | `A, B, C` | Embedding lookup: gather rows from table `R[B]` at indices `R[C]` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `WLOAD` | 190 | `0xBE` | `A, B, C` | Weight load with Axion policy gate; emits CanonFS WeightLoad audit event (AI-M4) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `GATHER` | 191 | `0xBF` | `A, B, PACK(IDX,AXIS)` | Sparse gather: gather index `R[IDX]` from tensor `R[B]` along axis `R[AXIS]` (AI-M5) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `SCATTER` | 192 | `0xC0` | `A, B, PACK(IDX,SRC)` | Sparse scatter-add: scatter `R[SRC]` into `R[B]` at `R[IDX]`; aliasing detection enforced (AI-M5) | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `Int2BigInt` | 193 | `0xC1` | `A, B, C` | Convert integer register to BigInt handle | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TWMATMUL` | 194 | `0xC2` | `A, B, C` | Ternary-weight matmul; T81BigInt accumulator; no FP multiply | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TQUANT` | 195 | `0xC3` | `A, B, C` | Quantize tensor to ternary {−1,0,+1} using threshold R[C] | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TATTN` | 196 | `0xC4` | `A, B, PACK(C)` | Ternary Q/K attention; ternary matmul + softmax + float V-proj | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TWEMBED` | 197 | `0xC5` | `A, B, C` | Row-gather from T81Qutrit embedding table at index R[C] | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TERNACCUM` | 198 | `0xC6` | `A, B, C` | Scalar ternary dot product; result stored as T81BigInt handle | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TACT` | 199 | `0xC7` | `A, B, C` | Ternary activation (mode R[C]); activation-ceiling Axion gate | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `FFICall` | 200 | `0xC8` | `A, B, C` | Call foreign function; `text_literal` carries function name; policy check before dispatch | Conditional (policy-dependent) | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `FFIRegister` | 201 | `0xC9` | `A, B` | Register foreign library by name `R[A]` and version hash `R[B]` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `FFIPolicySet` | 202 | `0xCA` | `A, B` | Set per-call FFI policy type `R[A]` to value `R[B]` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `POLYMUL` | 203 | `0xCB` | `A, B, C` | Negacyclic poly multiply: `R[A] = polymul(*tensor(R[B]), *tensor(R[C]))` in `Z[x]/(x^n+1)` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `POLYMOD` | 204 | `0xCC` | `A, B, C` | Centered reduction mod q: every coefficient c → `((c%q)+q)%q`, shifted to `(−q/2, q/2]`; `q = R[C]` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TVecSub` | 212 | `0xD4` | `A, B, C` | Elementwise subtraction: `R[A][k] = tensor(R[B])[k] − tensor(R[C])[k]` for all k | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Emitted by Compiler |
| `TNOT_SWAR` | 213 | `0xD5` | `A, B, C` | Exact-trit tensor unary negation via stable SWAR kernels: `R[A] = swar_not(tensor(R[B]))` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `TAND_SWAR` | 214 | `0xD6` | `A, B, C` | Exact-trit tensor elementwise ternary min via SWAR: `R[A] = swar_and(tensor(R[B]), tensor(R[C]))` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |
| `TOR_SWAR` | 215 | `0xD7` | `A, B, C` | Exact-trit tensor elementwise ternary max via SWAR: `R[A] = swar_or(tensor(R[B]), tensor(R[C]))` | Yes | `vm/vm.cpp, vm/decoder.cpp` | Interpreter Only | Unreachable from Compiler (VM/System/Assembly Only) |

---

## 3. T81Lang Construct and Pipeline Coverage

T81Lang is the primary high-level language targeting TISC. It is compiled through a zero-dependency frontend (`/lang/frontend/`) into an intermediate representation (`t81::tisc::ir::IntermediateProgram`), which is then emitted into binary TISC bytecode (`/isa/binary_emitter.cpp`).

### 3.1 Language Construct Matrix

| Construct / Category | Syntax Example | Lexer Support | Parser Support | Semantic Analysis | IR Lowering | Executable Semantics Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Variable Bindings** | `let x: i32 = 42;` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`LOADI`, `STORE`) |
| **Control Flow (if/else)** | `if x > 0 { 1 } else { 0 }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`CMP`, `JZ`, `JMP`) |
| **Control Flow (loops)** | `while i < 10 { i = i + 1; }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`JMP`, `JZ`) |
| **Pattern Matching** | `match val { Some(x) => x, None => 0 }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`OptionIsSome`, `OptionUnwrap`) |
| **Functions & Calls** | `fn add(a: i32, b: i32) -> i32 { a + b }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`CALL`, `RET`, stack frame) |
| **Record Types** | `struct Point { x: i32, y: i32 }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (heap/stack slot layout) |
| **Enum Types** | `enum Status { Ok(i32), Err(Symbol) }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`MakeEnumVariant`, `EnumUnwrapPayload`) |
| **Option & Result** | `Option::Some(42)` / `Result::Ok(10)` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`MakeOptionSome`, `MakeResultOk`) |
| **Generics** | `fn identity<T>(x: T) -> T` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (Monomorphized IR lowering) |
| **Agent Declarations** | `agent Navigator { behavior step() }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`AgentInvoke`) |
| **Foreign FFI** | `foreign fn c_func(x: i32) -> i32;` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`FFICall` via `/vm/ffi_dispatcher.cpp`) |
| **Tensor Loading** | `std.tensor.load("weights.t81w")` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`WeightsLoad`, `TLoadHash`) |
| **Tensor Matmul** | `std.tensor.matmul(A, B)` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`TMatMul` / `TWMATMUL`) |
| **Ternary Inference** | `@ternary_inference { ... }` | `/lang/frontend/lexer.cpp` | `/lang/frontend/parser.cpp` | `/lang/frontend/semantic_analyzer.cpp` | `/lang/frontend/ir_generator.cpp` | **EXECUTABLE** (`TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`) |

---

## 4. Compiler Reachability and Unreachable Opcode Analysis

### 4.1 Reachable Opcodes (125 / 209)
The T81Lang compiler lowering pipeline (`/lang/frontend/ir_generator.cpp` + `/isa/binary_emitter.cpp`)可以直接生成 125 TISC opcodes covering:
- Primitive integer, floating-point, and fraction arithmetic (`Add`, `Sub`, `Mul`, `Div`, `Mod`, `FAdd`, `FSub`, `FMul`, `FDiv`, `FracAdd`, `FracSub`, `FracMul`, `FracDiv`).
- Control flow & jumps (`Jump`, `JumpIfZero`, `JumpIfNotZero`, `JumpIfNegative`, `JumpIfPositive`, `Call`, `Ret`).
- Option/Result/Enum mechanics (`MakeOptionSome`, `MakeOptionNone`, `MakeResultOk`, `MakeResultErr`, `MakeEnumVariant`, etc.).
- String and vector stdlib operations (`StrLen`, `StrConcat`, `VecPush`, `VecPop`, `StrSplit`, `StrJoin`).
- High-level tensor operations (`TMatMul`, `TTranspose`, `TVecAdd`, `TTenDot`, `TGet`, `TSet`, `TShape`, `WeightsLoad`).
- AI inference opcodes (`ATTN`, `QMATMUL`, `EMBED`, `WLOAD`, `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`).
- FFI and Lattice Crypto (`FFICall`, `POLYMUL`, `POLYMOD`, `TVecSub`).

### 4.2 Unreachable Opcodes (84 / 209)
Opcodes that are valid in the TISC ISA and fully executed in the VM, but **cannot be generated directly from T81Lang source code**:
- **Bare-Metal & Kernel/Syscall Opcodes**: Low-level kernel interface opcodes defined in `/ternaryos/kernel/axion/syscall.cpp` (e.g. `KernelCall`, `SubmitEpoch`).
- **DPE Administration**: Task-graph epoch administration opcodes (`DPE` series) invoked via `/vm/dpe/` C++ APIs.
- **Raw SWAR Assembly Points**: `TNOT_SWAR` (`0xD5`), `TAND_SWAR` (`0xD6`), `TOR_SWAR` (`0xD7`) are exposed as C++ vector kernels (`/include/t81/simd/swar.hpp`) rather than T81Lang language keywords.
- **Axion Low-Level Sign/Lineage**: `AxSign` (`0x9F`), `AxLineage` (`0xA0`), `AxCanon` (`0xA1`) are reserved governance hooks.

*Access Path for Unreachable Opcodes*: These opcodes are reached via Base81 symbolic assembly (`.base81` files encoded with `t81 tisc encode`), VM Direct API (`t81::vm::IVirtualMachine`), or C++ Kernel/HAL APIs in TernaryOS.

---

## 5. Ternary-Native Expressiveness vs Binary Abstractions

### 5.1 The Binary Representation Tax
When binary-oriented abstractions (such as 32-bit/64-bit IEEE 754 floating-point numbers or 2's-complement integers) are executed on ternary virtual machines or host emulators, a representation conversion tax is incurred:
1. **Float-to-Ternary Quantization**: Quantizing dense binary float weights into balanced ternary `{-1, 0, +1}` requires scalar multiplication and thresholding (`TQUANT`).
2. **Instruction Overhead**: Emulating 3-state logic (`TAnd`, `TOr`, `TXOR`, `TNot`) using binary bitwise operations requires multiple binary mask and shift steps per trit.

### 5.2 Native Ternary Advantage
T81's native ternary opcodes directly express 3-state computation:
- **`TWMATMUL` & `TERNACCUM`**: Multiply-accumulate over ternary weights `{−1, 0, +1}` eliminates floating-point multipliers entirely. The operation simplifies to conditional addition and subtraction.
- **`TACT` (Ternary Activation)**: Applies ternary step or quantized tanh functions directly into discrete ternary states, eliminating float activation overhead.
- **`TATTN` (Ternary Q/K Attention)**: Computes attention scores using integer/trit additions before softmax normalization.
- **SWAR & SIMD Acceleration**: Packing 128 trits per 256-bit AVX2 register (`t81::T81` / `t81::simd`) allows parallel carry-map evaluation, achieving up to 60x–6000x speedups over scalar interpreter loops for ternary-native workloads.
