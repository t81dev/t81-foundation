// tests/determinism/codec/ternary_codec_test.cpp
//
// RFC-0032 Phase 1 gate criteria — determinism tests for the promoted ternary codec.
//
// Acceptance criteria covered:
//   [C01-01]  pack_ternary_to_base81 produces bit-exact output for identical input trits.
//   [C01-02]  unpack_base81_to_ternary is the exact inverse of pack_ternary_to_base81.
//   [C01-03]  pack → unpack round-trip preserves every trit value (Neg, Zero, Pos).
//   [C01-04]  quantize_threshold correctly classifies values below, within, and above
//             thresholds using only integer comparisons.
//   [C01-05]  dequantize maps Neg→−scale, Zero→0, Pos→+scale for arbitrary scale.
//   [C01-06]  Round-trip invariant: quantize_threshold(dequantize(trits, s), −s−1, s+1)
//             reconstructs the original trit sequence unchanged.
//   [C01-07]  pack_ternary_to_base81 result is stable across two calls with identical input
//             (no hidden state, no timing dependency).
//   [C01-08]  Packing of a length not divisible by 4 unpacks to the original count exactly.

#include "t81/math/quantization/ternary_codec.hpp"

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>

using namespace t81::math::quantization;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
}

// ── [C01-01] Bit-exact packing ────────────────────────────────────────────────

static void test_pack_bit_exact() {
  std::printf("\n[C01-01] pack_ternary_to_base81 produces bit-exact output\n");

  const std::vector<TritValue> trits = {
    TritValue::Pos, TritValue::Zero, TritValue::Neg, TritValue::Pos,
    TritValue::Neg, TritValue::Neg,  TritValue::Zero, TritValue::Pos,
  };

  const auto pack1 = pack_ternary_to_base81(trits);
  const auto pack2 = pack_ternary_to_base81(trits);

  check(pack1.size() == pack2.size(), "[C01-01] repeated pack: same size");
  bool identical = (pack1 == pack2);
  check(identical, "[C01-01] repeated pack: bit-exact identical output");
}

// ── [C01-02/03] Round-trip: pack → unpack ─────────────────────────────────────

static void test_roundtrip_full() {
  std::printf("\n[C01-02/03] pack → unpack round-trip (length divisible by 4)\n");

  const std::vector<TritValue> original = {
    TritValue::Pos, TritValue::Zero, TritValue::Neg, TritValue::Pos,
    TritValue::Neg, TritValue::Neg,  TritValue::Zero, TritValue::Zero,
    TritValue::Pos, TritValue::Pos,  TritValue::Neg,  TritValue::Zero,
  };

  const auto packed   = pack_ternary_to_base81(original);
  const auto unpacked = unpack_base81_to_ternary(packed, original.size());

  check(unpacked.size() == original.size(), "[C01-02] unpacked.size() == original.size()");
  check(unpacked == original,               "[C01-03] every trit value preserved in round-trip");
}

// ── [C01-08] Non-multiple-of-4 length ─────────────────────────────────────────

static void test_roundtrip_partial_group() {
  std::printf("\n[C01-08] round-trip with length not divisible by 4\n");

  const std::vector<TritValue> original = {
    TritValue::Neg, TritValue::Pos, TritValue::Zero,  // 3 trits — one partial group
  };

  const auto packed   = pack_ternary_to_base81(original);
  const auto unpacked = unpack_base81_to_ternary(packed, original.size());

  check(unpacked.size() == 3u,   "[C01-08] unpacked.size() == 3");
  check(unpacked == original,    "[C01-08] partial-group round-trip preserves trits");
}

static void test_roundtrip_length_5() {
  const std::vector<TritValue> original = {
    TritValue::Pos, TritValue::Neg, TritValue::Zero, TritValue::Pos, TritValue::Neg,  // 5 trits
  };
  const auto packed   = pack_ternary_to_base81(original);
  const auto unpacked = unpack_base81_to_ternary(packed, original.size());
  check(unpacked.size() == 5u, "[C01-08] length-5 round-trip: size");
  check(unpacked == original,  "[C01-08] length-5 round-trip: content");
}

