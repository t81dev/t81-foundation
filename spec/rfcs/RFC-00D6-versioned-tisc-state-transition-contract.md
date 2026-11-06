# RFC-00D6: Versioned TISC State-Transition Contract and Profile Selection Matrix

**Status:** draft — proposed requirements, not binding production behavior
**Type:** standards-track
**Created:** 2026-11-05
**Updated:** 2026-11-06
**Applies-To:** TISC ISA specification (`spec/tisc-spec.md`, `spec/tisc/`), hosted `T81VM` (`vm/vm.cpp`), freestanding Axion OS evaluator (`ternaryos/hal/cpa004_evaluator.cpp`), LLVM/MLIR lowering backends (`lang/llvm/tisc_to_llvm.cpp`), and cross-profile bytecode artifacts
**Supersedes:** nothing; establishes the formal decision matrix and migration framework for ISA-001 reconciliation findings

______________________________________________________________________

## 1. Executive Summary & Governance Authority

This RFC establishes an authoritative, versioned state-transition contract for the Ternary Instruction Set Computer (TISC) and presents a comprehensive maintainer decision matrix addressing known semantic discrepancies between normative specifications, runtime implementations, compiler backends, and frozen experimental artifacts.

### 1.1 Non-Normative Draft Posture
This document is an **authoritative draft proposal and maintainer decision instrument** prepared under task **T81-ISA-002**. In accordance with T81 governance rules:
- **No silent semantic choices are adopted**: Competing interpretations are explicitly detailed with historical evidence, compatibility impacts, and trade-offs.
- **No production code or frozen artifacts are altered**: Runtime implementations (`vm/vm.cpp`, `ternaryos/`), test suites, and CPA evidence packets remain untouched.
- **Explicit maintainer authorization is required**: Any change to normative instruction semantics, opcode registries, or execution profiles requires formal signature in Section 10 before implementation or promotion. Opcode inventory expansion and assignment modifications remain strictly proposed options pending maintainer review.

### 1.2 The Research & Conformance Context
The Cross-Profile Architecture (CPA) program successfully demonstrated cross-profile deterministic execution across hosted `T81VM` and bare-metal freestanding Axion OS (post-`ExitBootServices` AArch64 QEMU):
1. **CPA-004/005**: Proved bit-exact trace parity (PC=27, SP=283, 24 instructions, 25 observation boundaries) on the canonical 351-byte program (SHA-256: `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`).
2. **CPA-006**: Achieved **PASS — BOUNDED CONFORMANCE** across 19 independently frozen conformance cases (C01–C12), validating 1,190 state records over 57 hosted runs and 57 fresh QEMU boots.
3. **ISA-001**: A read-only semantic audit established that while hosted and freestanding runtimes execute identically for the admitted 17-opcode CPA subset, this executed profile diverges fundamentally from the normative specification in `spec/tisc-spec.md` and `include/t81/llvm/tisc_to_llvm.hpp`.

Crucially, **raw 13-byte instruction records carry no inherent profile tag or version header**. Consequently, any semantic reconciliation must be achieved through explicit, artifact-bound ISA profile selection rather than retroactive reinterpretation of existing binaries.

______________________________________________________________________

## 2. Structural Lineage and System Roles

T81 system architecture strictly delineates computational identity:
- **T81**: The experimental computational architecture and mathematical system.
- **TISC**: The computational instruction contract (encodings, operand layouts, and state transition invariants).
- **Hosted T81VM**: The userspace virtual machine execution profile (`vm/vm.cpp`).
- **Freestanding Axion OS (TernaryOS)**: The bare-metal operating system execution profile executing on AArch64 hardware/QEMU (`ternaryos/`).
- **DAIOS**: Forward-looking autonomous systems research; completely separate from baseline TISC state-transition semantics.

Historical CPA conformance results validate that hosted T81VM and freestanding Axion implement the *same execution profile*; they do not validate that this implemented profile conforms to `spec/tisc-spec.md`.

______________________________________________________________________

## 3. Discrepancy Register & Competing Interpretations

### 3.1 Conflict 1: Memory Access Operands (LOAD and STORE)

#### Competing Interpretations
- **Interpretation A (Implemented CPA Profile — Direct Addressing)**:
  - `LOAD` (`0x03`): `R[A] = MEM[B]` where operand `B` is a direct immediate logical memory address ($27 \le B < 1563$).
  - `STORE` (`0x04`): `MEM[A] = R[B]` where operand `A` is a direct immediate logical memory address ($27 \le A < 1563$), and operand `B` is a source register index ($0 \le B \le 80$).
  - Operand `C` is unused (must be 0).
- **Interpretation B (Normative Specification — Register-Indirect Addressing)**:
  - `LOAD`: `R[RD] = MEM[R[RS]]` where operand `B` specifies source register `RS`, whose payload contains the memory address.
  - `STORE`: `MEM[R[RS]] = R[RD]` where operand `A` specifies pointer register `RS` and operand `B` specifies source register `RD`.

