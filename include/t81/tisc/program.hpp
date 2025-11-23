#pragma once

#include <cstdint>
#include <vector>
#include "t81/tisc/opcodes.hpp"

namespace t81::tisc {
struct Insn {
  Opcode opcode{Opcode::Nop};
  std::int32_t a{0};
  std::int32_t b{0};
  std::int32_t c{0};
};

struct Program {
  std::vector<Insn> insns;
};
}  // namespace t81::tisc

