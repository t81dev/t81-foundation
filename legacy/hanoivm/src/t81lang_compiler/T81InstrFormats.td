//===-- InstrFormats.td - T81 ISA Instruction Formats --------*- tablegen -*-===//
//
// NewBook-aligned instruction format classes for the T81 target.
// This file defines target-specific base classes and mnemonic-level
// format families (R/I/L/S/B/J), but intentionally avoids freezing the
// binary encoding until the opcode map is finalized.
//
// How to use:
//  * Derive concrete instruction defs in T81InstrInfo.td from these
//    format classes (e.g., def ADD : T81R<"add", ...> { ... }).
//  * When encodings are ready, flip `let hasEncoding = 1;` and fill the
//    `bits<N> Inst` layout in each format (see TODO sections below).
//
// NOTE: We keep this file strictly about *formats* and MC interface flags.
//       Scheduling classes and selection patterns live elsewhere.
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"
include "llvm/Target/TargetInstrInfo.td"
include "T81RegisterInfo.td"

//------------------------------------------------------------------------------
// Target-level base for all encoded instructions
//------------------------------------------------------------------------------
class InstT81<string asm, dag outs, dag ins> : Instruction {
  let Namespace      = "T81";
  let AsmString      = asm;
  let OutOperandList = outs;
  let InOperandList  = ins;
  let hasSideEffects = 0;
  let isCodeGenOnly  = 0;
  // Until encodings are frozen. Set to 1 and define Inst bits when ready.
  let hasEncoding    = 0;
}

//------------------------------------------------------------------------------
// Generic format traits
//------------------------------------------------------------------------------
// R  = reg-reg (rd, rs1, rs2)
// I  = reg-imm (rd, imm)
// L  = load (rd, [base + imm])
// S  = store (rs, [base + imm])
// B  = conditional branch (pc-rel target)
// J  = jump/call (pc-rel target)
// U  = unary (rd, rs1)
// M  = move (rd, rs1)

// NOTE: These classes carry *semantic flags* only. Encoding is added later.

class T81R<string asm> : InstT81<asm,
                                 (outs T81Reg:$rd),
                                 (ins  T81Reg:$rs1, T81Reg:$rs2)> {
  // TODO(encoding): set hasEncoding=1 and add bits<N> Inst with fields
  //   OPC | RD | RS1 | RS2 | FLAGS
}

class T81U<string asm> : InstT81<asm,
                                 (outs T81Reg:$rd),
                                 (ins  T81Reg:$rs1)> {
  // TODO(encoding): unary op encoding
}

class T81M<string asm> : InstT81<asm,
                                 (outs T81Reg:$rd),
                                 (ins  T81Reg:$rs1)> {
  let isAsCheapAsAMove = 1;
  // TODO(encoding): move encoding (may alias to ADD rd, rs1, r0)
}

class T81I<string asm> : InstT81<asm,
                                 (outs T81Reg:$rd),
                                 (ins  i32imm:$imm)> {
  // TODO(encoding): li/const materialization
}

class T81L<string asm> : InstT81<asm,
                                 (outs T81Reg:$rd),
                                 (ins  T81Reg:$base, i32imm:$off)> {
  let mayLoad = 1;
  // TODO(encoding): load [base + off]
}

class T81S<string asm> : InstT81<asm,
                                 (outs),
                                 (ins  T81Reg:$rs, T81Reg:$base, i32imm:$off)> {
  let mayStore = 1;
  // TODO(encoding): store [base + off]
}

class T81B<string asm> : InstT81<asm,
                                 (outs),
                                 (ins  brtarget:$target)> {
  let isBranch            = 1;
  let isTerminator        = 1;
  let isConditionalBranch = 1;
  // TODO(encoding): pc-rel branch immediate
}

class T81J<string asm> : InstT81<asm,
                                 (outs),
                                 (ins  brtarget:$target)> {
  let isBranch   = 1;     // for analysis
  let isTerminator = 1;
  let isBarrier  = 1;
  // TODO(encoding): pc-rel jump/call immediate
}

class T81RET<string asm> : InstT81<asm, (outs), (ins)> {
  let isReturn   = 1;
  let isTerminator = 1;
  let isBarrier  = 1;
  // TODO(encoding): dedicated opcode or alias
}

//------------------------------------------------------------------------------
// Canonical NewBook mnemonics using the above formats
//------------------------------------------------------------------------------
// Arithmetic / Logic (two-source)
// (Concrete instruction *definitions* still live in T81InstrInfo.td; here we
//  only declare format aliases so you can reuse them.)

def FMT_ADD  : T81R<"add $rd, $rs1, $rs2">;

def FMT_SUB  : T81R<"sub $rd, $rs1, $rs2">;

def FMT_MUL  : T81R<"mul $rd, $rs1, $rs2">;

def FMT_DIVU : T81R<"divu $rd, $rs1, $rs2">;

def FMT_REMU : T81R<"remu $rd, $rs1, $rs2">;

def FMT_AND  : T81R<"and $rd, $rs1, $rs2">;

def FMT_OR   : T81R<"or  $rd, $rs1, $rs2">;

def FMT_XOR  : T81R<"xor $rd, $rs1, $rs2">;

// Unary / Move / Immediate

def FMT_NEG  : T81U<"neg $rd, $rs1">;

def FMT_INC  : T81U<"inc $rd, $rs1">;

def FMT_DEC  : T81U<"dec $rd, $rs1">;

def FMT_MOVE : T81M<"move $rd, $rs1">;

def FMT_LI   : T81I<"li $rd, $imm">;

// Memory

def FMT_LOAD  : T81L<"load $rd, [$base, $off]">;

def FMT_STORE : T81S<"store $rs, [$base, $off]">;

// Control flow

def FMT_JMP : T81J<"jmp $target">;

def FMT_JZ  : T81B<"jz $target">;

def FMT_JN  : T81B<"jn $target">;

def FMT_JP  : T81B<"jp $target">;

def FMT_CALL: T81J<"call $target"> { let isCall = 1; }

def FMT_RET : T81RET<"ret">;

//------------------------------------------------------------------------------
// Encoding guide (fill once opcode map is frozen)
//------------------------------------------------------------------------------
// When you are ready to commit the binary encoding, do the following:
//  1) Set `let hasEncoding = 1;` in InstT81 or per-format class.
//  2) Introduce a constant width for the instruction word, e.g.:
//       def T81InstWidth : int = 32; // or 40/48 per NewBook mapping
//     Then add `bits<T81InstWidth> Inst;` to each format.
//  3) Carve fields with TableGen slices, e.g.:
//       bits<6>  OPC;
//       bits<5>  RD;
//       bits<5>  RS1;
//       bits<5>  RS2;
//       bits<11> IMM;
//     And assign them into Inst in the order required by your assembler/MC:
//       let Inst{31-26} = OPC;
//       let Inst{25-21} = RD;
//       ...
//  4) Add `DecoderNamespace = "T81";` and any `Predicates` you need.
//  5) Provide AsmParser/AsmWriter hooks and aliases if user syntax differs.
//  6) Add llvm-mc tests for assembly ↔ encoding round-trips.
//
// For loads/stores, make sure the immediate field matches the address mode
// range (sign-extended if negative offsets are allowed). For branches/jumps,
// define PC-relative immediates with the correct scale/align.
//===----------------------------------------------------------------------===//
