#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "t81/fraction.hpp"
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
  std::vector<double> float_pool;
  std::vector<t81::T81Fraction> fraction_pool;
  std::vector<std::string> symbol_pool;
};
}  // namespace t81::tisc