#### Exact Code & Documentary Evidence
- **Implemented Runtime (`vm/vm.cpp:1811-1827`)**:
  ```cpp
  case t81::tisc::Opcode::Load: {
    if (!reg_ok(insn.a)) { trap = Trap::DecodeFault; break; }
    if (!check_mem(insn.opcode, insn.b, "memory load")) { trap = Trap::BoundsFault; break; }
    std::size_t addr = static_cast<std::size_t>(insn.b);
    ctx.registers[insn.a] = state_.memory[addr];
    ctx.register_tags[insn.a] = state_.memory_tags[addr];
    ...
  }
  ```
- **Implemented Runtime (`vm/vm.cpp:2486-2501`)**:
  ```cpp
  case t81::tisc::Opcode::Store: {
    if (!reg_ok(insn.b)) { trap = Trap::DecodeFault; break; }
    if (!check_mem(insn.opcode, insn.a, "memory store")) { trap = Trap::BoundsFault; break; }
    std::size_t addr = static_cast<std::size_t>(insn.a);
    state_.memory[addr] = ctx.registers[insn.b];
    state_.memory_tags[addr] = ctx.register_tags[insn.b];
    ...
  }
  ```
- **Freestanding Evaluator (`ternaryos/hal/cpa004_evaluator.cpp:218-228`)**:
  ```cpp
  case Load:
    value = state.memory[i.b];
    state.tags[i.a] = state.memory_tags[i.b];
    write = true;
    break;
  case Store:
    state.memory[i.a] = lhs;
    state.memory_tags[i.a] = state.tags[i.b];
    break;
  ```
- **Informal Semantics (`spec/tisc/opcode-semantics.md:34-35`)**:
  - `0x03 LOAD`: `A: Dest, B: Addr` — "Loads value from memory address B into R[A]".
  - `0x04 STORE`: `A: Addr, B: Src` — "Stores value in R[B] to memory address A".
- **Normative Specification (`spec/tisc-spec.md:1291-1304`)**:
  - `LOAD RD, [RS]`: `R[RD] := MEM[R[RS]]` (Interpret `R[RS]` as address).
  - `STORE [RS], RD`: `MEM[R[RS]] := R[RD]`.
- **LLVM Lowering (`lang/llvm/tisc_to_llvm.cpp:557-564`)**:
  - Treats operand `b` as immediate address for `Load` and `a` as immediate address for `Store`.

#### Scope & Compatibility Impact
- In the canonical CPA program (PC 11, 12, 24, 25) and CPA-006 Case C06, instructions execute `LOAD R2, [81]` and `STORE [82], R1`.
- If operands `81` and `82` were reinterpreted as register indices under the normative register-indirect rule, execution would immediately trap (`DecodeFault`) because the mandatory architectural register window is restricted to $R_0..R_{80}$.
- Reinterpreting raw bytecode as register-indirect breaks 100% of historical CPA binaries. Conversely, keeping direct addressing restricts memory access to a fixed 32-bit immediate space without runtime dynamic indexing (pointer math).

---

### 3.2 Conflict 2: Control Flow Branches (Target Addressing & Condition Evaluation)

#### Competing Interpretations
- **Interpretation A (Implemented CPA Profile — Direct Immediate Target + Register Zero-Test)**:
  - `JMP` (`0x0A`): `PC = A` (unconditional jump to immediate instruction index `A`).
  - `JZ` (`0x0B`): Jump to immediate target `A` if `R[B] == 0`.
  - `JNZ` (`0x19`): Jump to immediate target `A` if `R[B] != 0`.
  - `JN` (`0x3C`): Jump to immediate target `A` if `FLAGS.negative == true`.
  - `JP` (`0x3D`): Jump to immediate target `A` if `FLAGS.positive == true`.
- **Interpretation B (Normative Specification — Register Target + Condition FLAGS)**:
  - `JMP RS`: `PC := R[RS]` (target address dynamically supplied from register `RS`).
  - `JZ RS`, `JNZ RS`, `JN RS`, `JP RS`: Branch condition tested strictly against central `FLAGS` (Zero, Negative, Positive); if condition is met, `PC := R[RS]`, else fallthrough (`PC := PC + 1`).
- **Interpretation C (Compiler Lowering Divergence — LLVM Backend)**:
  - `lang/llvm/tisc_to_llvm.cpp:437-463` implements immediate target `A`, but evaluates the condition against `R[C]` instead of `R[B]`:
    `Value* cond = builder_.CreateICmpEQ(load_ireg(c), i64(0), "jz_cond");`

