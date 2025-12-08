#include <cassert>
#include <vector>

#include "t81/tisc/encoding.hpp"

int main() {
  using namespace t81::tisc;

  Program program;
  program.insns.push_back({Opcode::Mov, 1, 0, 0});
  program.insns.push_back({Opcode::Push, 1, 0, 0});
  program.insns.push_back({Opcode::TAnd, 2, 1, 0});
  program.insns.push_back({Opcode::TXor, 3, 1, 2});
  program.insns.push_back({Opcode::AxRead, 4, 42, 0});
  program.insns.push_back({Opcode::AxVerify, 5, 0, 0});

  auto bytes = encode(program);
  auto decoded = decode(bytes);
  assert(decoded.has_value());
  [[maybe_unused]] const Program& round = decoded.value();
  assert(round.insns.size() == program.insns.size());
  for (std::size_t i = 0; i < program.insns.size(); ++i) {
    const auto& lhs = program.insns[i];
    const auto& rhs = round.insns[i];
    assert(lhs.opcode == rhs.opcode);
    assert(lhs.a == rhs.a);
    assert(lhs.b == rhs.b);
    assert(lhs.c == rhs.c);
  }

  if (!bytes.empty()) {
    bytes[0] = static_cast<std::byte>(0xFF);
    auto bad = decode(bytes);
    assert(!bad.has_value());
    assert(bad.error() == EncodingError::InvalidOpcode);
  }

  return 0;
}
