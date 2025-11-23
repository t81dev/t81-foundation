#pragma once
#include <array>
#include <cstdint>
#include "t81/ir/opcodes.hpp"

namespace t81::ir {

struct Insn {
  Opcode                 op{Opcode::Nop};
  std::array<uint32_t,3> ops{{0,0,0}};  // generic operands (regs/ids)
  uint64_t               imm{0};        // immediate / address
  uint32_t               flags{0};      // misc flags
  uint32_t               _reserved{0};  // keep 32-byte alignment
};

// -------- helpers --------
inline Insn make0(Opcode op) {
  return Insn{op, {0,0,0}, 0, 0, 0};
}
inline Insn make1(Opcode op, uint32_t a) {
  Insn i; i.op = op; i.ops[0]=a; return i;
}
inline Insn make2(Opcode op, uint32_t a, uint32_t b) {
  Insn i; i.op = op; i.ops[0]=a; i.ops[1]=b; return i;
}
inline Insn make3(Opcode op, uint32_t a, uint32_t b, uint32_t c) {
  Insn i; i.op = op; i.ops[0]=a; i.ops[1]=b; i.ops[2]=c; return i;
}
inline Insn make_imm(Opcode op, uint64_t imm, uint32_t flags=0) {
  Insn i; i.op = op; i.imm = imm; i.flags = flags; return i;
}

} // namespace t81::ir
