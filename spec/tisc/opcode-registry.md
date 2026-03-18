# TISC Opcode Listing
Status: Canonical / Frozen (v1.1.0)
Version: 1.1.0
Source of Truth: VM + opcode headers

## 1. Encoding Overview
- **Instruction Word Format**: Fixed-width 13-byte encoding.
  - Byte 0: Opcode (`uint8_t`)
  - Bytes 1-4: Operand A (`int32_t`, little-endian)
  - Bytes 5-8: Operand B (`int32_t`, little-endian)
  - Bytes 9-12: Operand C (`int32_t`, little-endian)
- **Operands**: All instructions encode three 32-bit signed integer operands (A, B, C). Unused operands for a specific opcode are ignored by the VM but must be present in the binary stream (typically zeroed).
- **Reserved Ranges**: Opcodes 200 (0xC8) through 255 (0xFF) are reserved for future expansion. Opcodes 174 (0xAE)–199 (0xC7) are assigned: Map/Set scaffolding (174–185), TShape (186), AI-Native Inference / RFC-0026 (187–193), Ternary-Native Inference / RFC-0034 (194–199).
- **Determinism**: All implemented opcodes must exhibit bit-exact deterministic behavior across all platforms. Floating point operations (FAdd, etc.) use `T81Float` or deterministic software implementations where hardware checks fail.

## 2. Opcode Categories

### 2.1 Arithmetic
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Add | 5 (0x05) | A, B, C | Integer Addition | Yes | core/vm/vm.cpp |
| Sub | 6 (0x06) | A, B, C | Integer Subtraction | Yes | core/vm/vm.cpp |
| Mul | 7 (0x07) | A, B, C | Integer Multiplication | Yes | core/vm/vm.cpp |
| Div | 8 (0x08) | A, B, C | Integer Division | Yes | core/vm/vm.cpp |
| Mod | 9 (0x09) | A, B, C | Integer Modulo | Yes | core/vm/vm.cpp |
| Inc | 13 (0x0D) | A, B, C | Integer Increment | Yes | core/vm/vm.cpp |
| Dec | 14 (0x0E) | A, B, C | Integer Decrement | Yes | core/vm/vm.cpp |
| Neg | 59 (0x3B) | A, B, C | Integer Negation | Yes | core/vm/vm.cpp |
| FAdd | 36 (0x24) | A, B, C | Float Addition | Yes | core/vm/vm.cpp |
| FSub | 37 (0x25) | A, B, C | Float Subtraction | Yes | core/vm/vm.cpp |
| FMul | 38 (0x26) | A, B, C | Float Multiplication | Yes | core/vm/vm.cpp |
| FDiv | 39 (0x27) | A, B, C | Float Division | Yes | core/vm/vm.cpp |
| FracAdd | 40 (0x28) | A, B, C | Fraction Addition | Yes | core/vm/vm.cpp |
| FracSub | 41 (0x29) | A, B, C | Fraction Subtraction | Yes | core/vm/vm.cpp |
| FracMul | 42 (0x2A) | A, B, C | Fraction Multiplication | Yes | core/vm/vm.cpp |
| FracDiv | 43 (0x2B) | A, B, C | Fraction Division | Yes | core/vm/vm.cpp |
| TVecAdd | 33 (0x21) | A, B, C | Tensor Vector Addition | Yes | core/vm/vm.cpp |
| TVecMul | 79 (0x4F) | A, B, C | Tensor Vector Multiplication | Yes | core/vm/vm.cpp |
| TMatMul | 34 (0x22) | A, B, C | Tensor Matrix Multiplication | Yes | core/vm/vm.cpp |
| TTenDot | 35 (0x23) | A, B, C | Tensor Dot Product | Yes | core/vm/vm.cpp |
| TNorm | 127 (0x7F) | A, B, C | Ternary Normalization | Yes | core/vm/vm.cpp |
| BitAnd | 167 (0xA7) | A, B, C | Bitwise AND | Yes | core/vm/vm.cpp |
| BitOr | 168 (0xA8) | A, B, C | Bitwise OR | Yes | core/vm/vm.cpp |
| BitXor | 169 (0xA9) | A, B, C | Bitwise XOR | Yes | core/vm/vm.cpp |
| BitNot | 170 (0xAA) | A, B, C | Bitwise NOT | Yes | core/vm/vm.cpp |
| BitShl | 171 (0xAB) | A, B, C | Bitwise Shift Left | Yes | core/vm/vm.cpp |
| BitShr | 172 (0xAC) | A, B, C | Bitwise Shift Right (Arithmetic) | Yes | core/vm/vm.cpp |
| BitUShr | 173 (0xAD) | A, B, C | Bitwise Shift Right (Logical) | Yes | core/vm/vm.cpp |

