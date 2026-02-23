# TISC Opcode Listing
Status: Canonical
Version: 1.1.0
Source of Truth: VM + opcode headers

## 1. Encoding Overview
- **Instruction Word Format**: Fixed-width 13-byte encoding.
  - Byte 0: Opcode (`uint8_t`)
  - Bytes 1-4: Operand A (`int32_t`, little-endian)
  - Bytes 5-8: Operand B (`int32_t`, little-endian)
  - Bytes 9-12: Operand C (`int32_t`, little-endian)
- **Operands**: All instructions encode three 32-bit signed integer operands (A, B, C). Unused operands for a specific opcode are ignored by the VM but must be present in the binary stream (typically zeroed).
- **Reserved Ranges**: Opcodes 167 through 255 are reserved for future expansion.
- **Determinism**: All implemented opcodes must exhibit bit-exact deterministic behavior across all platforms. Floating point operations (FAdd, etc.) use `T81Float` or deterministic software implementations where hardware checks fail.

## 2. Opcode Categories

### 2.1 Arithmetic
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Add | 5 (0x05) | A, B, C | Integer Addition | Yes | src/vm/vm.cpp |
| Sub | 6 (0x06) | A, B, C | Integer Subtraction | Yes | src/vm/vm.cpp |
| Mul | 7 (0x07) | A, B, C | Integer Multiplication | Yes | src/vm/vm.cpp |
| Div | 8 (0x08) | A, B, C | Integer Division | Yes | src/vm/vm.cpp |
| Mod | 9 (0x09) | A, B, C | Integer Modulo | Yes | src/vm/vm.cpp |
| Inc | 13 (0x0D) | A, B, C | Integer Increment | Yes | src/vm/vm.cpp |
| Dec | 14 (0x0E) | A, B, C | Integer Decrement | Yes | src/vm/vm.cpp |
| Neg | 59 (0x3B) | A, B, C | Integer Negation | Yes | src/vm/vm.cpp |
| FAdd | 36 (0x24) | A, B, C | Float Addition | Yes | src/vm/vm.cpp |
| FSub | 37 (0x25) | A, B, C | Float Subtraction | Yes | src/vm/vm.cpp |
| FMul | 38 (0x26) | A, B, C | Float Multiplication | Yes | src/vm/vm.cpp |
| FDiv | 39 (0x27) | A, B, C | Float Division | Yes | src/vm/vm.cpp |
| FracAdd | 40 (0x28) | A, B, C | Fraction Addition | Yes | src/vm/vm.cpp |
| FracSub | 41 (0x29) | A, B, C | Fraction Subtraction | Yes | src/vm/vm.cpp |
| FracMul | 42 (0x2A) | A, B, C | Fraction Multiplication | Yes | src/vm/vm.cpp |
| FracDiv | 43 (0x2B) | A, B, C | Fraction Division | Yes | src/vm/vm.cpp |
| TVecAdd | 33 (0x21) | A, B, C | Tensor Vector Addition | Yes | src/vm/vm.cpp |
| TVecMul | 79 (0x4F) | A, B, C | Tensor Vector Multiplication | Yes | src/vm/vm.cpp |
| TMatMul | 34 (0x22) | A, B, C | Tensor Matrix Multiplication | Yes | src/vm/vm.cpp |
| TTenDot | 35 (0x23) | A, B, C | Tensor Dot Product | Yes | src/vm/vm.cpp |
| TNorm | 127 (0x7F) | A, B, C | Ternary Normalization | Yes | src/vm/vm.cpp |

### 2.2 Control Flow
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Halt | 1 (0x01) | A, B, C | Halt Execution | Yes | src/vm/vm.cpp |
| Jump | 10 (0x0A) | A, B, C | Unconditional Jump | Yes | src/vm/vm.cpp |
| JumpIfZero | 11 (0x0B) | A, B, C | Jump if Zero | Yes | src/vm/vm.cpp |
| JumpIfNotZero | 25 (0x19) | A, B, C | Jump if Not Zero | Yes | src/vm/vm.cpp |
| Call | 26 (0x1A) | A, B, C | Function Call | Yes | src/vm/vm.cpp |
| Ret | 27 (0x1B) | A, B, C | Return from Call | Yes | src/vm/vm.cpp |
| JumpIfNegative | 60 (0x3C) | A, B, C | Jump if Negative | Yes | src/vm/vm.cpp |
| JumpIfPositive | 61 (0x3D) | A, B, C | Jump if Positive | Yes | src/vm/vm.cpp |
| AxHalt | 131 (0x83) | A, B, C | Axion Halt (Policy Deny) | Yes | src/vm/vm.cpp |