// ── [C01-04] quantize_threshold ───────────────────────────────────────────────

static void test_quantize_threshold() {
  std::printf("\n[C01-04] quantize_threshold classifies correctly\n");

  // Thresholds: neg < −100, pos > 100
  const std::vector<int32_t> values = {-200, -101, -100, 0, 100, 101, 200};
  const auto trits = quantize_threshold(values, -100, 100);

  check(trits.size() == 7u,                    "[C01-04] output size matches input");
  check(trits[0] == TritValue::Neg,            "[C01-04] -200 < -100 → Neg");
  check(trits[1] == TritValue::Neg,            "[C01-04] -101 < -100 → Neg");
  check(trits[2] == TritValue::Zero,           "[C01-04] -100 not < -100 → Zero (boundary)");
  check(trits[3] == TritValue::Zero,           "[C01-04]    0 → Zero");
  check(trits[4] == TritValue::Zero,           "[C01-04]  100 not > 100 → Zero (boundary)");
  check(trits[5] == TritValue::Pos,            "[C01-04]  101 > 100 → Pos");
  check(trits[6] == TritValue::Pos,            "[C01-04]  200 > 100 → Pos");
}

// ── [C01-05] dequantize ───────────────────────────────────────────────────────

static void test_dequantize() {
  std::printf("\n[C01-05] dequantize maps trits to scaled integers\n");

  constexpr int32_t scale = 42;
  const std::vector<TritValue> trits = {
    TritValue::Neg, TritValue::Zero, TritValue::Pos,
    TritValue::Pos, TritValue::Neg,
  };

  const auto out = dequantize(trits, scale);

  check(out.size() == 5u,    "[C01-05] output size matches");
  check(out[0] == -42,       "[C01-05] Neg → −scale");
  check(out[1] ==   0,       "[C01-05] Zero → 0");
  check(out[2] ==  42,       "[C01-05] Pos → +scale");
  check(out[3] ==  42,       "[C01-05] Pos → +scale (second)");
  check(out[4] == -42,       "[C01-05] Neg → −scale (second)");
}

// ── [C01-06] Round-trip invariant ─────────────────────────────────────────────

static void test_roundtrip_invariant() {
  std::printf("\n[C01-06] quantize(dequantize(trits, s), −s+1, s−1) == trits\n");

  // For scale=S, thresholds (−S+1, S−1) satisfy the invariant:
  //   −S < −S+1  →  dequantize(Neg,S)=−S  maps back to Neg  ✓
  //   0 is not < −S+1 and not > S−1        →  Zero stays Zero ✓
  //   +S > S−1   →  dequantize(Pos,S)=+S  maps back to Pos  ✓
  constexpr int32_t scale = 10;
  const int32_t neg_thresh = -scale + 1;  // = −9
  const int32_t pos_thresh =  scale - 1;  // = +9

  const std::vector<TritValue> original = {
    TritValue::Neg, TritValue::Zero, TritValue::Pos,
    TritValue::Pos, TritValue::Neg,  TritValue::Zero,
    TritValue::Zero, TritValue::Neg, TritValue::Pos,
  };

  const auto integers      = dequantize(original, scale);
  const auto reconstructed = quantize_threshold(integers, neg_thresh, pos_thresh);

  check(reconstructed == original,
        "[C01-06] round-trip invariant holds for all Neg/Zero/Pos trits");
}

// ── [C01-07] No hidden state ───────────────────────────────────────────────────

static void test_no_hidden_state() {
  std::printf("\n[C01-07] pack result is stable across two separate calls\n");

  const std::vector<TritValue> trits(100, TritValue::Pos);
  const auto a = pack_ternary_to_base81(trits);
  const auto b = pack_ternary_to_base81(trits);
  check(a == b, "[C01-07] 100×Pos: identical packing on repeated calls");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Ternary codec determinism tests (RFC-0032 Phase 1) ===\n");

  test_pack_bit_exact();
  test_roundtrip_full();
  test_roundtrip_partial_group();
  test_roundtrip_length_5();
  test_quantize_threshold();
  test_dequantize();
  test_roundtrip_invariant();
  test_no_hidden_state();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
