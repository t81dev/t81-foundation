#pragma once
#include <cstdint>
#include <array>
#include "t81/ir/opcodes.hpp"

namespace t81::ir {

// A tiny, POD-style instruction format suitable for serialization.
// Keep this stable; extend by adding fields at the tail if needed.
struct Insn {
  Opcode      op{Opcode::Nop};

  // Up to 3 generic 32-bit operands (register ids, tensor slots, etc.)
  std::array<uint32_t, 3> ops{{0,0,0}};

  // 64-bit immediate / address / small constant handle.
  uint64_t    imm{0};

  // Flags (bitfield, op-specific semantics).
  uint32_t    flags{0};

  // Reserved for future use; initialize to zero for forward-compat.
  uint32_t    _reserved{0};
};

// Simple helpers
constexpr Insn make0(Opcode op) {
  return Insn{op, {0,0,0}, 0, 0, 0};
}

constexpr Insn make1(Opcode op, uint32_t a) {
  return Insn{op, {a,0,0}, 0, 0, 0};
}

constexpr Insn make2(Opcode op, uint32_t a, uint32_t b) {
  return Insn{op, {a,b,0}, 0, 0, 0};
}

constexpr Insn make3(Opcode op, uint32_t a, uint32_t b, uint32_t c) {
  return Insn{op, {a,b,c}, 0, 0, 0};
}

constexpr Insn make_imm(Opcode op, uint64_t imm, uint32_t flags = 0) {
  return Insn{op, {0,0,0}, imm, flags, 0};
}

} // namespace t81::ir
