#include "t81/tisc/encoding.hpp"

#include <cstring>

namespace t81::tisc {
std::vector<std::byte> encode(const Program& program) {
  std::vector<std::byte> bytes;
  bytes.reserve(program.insns.size() * sizeof(Insn));
  for (const auto& insn : program.insns) {
    const auto opcode = static_cast<std::uint8_t>(insn.opcode);
    bytes.push_back(static_cast<std::byte>(opcode));
    auto append = [&bytes](std::int32_t v) {
      for (int i = 0; i < 4; ++i) {
        bytes.push_back(static_cast<std::byte>((v >> (i * 8)) & 0xFF));
      }
    };
    append(insn.a);
    append(insn.b);
    append(insn.c);
  }
  return bytes;
}

std::expected<Program, EncodingError> decode(const std::vector<std::byte>& bytes) {
  Program program;
  constexpr std::size_t kInsnSize = 1 + 4 * 3;
  if (bytes.size() % kInsnSize != 0) {
    return EncodingError::Truncated;
  }
  for (std::size_t offset = 0; offset < bytes.size(); offset += kInsnSize) {
    Insn insn;
    const auto opcode = static_cast<std::uint8_t>(bytes[offset]);
    if (opcode > static_cast<std::uint8_t>(Opcode::JumpIfZero)) {
      return EncodingError::InvalidOpcode;
    }
    insn.opcode = static_cast<Opcode>(opcode);
    auto read = [&bytes, offset](std::size_t index) {
      std::int32_t value = 0;
      for (int i = 0; i < 4; ++i) {
        value |= static_cast<std::int32_t>(bytes[offset + 1 + index * 4 + i]) << (i * 8);
      }
      return value;
    };
    insn.a = read(0);
    insn.b = read(1);
    insn.c = read(2);
    program.insns.push_back(insn);
  }
  return program;
}
}  // namespace t81::tisc