### 2.2 Control Flow
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Halt | 1 (0x01) | A, B, C | Halt Execution | Yes | core/vm/vm.cpp |
| Jump | 10 (0x0A) | A, B, C | Unconditional Jump | Yes | core/vm/vm.cpp |
| JumpIfZero | 11 (0x0B) | A, B, C | Jump if Zero | Yes | core/vm/vm.cpp |
| JumpIfNotZero | 25 (0x19) | A, B, C | Jump if Not Zero | Yes | core/vm/vm.cpp |
| Call | 26 (0x1A) | A, B, C | Function Call | Yes | core/vm/vm.cpp |
| Ret | 27 (0x1B) | A, B, C | Return from Call | Yes | core/vm/vm.cpp |
| JumpIfNegative | 60 (0x3C) | A, B, C | Jump if Negative | Yes | core/vm/vm.cpp |
| JumpIfPositive | 61 (0x3D) | A, B, C | Jump if Positive | Yes | core/vm/vm.cpp |
| AxHalt | 131 (0x83) | A, B, C | Axion Halt (Policy Deny) | Yes | core/vm/vm.cpp |

### 2.3 Comparison
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Cmp | 15 (0x0F) | A, B, C | Compare | Yes | core/vm/vm.cpp |
| Less | 62 (0x3E) | A, B, C | Less Than | Yes | core/vm/vm.cpp |
| LessEqual | 63 (0x3F) | A, B, C | Less Than or Equal | Yes | core/vm/vm.cpp |
| Greater | 64 (0x40) | A, B, C | Greater Than | Yes | core/vm/vm.cpp |
| GreaterEqual | 65 (0x41) | A, B, C | Greater Than or Equal | Yes | core/vm/vm.cpp |
| Equal | 66 (0x42) | A, B, C | Equal | Yes | core/vm/vm.cpp |
| NotEqual | 67 (0x43) | A, B, C | Not Equal | Yes | core/vm/vm.cpp |

### 2.4 Memory / Stack / Heap
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| LoadImm | 2 (0x02) | A, B, C | Load Immediate | Yes | core/vm/vm.cpp |
| Load | 3 (0x03) | A, B, C | Load from Memory | Yes | core/vm/vm.cpp |
| Store | 4 (0x04) | A, B, C | Store to Memory | Yes | core/vm/vm.cpp |
| Mov | 12 (0x0C) | A, B, C | Move Register | Yes | core/vm/vm.cpp |
| Push | 16 (0x10) | A, B, C | Push to Stack | Yes | core/vm/vm.cpp |
| Pop | 17 (0x11) | A, B, C | Pop from Stack | Yes | core/vm/vm.cpp |
| SetF | 44 (0x2C) | A, B, C | Set Flag Register | Yes | core/vm/vm.cpp |
| StackAlloc | 68 (0x44) | A, B, C | Allocate Stack Frame | Yes | core/vm/vm.cpp |
| StackFree | 69 (0x45) | A, B, C | Free Stack Frame | Yes | core/vm/vm.cpp |
| HeapAlloc | 70 (0x46) | A, B, C | Allocate Heap Block | Yes | core/vm/vm.cpp |
| HeapFree | 71 (0x47) | A, B, C | Free Heap Block | Yes | core/vm/vm.cpp |
| Canon | 128 (0x80) | A, B, C | Canonicalize Memory | Yes | core/vm/vm.cpp |
| MemZero | 129 (0x81) | A, B, C | Zero Memory Region | Yes | core/vm/vm.cpp |
| Copy | 130 (0x82) | A, B, C | Copy Memory Region | Yes | core/vm/vm.cpp |

