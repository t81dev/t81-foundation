#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include "t81/ir/encoding.hpp"

using namespace t81::ir;

static bool eq(const Insn& a, const Insn& b) {
  if (a.op != b.op) return false;
  if (a.ops != b.ops) return false;
  if (a.imm != b.imm) return false;
  if (a.flags != b.flags) return false;
  return true;
}

int main() {
  // Build a small "program"
  std::vector<Insn> prog;
  prog.push_back(make0(Opcode::Nop));
  prog.push_back(make_imm(Opcode::Jump, 0x1122334455667788ull, 0xA5A5A5A5u));
  prog.push_back(make3(Opcode::Add, 1, 2, 3));
  prog.push_back(make3(Opcode::BigMul, 7, 8, 9));
  prog.push_back(make3(Opcode::TMatMul, 10, 11, 12));
  prog.push_back(make_imm(Opcode::TReduce, /*axis*/1, /*flags*/0x00000003u));

  // Encode → bytes
  std::vector<uint8_t> bytes = encode_many(prog);
  assert(bytes.size() == prog.size() * 32);

  // Also test single encode/decode symmetry
  {
    uint8_t buf[32];
    encode(prog[2], buf);
    Insn r = decode(buf);
    assert(eq(r, prog[2]));
  }

  // Decode back → program
  auto round = decode_many(bytes.data(), bytes.size());
  assert(round.size() == prog.size());
  for (size_t i = 0; i < prog.size(); ++i) {
    assert(eq(round[i], prog[i]));
  }

  // Mutate a byte and ensure decode still works but yields difference
  bytes[32 + 0x18] ^= 0xFF; // flip a bit in flags of second instruction
  auto mutated = decode_many(bytes.data(), bytes.size());
  bool any_diff = false;
  for (size_t i = 0; i < prog.size(); ++i) {
    if (!eq(mutated[i], prog[i])) { any_diff = true; break; }
  }
  assert(any_diff);

  std::cout << "ir_encoding ok\n";
  return 0;
}