#### Exact Code & Documentary Evidence
- **Implemented Runtime (`vm/vm.cpp:2582-2622`)**:
  ```cpp
  case t81::tisc::Opcode::JumpIfZero:
    if (!reg_ok(insn.b)) { trap = Trap::DecodeFault; break; }
    const bool taken = ctx.registers[insn.b] == 0;
    record_branch_decision(taken);
    if (taken) { ctx.pc = static_cast<std::size_t>(insn.a); }
    break;
  case t81::tisc::Opcode::JumpIfNotZero:
    if (!reg_ok(insn.b)) { trap = Trap::DecodeFault; break; }
    const bool taken = ctx.registers[insn.b] != 0;
    record_branch_decision(taken);
    if (taken) { ctx.pc = static_cast<std::size_t>(insn.a); }
    break;
  case t81::tisc::Opcode::JumpIfNegative:
    const bool taken = ctx.flags.negative;
    record_branch_decision(taken);
    if (taken) { ctx.pc = static_cast<std::size_t>(insn.a); }
    break;
  ```
- **Freestanding Evaluator (`ternaryos/hal/cpa004_evaluator.cpp:259-267`)**:
  ```cpp
  case Jz:  if (lhs == 0) state.pc = unsigned(i.a); break;
  case Jnz: if (lhs != 0) state.pc = unsigned(i.a); break;
  case Jn:  if (state.negative) state.pc = unsigned(i.a); break;
  ```
- **Normative Specification (`spec/tisc-spec.md:1322-1334`)**:
  - `JMP RS`: `PC := R[RS]`
  - `JZ RS / JNZ RS / JN RS / JP RS`: "Branch based on FLAGS; if condition satisfied, `PC := R[RS]`, else `PC := PC + 1`."
- **Critical Concrete Fixture Divergence**:
  - In CPA-004 canonical program at PC 19: an instruction sequence leaves the system `FLAGS.zero == true`, but register $R_{42} = 1$.
  - Under Interpretation A (runtime), `JNZ A=23, B=42` checks $R_{42} \ne 0 \implies 1 \ne 0 \implies \text{taken}$.
  - Under Interpretation B (normative), `JNZ` evaluates `!FLAGS.zero` $\implies !(\text{true}) \implies \text{false} \implies \text{not taken}$.
  - The text and executable profile are irreconcilably divergent for this fixture.

#### Scope & Compatibility Impact
- Immediate target vs register-indirect target fundamentally alters compiler code generation and control-flow graph (CFG) analysis.
- Testing register `R[B]` directly decouples `JZ`/`JNZ` from prior arithmetic flag updates (`CMP`, `ADD`), creating a dual execution model where `JZ`/`JNZ` ignore flags while `JN`/`JP` depend on flags.
- The LLVM backend's use of operand `C` is a critical internal bug relative to both the VM and the spec.

---

### 3.3 Conflict 3: Scalar TXor Semantics and Definition

#### Competing Interpretations
- **Interpretation A (Implemented CPA Profile — Balanced Wrapped Difference)**:
  - Operands $x, y \in \{-1, 0, +1\}$.
  - $\text{TXor}(x, y) = \text{wrap}_3(x - y) = ((x - y + 1) \bmod 3) - 1$.
  - Truth table:
    | $x \backslash y$ | $-1$ | $0$ | $+1$ |
    | :---: | :---: | :---: | :---: |
    | **$-1$** | $0$ | $-1$ | $+1$ |
    | **$0$** | $+1$ | $0$ | $-1$ |
    | **$+1$** | $-1$ | $+1$ | $0$ |
  - Note: Non-commutative ($\text{TXor}(+1, 0) = +1$, $\text{TXor}(0, +1) = -1$).
- **Interpretation B (Documentary / Algebraic Wording — Balanced Sum or Symmetric XOR)**:
  - Documentary descriptions in `spec/tisc/opcode-semantics.md:52` designate `0x15 TXOR` as "Ternary XOR / Sum".
  - Balanced sum modulo 3: $\text{wrap}_3(x + y)$ is commutative.
  - Symmetrical ternary XOR: $(x \ne 0 \land y == 0) \implies x$, etc.

#### Exact Code & Documentary Evidence
- **Implemented Runtime (`vm/vm.cpp:3033-3042`)**:
  ```cpp
  } else {
    result = lhs - rhs;
    if (result > 1) { result = -1; }
    if (result < -1) { result = 1; }
  }
  ```
- **Freestanding Evaluator (`ternaryos/hal/cpa004_evaluator.cpp:250-256`)**:
  ```cpp
  value = lhs - rhs;
  if (value > 1) value -= 3;
  else if (value < -1) value += 3;
  ```
- **Tests (`tests/cpp/test_packed_trit_vector.cpp`)**:
  - Explicitly asserts the wrapped difference convention.
- **Specification Ambiguity (`spec/tisc-spec.md:1229-1234` & `spec/tisc/opcode-semantics.md:52`)**:
  - Lacks an explicit scalar truth table while labeling the operation "XOR / Sum".

