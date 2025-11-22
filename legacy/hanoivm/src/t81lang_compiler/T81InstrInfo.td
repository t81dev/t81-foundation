//===-- T81InstrInfo.td - TISC Instruction Definitions -------*- tablegen -*-===//
//
// Canonical T81 (Ternary) instructions as specified by the NewBook ISA.
// This file defines core arithmetic/logic, memory, control-flow, and
// stack/ABI-related instructions for the LLVM backend.
//
// Notes:
//  * Pseudo-instructions are marked isPseudo/isCodeGenOnly and expand during
//    ISel/PEI or target-specific passes.
//  * Real memory ops are flagged with mayLoad/mayStore.
//  * Control-flow is explicit and deterministic; conditional branches are
//    modeled via flags and branch analysis hooks in T81InstrInfo.
//
// Fill in MC encodings and scheduling once the opcode map is frozen.
//===----------------------------------------------------------------------===//

include "llvm/Target/TargetInstrInfo.td"
include "T81RegisterInfo.td"

//------------------------------------------------------------------------------
// Target-level InstrInfo anchor
//------------------------------------------------------------------------------

def T81Instrs : InstrInfo;

//------------------------------------------------------------------------------
// Common operand kinds
//------------------------------------------------------------------------------
// Base register class is expected to be defined in T81RegisterInfo.td as T81Reg.
// Addressing: canonical form is [base + imm]. If you later support indexed
// addressing, add a ComplexPattern and split forms (LOADri/LOADrr, etc.).

def simm32 : Operand<i32>;

def T81Addr : Operand<iPTR> {
  // Logical placeholder; assembly prints as [base + imm].
  // SelectionDAG/GlobalISel should build this from (add base, imm).
}

//------------------------------------------------------------------------------
// Base instruction class
//------------------------------------------------------------------------------
class T81InstBase<string opcstr> : Instruction {
  let Namespace       = "T81";
  let OutOperandList  = (outs);
  let InOperandList   = (ins);
  let hasSideEffects  = 0;
  let isCodeGenOnly   = 0;
  let hasNoSchedulingInfo = 1; // until SchedRW mappings are added
  // Each concrete def provides its full AsmString.
}

//------------------------------------------------------------------------------
// Arithmetic / Logic (two-source forms per NewBook)
//------------------------------------------------------------------------------

def TADD : T81InstBase<"add"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "add $rd, $rs1, $rs2";
  let Commutable     = 1;
}

def TSUB : T81InstBase<"sub"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "sub $rd, $rs1, $rs2";
}

def TMUL : T81InstBase<"mul"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "mul $rd, $rs1, $rs2";
  let Commutable     = 1;
}

// Unsigned divide; add signed and remainder variants when enabled in the ISA.

def TDIVU : T81InstBase<"divu"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "divu $rd, $rs1, $rs2";
}


def TREMU : T81InstBase<"remu"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "remu $rd, $rs1, $rs2";
}

// Ternary logical ops (two-source). If the algebra is commutative, mark so.

def TAND : T81InstBase<"and"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "and $rd, $rs1, $rs2";
  let Commutable     = 1;
}

def TOR  : T81InstBase<"or"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "or $rd, $rs1, $rs2";
  let Commutable     = 1;
}

// Canonical two-input XOR per NewBook. (Replaces the earlier TXOR3.)

def TXOR : T81InstBase<"xor"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1, T81Reg:$rs2);
  let AsmString      = "xor $rd, $rs1, $rs2";
  let Commutable     = 1;
}

// Unary helpers (pseudo or real depending on HW): NEG, INC, DEC

def TNEG : T81InstBase<"neg"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1);
  let AsmString      = "neg $rd, $rs1";
}

def TINC : T81InstBase<"inc"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1);
  let AsmString      = "inc $rd, $rs1";
}

def TDEC : T81InstBase<"dec"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1);
  let AsmString      = "dec $rd, $rs1";
}

//------------------------------------------------------------------------------
// Moves and immediates
//------------------------------------------------------------------------------

def TMOVE : T81InstBase<"move"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$rs1);
  let AsmString      = "move $rd, $rs1";
  let isAsCheapAsAMove = 1;
}

// Load-immediate; keep encoding constraints in MC later.

def TLI : T81InstBase<"li"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  simm32:$imm);
  let AsmString      = "li $rd, $imm";
}

//------------------------------------------------------------------------------
// Memory (canonical [base + imm] form). Add indexed forms later if needed.
//------------------------------------------------------------------------------

def TLOAD : T81InstBase<"load"> {
  let OutOperandList = (outs T81Reg:$rd);
  let InOperandList  = (ins  T81Reg:$base, simm32:$off);
  let AsmString      = "load $rd, [$base, $off]";
  let mayLoad        = 1;
}


