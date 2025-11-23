#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include "t81/ir/opcodes.hpp"
#include "t81/ir/insn.hpp"
#include "t81/ir/encoding.hpp"

static bool eq(const t81::ir::Insn& a, const t81::ir::Insn& b) {
  if (a.op != b.op) return false;
  if (a.ops != b.ops) return false;
  if (a.imm != b.imm) return false;
  if (a.flags != b.flags) return false;
  return true;
}

int main() {
  using namespace t81::ir;

  // Single encode/decode roundtrip
  Insn i = make3(Opcode::BigMul, /*a=*/7, /*b=*/42, /*c=*/3);
  i.imm   = 0x1122334455667788ULL;
  i.flags = 0xA5A5A5A5;

  uint8_t buf[32];
  encode(i, buf);
  Insn j = decode(buf);
  assert(eq(i, j));

  // Many encode/decode roundtrip
  std::vector<Insn> prog = {
    make0(Opcode::Nop),
    make1(Opcode::Add, 1),
    make2(Opcode::TDot, 2, 3),
    make_imm(Opcode::Jump, 0xDEADBEEF),
    make3(Opcode::TSlice2D, 10, 20, 30),
  };
  auto bytes = encode_many(prog);
  auto got   = decode_many(bytes.data(), bytes.size());
  assert(got.size() == prog.size());
  for (size_t k = 0; k < prog.size(); ++k) assert(eq(prog[k], got[k]));

  std::cout << "ir_encoding ok\n";
  return 0;
}