#### Scope & Compatibility Impact
- Historical CPA suites (CPA-004 PC 15, CPA-006 Case C03) strictly encode and verify the wrapped difference convention.
- Mathematical clarity is required: difference is not sum, and ternary difference is non-commutative.

---

### 3.4 Conflict 4: Opcode Inventory, Extension Assignments, and SWAR Collisions

#### Competing Inventories
1. **174 Capability-Frozen Core**:
   - `spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md` defines opcodes `0x00` through `0xAD` (0–173) as immutable.
   - States that `0xAE` (174) through `0xFF` (255) are strictly reserved and MUST trap.
2. **209 Registry Entries**:
   - `spec/tisc/opcode-registry.md` tables define 209 opcodes, verified by `scripts/ci/check_tisc_freeze_integrity.py`.
3. **244 Named C++ Header Enums**:
   - `include/t81/isa/opcodes.hpp` defines 244 named opcodes across 247 numerical values (0–246).
   - Numerical holes: values **221, 222, 223** are undefined and have no enum name.
   - Values **247–255** are undefined.
4. **17 Admitted CPA Subsets**:
   - CPA-004/005 and CPA-006 isolate and admit exactly 17 opcodes: `Nop`, `Halt`, `LoadImm`, `Load`, `Store`, `Add`, `Mul`, `Div`, `Mod`, `JumpIfZero`, `Cmp`, `TNot`, `TAnd`, `TOr`, `TXor`, `JumpIfNotZero`, `Trap`, `JumpIfNegative`.

#### Direct Opcode Assignment Collisions
Between `spec/tisc/opcode-registry.md` and `include/t81/isa/opcodes.hpp`:
| Mnemonic | Registry Numerical Encoding | C++ Header (`opcodes.hpp`) Encoding | Collision / Impact |
| :--- | :---: | :---: | :--- |
| `TVecSub` | 212 (`0xD4`) | 205 (`0xCD`) | Conflict: Registry 205 is unassigned; Header 212 is `POLYMUL_NTT` |
| `TNOT_SWAR` | 213 (`0xD5`) | 206 (`0xCE`) | Conflict: Registry 206 is unassigned; Header 213 is `POLYDIV_NTT` |
| `TAND_SWAR` | 214 (`0xD6`) | 207 (`0xCF`) | Conflict: Registry 207 is unassigned; Header 214 is `POLYADD_NTT` |
| `TOR_SWAR` | 215 (`0xD7`) | 220 (`0xDC`) | Conflict: Registry 220 is unassigned; Header 215 is `POLYSUB_NTT` |

#### Exact Code & Documentary Evidence
- `tests/cpp/tisc_opcode_matrix_test.cpp:33-40` asserts header mappings: `TVecSub == 205`, `TNOT_SWAR == 206`, `TAND_SWAR == 207`.
- `spec/tisc/opcode-registry.md:1200-1250` records `TVecSub == 212`, `TNOT_SWAR == 213`, `TAND_SWAR == 214`, `TOR_SWAR == 215`.
- `scripts/ci/check_tisc_freeze_integrity.py` checks only the markdown registry up to 209 entries, remaining oblivious to the C++ header collision.

---

### 3.5 Conflict 5: Register File Window, R0 Invariance, and System Registers

#### State Definitions
- **Architectural Window**:
  - Runtime and CPA suites enforce $R_0..R_{80}$ (81 registers).
  - C++ definitions define up to 243 register slots (`R0..R242`).
- **Register $R_0$ Contract**:
  - $R_0$ is hardwired to integer 0. Any write to $R_0$ must either be discarded (no-op) or trigger a `DecodeFault`.
  - In CPA evaluators, $R_0$ is enforced as immutable zero; destination $R_0$ is rejected during operand validation (`dest(i.a)` checks $a > 0 \land a < 75$).
- **System Registers ($R_{75}..R_{80}$)**:
  Authoritative implementations in `vm/runtime_state_helpers.cpp:90-111` and verified by `tests/cpp/vm_system_registers_deterministic_test.cpp` define:
  - $R_{75}$: Global instruction execution tick count (`static_cast<int64_t>(instruction_count)`).
  - $R_{76}$: Bytecode lineage hash root (`compute_lineage_signature(program)`).
  - $R_{77}$: Rolling execution step digest / entropy signature (`compute_entropy_signature(...)`).
  - $R_{78}$: Active Constitutional Mask (`compute_constitutional_mask(state)` bitmask: bit 0 `kPolicyLoaded`, bit 1 `kMaxInstructions`, bit 2 `kMaxRecursion`, bit 3 `kMaxStack`, bit 4 `kMaxReflections`, bit 5 `kMaxMetaWrites`, bit 6 `kAllowedTensorHashes`, bit 7 `kMatchGuards`, bit 8 `kSegmentOrAxionRequirements`, bit 9 `kAlignmentRequirements`).
  - $R_{79}$: Call / recursion depth counter (`static_cast<int64_t>(std::max(ctx.stack_frames.size(), ctx.call_depth))`).
  - $R_{80}$: Context status / Axion capability seal word (`state.halted ? 0 : 1`).

