#include "t81/crypto/sha3.hpp"
#include "t81/isa/binary_io.hpp"
#include "t81/isa/program.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace fs = std::filesystem;

static bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "tisc_binary_io_determinism_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

static std::vector<uint8_t> read_u8(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!expect(static_cast<bool>(in), "failed to open binary output")) {
    return {};
  }
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static t81::T81BigInt make_repeated_digit_bigint(int digits, int digit) {
  t81::T81BigInt value = t81::T81BigInt::from_i64(0);
  const t81::T81BigInt ten = t81::T81BigInt::from_i64(10);
  const t81::T81BigInt d = t81::T81BigInt::from_i64(digit);
  for (int i = 0; i < digits; ++i) {
    value = value * ten + d;
  }
  return value;
}

static bool test_save_program_is_bit_stable_for_same_program() {
  t81::tisc::Program program;
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 1, 42, 0, t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0, t81::tisc::LiteralKind::Bool});
  program.insns.push_back(
      {t81::tisc::Opcode::LoadImm, 3, 1, 0, t81::tisc::LiteralKind::BigIntHandle});
  program.insns.push_back({t81::tisc::Opcode::Print, 1, 0, 0, t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::Print, 2, 0, 0, t81::tisc::LiteralKind::Bool});
  program.insns.push_back({t81::tisc::Opcode::Print, 3, 0, 0, t81::tisc::LiteralKind::Int});
  program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0, t81::tisc::LiteralKind::Int});
  program.float_pool = {1.25, -2.5};
  program.bigint_pool = {
      t81::T81BigInt::from_i64(9223372036854775807LL) + t81::T81BigInt::one(),
      t81::T81BigInt::neg(make_repeated_digit_bigint(60, 7)),
  };
  program.symbol_pool = {"alpha", "beta"};
  program.axion_policy_text = "(policy (tier 1))";
  program.match_metadata_text = "(match-metadata)";

  const fs::path f1 = fs::temp_directory_path() / "t81-binary-io-determinism-1.tisc";
  const fs::path f2 = fs::temp_directory_path() / "t81-binary-io-determinism-2.tisc";

  t81::tisc::save_program(program, f1.string());
  t81::tisc::save_program(program, f2.string());

  const auto b1 = read_u8(f1);
  const auto b2 = read_u8(f2);
  if (!expect(!b1.empty() && !b2.empty(), "serialized outputs are unexpectedly empty")) {
    return false;
  }
  if (!expect(b1 == b2, "binary outputs diverged for identical program")) {
    return false;
  }

  const auto h1 = t81::crypto::sha3_512_hex(b1);
  const auto h2 = t81::crypto::sha3_512_hex(b2);
  if (!expect(h1 == h2, "hash outputs diverged for identical program")) {
    return false;
  }

  const auto loaded = t81::tisc::load_program(f1.string());
  if (!expect(loaded.insns.size() == program.insns.size(), "loaded instruction count mismatch")) {
    return false;
  }
  if (!expect(loaded.insns[1].literal_kind == t81::tisc::LiteralKind::Bool,
              "literal kind roundtrip mismatch")) {
    return false;
  }
  if (!expect(loaded.insns[2].literal_kind == t81::tisc::LiteralKind::BigIntHandle,
              "bigint literal kind roundtrip mismatch")) {
    return false;
  }
  if (!expect(loaded.bigint_pool.size() == program.bigint_pool.size(),
              "bigint pool size roundtrip mismatch")) {
    return false;
  }
  for (std::size_t i = 0; i < program.bigint_pool.size(); ++i) {
    if (!expect(loaded.bigint_pool[i].to_string() == program.bigint_pool[i].to_string(),
                "bigint pool payload roundtrip mismatch")) {
      return false;
    }
  }

  const fs::path f3 = fs::temp_directory_path() / "t81-binary-io-determinism-3.tisc";
  t81::tisc::save_program(loaded, f3.string());
  const auto b3 = read_u8(f3);
  if (!expect(!b3.empty(), "reserialized output is unexpectedly empty")) {
    return false;
  }
  if (!expect(b1 == b3, "binary output drift after load/save roundtrip")) {
    return false;
  }

  std::error_code ec;
  fs::remove(f1, ec);
  fs::remove(f2, ec);
  fs::remove(f3, ec);
  return true;
}

int main() {
  if (!test_save_program_is_bit_stable_for_same_program()) {
    return 1;
  }
  std::cout << "tisc binary io determinism test passed!\n";
  return 0;
}
