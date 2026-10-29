#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"

#include <cassert>
#include <iostream>
#include <unordered_set>

static void test_opcode_name_and_uniqueness() {
  std::unordered_set<std::string_view> names;
  std::unordered_set<std::uint8_t> values;
  for (auto op : t81::tisc::kAllOpcodes) {
    auto name = t81::tisc::opcode_name(op);
    if (name == "Unknown") {
      continue;
    }
    assert(!name.empty());
    [[maybe_unused]] auto [_, inserted_name] = names.insert(name);
    assert(inserted_name);
    [[maybe_unused]] auto [__, inserted_val] = values.insert(static_cast<std::uint8_t>(op));
    assert(inserted_val);
  }
}

static void test_frozen_isa_mappings() {
  using namespace t81::tisc;
  assert(static_cast<std::uint8_t>(Opcode::Int2BigInt) == 193);
  assert(static_cast<std::uint8_t>(Opcode::TWMATMUL) == 194);
  assert(static_cast<std::uint8_t>(Opcode::TQUANT) == 195);
  assert(static_cast<std::uint8_t>(Opcode::TATTN) == 196);
  assert(static_cast<std::uint8_t>(Opcode::TWEMBED) == 197);
  assert(static_cast<std::uint8_t>(Opcode::TERNACCUM) == 198);
  assert(static_cast<std::uint8_t>(Opcode::TACT) == 199);
  assert(static_cast<std::uint8_t>(Opcode::FFICall) == 200);
  assert(static_cast<std::uint8_t>(Opcode::FFIRegister) == 201);
  assert(static_cast<std::uint8_t>(Opcode::FFIPolicySet) == 202);
  assert(static_cast<std::uint8_t>(Opcode::POLYMUL) == 203);
  assert(static_cast<std::uint8_t>(Opcode::POLYMOD) == 204);
  assert(static_cast<std::uint8_t>(Opcode::TVecSub) == 205);
  assert(static_cast<std::uint8_t>(Opcode::TNOT_SWAR) == 206);
  assert(static_cast<std::uint8_t>(Opcode::TAND_SWAR) == 207);
  assert(static_cast<std::uint8_t>(Opcode::STOCHASTIC_DECODE) == 208);
  assert(static_cast<std::uint8_t>(Opcode::TOR_SWAR) == 220);
  assert(static_cast<std::uint8_t>(Opcode::NEURAL_FWD) == 224);
  assert(static_cast<std::uint8_t>(Opcode::VLoad) == 240);
  assert(static_cast<std::uint8_t>(Opcode::AgentInvoke) == 246);
}

static void test_full_opcode_roundtrip_encoding() {
  t81::tisc::Program program;
  program.insns.reserve(t81::tisc::kAllOpcodes.size());
  int i = 0;
  for (auto op : t81::tisc::kAllOpcodes) {
    // kAllOpcodes enumerates numeric slots, including reserved values.
    // Only declared instructions belong to a successful program round trip.
    if (t81::tisc::opcode_name(op) == "Unknown") {
      continue;
    }
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
    assert(decoded->insns[idx].a == program.insns[idx].a);
    assert(decoded->insns[idx].b == program.insns[idx].b);
    assert(decoded->insns[idx].c == program.insns[idx].c);
  }
}

static void test_reserved_opcode_rejection() {
  std::size_t rejected = 0;
  for (unsigned raw = 0; raw < 256; ++raw) {
    if (t81::tisc::is_valid_opcode(static_cast<std::uint8_t>(raw))) {
      continue;
    }
    t81::tisc::Program program;
    t81::tisc::Insn insn{};
    insn.opcode = static_cast<t81::tisc::Opcode>(raw);
    program.insns.push_back(insn);
    auto decoded = t81::tisc::decode(t81::tisc::encode(program));
    assert(!decoded.has_value());
    assert(decoded.error() == t81::tisc::EncodingError::InvalidOpcode);
    ++rejected;
  }
  assert(rejected > 0);
}

int main() {
  test_opcode_name_and_uniqueness();
  test_frozen_isa_mappings();
  test_full_opcode_roundtrip_encoding();
  test_reserved_opcode_rejection();
  std::cout << "tisc opcode matrix tests passed!\n";
  return 0;
}