---

### 3.6 Conflict 6: Deterministic PC Progression and Fault/Trap Side-Effect Ordering

A fundamental semantic divergence exists between the observable trap behavior of the legacy hosted/freestanding runtime and the idealized atomic-fault semantics proposed for forward profiles.

#### Legacy Hosted & CPA Runtime Behavior (Preserved in Historical Evidence)
In the existing implementation (`vm/vm.cpp:639, 7220-7245` and `ternaryos/hal/cpa004_evaluator.cpp:202-277`):
1. **Instruction Fetch & Increment**: Before opcode dispatch, the execution sequence advances: `const unsigned before = state.pc; const Instruction i = decode(bytes, state.pc++); ++state.tick;`.
2. **Trap Detection**: If an instruction faults (e.g. `BoundsFault` on invalid address in C07, `DivisionFault` on divide-by-zero in C08, `TrapInstruction` in C09):
   - In hosted VM: `restore_pc_on_trap` is only set for policy/security denials (`Trap::SecurityFault`, `Trap::ActivationFault` in `vm.cpp:6851`). For arithmetic, bounds, and decode faults, `restore_pc_on_trap` remains `false`.
   - Consequently, **PC advances** to `pc_before + 1`.
   - **Tick / System Registers Update**: The execution tick increments (`state.tick == sequence == 1`), and system registers $R_{75}$ (tick) and $R_{77}$ (entropy signature) are synchronized to reflect the faulting step.
   - **No Destination Register or Memory Mutation**: While PC and tick advance, the destination register payload and memory contents are not committed (`write = false`).
3. **CPA Conformance Trace Reality**:
   - In CPA-006 Case C07-negative (Load with invalid address `b = -1`): sequence 0 records initial state (`PC=0, sequence=0, R75=0`). Sequence 1 records terminal trap observation: `status: "trap", trap: "BoundsFault", state: { pc: 1, sequence: 1, R75: 1 }`.
   - In CPA-006 Case C08-div (division by zero at step 3): `state.pc` advances from 2 to 3, sequence increments to 3, and `R75` reflects 3 before trap termination.
   - **Crucial Rule**: Any attempt to retroactively mandate strict zero-step or zero-PC-advance fault semantics on historical binaries would immediately invalidate 100% of CPA-006 trap cases (C07-negative, C07-upper, C07-code, C08-div, C08-mod, C09).

#### Proposed Forward Profile (`tisc-v2.0` — Atomic Transactional Fault Semantics)
For the forward specification profile (`tisc-v2.0`), atomic transactional fault semantics are proposed for clean exception/interrupt recovery:
1. **Atomic Phase Ordering**:
   - Step 1 (Fetch & Decode): Read instruction at `PC`. Decode operands.
   - Step 2 (Pre-Commit Checks): Verify address bounds, alignment, stack limits, and divisor non-zero.
   - Step 3 (Fault Boundary): If any check fails, generate fault immediately. In `tisc-v2.0`, `PC` remains pointing to the faulting instruction (`PC = pc_before`), tick counters do not increment, and no register/memory mutations occur.
   - Step 4 (Commit): Only on complete success, commit result, update `FLAGS`, increment tick/lineage, and advance `PC = PC + 1` (or branch target).

______________________________________________________________________

## 4. Proposed Solution: Versioned ISA Profile Architecture

Because raw 13-byte binaries contain no embedded metadata or version flags, T81 must establish an **explicit, artifact-bound ISA profile selection model**.

```
+-------------------------------------------------------------------------------+
|                       Explicit ISA Profile Architecture                       |
+-------------------------------------------------------------------------------+
                                      |
         +----------------------------+----------------------------+
         |                                                         |
         v                                                         v
+-------------------------------+                         +-------------------------------+
|   Profile: tisc-cpa-v1.0      |                         |      Profile: tisc-v2.0       |
|  (Historical Experimental)    |                         |     (Canonical Unified)       |
+-------------------------------+                         +-------------------------------+
| * Immediate Memory (Load/Store) |                       | * Register-Indirect Mem (LD/ST) |
| * Direct Target + Reg JZ/JNZ  |                         | * Direct Addr: LOADABS/STOREABS|
| * Wrapped Diff TXor           |                         | * Register Target JMP/CALL    |
| * Admitted 17-Opcode Subset   |                         | * Pure FLAGS-based Branches   |
| * Architectural R0..R80       |                         | * Full 244-Opcode Canonical   |
| * Observable Trap Side-Effects|                         | * Atomic Fault Transactions   |
| * Bit-for-bit CPA Validation  |                         | * Explicit Container / Magic  |
+-------------------------------+                         +-------------------------------+
```