### 2.3 Comparison
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Cmp | 15 (0x0F) | A, B, C | Compare | Yes | src/vm/vm.cpp |
| Less | 62 (0x3E) | A, B, C | Less Than | Yes | src/vm/vm.cpp |
| LessEqual | 63 (0x3F) | A, B, C | Less Than or Equal | Yes | src/vm/vm.cpp |
| Greater | 64 (0x40) | A, B, C | Greater Than | Yes | src/vm/vm.cpp |
| GreaterEqual | 65 (0x41) | A, B, C | Greater Than or Equal | Yes | src/vm/vm.cpp |
| Equal | 66 (0x42) | A, B, C | Equal | Yes | src/vm/vm.cpp |
| NotEqual | 67 (0x43) | A, B, C | Not Equal | Yes | src/vm/vm.cpp |

### 2.4 Memory / Stack / Heap
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| LoadImm | 2 (0x02) | A, B, C | Load Immediate | Yes | src/vm/vm.cpp |
| Load | 3 (0x03) | A, B, C | Load from Memory | Yes | src/vm/vm.cpp |
| Store | 4 (0x04) | A, B, C | Store to Memory | Yes | src/vm/vm.cpp |
| Mov | 12 (0x0C) | A, B, C | Move Register | Yes | src/vm/vm.cpp |
| Push | 16 (0x10) | A, B, C | Push to Stack | Yes | src/vm/vm.cpp |
| Pop | 17 (0x11) | A, B, C | Pop from Stack | Yes | src/vm/vm.cpp |
| SetF | 44 (0x2C) | A, B, C | Set Flag Register | Yes | src/vm/vm.cpp |
| StackAlloc | 68 (0x44) | A, B, C | Allocate Stack Frame | Yes | src/vm/vm.cpp |
| StackFree | 69 (0x45) | A, B, C | Free Stack Frame | Yes | src/vm/vm.cpp |
| HeapAlloc | 70 (0x46) | A, B, C | Allocate Heap Block | Yes | src/vm/vm.cpp |
| HeapFree | 71 (0x47) | A, B, C | Free Heap Block | Yes | src/vm/vm.cpp |
| Canon | 128 (0x80) | A, B, C | Canonicalize Memory | Yes | src/vm/vm.cpp |
| MemZero | 129 (0x81) | A, B, C | Zero Memory Region | Yes | src/vm/vm.cpp |
| Copy | 130 (0x82) | A, B, C | Copy Memory Region | Yes | src/vm/vm.cpp |

### 2.5 Type Conversion
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| I2F | 29 (0x1D) | A, B, C | Int to Float | Yes | src/vm/vm.cpp |
| F2I | 30 (0x1E) | A, B, C | Float to Int | Yes | src/vm/vm.cpp |
| I2Frac | 31 (0x1F) | A, B, C | Int to Fraction | Yes | src/vm/vm.cpp |
| Frac2I | 32 (0x20) | A, B, C | Fraction to Int | Yes | src/vm/vm.cpp |
| F2Frac | 163 (0xA3) | A, B, C | Float to Fraction | Yes | src/vm/vm.cpp |
| Frac2F | 164 (0xA4) | A, B, C | Fraction to Float | Yes | src/vm/vm.cpp |

### 2.6 Logic
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| TNot | 18 (0x12) | A, B, C | Ternary Not | Yes | src/vm/vm.cpp |
| TAnd | 19 (0x13) | A, B, C | Ternary And | Yes | src/vm/vm.cpp |
| TOr | 20 (0x14) | A, B, C | Ternary Or | Yes | src/vm/vm.cpp |
| TXor | 21 (0x15) | A, B, C | Ternary Xor | Yes | src/vm/vm.cpp |

