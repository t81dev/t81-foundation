# HanoiVM / TISC Opcode Reference

## Overview

The **HanoiVM** (Virtual Machine) executes programs compiled to the **TISC** (Ternary Instruction Set Computer) bytecode. It is designed for the T81 ternary computing stack, emphasizing deterministic execution, cognitive tier management, and secure tensor operations.

The instruction format is a fixed 13-byte sequence:
- **Opcode** (1 byte): The operation identifier.
- **Operand A** (4 bytes, little-endian): Typically the destination register index.
- **Operand B** (4 bytes, little-endian): Source register index, immediate value, or handle.
- **Operand C** (4 bytes, little-endian): Source register index or secondary immediate.

**Notation Legend:**
- `R[A]`, `R[B]`, `R[C]`: General-purpose registers (indexed 0–74).
- `Imm`: Immediate value (stored in Operand B or C).
- `Mem[addr]`: Memory access at the given address.
- `Handle`: An index into a resource pool (Tensor, String, Float, etc.).
- `Tier`: Cognitive Tier level (1–5).

**Global Behaviors:**
- **Axion Checks**: Every instruction is subject to `Axion` policy verification before execution. Policies can deny execution based on opcode, operands, or system state.
- **Type Promotion**: Operations on `int` generally remain `int`. Floating-point operations use `FloatHandle` or `FractionHandle`. Tensor operations automatically promote compatible inputs (e.g., `WeightsTensorHandle`) to `TensorHandle`.
- **Traps**: Execution halts on traps, including `DecodeFault` (invalid instruction), `BoundsFault` (memory access violation), `TypeFault` (invalid operand type), and `SecurityFault` (Axion policy denial).

---