### 4.1 Profile 1: `tisc-cpa-v1.0` (Historical Experimental Conformance Profile)
- **Scope**: Exactly preserves the semantics executed in CPA-004, CPA-005, and CPA-006.
- **Opcode Window**: Strictly admitted 17 opcodes (`0x00..0x1C` subset + `0x3C`).
- **Memory**: Immediate addressing (`Load A, B` $\implies R[A] = \text{MEM}[B]$; `Store A, B` $\implies \text{MEM}[A] = R[B]$).
- **Branches**: Immediate target `A`; `JZ`/`JNZ` test register `R[B]`; `JN` tests `FLAGS.negative`.
- **TXor**: Wrapped difference $\text{wrap}_3(x - y)$.
- **Faults**: Preserves legacy observable fault behavior (PC advances to next instruction, tick increments, system registers sync).
- **Guarantees**: All historical raw13 binaries, golden traces (`expected.json`), and SHA-256 witnesses remain 100% valid and verified without modification.

### 4.2 Profile 2: `tisc-v2.0` (Forward Canonical Unified Profile)
- **Scope**: Reconciles normative specifications with scalable compiler targets.
- **Memory**:
  - `LOAD R[A], [R[B]]` (`0x03`): Register-indirect memory load.
  - `STORE [R[A]], R[B]` (`0x04`): Register-indirect memory store.
  - `LOADABS R[A], Addr` (New/Explicit): Immediate direct address load.
  - `STOREABS Addr, R[B]` (New/Explicit): Immediate direct address store.
- **Branches**:
  - `JMP R[A]`: Register-indirect jump.
  - `JMPABS Addr`: Immediate jump.
  - `JZ / JNZ / JN / JP`: Pure `FLAGS`-driven conditional branching.
- **TXor**: Formally codified scalar balanced difference, with documentation updated to remove ambiguous "Sum" terminology.
- **Faults**: Strict atomic fault semantics (zero state changes, PC preserved at faulting instruction).
- **Inventory**: Subject to maintainer approval under DM-06 and DM-07, formalizes 244 opcodes with holes 221–223 and 247–255 reserved.

______________________________________________________________________

## 5. Artifact Binding, Envelope Specification, and Precedence

To eliminate execution ambiguity, raw instruction streams must be encapsulated or explicitly bound at runtime boundaries.

### 5.1 Profile Resolution Precedence Hierarchy
When multiple conflicting profile sources or hints are present, conforming implementations MUST evaluate profile selection according to the following strict descending order of precedence:

```
+-------------------------------------------------------------------------+
|                  Profile Resolution Precedence Hierarchy                |
+-------------------------------------------------------------------------+
  1. Explicit CLI Execution Flag:    --isa-profile <name>
     Overrides all metadata; enables deterministic operational testing.
                                      |
                                      v
  2. Container Manifest Envelope:    CanonFS manifest / computational object
     Explicit JSON metadata field ("isa_profile": "tisc-cpa-v1.0").
                                      |
                                      v
  3. Binary Embedded Note Section:   Standalone UEFI / ELF .rodata note
     Embedded string in .rodata note section (.tisc_profile).
                                      |
                                      v
  4. Fail-Closed Default Rejection:  Reject execution immediately
     Raw .raw13 bytecode lacking flags or metadata fails closed (UnsupportedProfile).
+-------------------------------------------------------------------------+
```

1. **Level 1 — Explicit CLI Invocation Flag (`--isa-profile <name>`)**:
   Highest precedence. An explicit operator or test harness argument directly forces the execution engine into a specific profile (e.g. `t81 vm run --isa-profile tisc-cpa-v1.0 binary.raw13`). If an artifact's embedded metadata conflicts with an explicit CLI flag, the CLI flag MUST either strictly override with an Axion warning log or fail closed with a configuration fault.
2. **Level 2 — Container Manifest Envelope (CanonFS / ADB)**:
   Second precedence. Governed object containers (CanonFS objects, Accountable Decision Bundles) declare the intended profile in their canonical manifest metadata (`"isa_profile": "tisc-cpa-v1.0"`).
3. **Level 3 — Binary Embedded Note Section (.rodata note)**:
   Third precedence. Freestanding EFI executables or compiled TISC binary images embed a dedicated read-only ELF/COFF note section (`.tisc_profile`) declaring profile identity.
4. **Level 4 — Fail-Closed Default (No Implicit Fallback)**:
   Lowest precedence / Default. Any raw 13-byte bytecode stream lacking an explicit CLI profile flag, container envelope, or binary note section MUST NOT be executed under an assumed or guessing profile. The runtime MUST halt immediately with `Trap::UnsupportedProfile` before decoding or stepping instructions.

______________________________________________________________________

