#pragma once

#include <cstdint>

namespace t81::tisc {
// Opcode definitions per spec/tisc-spec.md. This is a subset placeholder.
enum class Opcode : std::uint8_t {
  Nop = 0,
  Halt = 1,
  LoadImm = 2,
  Load = 3,
  Store = 4,
  Add = 5,
  Sub = 6,
  Mul = 7,
  Div = 8,
  Mod = 9,
  Jump = 10,
  JumpIfZero = 11,
};
}  // namespace t81::tisc