### 2.7 Axion / Policy
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| AxRead | 22 (0x16) | A, B, C | Read Axion Guard | Yes | src/vm/vm.cpp |
| AxSet | 23 (0x17) | A, B, C | Set Axion Guard | Yes | src/vm/vm.cpp |
| AxVerify | 24 (0x18) | A, B, C | Verify Axion State | Yes | src/vm/vm.cpp |
| Trap | 28 (0x1C) | A, B, C | Raise Trap | Yes | src/vm/vm.cpp |
| Assert | 132 (0x84) | A, B, C | Runtime Assertion | Yes | src/vm/vm.cpp |
| AxCheck | 158 (0x9E) | A, B, C | Axion Check (Stub) | Yes | src/vm/vm.cpp |
| AxSign | 159 (0x9F) | A, B, C | Axion Sign (Stub) | Yes | src/vm/vm.cpp |
| AxLineage | 160 (0xA0) | A, B, C | Axion Lineage (Stub) | Yes | src/vm/vm.cpp |
| AxCanon | 161 (0xA1) | A, B, C | Axion Canon (Stub) | Yes | src/vm/vm.cpp |
| AxReport | 162 (0xA2) | A, B, C | Axion Report (Stub) | Yes | src/vm/vm.cpp |

### 2.8 Tensor / Neural
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| ChkShape | 45 (0x2D) | A, B, C | Check Tensor Shape | Yes | src/vm/vm.cpp |
| WeightsLoad | 72 (0x48) | A, B, C | Load Weights Tensor | Yes | src/vm/vm.cpp |
| TExp | 73 (0x49) | A, B, C | Tensor Exp | Yes | src/vm/vm.cpp |
| TSqrt | 74 (0x4A) | A, B, C | Tensor Sqrt | Yes | src/vm/vm.cpp |
| TSiLU | 75 (0x4B) | A, B, C | Tensor SiLU | Yes | src/vm/vm.cpp |
| TSoftmax | 76 (0x4C) | A, B, C | Tensor Softmax | Yes | src/vm/vm.cpp |
| TRMSNorm | 77 (0x4D) | A, B, C | Tensor RMSNorm | Yes | src/vm/vm.cpp |
| TRoPE | 78 (0x4E) | A, B, C | Tensor RoPE | Yes | src/vm/vm.cpp |
| TTranspose | 80 (0x50) | A, B, C | Tensor Transpose | Yes | src/vm/vm.cpp |
| TGet | 107 (0x6B) | A, B, C | Tensor Get Element | Yes | src/vm/vm.cpp |
| TNew | 108 (0x6C) | A, B, C | Tensor New | Yes | src/vm/vm.cpp |
| TSet | 109 (0x6D) | A, B, C | Tensor Set Element | Yes | src/vm/vm.cpp |
| TLoadHash | 121 (0x79) | A, B, C | Load Tensor by Hash | Yes | src/vm/vm.cpp |
| TID | 122 (0x7A) | A, B, C | Tensor Identity | Yes | src/vm/vm.cpp |
| TNeuralFwd | 165 (0xA5) | A, B, C | Neural Forward (Stub) | Yes | src/vm/vm.cpp |
| TNeuralBwd | 166 (0xA6) | A, B, C | Neural Backward (Stub) | Yes | src/vm/vm.cpp |

### 2.9 Math (Float)
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| FSin | 81 (0x51) | A, B, C | Float Sin | Yes | src/vm/vm.cpp |
| FCos | 82 (0x52) | A, B, C | Float Cos | Yes | src/vm/vm.cpp |
| FTan | 83 (0x53) | A, B, C | Float Tan | Yes | src/vm/vm.cpp |
| FAsin | 84 (0x54) | A, B, C | Float Asin | Yes | src/vm/vm.cpp |
| FAcos | 85 (0x55) | A, B, C | Float Acos | Yes | src/vm/vm.cpp |
| FAtan | 86 (0x56) | A, B, C | Float Atan | Yes | src/vm/vm.cpp |
| FSinh | 87 (0x57) | A, B, C | Float Sinh | Yes | src/vm/vm.cpp |
| FCosh | 88 (0x58) | A, B, C | Float Cosh | Yes | src/vm/vm.cpp |
| FTanh | 89 (0x59) | A, B, C | Float Tanh | Yes | src/vm/vm.cpp |
| FSqrt | 90 (0x5A) | A, B, C | Float Sqrt | Yes | src/vm/vm.cpp |
| FExp | 91 (0x5B) | A, B, C | Float Exp | Yes | src/vm/vm.cpp |
| FLog | 92 (0x5C) | A, B, C | Float Log | Yes | src/vm/vm.cpp |
| FPow | 93 (0x5D) | A, B, C | Float Pow | Yes | src/vm/vm.cpp |