## Core Arithmetic & Control-Flow Opcodes

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x00` | `NOP` | - | - | No Operation. Can carry a symbol handle in `B` for logging. | Axion log event |
| `0x01` | `HALT` | - | - | Stops execution of the current context. | Sets `halted` flag |
| `0x02` | `LOADIMM` | `A: Dest, B: Imm/Handle` | - | Loads immediate `B` into `R[A]`. Type determined by literal kind (Int, Float, Symbol, etc.). | DecodeFault |
| `0x03` | `LOAD` | `A: Dest, B: Addr` | - | Loads value from memory address `B` into `R[A]`. | BoundsFault |
| `0x04` | `STORE` | `A: Addr, B: Src` | - | Stores value in `R[B]` to memory address `A`. | BoundsFault |
| `0x05` | `ADD` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = R[B] + R[C]` (Integer). | DecodeFault |
| `0x06` | `SUB` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = R[B] - R[C]` (Integer). | DecodeFault |
| `0x07` | `MUL` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = R[B] * R[C]` (Integer). | DecodeFault |
| `0x08` | `DIV` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = R[B] / R[C]` (Integer). | DivisionFault |
| `0x09` | `MOD` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = R[B] % R[C]` (Integer). | DivisionFault |
| `0x0A` | `JUMP` | `A: Addr` | - | Unconditional jump to address `A`. | DecodeFault |
| `0x0B` | `JUMPIFZERO` | `A: Addr, B: Cond` | - | Jump to `A` if `R[B] == 0`. | DecodeFault |
| `0x0C` | `MOV` | `A: Dest, B: Src` | - | Copies `R[B]` to `R[A]`. | DecodeFault |
| `0x0D` | `INC` | `A: Reg` | - | Increments `R[A]` by 1. | DecodeFault |
| `0x0E` | `DEC` | `A: Reg` | - | Decrements `R[A]` by 1. | DecodeFault |
| `0x0F` | `CMP` | `A: Src1, B: Src2` | - | Compares `R[A]` and `R[B]`, sets flags (Zero, Positive, Negative). | DecodeFault |
| `0x10` | `PUSH` | `A: Src` | -1 | Pushes `R[A]` onto the stack. | StackFault |
| `0x11` | `POP` | `A: Dest` | +1 | Pops top of stack into `R[A]`. | StackFault |
| `0x12` | `TNOT` | `A: Dest, B: Src` | - | Ternary NOT: `-1 -> 1`, `0 -> 0`, `1 -> -1`. | DecodeFault |
| `0x13` | `TAND` | `A: Dest, B: Src1, C: Src2` | - | Ternary AND (Min function). | DecodeFault |
| `0x14` | `TOR` | `A: Dest, B: Src1, C: Src2` | - | Ternary OR (Max function). | DecodeFault |
| `0x15` | `TXOR` | `A: Dest, B: Src1, C: Src2` | - | Ternary XOR / Sum. | DecodeFault |
| `0x19` | `JUMPIFNOTZERO` | `A: Addr, B: Cond` | - | Jump to `A` if `R[B] != 0`. | DecodeFault |
| `0x1A` | `CALL` | `B: Addr` | -1 | Push `PC` to stack and jump to `B`. Checks recursion depth/tier. | StackFault, SecurityFault |
| `0x1B` | `RET` | - | +1 | Pop return address from stack and jump to it. | StackFault |
| `0x1C` | `TRAP` | - | - | Explicitly triggers a trap. | TrapInstruction |
| `0x3B` | `NEG` | `A: Dest, B: Src` | - | `R[A] = -R[B]` (Integer). | DecodeFault |
| `0x3C` | `JUMPIFNEGATIVE` | `A: Addr` | - | Jump to `A` if `Negative` flag is set. | DecodeFault |
| `0x3D` | `JUMPIFPOSITIVE` | `A: Addr` | - | Jump to `A` if `Positive` flag is set. | DecodeFault |
| `0x3E` | `LESS` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = (R[B] < R[C])`. | TypeFault |
| `0x3F` | `LESSEQUAL` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = (R[B] <= R[C])`. | TypeFault |
| `0x40` | `GREATER` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = (R[B] > R[C])`. | TypeFault |
| `0x41` | `GREATEREQUAL` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = (R[B] >= R[C])`. | TypeFault |
| `0x42` | `EQUAL` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = (R[B] == R[C])`. | TypeFault |
| `0x43` | `NOTEQUAL` | `A: Dest, B: Src1, C: Src2` | - | `R[A] = (R[B] != R[C])`. | TypeFault |
| `0x62` | `PRINT` | `A: Reg` | - | Prints value in `R[A]` to output. | TypeFault |
| `0x63` | `STRLEN` | `A: Dest, B: StrHandle` | - | Gets length of string at `R[B]`. | DecodeFault |
| `0x6E` | `STRCONCAT` | `A: Dest, B: Str1, C: Str2` | - | Concatenates strings. | DecodeFault |
| `0x84` | `ASSERT` | `A: Cond` | - | Traps if `R[A]` is 0. | AssertionFailed |

---

## Type Promotion / Demotion Opcodes

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x1D` | `I2F` | `A: Dest, B: Int` | - | Converts Integer `R[B]` to Float Handle `R[A]`. | DecodeFault |
| `0x1E` | `F2I` | `A: Dest, B: Float` | - | Converts Float Handle `R[B]` to Integer `R[A]`. | TypeFault |
| `0x1F` | `I2FRAC` | `A: Dest, B: Int` | - | Converts Integer `R[B]` to Fraction Handle `R[A]`. | DecodeFault |
| `0x20` | `FRAC2I` | `A: Dest, B: Frac` | - | Converts Fraction Handle `R[B]` to Integer `R[A]` (if denominator is 1). | TypeFault |
| `0xA3` | `F2FRAC` | `A: Dest, B: Float` | - | Converts Float `R[B]` to Fraction `R[A]`. | TypeFault |
| `0xA4` | `FRAC2F` | `A: Dest, B: Frac` | - | Converts Fraction `R[B]` to Float `R[A]`. | TypeFault |
| `0x2E` | `MAKEOPTIONSOME` | `A: Dest, B: Val` | - | Creates `Option::Some(R[B])`. | DecodeFault |
| `0x2F` | `MAKEOPTIONNONE` | `A: Dest` | - | Creates `Option::None`. | DecodeFault |
| `0x30` | `MAKERESULTOK` | `A: Dest, B: Val` | - | Creates `Result::Ok(R[B])`. | DecodeFault |
| `0x31` | `MAKERESULTERR` | `A: Dest, B: Val` | - | Creates `Result::Err(R[B])`. | DecodeFault |
| `0x32` | `OPTIONISSOME` | `A: Dest, B: Opt` | - | Checks if Option `R[B]` is Some. | TypeFault |
| `0x33` | `OPTIONUNWRAP` | `A: Dest, B: Opt` | - | Unwraps Option `R[B]` or traps if None. | DecodeFault |
| `0x78` | `MAKECOMPLEX` | `A: Dest, B: Re, C: Im` | - | Creates Complex number from Integers `R[B]`, `R[C]`. | TypeFault |

