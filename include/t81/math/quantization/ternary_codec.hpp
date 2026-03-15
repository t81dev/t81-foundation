// include/t81/math/quantization/ternary_codec.hpp
//
// Deterministic ternary quantization codec — RFC-0031 §Deterministic AI Arithmetic Contract.
// Promoted from experiments/ai/quantization/ per RFC-0032 Phase 1 (C-01).
//
// All entry points are integer-only. No floating-point, no timing, no external dependencies.
// Bit-exact across x86-64 and ARM64 for identical inputs (RFC-0002 §3).

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace t81::math::quantization {

/// Canonical ternary trit value: negative (−1), zero (0), or positive (+1).
enum class TritValue : int8_t { Neg = -1, Zero = 0, Pos = 1 };

// ── Base-81 canonical packing ──────────────────────────────────────────────────

/// Pack a sequence of trits into canonical Base-81 bytes (4 trits per byte).
///
/// Encoding uses positional base-3 with MSB trit first:
///   byte = trit[0]*27 + trit[1]*9 + trit[2]*3 + trit[3]
///   TritValue::Neg  → base-3 digit 0
///   TritValue::Zero → base-3 digit 1
///   TritValue::Pos  → base-3 digit 2
///
/// Byte values are in [0, 80] (the full Base-81 alphabet).
/// The last group is zero-padded (digit 1 = Zero) if size is not a multiple of 4;
/// padding is stripped by original_count on decode.
///
/// Deterministic: produces identical bit patterns on all supported platforms.
[[nodiscard]] std::vector<uint8_t>
pack_ternary_to_base81(const std::vector<TritValue>& trits);

/// Unpack Base-81 bytes back to exactly `original_count` trits.
/// Inverse of pack_ternary_to_base81: unpack(pack(t)) == t for all valid t.
[[nodiscard]] std::vector<TritValue>
unpack_base81_to_ternary(const std::vector<uint8_t>& packed,
                         std::size_t                  original_count);

// ── Integer-domain threshold quantization ─────────────────────────────────────

/// Quantize integer fixed-point values to trits using explicit thresholds.
///
///   value < neg_threshold → TritValue::Neg
///   value > pos_threshold → TritValue::Pos
///   otherwise             → TritValue::Zero
///
/// All comparisons are signed 32-bit integer comparisons; no floating-point.
[[nodiscard]] std::vector<TritValue>
quantize_threshold(const std::vector<int32_t>& values,
                   int32_t                      neg_threshold,
                   int32_t                      pos_threshold);

// ── Dequantization (round-trip partner) ───────────────────────────────────────

/// Map trits back to scaled integer values.
///
///   TritValue::Neg  → −scale
///   TritValue::Zero →  0
///   TritValue::Pos  → +scale
///
/// Round-trip invariant (RFC-0031): given thresholds satisfying
///   −scale < neg_threshold < 0  and  0 < pos_threshold < +scale,
///   quantize_threshold(dequantize(trits, scale), neg_threshold, pos_threshold) == trits
/// holds for all inputs in the representable range.
///
/// Concretely with scale=S, thresholds=(−S+1, S−1):
///   dequantize(Neg,  S) = −S  →  −S < −S+1  → quantizes back to Neg  ✓
///   dequantize(Zero, S) =  0  →  0 is not < −S+1 and not > S−1       → Zero ✓
///   dequantize(Pos,  S) = +S  →  +S > S−1   → quantizes back to Pos  ✓
[[nodiscard]] std::vector<int32_t>
dequantize(const std::vector<TritValue>& trits, int32_t scale);

}  // namespace t81::math::quantization