### 2.5 Type Conversion
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| I2F | 29 (0x1D) | A, B, C | Int to Float | Yes | core/vm/vm.cpp |
| F2I | 30 (0x1E) | A, B, C | Float to Int | Yes | core/vm/vm.cpp |
| I2Frac | 31 (0x1F) | A, B, C | Int to Fraction | Yes | core/vm/vm.cpp |
| Frac2I | 32 (0x20) | A, B, C | Fraction to Int | Yes | core/vm/vm.cpp |
| F2Frac | 163 (0xA3) | A, B, C | Float to Fraction | Yes | core/vm/vm.cpp |
| Frac2F | 164 (0xA4) | A, B, C | Fraction to Float | Yes | core/vm/vm.cpp |

### 2.6 Logic
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| TNot | 18 (0x12) | A, B, C | Ternary Not | Yes | core/vm/vm.cpp |
| TAnd | 19 (0x13) | A, B, C | Ternary And | Yes | core/vm/vm.cpp |
| TOr | 20 (0x14) | A, B, C | Ternary Or | Yes | core/vm/vm.cpp |
| TXor | 21 (0x15) | A, B, C | Ternary Xor | Yes | core/vm/vm.cpp |

### 2.7 Axion / Policy
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| AxRead | 22 (0x16) | A, B, C | Read Axion Guard | Yes | core/vm/vm.cpp |
| AxSet | 23 (0x17) | A, B, C | Set Axion Guard | Yes | core/vm/vm.cpp |
| AxVerify | 24 (0x18) | A, B, C | Verify Axion State | Yes | core/vm/vm.cpp |
| Trap | 28 (0x1C) | A, B, C | Raise Trap | Yes | core/vm/vm.cpp |
| Assert | 132 (0x84) | A, B, C | Runtime Assertion | Yes | core/vm/vm.cpp |
| AxCheck | 158 (0x9E) | A, B, C | Axion Check (deny fails closed) | Yes | core/vm/vm.cpp |
| AxSign | 159 (0x9F) | A, B, C | Axion Sign (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| AxLineage | 160 (0xA0) | A, B, C | Axion Lineage (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| AxCanon | 161 (0xA1) | A, B, C | Axion Canon (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| AxReport | 162 (0xA2) | A, B, C | Axion Report (policy-checked) | Yes | core/vm/vm.cpp |

### 2.8 Tensor / Neural
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| ChkShape | 45 (0x2D) | A, B, C | Check Tensor Shape | Yes | core/vm/vm.cpp |
| WeightsLoad | 72 (0x48) | A, B, C | Load Weights Tensor | Yes | core/vm/vm.cpp |
| TExp | 73 (0x49) | A, B, C | Tensor Exp | Yes | core/vm/vm.cpp |
| TSqrt | 74 (0x4A) | A, B, C | Tensor Sqrt | Yes | core/vm/vm.cpp |
| TSiLU | 75 (0x4B) | A, B, C | Tensor SiLU | Yes | core/vm/vm.cpp |
| TSoftmax | 76 (0x4C) | A, B, C | Tensor Softmax | Yes | core/vm/vm.cpp |
| TRMSNorm | 77 (0x4D) | A, B, C | Tensor RMSNorm | Yes | core/vm/vm.cpp |
| TRoPE | 78 (0x4E) | A, B, C | Tensor RoPE | Yes | core/vm/vm.cpp |
| TTranspose | 80 (0x50) | A, B, C | Tensor Transpose | Yes | core/vm/vm.cpp |
| TGet | 107 (0x6B) | A, B, C | Tensor Get Element | Yes | core/vm/vm.cpp |
| TNew | 108 (0x6C) | A, B, C | Tensor New | Yes | core/vm/vm.cpp |
| TSet | 109 (0x6D) | A, B, C | Tensor Set Element | Yes | core/vm/vm.cpp |
| TLoadHash | 121 (0x79) | A, B, C | Load Tensor by Hash | Yes | core/vm/vm.cpp |
| TID | 122 (0x7A) | A, B, C | Tensor Identity | Yes | core/vm/vm.cpp |
| TNeuralFwd | 165 (0xA5) | A, B, C | Neural Forward (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| TNeuralBwd | 166 (0xA6) | A, B, C | Neural Backward (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |

### 2.9 Math (Float)
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| FSin | 81 (0x51) | A, B, C | Float Sin | Yes | core/vm/vm.cpp |
| FCos | 82 (0x52) | A, B, C | Float Cos | Yes | core/vm/vm.cpp |
| FTan | 83 (0x53) | A, B, C | Float Tan | Yes | core/vm/vm.cpp |
| FAsin | 84 (0x54) | A, B, C | Float Asin | Yes | core/vm/vm.cpp |
| FAcos | 85 (0x55) | A, B, C | Float Acos | Yes | core/vm/vm.cpp |
| FAtan | 86 (0x56) | A, B, C | Float Atan | Yes | core/vm/vm.cpp |
| FSinh | 87 (0x57) | A, B, C | Float Sinh | Yes | core/vm/vm.cpp |
| FCosh | 88 (0x58) | A, B, C | Float Cosh | Yes | core/vm/vm.cpp |
| FTanh | 89 (0x59) | A, B, C | Float Tanh | Yes | core/vm/vm.cpp |
| FSqrt | 90 (0x5A) | A, B, C | Float Sqrt | Yes | core/vm/vm.cpp |
| FExp | 91 (0x5B) | A, B, C | Float Exp | Yes | core/vm/vm.cpp |
| FLog | 92 (0x5C) | A, B, C | Float Log | Yes | core/vm/vm.cpp |
| FPow | 93 (0x5D) | A, B, C | Float Pow | Yes | core/vm/vm.cpp |

### 2.10 Meta / System
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MetaRead | 94 (0x5E) | A, B, C | Read Meta Memory | Yes | core/vm/vm.cpp |
| MetaWrite | 95 (0x5F) | A, B, C | Write Meta Memory | Yes | core/vm/vm.cpp |
| MetaReflect | 96 (0x60) | A, B, C | Reflection Capture | Yes | core/vm/vm.cpp |
| MetaRefine | 97 (0x61) | A, B, C | Meta Refinement | Yes | core/vm/vm.cpp |
| Print | 98 (0x62) | A, B, C | System Print | Yes | core/vm/vm.cpp |

### 2.11 String / Vector
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| StrLen | 99 (0x63) | A, B, C | String Length | Yes | core/vm/vm.cpp |
| StrEmpty | 100 (0x64) | A, B, C | String Is Empty | Yes | core/vm/vm.cpp |
| VecLen | 101 (0x65) | A, B, C | Vector Length | Yes | core/vm/vm.cpp |
| VecEmpty | 102 (0x66) | A, B, C | Vector Is Empty | Yes | core/vm/vm.cpp |
| VecFirst | 103 (0x67) | A, B, C | Vector First Element | Yes | core/vm/vm.cpp |
| VecLast | 104 (0x68) | A, B, C | Vector Last Element | Yes | core/vm/vm.cpp |
| VecPush | 105 (0x69) | A, B, C | Vector Push | Yes | core/vm/vm.cpp |
| VecPop | 106 (0x6A) | A, B, C | Vector Pop | Yes | core/vm/vm.cpp |
| StrConcat | 110 (0x6E) | A, B, C | String Concat | Yes | core/vm/vm.cpp |
| StrStartsWith | 111 (0x6F) | A, B, C | String Starts With | Yes | core/vm/vm.cpp |
| StrEndsWith | 112 (0x70) | A, B, C | String Ends With | Yes | core/vm/vm.cpp |
| StrContains | 113 (0x71) | A, B, C | String Contains | Yes | core/vm/vm.cpp |
| StrIndexOf | 114 (0x72) | A, B, C | String Index Of | Yes | core/vm/vm.cpp |
| StrReplace | 115 (0x73) | A, B, C | String Replace | Yes | core/vm/vm.cpp |
| StrVecNew | 116 (0x74) | A, B, C | New String Vector | Yes | core/vm/vm.cpp |
| StrVecPush | 117 (0x75) | A, B, C | String Vector Push | Yes | core/vm/vm.cpp |
| StrSplit | 118 (0x76) | A, B, C | String Split | Yes | core/vm/vm.cpp |
| StrJoin | 119 (0x77) | A, B, C | String Join | Yes | core/vm/vm.cpp |

### 2.12 Options / Results / Enums
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MakeOptionSome | 46 (0x2E) | A, B, C | Create Option Some | Yes | core/vm/vm.cpp |
| MakeOptionNone | 47 (0x2F) | A, B, C | Create Option None | Yes | core/vm/vm.cpp |
| MakeResultOk | 48 (0x30) | A, B, C | Create Result Ok | Yes | core/vm/vm.cpp |
| MakeResultErr | 49 (0x31) | A, B, C | Create Result Err | Yes | core/vm/vm.cpp |
| OptionIsSome | 50 (0x32) | A, B, C | Check Option Some | Yes | core/vm/vm.cpp |
| OptionUnwrap | 51 (0x33) | A, B, C | Unwrap Option | Yes | core/vm/vm.cpp |
| ResultIsOk | 52 (0x34) | A, B, C | Check Result Ok | Yes | core/vm/vm.cpp |
| ResultUnwrapOk | 53 (0x35) | A, B, C | Unwrap Result Ok | Yes | core/vm/vm.cpp |
| ResultUnwrapErr | 54 (0x36) | A, B, C | Unwrap Result Err | Yes | core/vm/vm.cpp |
| MakeEnumVariant | 55 (0x37) | A, B, C | Create Enum Variant | Yes | core/vm/vm.cpp |
| MakeEnumVariantPayload | 56 (0x38) | A, B, C | Create Enum Payload | Yes | core/vm/vm.cpp |
| EnumIsVariant | 57 (0x39) | A, B, C | Check Enum Variant | Yes | core/vm/vm.cpp |
| EnumUnwrapPayload | 58 (0x3A) | A, B, C | Unwrap Enum Payload | Yes | core/vm/vm.cpp |

### 2.13 Complex
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MakeComplex | 120 (0x78) | A, B, C | Create Complex Num | Yes | core/vm/vm.cpp |

### 2.14 Cognitive Tier / Reflection
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| SymLoad | 133 (0x85) | A, B, C | Symbol Load | Yes | core/vm/vm.cpp |
| SymRewrite | 134 (0x86) | A, B, C | Symbol Rewrite | Yes | core/vm/vm.cpp |
| SymConfluence | 135 (0x87) | A, B, C | Symbol Confluence | Yes | core/vm/vm.cpp |
| SymCanon | 136 (0x88) | A, B, C | Symbol Canonicalize | Yes | core/vm/vm.cpp |
| SymBind | 137 (0x89) | A, B, C | Symbol Bind | Yes | core/vm/vm.cpp |
| ReflCap | 138 (0x8A) | A, B, C | Reflection Capture | Yes | core/vm/vm.cpp |
| ReflJustify | 139 (0x8B) | A, B, C | Reflection Justify | Yes | core/vm/vm.cpp |
| ReflCheck | 140 (0x8C) | A, B, C | Reflection Check | Yes | core/vm/vm.cpp |
| ReflTrace | 141 (0x8D) | A, B, C | Reflection Trace | Yes | core/vm/vm.cpp |
| ReflSeal | 142 (0x8E) | A, B, C | Reflection Seal | Yes | core/vm/vm.cpp |
| Recurse | 143 (0x8F) | A, B, C | Recursive Descent | Yes | core/vm/vm.cpp |
| Contract | 144 (0x90) | A, B, C | Recursive Contract | Yes | core/vm/vm.cpp |
| Entropy | 145 (0x91) | A, B, C | Get Entropy | Yes | core/vm/vm.cpp |
| Depth | 146 (0x92) | A, B, C | Get Depth | Yes | core/vm/vm.cpp |
| Terminate | 147 (0x93) | A, B, C | Terminate Recursion | Yes | core/vm/vm.cpp |
| Merge | 148 (0x94) | A, B, C | Merge State | Yes | core/vm/vm.cpp |
| Gossip | 149 (0x95) | A, B, C | Gossip State | Yes | core/vm/vm.cpp |
| TickSync | 150 (0x96) | A, B, C | Tick Synchronization | Yes | core/vm/vm.cpp |
| Coherence | 151 (0x97) | A, B, C | Coherence Check | Yes | core/vm/vm.cpp |
| DistSeal | 152 (0x98) | A, B, C | Distributed Seal | Yes | core/vm/vm.cpp |
| InfSeed | 153 (0x99) | A, B, C | Infinite Seed | Yes | core/vm/vm.cpp |
| InfExpand | 154 (0x9A) | A, B, C | Infinite Expand | Yes | core/vm/vm.cpp |
| InfCollapse | 155 (0x9B) | A, B, C | Infinite Collapse | Yes | core/vm/vm.cpp |
| InfConverge | 156 (0x9C) | A, B, C | Infinite Converge | Yes | core/vm/vm.cpp |
| InfSignature | 157 (0x9D) | A, B, C | Infinite Signature | Yes | core/vm/vm.cpp |

### 2.15 Network / Async
| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| NSend | 123 (0x7B) | A, B, C | Network Send (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| NRecv | 124 (0x7C) | A, B, C | Network Recv (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| VWait | 125 (0x7D) | A, B, C | Async Wait (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |
| VYield | 126 (0x7E) | A, B, C | Async Yield (Unimplemented; fail-closed) | Yes | core/vm/vm.cpp |

### 2.16 Map / Set Scaffolding

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| MapNew | 174 (0xAE) | A, B, C | Create new Map | Yes | core/vm/vm.cpp |
| MapPut | 175 (0xAF) | A, B, C | Insert key-value into Map | Yes | core/vm/vm.cpp |
| MapGet | 176 (0xB0) | A, B, C | Get value from Map by key | Yes | core/vm/vm.cpp |
| MapHas | 177 (0xB1) | A, B, C | Check key presence in Map | Yes | core/vm/vm.cpp |
| MapRemove | 178 (0xB2) | A, B, C | Remove key from Map | Yes | core/vm/vm.cpp |
| MapKeys | 179 (0xB3) | A, B, C | Get all keys from Map | Yes | core/vm/vm.cpp |
| MapSize | 180 (0xB4) | A, B, C | Get Map size | Yes | core/vm/vm.cpp |
| SetNew | 181 (0xB5) | A, B, C | Create new Set | Yes | core/vm/vm.cpp |
| SetAdd | 182 (0xB6) | A, B, C | Add element to Set | Yes | core/vm/vm.cpp |
| SetRemove | 183 (0xB7) | A, B, C | Remove element from Set | Yes | core/vm/vm.cpp |
| SetHas | 184 (0xB8) | A, B, C | Check element in Set | Yes | core/vm/vm.cpp |
| SetSize | 185 (0xB9) | A, B, C | Get Set size | Yes | core/vm/vm.cpp |

### 2.17 Tensor Shape

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| TShape | 186 (0xBA) | A, B, C | Get tensor dimension size: `R[A] = shape[R[B]][C]` | Yes | core/vm/vm.cpp |

### 2.18 AI-Native Inference (RFC-0026)

All opcodes in this class operate on TensorHandle registers. All are Tier 2+ only. All are subject to Axion pre-instruction verification.

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| ATTN | 187 (0xBB) | A, PACK(B,C) | Scaled dot-product attention: `R[A] = softmax(Q·Kᵀ/√dₖ)·V` | Yes | core/vm/vm.cpp |
| QMATMUL | 188 (0xBC) | A, PACK(B,C) | Quantized matmul: `R[A] = dequantize(R_WT, R_SCALE) · R_ACT` | Yes | core/vm/vm.cpp |
| EMBED | 189 (0xBD) | A, B, C | Embedding lookup: gather rows from table `R[B]` at indices `R[C]` | Yes | core/vm/vm.cpp |
| WLOAD | 190 (0xBE) | A, B, C | Weight load with Axion policy gate; emits CanonFS WeightLoad audit event (AI-M4) | Yes | core/vm/vm.cpp |
| GATHER | 191 (0xBF) | A, B, PACK(IDX,AXIS) | Sparse gather: gather index `R[IDX]` from tensor `R[B]` along axis `R[AXIS]` (AI-M5) | Yes | core/vm/vm.cpp |
| SCATTER | 192 (0xC0) | A, B, PACK(IDX,SRC) | Sparse scatter-add: scatter `R[SRC]` into `R[B]` at `R[IDX]`; aliasing detection enforced (AI-M5) | Yes | core/vm/vm.cpp |
| Int2BigInt | 193 (0xC1) | A, B, C | Convert integer register to BigInt handle | Yes | core/vm/vm.cpp |

### 2.19 Ternary-Native Inference (RFC-0034)

All opcodes in this class require Tier 2+. Weights must be in {−1, 0, +1} (T81Qutrit domain).
TACT carries a post-execute Axion activation-ceiling gate (§5.17.6).

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| TWMATMUL | 194 (0xC2) | A, B, C | Ternary-weight matmul; T81BigInt accumulator; no FP multiply | Yes | core/vm/vm.cpp |
| TQUANT | 195 (0xC3) | A, B, C | Quantize tensor to ternary {−1,0,+1} using threshold R[C] | Yes | core/vm/vm.cpp |
| TATTN | 196 (0xC4) | A, B, PACK(C) | Ternary Q/K attention; ternary matmul + softmax + float V-proj | Yes | core/vm/vm.cpp |
| TWEMBED | 197 (0xC5) | A, B, C | Row-gather from T81Qutrit embedding table at index R[C] | Yes | core/vm/vm.cpp |
| TERNACCUM | 198 (0xC6) | A, B, C | Scalar ternary dot product; result stored as T81BigInt handle | Yes | core/vm/vm.cpp |
| TACT | 199 (0xC7) | A, B, C | Ternary activation (mode R[C]); activation-ceiling Axion gate | Yes | core/vm/vm.cpp |

#### TACT Modes

| Mode Byte | Name | Semantics |
| :--- | :--- | :--- |
| `0x01` | TernaryStep | `x > 0.5 → +1; x < −0.5 → −1; else 0` |
| `0x02` | TanhQuantized | `tanh(x) > 0.5 → +1; tanh(x) < −0.5 → −1; else 0` |

Post-execute Axion `activation-ceiling` verdict model:

| Verdict | Effect |
| :--- | :--- |
| Allow | RD committed; PC advances normally |
| Quarantine | RD not committed; PC does not advance; `SecurityFault` raised |
| Deny | RD not committed; `ActivationFault` raised |

### 2.20 Governed Foreign Function Interface (RFC-0036 + RFC-00B8)

Opcodes for governed calls to external (non-T81) functions. All are subject to
`FFIDispatcher` policy enforcement, resource quotas, and Axion audit events.

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| FFICall | 200 (0xC8) | A, B, C | Call foreign function; `text_literal` carries function name; policy check before dispatch | Conditional (policy-dependent) | core/vm/vm.cpp |
| FFIRegister | 201 (0xC9) | A, B | Register foreign library by name `R[A]` and version hash `R[B]` | Yes | core/vm/vm.cpp |
| FFIPolicySet | 202 (0xCA) | A, B | Set per-call FFI policy type `R[A]` to value `R[B]` | Yes | core/vm/vm.cpp |

### 2.21 Ternary Lattice Cryptography (RFC-0038)

Negacyclic polynomial arithmetic over `{−1, 0, +1}` coefficients in `Z[x]/(x^n + 1)`.
No integer multiplications — only add/sub/trit-flip. T81BigInt-exact. Tier 2+.

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| POLYMUL | 203 (0xCB) | A, B, C | Negacyclic poly multiply: `R[A] = polymul(*tensor(R[B]), *tensor(R[C]))` in `Z[x]/(x^n+1)` | Yes | core/vm/vm.cpp |
| POLYMOD | 204 (0xCC) | A, B, C | Centered reduction mod q: every coefficient c → `((c%q)+q)%q`, shifted to `(−q/2, q/2]`; `q = R[C]` | Yes | core/vm/vm.cpp |

### 2.22 NTRU-KEM Polynomial Ring Arithmetic (RFC-0039)

Elementwise polynomial subtraction over tensor handles; completes the `{+, −, ×, mod}` ring surface.

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| TVecSub | 212 (0xD4) | A, B, C | Elementwise subtraction: `R[A][k] = tensor(R[B])[k] − tensor(R[C])[k]` for all k | Yes | core/vm/vm.cpp |

### 2.23 SWAR Tensor Operations (RFC-0040)

Explicit SWAR dispatch over `ExactTrit` tensor handles. These opcodes are stable
entry points for RFC-0040 semantics and bypass scalar ternary-trit execution.

| Mnemonic | Numeric Encoding | Operands | Description | Deterministic | Implementation Location |
| :--- | :--- | :--- | :--- | :--- | :--- |
| TNOT_SWAR | 213 (0xD5) | A, B, C | Exact-trit tensor unary negation via stable SWAR kernels: `R[A] = swar_not(tensor(R[B]))` | Yes | core/vm/vm.cpp |
| TAND_SWAR | 214 (0xD6) | A, B, C | Exact-trit tensor elementwise ternary min via SWAR: `R[A] = swar_and(tensor(R[B]), tensor(R[C]))` | Yes | core/vm/vm.cpp |
| TOR_SWAR | 215 (0xD7) | A, B, C | Exact-trit tensor elementwise ternary max via SWAR: `R[A] = swar_or(tensor(R[B]), tensor(R[C]))` | Yes | core/vm/vm.cpp |

## 3. Reserved / Unused Opcodes

- **Nop (0x00)**: No Operation.
- **Reserved**: Opcodes 216 (0xD8) through 255 (0xFF) are reserved for future standardization.

## 4. Implementation Consistency Audit

- **VM Opcode Count**: 209 defined opcodes (0x00–0xD7); includes RFC-0034, RFC-00B8, RFC-0038, RFC-0039, RFC-0040 additions.
- **Header Enum Count**: 209 entries in `include/t81/isa/opcodes.hpp`.
- **Coverage**: All opcodes defined in `include/t81/isa/opcodes.hpp` are present in `core/vm/vm.cpp` dispatch switch.
- **Discrepancies**: None found.
