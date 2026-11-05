#include "t81/isa/encoding.hpp"
#include "t81/isa/opcodes.hpp"

#include "test_runtime_check.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <unordered_set>
#include <vector>

// Undefined in the current header, including the holes inside kAllOpcodes.
// Keep this explicit so a missing opcode_name case cannot silently reduce coverage.
static constexpr std::array<std::uint8_t, 12> kUndefinedEncodings = {
    221, 222, 223, 247, 248, 249, 250, 251, 252, 253, 254, 255};

static constexpr unsigned kOpcodeByteCount = std::numeric_limits<std::uint8_t>::max() + 1u;

static void test_opcode_name_and_uniqueness() {
  std::unordered_set<std::string_view> names;
  std::unordered_set<std::uint8_t> values;
  // kAllOpcodes is a numeric interval, not a list of only defined opcodes.
  // Inspect the entire byte domain using the header's named opcode definitions.
  for (unsigned raw = 0; raw < kOpcodeByteCount; ++raw) {
    const auto op = static_cast<t81::tisc::Opcode>(raw);
    auto name = t81::tisc::opcode_name(op);
    if (name == "Unknown") {
      continue;
    }
    T81_TEST_CHECK(!name.empty());
    T81_TEST_CHECK(names.insert(name).second);
    T81_TEST_CHECK(values.insert(static_cast<std::uint8_t>(op)).second);
  }
  T81_TEST_CHECK(values.size() == kOpcodeByteCount - kUndefinedEncodings.size());
}

static void test_frozen_isa_mappings() {
  using namespace t81::tisc;
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::Int2BigInt) == 193);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TWMATMUL) == 194);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TQUANT) == 195);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TATTN) == 196);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TWEMBED) == 197);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TERNACCUM) == 198);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TACT) == 199);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::FFICall) == 200);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::FFIRegister) == 201);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::FFIPolicySet) == 202);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::POLYMUL) == 203);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::POLYMOD) == 204);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TVecSub) == 205);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TNOT_SWAR) == 206);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TAND_SWAR) == 207);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::STOCHASTIC_DECODE) == 208);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::TOR_SWAR) == 220);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::NEURAL_FWD) == 224);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::VLoad) == 240);
  T81_TEST_CHECK(static_cast<std::uint8_t>(Opcode::AgentInvoke) == 246);
}

static void test_full_opcode_roundtrip_encoding() {
  t81::tisc::Program program;
  program.insns.reserve(kOpcodeByteCount - kUndefinedEncodings.size());
  int i = 0;
  for (unsigned raw = 0; raw < kOpcodeByteCount; ++raw) {
    const auto op = static_cast<t81::tisc::Opcode>(raw);
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

  T81_TEST_CHECK(program.insns.size() == kOpcodeByteCount - kUndefinedEncodings.size());
  auto bytes = t81::tisc::encode(program);
  T81_TEST_CHECK(bytes.size() == 13 * program.insns.size());
  auto decoded = t81::tisc::decode(bytes);
  T81_TEST_CHECK(decoded.has_value());
  T81_TEST_CHECK(decoded->insns.size() == program.insns.size());

  for (std::size_t idx = 0; idx < program.insns.size(); ++idx) {
    const auto& actual = decoded->insns[idx];
    const auto& expected = program.insns[idx];
    T81_TEST_CHECK(actual.opcode == expected.opcode);
    T81_TEST_CHECK(actual.a == expected.a);
    T81_TEST_CHECK(actual.b == expected.b);
    T81_TEST_CHECK(actual.c == expected.c);
  }
}

static void test_undefined_opcode_rejection() {
  using namespace t81::tisc;
  Program program;
  program.insns = {{Opcode::Nop, 0, 0, 0}, {Opcode::Nop, 0, 0, 0},
                   {Opcode::Halt, 0, 0, 0}};
  const auto valid_bytes = encode(program);
  T81_TEST_CHECK(valid_bytes.size() == 3 * 13);

  for (auto raw : kUndefinedEncodings) {
    T81_TEST_CHECK(opcode_name(static_cast<Opcode>(raw)) == "Unknown");
    T81_TEST_CHECK(!is_valid_opcode(raw));

    // Reject an invalid first instruction and one following a valid prefix.
    std::vector<std::byte> single(13, std::byte{0});
    single[0] = static_cast<std::byte>(raw);
    const auto rejected_single = decode(single);
    T81_TEST_CHECK(!rejected_single.has_value());
    T81_TEST_CHECK(rejected_single.error() == EncodingError::InvalidOpcode);

    auto mixed = valid_bytes;
    mixed[13] = static_cast<std::byte>(raw);
    const auto rejected_mixed = decode(mixed);
    T81_TEST_CHECK(!rejected_mixed.has_value());
    T81_TEST_CHECK(rejected_mixed.error() == EncodingError::InvalidOpcode);
  }
}

int main() {
  test_opcode_name_and_uniqueness();
  test_frozen_isa_mappings();
  test_full_opcode_roundtrip_encoding();
  test_undefined_opcode_rejection();
  std::cout << "tisc opcode matrix tests passed: "
            << kOpcodeByteCount - kUndefinedEncodings.size() << " defined opcodes, "
            << kUndefinedEncodings.size() << " undefined encodings rejected.\n";
  return 0;
}
