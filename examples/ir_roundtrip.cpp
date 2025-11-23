#include <iostream>
#include <vector>
#include "t81/t81.hpp"

int main() {
  using namespace t81::ir;

  // Build a tiny program
  std::vector<Insn> prog;
  prog.push_back(make0(Opcode::Nop));
  prog.push_back(make_imm(Opcode::Jump, 0x1000ull));
  prog.push_back(make3(Opcode::Add, 1, 2, 3));
  prog.push_back(make3(Opcode::TMatMul, 4, 5, 6));
  prog.push_back(make_imm(Opcode::TReduce, /*axis*/1, /*flags*/0x00000003u));

  // Encode to bytes
  auto bytes = encode_many(prog);

  // Decode back
  auto round = decode_many(bytes.data(), bytes.size());

  // Print a simple listing
  std::cout << "IR roundtrip (" << round.size() << " insns)\n";
  for (size_t i = 0; i < round.size(); ++i) {
    const auto& ins = round[i];
    std::cout << i << ": op=0x" << std::hex << (unsigned)static_cast<uint16_t>(ins.op)
              << std::dec
              << " ops=[" << ins.ops[0] << "," << ins.ops[1] << "," << ins.ops[2] << "]"
              << " imm=" << ins.imm
              << " flags=0x" << std::hex << ins.flags << std::dec
              << "\n";
  }

  // Quick sanity: sizes
  if (bytes.size() != round.size() * 32) {
    std::cerr << "encoding size mismatch\n";
    return 1;
  }

  std::cout << "ok\n";
  return 0;
}