---

## Cognitive Tier 3 – Recursion & Distribution

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x8F` | `RECURSE` | - | - | Increases recursion depth; verifies Tier 3 limits. | SecurityFault |
| `0x90` | `CONTRACT` | `B: Entropy` | - | Verifies entropy contraction against previous frame. | DecodeFault |
| `0x91` | `ENTROPY` | `A: Dest` | - | Calculates current system entropy (stack + heap + depth). | DecodeFault |
| `0x92` | `DEPTH` | `A: Dest` | - | returns current recursion depth to `R[A]`. | DecodeFault |
| `0x93` | `TERMINATE` | - | - | Decreases recursion depth (pops recursion frame). | - |
| `0x94` | `MERGE` | `A: Dest` | - | Merges distributed state from inbox. | Axion Event |
| `0x95` | `GOSSIP` | `B: Val` | - | Broadcasts value `R[B]` to distributed network (Tier 4). | Axion Event |
| `0x96` | `TICKSYNC` | `A: RemoteTick` | - | Synchronizes local tick with remote tick `R[A]`. | Axion Event |
| `0x97` | `COHERENCE` | `A: Dest` | - | Checks coherence (tick drift) and stores in `R[A]`. | Axion Event |
| `0x98` | `DISTSEAL` | `A: Dest` | - | Seals distributed state and returns seal hash. | Axion Event |

---

## Cognitive Tier 4 – Reflection & Justification

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x8A` | `REFLCAP` | `A: Dest, B: Desc` | - | Captures current state (PC, Regs) into a Reflective Frame. | SecurityFault |
| `0x8B` | `REFLJUSTIFY` | `A: Frame, B: Text` | - | Adds a justification step (text) to the frame. | DecodeFault |
| `0x8C` | `REFLCHECK` | `A: Dest, B: Frame, C: Crit` | - | Verifies frame against criteria `R[C]`. | TypeFault |
| `0x8D` | `REFLTRACE` | `A: Frame` | - | Appends current register state to frame's trace. | BoundsFault |
| `0x8E` | `REFLSEAL` | `A: Frame` | - | Seals the reflective frame (hashing content). | BoundsFault |

---