def TSTORE : T81InstBase<"store"> {
  let InOperandList  = (ins  T81Reg:$rs, T81Reg:$base, simm32:$off);
  let AsmString      = "store $rs, [$base, $off]";
  let mayStore       = 1;
}

//------------------------------------------------------------------------------
// Stack / Frame
//------------------------------------------------------------------------------
// SP adjustment is modeled as a pseudo when emitted by PEI.

def TSPADJ : T81InstBase<"spadj"> {
  let InOperandList  = (ins  simm32:$amount);
  let AsmString      = "spadj $amount";
  let hasSideEffects = 1;         // touches SP
  let isPseudo       = 1;
  let isCodeGenOnly  = 1;
}

// PUSH/POP are often pseudos expanded to STORE/LOAD plus SP adjust.

def TPUSH : T81InstBase<"push"> {
  let InOperandList  = (ins  T81Reg:$rs);
  let AsmString      = "push $rs";
  let hasSideEffects = 1;
  let isPseudo       = 1;
  let isCodeGenOnly  = 1;
}

def TPOP : T81InstBase<"pop"> {
  let OutOperandList = (outs T81Reg:$rd);
  let AsmString      = "pop $rd";
  let hasSideEffects = 1;
  let isPseudo       = 1;
  let isCodeGenOnly  = 1;
}

//------------------------------------------------------------------------------
// Control Flow (deterministic; flag-driven conditional branches)
//------------------------------------------------------------------------------

def TJMP : T81InstBase<"jmp"> {
  let InOperandList  = (ins brtarget:$target);
  let AsmString      = "jmp $target";
  let isBranch       = 1;
  let isTerminator   = 1;
  let isBarrier      = 1;
}

// Branch on ZERO / NEG / POS flags (choose the set your CC uses).

def TJZ : T81InstBase<"jz"> {
  let InOperandList  = (ins brtarget:$target);
  let AsmString      = "jz $target";
  let isBranch       = 1;
  let isTerminator   = 1;
  let isConditionalBranch = 1;
}

def TJN : T81InstBase<"jn"> {
  let InOperandList  = (ins brtarget:$target);
  let AsmString      = "jn $target";
  let isBranch       = 1;
  let isTerminator   = 1;
  let isConditionalBranch = 1;
}

def TJP : T81InstBase<"jp"> {
  let InOperandList  = (ins brtarget:$target);
  let AsmString      = "jp $target";
  let isBranch       = 1;
  let isTerminator   = 1;
  let isConditionalBranch = 1;
}

// Calls/returns.

def TCALL : T81InstBase<"call"> {
  let InOperandList  = (ins brtarget:$target);
  let AsmString      = "call $target";
  let isCall         = 1;
  let isBarrier      = 1;
}

def TRET : T81InstBase<"ret"> {
  let AsmString      = "ret";
  let isReturn       = 1;
  let isTerminator   = 1;
  let isBarrier      = 1;
}

//------------------------------------------------------------------------------
// Selection patterns (starter set). Extend in <Target>ISelLowering.
//------------------------------------------------------------------------------
// These are illustrative. Use your actual VT/legalization setup in TargetLowering.

def : Pat<(add T81Reg:$x, T81Reg:$y),                 (TADD  $x, $y)>;

def : Pat<(sub T81Reg:$x, T81Reg:$y),                 (TSUB  $x, $y)>;

def : Pat<(mul T81Reg:$x, T81Reg:$y),                 (TMUL  $x, $y)>;

def : Pat<(udiv T81Reg:$x, T81Reg:$y),                (TDIVU $x, $y)>;

def : Pat<(urem T81Reg:$x, T81Reg:$y),                (TREMU $x, $y)>;

def : Pat<(and  T81Reg:$x, T81Reg:$y),                (TAND  $x, $y)>;

def : Pat<(or   T81Reg:$x, T81Reg:$y),                (TOR   $x, $y)>;

def : Pat<(xor  T81Reg:$x, T81Reg:$y),                (TXOR  $x, $y)>;

// move/select lowering are target-specific; 'li' will lower from constant materialization.

//------------------------------------------------------------------------------
// TODOs for MC & Sched
//------------------------------------------------------------------------------
// * Add Inst<...> records with Bits<> fields once the opcode layout is frozen.
// * Provide AsmParser/AsmWriter aliases if the user syntax differs from MC names.
// * Replace hasNoSchedulingInfo with concrete SchedRW entries (ALU/LOAD/STORE/BR).
// * Add pseudo expansions for PUSH/POP/TSPADJ in PEI/expandPostRAPseudos.
//===----------------------------------------------------------------------===//
