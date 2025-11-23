#include <cstdint>
#include <iostream>
#include <vector>
#include "t81/ir/opcodes.hpp"
#include "t81/ir/insn.hpp"
#include "t81/ir/encoding.hpp"

int main() {
  using namespace t81::ir;

  // Build a tiny program
  std::vector<Insn> prog = {
    make0(Opcode::Nop),
    make3(Opcode::BigMul, 1, 2, 3),
    make_imm(Opcode::Jump, 0xDEADBEEFCAFEBABEull),
    make2(Opcode::TDot, 7, 7),
  };

  // Encode -> bytes
  auto bytes = encode_many(prog);
  std::cout << "encoded bytes: " << bytes.size() << "\n";

  // Decode -> instructions
  auto got = decode_many(bytes.data(), bytes.size());
  std::cout << "decoded insns: " << got.size() << "\n";

  // Print a quick summary
  for (size_t i = 0; i < got.size(); ++i) {
    const auto& ins = got[i];
    std::cout << i << ": op=" << static_cast<uint16_t>(ins.op)
              << " ops=(" << ins.ops[0] << "," << ins.ops[1] << "," << ins.ops[2] << ")"
              << " imm=0x" << std::hex << ins.imm << std::dec
              << " flags=0x" << std::hex << ins.flags << std::dec
              << "\n";
  }

  return 0;
}