## Cognitive Tier 5 – Infinite & Series Compression

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x99` | `INFSEED` | `A: Dest, B: Val` | - | Seeds an Infinite Canonical Form with value `R[B]`. | TypeFault |
| `0x9A` | `INFEXPAND` | `A: Form, B: Ratio` | - | Expands infinite form with ratio `R[B]`. | BoundsFault |
| `0x9B` | `INFCOLLAPSE` | `A: Form` | - | Collapses infinite form to its limit/sum. | BoundsFault |
| `0x9C` | `INFCONVERGE` | `A: Dest, B: Form` | - | Checks if infinite form converges. | BoundsFault |
| `0x9D` | `INFSIGNATURE` | `A: Dest, B: Form` | - | Generates signature hash for the infinite form. | BoundsFault |

---

## Neural & Tensor Primitives (RFC-0014)

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x48` | `WEIGHTSLOAD` | `A: Dest, B: NameHandle` | - | Loads pre-trained weights by name. | DecodeFault |
| `0xA5` | `TNEURALFWD` | `A: Dest, B: Input` | - | Performs neural forward pass (Identity/Stub). | Axion Event |
| `0xA6` | `TNEURALBWD` | `A: Model` | - | Performs neural backward pass (Stub). | Axion Event |
| `0x21` | `TVECADD` | `A: Dest, B: T1, C: T2` | - | Tensor element-wise addition. | ShapeFault |
| `0x4F` | `TVECMUL` | `A: Dest, B: T1, C: T2` | - | Tensor element-wise multiplication. | ShapeFault |
| `0x22` | `TMATMUL` | `A: Dest, B: T1, C: T2` | - | Tensor Matrix Multiplication. | ShapeFault |
| `0x23` | `TTENDOT` | `A: Dest, B: T1, C: T2` | - | Tensor Dot Product (Contraction). | ShapeFault |
| `0x50` | `TTRANSPOSE` | `A: Dest, B: Tens` | - | Transposes tensor (2D). | ShapeFault |
| `0x49` | `TEXP` | `A: Dest, B: Tens` | - | Element-wise Exponential. | DecodeFault |
| `0x4A` | `TSQRT` | `A: Dest, B: Tens` | - | Element-wise Square Root. | DecodeFault |
| `0x4B` | `TSILU` | `A: Dest, B: Tens` | - | Applies SiLU activation. | Axion Event |
| `0x4C` | `TSOFTMAX` | `A: Dest, B: Tens` | - | Applies Softmax activation. | Axion Event |
| `0x4D` | `TRMSNORM` | `A: Dest, B: Tens, C: W` | - | Applies RMSNorm with weights `R[C]`. | ShapeFault |
| `0x4E` | `TROPE` | `A: Dest, B: Tens, C: Pos` | - | Applies Rotary Positional Embedding. | ShapeFault |
| `0x7F` | `TNORM` | `A: Dest, B: Src` | - | Normalizes/Clamps trit value. | DecodeFault |

---

## CanonFS / Tensor Loading & Integrity

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x79` | `TLOADHASH` | `A: Dest, B: HashStr` | - | Loads tensor from CanonFS by SHA3-256 hash. Verifies integrity. | SecurityFault, BoundsFault |
| `0x80` | `CANON` | `A: Addr` | - | Canonicalizes memory/register at `R[A]`. | Axion Event |
| `0x81` | `MEMZERO` | `A: Addr, B: Size` | - | Zeroes out memory region. | BoundsFault |
| `0x82` | `COPY` | `A: Dst, B: Src, C: Len` | - | Copies memory region. | BoundsFault |

---

## Axion / Policy / Safety Enforcement Opcodes & Hooks

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x16` | `AXREAD` | `A: Dest, B: GuardAddr` | - | Reads guarded memory with Axion verification. | SecurityFault |
| `0x17` | `AXSET` | `A: GuardAddr, B: Val` | - | Sets guarded memory with Axion verification. | SecurityFault |
| `0x18` | `AXVERIFY` | `A: Dest` | - | Returns Axion policy verification status (Allow/Deny). | SecurityFault |
| `0x83` | `AXHALT` | - | - | Emergency halt triggered by Axion. | SecurityFault |
| `0x9E` | `AXCHECK` | - | - | Stub: Explicit Axion Policy Check. | Axion Event |
| `0x9F` | `AXSIGN` | - | - | Stub: Signs execution state. | Axion Event |
| `0xA0` | `AXLINEAGE` | - | - | Stub: Records lineage event. | Axion Event |
| `0xA1` | `AXCANON` | - | - | Stub: Enforces canonicalization policy. | Axion Event |
| `0xA2` | `AXREPORT` | - | - | Stub: Generates safety report. | Axion Event |

---