## 6. Comprehensive Maintainer Decision Matrix

The following decision matrix enumerates all unresolved semantic conflicts. Each item presents the competing alternatives, compatibility impact, and a proposed resolution requiring formal maintainer signature. **All items remain strictly PENDING REVIEW.**

| ID | Topic / Discrepancy | Option A | Option B | Option C (Alternative) | Historical Compatibility Impact | Proposed Resolution | Status |
| :---: | :--- | :--- | :--- | :--- | :--- | :--- | :---: |
| **DM-01** | **Memory Operands** (`LOAD`/`STORE`) | Retain direct immediate addressing as sole meaning of `0x03`/`0x04`. | Enforce register-indirect `MEM[R[B]]` per `tisc-spec.md`; reject immediate operands. | Split into Profiles: `tisc-cpa-v1.0` uses direct; `tisc-v2.0` introduces separate indirect and direct opcodes. | Option B breaks 100% of CPA-004/CPA-006 binaries. Option A prevents dynamic pointer dereferencing. Option C preserves historical artifacts while unlocking dynamic memory for compiler backends. | **Proposed: Option C** (Preserve legacy profile; introduce distinct opcodes in v2). | **PENDING REVIEW** |
| **DM-02** | **Branch Targets** (`JMP`, `JZ`, `JNZ`) | Target is immediate PC index in operand `A`. | Target is register-indirect `PC = R[RS]`. | Immediate target in legacy profile; register target in `tisc-v2.0` (with dedicated `JMPABS` for immediate). | Option B breaks all CPA control flow and LLVM lowering. Option C maintains CFG simplicity. | **Proposed: Option C** (Immediate target in legacy; indirect target in v2). | **PENDING REVIEW** |
| **DM-03** | **Branch Conditions** (`JZ`, `JNZ`) | Test register `R[B] == 0` directly, bypassing `FLAGS`. | Test `FLAGS.zero` set by preceding arithmetic/`CMP`. | Support both: `JZ_REG` vs `JZ_FLAG`. | Option B breaks CPA-004 fixture at PC19 where $R_{42}=1$ while `FLAGS.zero=true`. Option A decouples condition from standard ALU flags. | **Proposed: Option C** (Register-test in legacy; pure FLAGS in v2). | **PENDING REVIEW** |
| **DM-04** | **LLVM Lowering Bug** | Keep LLVM using operand `C` for condition test. | Align LLVM lowering to operand `B` immediately. | Defer LLVM backend changes until profile versioning is adopted. | LLVM lowering is currently broken relative to VM runtime. | **Proposed: Option B** (Align LLVM lowering to operand B). | **PENDING REVIEW** |
| **DM-05** | **Scalar TXor Semantics** | Formally ratify wrapped difference `wrap3(lhs - rhs)`. | Re-implement as symmetric balanced sum `wrap3(lhs + rhs)`. | Rename opcode to `TSUB3` and introduce separate commutative `TXOR`. | Option B breaks CPA-004 PC15 and CPA-006 C03. Option A maintains 100% test parity but requires documentation correction. | **Proposed: Option A** (Ratify wrapped difference; correct docs to remove "Sum"). | **PENDING REVIEW** |
| **DM-06** | **Opcode Inventory Unification** | Freeze core strictly at 174; deprecate 175–246. | Expand canonical registry to 244 named opcodes matching header, reserving 221–223 and 247–255. | Retain 209 in registry and treat 210–246 as experimental unverified. | Option A breaks AI-native and SWAR instructions. Option B unifies header, test matrix, and specification. | **Proposed: Option B** (Unify at 244 opcodes pending maintainer sign-off). | **PENDING REVIEW** |
| **DM-07** | **SWAR / Extension Collisions** | Ratify header assignments (205, 206, 207, 220). | Ratify registry assignments (212, 213, 214, 215). | Reallocate SWAR opcodes to a clean reserved block in 224–246. | C++ header and `tisc_opcode_matrix_test.cpp` already assert 205–207, 220. Registry values collide with polynomial opcodes. | **Proposed: Option A** (Ratify header assignments 205–207, 220). | **PENDING REVIEW** |
| **DM-08** | **Register $R_0$ Invariance** | Enforce destination $R_0$ as `DecodeFault`. | Allow writes to $R_0$ but silently discard (hardwired zero). | Allow unrestricted writes to $R_0$. | CPA evaluators reject destination $R_0$ during operand admission. Discarding writes is standard in RISC (e.g., MIPS/RISC-V). | **Proposed: Option A** (Enforce destination R0 as DecodeFault). | **PENDING REVIEW** |
| **DM-09** | **Profile Binding Mechanism** | Reject standalone `.raw13`; require CanonFS or container envelope. | Embed 16-byte magic header into all new bytecode binaries. | Allow raw binaries but require explicit CLI flag `--isa-profile`. | Raw binaries lack self-description. Option C provides seamless backward compatibility for testing. | **Proposed: Options B & C** (Container envelope for v2; CLI flag for raw binaries). | **PENDING REVIEW** |