### 2.10 Meta / System
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MetaRead | 94 (0x5E) | A, B, C | Read Meta Memory | Yes | src/vm/vm.cpp |
| MetaWrite | 95 (0x5F) | A, B, C | Write Meta Memory | Yes | src/vm/vm.cpp |
| MetaReflect | 96 (0x60) | A, B, C | Reflection Capture | Yes | src/vm/vm.cpp |
| MetaRefine | 97 (0x61) | A, B, C | Meta Refinement | Yes | src/vm/vm.cpp |
| Print | 98 (0x62) | A, B, C | System Print | Yes | src/vm/vm.cpp |

### 2.11 String / Vector
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| StrLen | 99 (0x63) | A, B, C | String Length | Yes | src/vm/vm.cpp |
| StrEmpty | 100 (0x64) | A, B, C | String Is Empty | Yes | src/vm/vm.cpp |
| VecLen | 101 (0x65) | A, B, C | Vector Length | Yes | src/vm/vm.cpp |
| VecEmpty | 102 (0x66) | A, B, C | Vector Is Empty | Yes | src/vm/vm.cpp |
| VecFirst | 103 (0x67) | A, B, C | Vector First Element | Yes | src/vm/vm.cpp |
| VecLast | 104 (0x68) | A, B, C | Vector Last Element | Yes | src/vm/vm.cpp |
| VecPush | 105 (0x69) | A, B, C | Vector Push | Yes | src/vm/vm.cpp |
| VecPop | 106 (0x6A) | A, B, C | Vector Pop | Yes | src/vm/vm.cpp |
| StrConcat | 110 (0x6E) | A, B, C | String Concat | Yes | src/vm/vm.cpp |
| StrStartsWith | 111 (0x6F) | A, B, C | String Starts With | Yes | src/vm/vm.cpp |
| StrEndsWith | 112 (0x70) | A, B, C | String Ends With | Yes | src/vm/vm.cpp |
| StrContains | 113 (0x71) | A, B, C | String Contains | Yes | src/vm/vm.cpp |
| StrIndexOf | 114 (0x72) | A, B, C | String Index Of | Yes | src/vm/vm.cpp |
| StrReplace | 115 (0x73) | A, B, C | String Replace | Yes | src/vm/vm.cpp |
| StrVecNew | 116 (0x74) | A, B, C | New String Vector | Yes | src/vm/vm.cpp |
| StrVecPush | 117 (0x75) | A, B, C | String Vector Push | Yes | src/vm/vm.cpp |
| StrSplit | 118 (0x76) | A, B, C | String Split | Yes | src/vm/vm.cpp |
| StrJoin | 119 (0x77) | A, B, C | String Join | Yes | src/vm/vm.cpp |

### 2.12 Options / Results / Enums
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MakeOptionSome | 46 (0x2E) | A, B, C | Create Option Some | Yes | src/vm/vm.cpp |
| MakeOptionNone | 47 (0x2F) | A, B, C | Create Option None | Yes | src/vm/vm.cpp |
| MakeResultOk | 48 (0x30) | A, B, C | Create Result Ok | Yes | src/vm/vm.cpp |
| MakeResultErr | 49 (0x31) | A, B, C | Create Result Err | Yes | src/vm/vm.cpp |
| OptionIsSome | 50 (0x32) | A, B, C | Check Option Some | Yes | src/vm/vm.cpp |
| OptionUnwrap | 51 (0x33) | A, B, C | Unwrap Option | Yes | src/vm/vm.cpp |
| ResultIsOk | 52 (0x34) | A, B, C | Check Result Ok | Yes | src/vm/vm.cpp |
| ResultUnwrapOk | 53 (0x35) | A, B, C | Unwrap Result Ok | Yes | src/vm/vm.cpp |
| ResultUnwrapErr | 54 (0x36) | A, B, C | Unwrap Result Err | Yes | src/vm/vm.cpp |
| MakeEnumVariant | 55 (0x37) | A, B, C | Create Enum Variant | Yes | src/vm/vm.cpp |
| MakeEnumVariantPayload | 56 (0x38) | A, B, C | Create Enum Payload | Yes | src/vm/vm.cpp |
| EnumIsVariant | 57 (0x39) | A, B, C | Check Enum Variant | Yes | src/vm/vm.cpp |
| EnumUnwrapPayload | 58 (0x3A) | A, B, C | Unwrap Enum Payload | Yes | src/vm/vm.cpp |

