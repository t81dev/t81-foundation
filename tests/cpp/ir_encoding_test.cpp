#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include "t81/ir/encoding.hpp"

using namespace t81::ir;

[[maybe_unused]] static bool eq(const Insn& a, const Insn& b) {
  if (a.op != b.op) return false;
  if (a.ops != b.ops) return false;
  if (a.imm != b.imm) return false;
  if (a.flags != b.flags) return false;
  return true;
}

int main() {
  // Build a program covering all major opcode blocks
  [[maybe_unused]] std::vector<Insn> prog;
  prog.push_back(make0(Opcode::Nop));
  prog.push_back(
      make_imm(Opcode::Jump, 0x1122334455667788ull, OP_FLAG_BRANCH | OP_FLAG_TERMINATOR));
  prog.push_back(make3(Opcode::Add, 1, 2, 3));
  prog.push_back(make3(Opcode::BigMul, 7, 8, 9));
  prog.push_back(make3(Opcode::TMatMul, 10, 11, 12));
  prog.push_back(make_imm(Opcode::TReduce, /*axis*/ 1, /*flags*/ 0x00000003u));

  // Test new opcodes
  prog.push_back(make2(Opcode::Load, 1, 100));  // load r1, [r100]
  prog.push_back(make0(Opcode::AxRead));
  prog.push_back(make1(Opcode::CapCheck, 5));
  prog.push_back(make0(Opcode::Halt));

  // Encode → bytes
  [[maybe_unused]] std::vector<uint8_t> bytes = encode_many(prog);
  assert(bytes.size() == prog.size() * 32);

  // Decode back → program
  [[maybe_unused]] auto round = decode_many(bytes.data(), bytes.size());
  assert(round.size() == prog.size());
  for (size_t i = 0; i < prog.size(); ++i) {
    assert(eq(round[i], prog[i]));
  }

  // Verify OpcodeDesc and Flags
  {
    [[maybe_unused]] auto desc = get_opcode_desc(Opcode::Halt);
    assert(desc.flags & OP_FLAG_TERMINATOR);

    desc = get_opcode_desc(Opcode::Jump);
    assert(desc.flags & OP_FLAG_BRANCH);

    desc = get_opcode_desc(Opcode::AxRead);
    assert(desc.flags & OP_FLAG_PRIVILEGED);

    desc = get_opcode_desc(Opcode::Store);
    assert(desc.flags & OP_FLAG_MEMORY);
  }

  std::cout << "ir_encoding (expanded) ok\n";
  return 0;
}
