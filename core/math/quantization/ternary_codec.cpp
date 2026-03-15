// core/math/quantization/ternary_codec.cpp
//
// Deterministic ternary quantization codec — RFC-0031 §Deterministic AI Arithmetic Contract.
// Promoted from experiments/ai/quantization/ per RFC-0032 Phase 1 (C-01).
//
// Float-domain metric computations (MSE, PSNR) have been removed from this translation unit.
// They are available under T81_BUILD_DIAGNOSTICS in tools/diagnostics/ternary_codec_metrics.cpp.

#include "t81/math/quantization/ternary_codec.hpp"

#include <cassert>

namespace t81::math::quantization {

// ── Base-81 digit encoding ─────────────────────────────────────────────────────
//
// 4 trits are packed into 1 byte using base-3 positional encoding:
//
//   byte = trit[0]*27 + trit[1]*9 + trit[2]*3 + trit[3]   (MSB trit first)
//
// where each trit is encoded as a base-3 digit:
//   TritValue::Neg  → 0
//   TritValue::Zero → 1
//   TritValue::Pos  → 2
//
// Maximum byte value: 2*27 + 2*9 + 2*3 + 2 = 80.  All values are in [0, 80],
// which fits in uint8_t and is exactly the Base-81 alphabet.

static constexpr uint8_t kDigitNeg  = 0u;
static constexpr uint8_t kDigitZero = 1u;
static constexpr uint8_t kDigitPos  = 2u;

static constexpr uint8_t trit_to_digit(TritValue t) noexcept {
  switch (t) {
    case TritValue::Neg:  return kDigitNeg;
    case TritValue::Pos:  return kDigitPos;
    default:              return kDigitZero;
  }
}

static constexpr TritValue digit_to_trit(uint8_t d) noexcept {
  if (d == kDigitNeg) return TritValue::Neg;  // 0
  if (d == kDigitPos) return TritValue::Pos;  // 2
  return TritValue::Zero;                      // 1 (or any other value → Zero)
}

// Powers of 3 for positional encoding (MSB first at index 0).
static constexpr uint8_t kPow3[4] = {27u, 9u, 3u, 1u};

// ── pack_ternary_to_base81 ─────────────────────────────────────────────────────

std::vector<uint8_t> pack_ternary_to_base81(const std::vector<TritValue>& trits) {
  const std::size_t n = trits.size();
  std::vector<uint8_t> packed;
  packed.reserve((n + 3u) / 4u);

  for (std::size_t i = 0; i < n; i += 4u) {
    // Encode up to 4 trits into one byte using positional base-3 (MSB first).
    // Partial final groups are zero-padded on the right; padding is stripped
    // on decode via original_count.
    uint8_t byte = 0u;
    for (int j = 0; j < 4; ++j) {
      const std::size_t idx = i + static_cast<std::size_t>(j);
      const uint8_t digit = (idx < n) ? trit_to_digit(trits[idx]) : kDigitZero;
      byte = static_cast<uint8_t>(byte + kPow3[j] * digit);
    }
    packed.push_back(byte);
  }

  return packed;
}

// ── unpack_base81_to_ternary ───────────────────────────────────────────────────

std::vector<TritValue> unpack_base81_to_ternary(const std::vector<uint8_t>& packed,
                                                 std::size_t                  original_count) {
  std::vector<TritValue> trits;
  trits.reserve(original_count);

  for (std::size_t i = 0; i < packed.size() && trits.size() < original_count; ++i) {
    // Decode 4 base-3 digits from one byte (MSB digit first).
    // The byte encodes: d[0]*27 + d[1]*9 + d[2]*3 + d[3].
    uint8_t digits[4];
    uint8_t byte = packed[i];
    for (int j = 3; j >= 0; --j) {  // extract LSB digit first
      digits[j] = byte % 3u;
      byte /= 3u;
    }
    for (int j = 0; j < 4 && trits.size() < original_count; ++j) {
      trits.push_back(digit_to_trit(digits[j]));
    }
  }

  return trits;
}

// ── quantize_threshold ────────────────────────────────────────────────────────

std::vector<TritValue> quantize_threshold(const std::vector<int32_t>& values,
                                          int32_t                      neg_threshold,
                                          int32_t                      pos_threshold) {
  std::vector<TritValue> trits;
  trits.reserve(values.size());

  for (const int32_t v : values) {
    if      (v < neg_threshold) trits.push_back(TritValue::Neg);
    else if (v > pos_threshold) trits.push_back(TritValue::Pos);
    else                        trits.push_back(TritValue::Zero);
  }

  return trits;
}

// ── dequantize ────────────────────────────────────────────────────────────────

std::vector<int32_t> dequantize(const std::vector<TritValue>& trits, int32_t scale) {
  std::vector<int32_t> out;
  out.reserve(trits.size());

  for (const TritValue t : trits) {
    switch (t) {
      case TritValue::Neg:  out.push_back(-scale); break;
      case TritValue::Pos:  out.push_back( scale); break;
      default:              out.push_back(0);      break;
    }
  }

  return out;
}

}  // namespace t81::math::quantization
