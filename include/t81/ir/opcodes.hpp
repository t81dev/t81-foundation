#pragma once
#include <cstdint>

namespace t81::ir {

// Minimal opcode set placeholder.
// Extend by mirroring legacy .td/.cweb defs as you migrate.
enum class Opcode : uint16_t {
  Nop = 0,

  // Arithmetic (scalar)
  Add, Sub, Mul, Div, Mod,

  // BigInt
  BigAdd, BigSub, BigMul, BigMod, BigGcd,

  // Tensor
  TDot, TTranspose, TSlice2D, TReshape,

  // Control
  Jump, JumpIfZero, JumpIfNeg, Call, Ret,

  // Memory (abstract / VM-level)
  Load, Store,

  // CanonFS / Capability
  CapCheck, CapGrant,

  // Reserved range for experimental ops
  _FirstExperimental = 0x4000
};

} // namespace t81::ir
