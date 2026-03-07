#pragma once

#include <cstdint>
#include "t81/isa/opcodes.hpp"

namespace t81::isa {

// Compatibility alias for RFC-0026 phase-1 opcode subset.
// Canonical opcode ownership is t81::tisc::Opcode.
enum class AIOpcode : std::uint8_t {
  ATTN,
  QMATMUL,
  EMBED,
  WLOAD,
  GATHER,
  SCATTER,
};

[[nodiscard]] constexpr t81::tisc::Opcode to_tisc_opcode(AIOpcode op) {
  switch (op) {
    case AIOpcode::ATTN:    return t81::tisc::Opcode::ATTN;
    case AIOpcode::QMATMUL: return t81::tisc::Opcode::QMATMUL;
    case AIOpcode::EMBED:   return t81::tisc::Opcode::EMBED;
    case AIOpcode::WLOAD:   return t81::tisc::Opcode::WLOAD;
    case AIOpcode::GATHER:  return t81::tisc::Opcode::GATHER;
    case AIOpcode::SCATTER: return t81::tisc::Opcode::SCATTER;
  }
  return t81::tisc::Opcode::Nop;
}

[[nodiscard]] constexpr bool is_phase1_ai_opcode(t81::tisc::Opcode op) {
  return op == t81::tisc::Opcode::ATTN    || op == t81::tisc::Opcode::QMATMUL ||
         op == t81::tisc::Opcode::EMBED   || op == t81::tisc::Opcode::WLOAD   ||
         op == t81::tisc::Opcode::GATHER  || op == t81::tisc::Opcode::SCATTER;
}

} // namespace t81::isa
