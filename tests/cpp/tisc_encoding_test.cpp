#include <cassert>
#include <vector>

#include "t81/isa/encoding.hpp"

int main() {
  using namespace t81::tisc;

  [[maybe_unused]] Program program;
  program.insns.push_back({Opcode::Mov, 1, 0, 0});
  program.insns.push_back({Opcode::Push, 1, 0, 0});
  program.insns.push_back({Opcode::TAnd, 2, 1, 0});
  program.insns.push_back({Opcode::TXor, 3, 1, 2});
  program.insns.push_back({Opcode::AxRead, 4, 42, 0});
  program.insns.push_back({Opcode::AxVerify, 5, 0, 0});

  [[maybe_unused]] auto bytes = encode(program);
  [[maybe_unused]] auto decoded = decode(bytes);
  assert(decoded.has_value());
  const Program& round = decoded.value();
  assert(round.insns.size() == program.insns.size());
  for (std::size_t i = 0; i < program.insns.size(); ++i) {
    [[maybe_unused]] const auto& lhs = program.insns[i];
    [[maybe_unused]] const auto& rhs = round.insns[i];
    assert(lhs.opcode == rhs.opcode);
    assert(lhs.a == rhs.a);
    assert(lhs.b == rhs.b);
    assert(lhs.c == rhs.c);
  }

  if (!bytes.empty()) {
    bytes[0] = static_cast<std::byte>(0xFF);
    [[maybe_unused]] auto bad = decode(bytes);
    assert(!bad.has_value());
    assert(bad.error() == EncodingError::InvalidOpcode);
  }

  return 0;
}