## Concurrency & Segment Management

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x7B` | `NSEND` | `B: Handle` | - | Sends handle/data to network (Placeholder). | Axion Event |
| `0x7C` | `NRECV` | `A: Dest` | - | Receives data from network (Placeholder). | Axion Event |
| `0x7D` | `VWAIT` | `A: Dest, B: Promise` | - | Waits on a promise/handle. | Axion Event |
| `0x7E` | `VYIELD` | `B: Handle` | - | Yields execution result. | Axion Event |
| `0x44` | `STACKALLOC` | `A: Dest, B: Size` | - | Allocates stack frame of `B` bytes. | StackFault |
| `0x45` | `STACKFREE` | `A: Addr, B: Size` | - | Frees stack frame. | StackFault |
| `0x46` | `HEAPALLOC` | `A: Dest, B: Size` | - | Allocates heap block. | BoundsFault |
| `0x47` | `HEAPFREE` | `A: Addr, B: Size` | - | Frees heap block. | BoundsFault |

---

## Miscellaneous / Utility Opcodes

| Opcode | Mnemonic | Operands / Encoding | Stack Effect | Description / Semantics | Side Effects / Traps |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `0x5E` | `METAREAD` | `A:Dst, B:Seg, C:Addr` | - | Reflective read from memory segment `B` at `R[C]`. | SecurityFault |
| `0x5F` | `METAWRITE` | `A:Val, B:Seg, C:Addr` | - | Reflective write to memory segment `B`. | SecurityFault |
| `0x60` | `METAREFLECT` | `A: Dest` | - | Captures full VM state snapshot. | SecurityFault |
| `0x61` | `METAREFINE` | `A:Res, B:Cmds, C:Cnt` | - | Applies atomic meta-refinement commands. | SecurityFault |
| `0x85` | `SYMLOAD` | `A: Dest, B: Sym` | - | Loads a symbolic graph from symbol `R[B]`. | DecodeFault |
| `0x86` | `SYMREWRITE` | `A: Graph, B: Match` | - | Rewrites graph `R[A]` using rule `R[B]->R[C]`. | TypeFault |
| `0x87` | `SYMCONFLUENCE` | `A: Dest, B: Graph` | - | Checks confluence of symbolic graph. | TypeFault |
| `0x88` | `SYMCANON` | `A: Graph` | - | Canonicalizes symbolic graph `R[A]`. | TypeFault |
| `0x89` | `SYMBIND` | - | - | Binds symbol (Placeholder). | - |
| `0x7A` | `TID` | `A: Dest, B: Tens` | - | Creates identity/copy of tensor `R[B]`. | DecodeFault |
| `0x6B` | `TGET` | `A: Dest, B: Tens, C:Idx` | - | Gets element at `R[C]` from tensor `R[B]`. | BoundsFault |
| `0x6D` | `TSET` | `A: Tens, B: Idx, C:Val` | - | Sets element at `R[B]` in tensor `R[A]`. | BoundsFault |
| `0x6C` | `TNEW` | `A: Dest, B: Size` | - | Allocates new tensor of size `R[B]`. | BoundsFault |

---

## Appendix: Opcode Map

1.  `0x00` NOP
2.  `0x01` HALT
3.  `0x02` LOADIMM
4.  `0x03` LOAD
5.  `0x04` STORE
6.  `0x05` ADD
7.  `0x06` SUB
8.  `0x07` MUL
9.  `0x08` DIV
10. `0x09` MOD
11. `0x0A` JUMP
12. `0x0B` JUMPIFZERO
13. `0x0C` MOV
14. `0x0D` INC
15. `0x0E` DEC
16. `0x0F` CMP
17. `0x10` PUSH
18. `0x11` POP
19. `0x12` TNOT
20. `0x13` TAND
21. `0x14` TOR
22. `0x15` TXOR
23. `0x16` AXREAD
24. `0x17` AXSET
25. `0x18` AXVERIFY
26. `0x19` JUMPIFNOTZERO
27. `0x1A` CALL
28. `0x1B` RET
29. `0x1C` TRAP
30. `0x1D` I2F
31. `0x1E` F2I
32. `0x1F` I2FRAC
33. `0x20` FRAC2I
34. `0x21` TVECADD
35. `0x22` TMATMUL
36. `0x23` TTENDOT
37. `0x24` FADD
38. `0x25` FSUB
39. `0x26` FMUL
40. `0x27` FDIV
41. `0x28` FRACADD
42. `0x29` FRACSUB
43. `0x2A` FRACMUL
44. `0x2B` FRACDIV
45. `0x2C` SETF
46. `0x2D` CHKSHAPE
47. `0x2E` MAKEOPTIONSOME
48. `0x2F` MAKEOPTIONNONE
49. `0x30` MAKERESULTOK
50. `0x31` MAKERESULTERR
51. `0x32` OPTIONISSOME
52. `0x33` OPTIONUNWRAP
53. `0x34` RESULTISOK
54. `0x35` RESULTUNWRAPOK
55. `0x36` RESULTUNWRAPERR
56. `0x37` MAKEENUMVARIANT
57. `0x38` MAKEENUMVARIANTPAYLOAD
58. `0x39` ENUMISVARIANT
59. `0x3A` ENUMUNWRAPPAYLOAD
60. `0x3B` NEG
61. `0x3C` JUMPIFNEGATIVE
62. `0x3D` JUMPIFPOSITIVE
63. `0x3E` LESS
64. `0x3F` LESSEQUAL
65. `0x40` GREATER
66. `0x41` GREATEREQUAL
67. `0x42` EQUAL
68. `0x43` NOTEQUAL
69. `0x44` STACKALLOC
70. `0x45` STACKFREE
71. `0x46` HEAPALLOC
72. `0x47` HEAPFREE
73. `0x48` WEIGHTSLOAD
74. `0x49` TEXP
75. `0x4A` TSQRT
76. `0x4B` TSILU
77. `0x4C` TSOFTMAX
78. `0x4D` TRMSNORM
79. `0x4E` TROPE
80. `0x4F` TVECMUL
81. `0x50` TTRANSPOSE
82. `0x51` FSIN
83. `0x52` FCOS
84. `0x53` FTAN
85. `0x54` FASIN
86. `0x55` FACOS
87. `0x56` FATAN
88. `0x57` FSINH
89. `0x58`FCOSH
90. `0x59` FTANH
91. `0x5A` FSQRT
92. `0x5B` FEXP
93. `0x5C` FLOG
94. `0x5D` FPOW
95. `0x5E` METAREAD
96. `0x5F` METAWRITE
97. `0x60` METAREFLECT
98. `0x61` METAREFINE
99. `0x62` PRINT
100. `0x63` STRLEN
101. `0x64` STREMPTY
102. `0x65` VECLEN
103. `0x66` VECEMPTY
104. `0x67` VECFIRST
105. `0x68` VECLAST
106. `0x69` VECPUSH
107. `0x6A` VECPOP
108. `0x6B` TGET
109. `0x6C` TNEW
110. `0x6D` TSET
111. `0x6E` STRCONCAT
112. `0x6F` STRSTARTSWITH
113. `0x70` STRENDSWITH
114. `0x71` STRCONTAINS
115. `0x72` STRINDEXOF
116. `0x73` STRREPLACE
117. `0x74` STRVECNEW
118. `0x75` STRVECPUSH
119. `0x76` STRSPLIT
120. `0x77` STRJOIN
121. `0x78` MAKECOMPLEX
122. `0x79` TLOADHASH
123. `0x7A` TID
124. `0x7B` NSEND
125. `0x7C` NRECV
126. `0x7D` VWAIT
127. `0x7E` VYIELD
128. `0x7F` TNORM
129. `0x80` CANON
130. `0x81` MEMZERO
131. `0x82` COPY
132. `0x83` AXHALT
133. `0x84` ASSERT
134. `0x85` SYMLOAD
135. `0x86` SYMREWRITE
136. `0x87` SYMCONFLUENCE
137. `0x88` SYMCANON
138. `0x89` SYMBIND
139. `0x8A` REFLCAP
140. `0x8B` REFLJUSTIFY
141. `0x8C` REFLCHECK
142. `0x8D` REFLTRACE
143. `0x8E` REFLSEAL
144. `0x8F` RECURSE
145. `0x90` CONTRACT
146. `0x91` ENTROPY
147. `0x92` DEPTH
148. `0x93` TERMINATE
149. `0x94` MERGE
150. `0x95` GOSSIP
151. `0x96` TICKSYNC
152. `0x97` COHERENCE
153. `0x98` DISTSEAL
154. `0x99` INFSEED
155. `0x9A` INFEXPAND
156. `0x9B` INFCOLLAPSE
157. `0x9C` INFCONVERGE
158. `0x9D` INFSIGNATURE
159. `0x9E` AXCHECK
160. `0x9F` AXSIGN
161. `0xA0` AXLINEAGE
162. `0xA1` AXCANON
163. `0xA2` AXREPORT
164. `0xA3` F2FRAC
165. `0xA4` FRAC2F
166. `0xA5` TNEURALFWD
167. `0xA6` TNEURALBWD
