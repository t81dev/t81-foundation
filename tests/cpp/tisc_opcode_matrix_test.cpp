#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"

#include <cassert>
#include <iostream>
#include <unordered_set>

static void test_opcode_name_and_uniqueness() {
  std::unordered_set<std::string_view> names;
  for (auto op : t81::tisc::kAllOpcodes) {
    auto name = t81::tisc::opcode_name(op);
    assert(!name.empty());
    [[maybe_unused]] auto [_, inserted] = names.insert(name);
    assert(inserted);
  }
}

static void test_full_opcode_roundtrip_encoding() {
  t81::tisc::Program program;
  program.insns.reserve(t81::tisc::kAllOpcodes.size());
  int i = 0;
  for (auto op : t81::tisc::kAllOpcodes) {
    t81::tisc::Insn insn{};
    insn.opcode = op;
    insn.a = i;
    insn.b = i * 3;
    insn.c = -i;
    program.insns.push_back(insn);
    ++i;
  }

  auto bytes = t81::tisc::encode(program);
  auto decoded = t81::tisc::decode(bytes);
  assert(decoded.has_value());
  assert(decoded->insns.size() == program.insns.size());

  for (std::size_t idx = 0; idx < program.insns.size(); ++idx) {
    assert(decoded->insns[idx].opcode == program.insns[idx].opcode);
  }
}

int main() {
  test_opcode_name_and_uniqueness();
  test_full_opcode_roundtrip_encoding();
  std::cout << "tisc opcode matrix tests passed!\n";
  return 0;
}
