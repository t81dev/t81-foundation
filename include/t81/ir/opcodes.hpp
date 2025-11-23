#pragma once
#include <cstdint>

namespace t81::ir {

// Minimal opcode set to support examples/tests.
// Values are stable; extend by appending only.
enum class Opcode : uint16_t {
  // --- Meta / control ---
  Nop        = 0x0000,
  Halt       = 0x0001,
  Jump       = 0x0002,  // imm = target address

  // --- Integer / scalar ALU (placeholders) ---
  Add        = 0x0100,
  Sub        = 0x0101,
  Mul        = 0x0102,
  Div        = 0x0103,

  // --- BigInt ops (T243) ---
  BigAdd     = 0x0200,
  BigSub     = 0x0201,
  BigMul     = 0x0202,
  BigDiv     = 0x0203,
  BigMod     = 0x0204,
  BigCmp     = 0x0205,

  // --- Tensor ops (T729) ---
  TDot       = 0x0300,  // vector dot → {1}
  TTranspose = 0x0301,  // 2D transpose
  TSlice2D   = 0x0302,  // slice rows[r0:r1), cols[c0:c1)
  TReshape   = 0x0303,  // reshape with same element count
  TMatMul    = 0x0304,  // matmul  (m×k)·(k×n) → (m×n)
  TReduce    = 0x0305,  // reduce op along axis (imm flags)

  // --- Memory / IO (skeleton) ---
  Load       = 0x0400,
  Store      = 0x0401,

  // --- Capability / CanonFS (skeleton) ---
  CapCheck   = 0x0500,
  CapGrant   = 0x0501,
};

} // namespace t81::ir