______________________________________________________________________

## 7. Determinism and Safety Considerations

1. **Profile-Specific Trap Determinism**:
   - In profile `tisc-cpa-v1.0`: Implementations MUST preserve observable historical trap behavior (PC increments to `pc_before + 1`, execution tick advances, system registers $R_{75}/R_{77}$ synchronize, destination registers remain uncommitted).
   - In profile `tisc-v2.0`: Implementations MUST enforce atomic fault transactions (zero PC progression, zero tick advancement, zero register/memory modifications).
2. **Flag Determinism**: Flag updates occur solely upon valid instruction commit. Trapped instructions preserve pre-instruction flag state.
3. **Execution Budgeting**: Step limits (tick budgets) terminate execution with status `instruction_limit` without modifying current state registers.

______________________________________________________________________

## 8. Compatibility and Migration Strategy

### 8.1 Preservation of Historical Evidence
Under no circumstances shall historical CPA evidence or frozen test vectors be altered:
- Canonical program SHA-256 (`b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`) remains permanently bound to `tisc-cpa-v1.0`.
- CPA-006 test suite (C01–C12) remains the permanent baseline verification fixture for `tisc-cpa-v1.0`.

### 8.2 Incremental Implementation Roadmap
Upon maintainer approval of Section 10:
1. **Phase 1 (Documentation Synchronization)**:
   - Synchronize `spec/tisc/opcode-registry.md` with C++ header assignments (resolve SWAR collisions and 244 opcode inventory).
   - Clarify `spec/tisc/opcode-semantics.md` for TXor wrapped difference.
2. **Phase 2 (Toolchain Alignment)**:
   - Repair LLVM lowering (`lang/llvm/tisc_to_llvm.cpp`) to test operand `B` for `JumpIfZero`/`JumpIfNotZero`.
   - Add explicit `--isa-profile` flag to `tools/cli/core/main.cpp`.
3. **Phase 3 (Profile v2 Development)**:
   - Draft formal `spec/tisc-v2-spec.md` implementing explicit register-indirect vs immediate memory and branch opcodes.
   - Author conformance test suite for profile `tisc-v2.0`.

______________________________________________________________________

## 9. References

- `spec/tisc-spec.md`: Normative TISC instruction set architecture specification.
- `spec/tisc/opcode-registry.md`: Current TISC opcode master registry (209 entries).
- `spec/tisc/opcode-semantics.md`: Operational definitions for core TISC opcodes.
- `spec/tisc/TISC_CAPABILITY_CONTRACT_v1.1.0.md`: Core capability freeze contract (0–173).
- `include/t81/isa/opcodes.hpp`: Canonical C++ enum class Opcode definition (244 entries).
- `tests/cpp/tisc_opcode_matrix_test.cpp`: Conformance test matrix validating 244 defined and 12 undefined opcodes.
- `vm/vm.cpp`: Hosted virtual machine reference implementation.
- `vm/runtime_state_helpers.cpp`: System register synchronization and constitutional mask computation.
- `tests/cpp/vm_system_registers_deterministic_test.cpp`: Verification suite for R76 lineage and R78 constitutional mask.
- `ternaryos/hal/cpa004_evaluator.cpp`: Freestanding Axion OS evaluator implementation.
- `lang/llvm/tisc_to_llvm.cpp`: LLVM IR translation backend.
- `experiments/cpa004/README.md`: Cross-Profile Architecture evaluation report (canonical 351-byte program).
- `experiments/cpa006/README.md`: Bounded cross-profile conformance report (19 cases, 1,190 state records).
- `docs/records/audits/T81_CPA_002_OPCODE_CONFORMANCE.md`: Opcode conformance gate repair audit.

______________________________________________________________________

## 10. Maintainer Review & Decision Record

*This section must be completed and signed by the repository maintainer before any normative specification or code change is merged.*

```
MAINTAINER AUTHORIZATION RECORD
Date of Review:    ____________________
Reviewing Officer: ____________________

Decision Summary:
[ ] DM-01 Memory Operands:             [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-02 Branch Target Addressing:    [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-03 Branch Condition Predicates: [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-04 LLVM Lowering Alignment:     [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-05 Scalar TXor Definition:      [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-06 Opcode Inventory Freeze:     [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-07 SWAR Collisions Resolution:  [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-08 Register R0 Invariance:      [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C
[ ] DM-09 Profile Selection Mechanism: [ ] Adopt Option A   [ ] Adopt Option B   [ ] Adopt Option C

Formal Maintainer Signature: _________________________________________
Disposition:                 [ ] APPROVED FOR IMPLEMENTATION
                             [ ] REVISION REQUIRED
                             [ ] REJECTED
```