### 2.13 Complex
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MakeComplex | 120 (0x78) | A, B, C | Create Complex Num | Yes | src/vm/vm.cpp |

### 2.14 Cognitive Tier / Reflection
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| SymLoad | 133 (0x85) | A, B, C | Symbol Load | Yes | src/vm/vm.cpp |
| SymRewrite | 134 (0x86) | A, B, C | Symbol Rewrite | Yes | src/vm/vm.cpp |
| SymConfluence | 135 (0x87) | A, B, C | Symbol Confluence | Yes | src/vm/vm.cpp |
| SymCanon | 136 (0x88) | A, B, C | Symbol Canonicalize | Yes | src/vm/vm.cpp |
| SymBind | 137 (0x89) | A, B, C | Symbol Bind | Yes | src/vm/vm.cpp |
| ReflCap | 138 (0x8A) | A, B, C | Reflection Capture | Yes | src/vm/vm.cpp |
| ReflJustify | 139 (0x8B) | A, B, C | Reflection Justify | Yes | src/vm/vm.cpp |
| ReflCheck | 140 (0x8C) | A, B, C | Reflection Check | Yes | src/vm/vm.cpp |
| ReflTrace | 141 (0x8D) | A, B, C | Reflection Trace | Yes | src/vm/vm.cpp |
| ReflSeal | 142 (0x8E) | A, B, C | Reflection Seal | Yes | src/vm/vm.cpp |
| Recurse | 143 (0x8F) | A, B, C | Recursive Descent | Yes | src/vm/vm.cpp |
| Contract | 144 (0x90) | A, B, C | Recursive Contract | Yes | src/vm/vm.cpp |
| Entropy | 145 (0x91) | A, B, C | Get Entropy | Yes | src/vm/vm.cpp |
| Depth | 146 (0x92) | A, B, C | Get Depth | Yes | src/vm/vm.cpp |
| Terminate | 147 (0x93) | A, B, C | Terminate Recursion | Yes | src/vm/vm.cpp |
| Merge | 148 (0x94) | A, B, C | Merge State | Yes | src/vm/vm.cpp |
| Gossip | 149 (0x95) | A, B, C | Gossip State | Yes | src/vm/vm.cpp |
| TickSync | 150 (0x96) | A, B, C | Tick Synchronization | Yes | src/vm/vm.cpp |
| Coherence | 151 (0x97) | A, B, C | Coherence Check | Yes | src/vm/vm.cpp |
| DistSeal | 152 (0x98) | A, B, C | Distributed Seal | Yes | src/vm/vm.cpp |
| InfSeed | 153 (0x99) | A, B, C | Infinite Seed | Yes | src/vm/vm.cpp |
| InfExpand | 154 (0x9A) | A, B, C | Infinite Expand | Yes | src/vm/vm.cpp |
| InfCollapse | 155 (0x9B) | A, B, C | Infinite Collapse | Yes | src/vm/vm.cpp |
| InfConverge | 156 (0x9C) | A, B, C | Infinite Converge | Yes | src/vm/vm.cpp |
| InfSignature | 157 (0x9D) | A, B, C | Infinite Signature | Yes | src/vm/vm.cpp |

### 2.15 Network / Async
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| NSend | 123 (0x7B) | A, B, C | Network Send (Stub) | Yes | src/vm/vm.cpp |
| NRecv | 124 (0x7C) | A, B, C | Network Recv (Stub) | Yes | src/vm/vm.cpp |
| VWait | 125 (0x7D) | A, B, C | Async Wait (Stub) | Yes | src/vm/vm.cpp |
| VYield | 126 (0x7E) | A, B, C | Async Yield (Stub) | Yes | src/vm/vm.cpp |

## 3. Reserved / Unused Opcodes
- **Nop (0x00)**: No Operation.
- **Reserved**: Opcodes 167 (0xA7) through 255 (0xFF) are reserved for future standardization.

## 4. Implementation Consistency Audit
- **VM Opcode Count**: 167 defined opcodes (0-166).
- **Header Enum Count**: 167 entries.
- **Coverage**: All opcodes defined in `include/t81/tisc/opcodes.hpp` are present in `src/vm/vm.cpp` dispatch switch.
- **Discrepancies**: None found.
