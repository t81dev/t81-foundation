//===-- T81InstrInfo.td - TISC Instruction Definitions -------*- tablegen -*-===//
//
// This file defines the core T81 (Ternary Instruction Set Computer) instructions
// used in the LLVM backend for HanoiVM.
//
//===----------------------------------------------------------------------===//

include "llvm/Target/TargetInstrInfo.td"
include "T81RegisterInfo.td"

def T81Instrs : InstrInfo;

//===----------------------------------------------------------------------===//
// Instruction Format Enums
//===----------------------------------------------------------------------===//

class T81InstBase<string opcstr> : Instruction {
  let Namespace = "T81";
  let OutOperandList = (outs);
  let InOperandList = (ins);
  let AsmString = !strconcat(opcstr, " $operands");
  let hasSideEffects = 0;
}

//===----------------------------------------------------------------------===//
// Ternary Arithmetic Instructions
//===----------------------------------------------------------------------===//

def TADD : T81InstBase<"tadd"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$src1, T81Reg:$src2);
  let AsmString = "tadd $dst, $src1, $src2";
}

def TSUB : T81InstBase<"tsub"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$src1, T81Reg:$src2);
  let AsmString = "tsub $dst, $src1, $src2";
}

def TMUL : T81InstBase<"tmul"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$src1, T81Reg:$src2);
  let AsmString = "tmul $dst, $src1, $src2";
}

def TDIVU : T81InstBase<"tdivu"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$src1, T81Reg:$src2);
  let AsmString = "tdivu $dst, $src1, $src2";
}

def TXOR3 : T81InstBase<"txor3"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$src1, T81Reg:$src2);
  let AsmString = "txor3 $dst, $src1, $src2";
}

//===----------------------------------------------------------------------===//
// Control Flow
//===----------------------------------------------------------------------===//

def TBR : T81InstBase<"tbr"> {
  let InOperandList = (ins brtarget:$target);
  let AsmString = "tbr $target";
}

def TBRZ : T81InstBase<"tbrz"> {
  let InOperandList = (ins T81Reg:$cond, brtarget:$target);
  let AsmString = "tbrz $cond, $target";
}

def TBRNZ : T81InstBase<"tbrnz"> {
  let InOperandList = (ins T81Reg:$cond, brtarget:$target);
  let AsmString = "tbrnz $cond, $target";
}

def TRET : T81InstBase<"tret"> {
  let AsmString = "tret";
}

//===----------------------------------------------------------------------===//
// Memory and Stack
//===----------------------------------------------------------------------===//

def TLD : T81InstBase<"tld"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Mem:$addr);
  let AsmString = "tld $dst, $addr";
}

def TST : T81InstBase<"tst"> {
  let InOperandList = (ins T81Reg:$src, T81Mem:$addr);
  let AsmString = "tst $src, $addr";
}

def TSPADJ : T81InstBase<"tspadj"> {
  let InOperandList = (ins i32imm:$amount);
  let AsmString = "tspadj $amount";
}

//===----------------------------------------------------------------------===//
// Ternary-Specific Extensions
//===----------------------------------------------------------------------===//

def TENT : T81InstBase<"tent"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$src);
  let AsmString = "tent $dst, $src";
}

def TSEL : T81InstBase<"tsel"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList  = (ins T81Reg:$cond, T81Reg:$if_true, T81Reg:$if_false);
  let AsmString = "tsel $dst, $cond, $if_true, $if_false";
}

def TDISPATCH : T81InstBase<"tdispatch"> {
  let InOperandList = (ins T81Reg:$macro);
  let AsmString = "tdispatch $macro";
}

def TMARKOV : T81InstBase<"tmarkov"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList = (ins T81Reg:$matrix, T81Reg:$state);
  let AsmString = "tmarkov $dst, $matrix, $state";
}

def TSYM : T81InstBase<"tsym"> {
  let OutOperandList = (outs T81Reg:$dst);
  let InOperandList = (ins T81Reg:$symbol);
  let AsmString = "tsym $dst, $symbol";
}

