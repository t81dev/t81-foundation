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
  Jump = 7,
  JumpIfZero = 8,
};
}  // namespace t81::tisc

