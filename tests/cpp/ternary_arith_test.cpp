#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "t81/ternary.hpp"
#include "t81/ternary/arith.hpp"

// Ensure assertions are ALWAYS evaluated, even in NDEBUG / Release builds
#define ALWAYS_ASSERT(cond)                                                                       \
  do {                                                                                            \
    if (!(cond)) {                                                                                \
      std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::abort();                                                                               \
    }                                                                                             \
  } while (0)

using namespace t81;

int main() {
  using t81::Trit;
  using t81::ternary::add;
  using t81::ternary::decode_i64;
  using t81::ternary::encode_i64;

  // --- encode/decode roundtrips ---
  {
    std::vector<int64_t> vals = {0,
                                 1,
                                 -1,
                                 2,
                                 -2,
                                 3,
                                 -3,
                                 7,
                                 -7,
                                 42,
                                 -42,
                                 123456789,
                                 -123456789,
                                 INT64_C(1) << 40,
                                 -(INT64_C(1) << 40)};
    for (auto v : vals) {
      auto ds = encode_i64(v);
      auto r = decode_i64(ds);
      ALWAYS_ASSERT(r == v);
    }
  }

  // --- addition correctness on small cases ---
  {
    // 5 + 7 = 12
    auto a = encode_i64(5);
    auto b = encode_i64(7);
    auto c = add(a, b);
    ALWAYS_ASSERT(decode_i64(c) == 12);

    // (-9) + 4 = -5
    auto m9 = encode_i64(-9);
    auto p4 = encode_i64(4);
    auto sum = add(m9, p4);
    ALWAYS_ASSERT(decode_i64(sum) == -5);

    // Biggish numbers
    auto x = encode_i64(1234567);
    auto y = encode_i64(-987654);
    auto z = add(x, y);
    ALWAYS_ASSERT(decode_i64(z) == (1234567 - 987654));
  }

  // --- normalization: no redundant leading zeros (MS trits) ---
  {
    auto d0 = encode_i64(0);
    ALWAYS_ASSERT(d0.size() == 1 && d0[0] == Trit::Zero);

    auto d1 = encode_i64(1);
    // Create c = d1 + encode_i64(-1) -> 0, should normalize to single zero
    auto c = add(d1, encode_i64(-1));
    ALWAYS_ASSERT(c.size() == 1 && c[0] == Trit::Zero);
  }

  // --- randomized-ish sweep (deterministic) ---
  {
    int64_t acc = 0;
    for (int i = -500; i <= 500; ++i) {
      auto ds = encode_i64(i);
      acc += decode_i64(ds);
    }
    // sum_{i=-500}^{500} i = 0
    ALWAYS_ASSERT(acc == 0);
  }

  // --- regression: original defect exact 41-trit premature saturation ---
  {
    // Construct exactly 41 trits: index 0 is Pos, indices 1..40 are Zero
    std::vector<Trit> trits_pos_41(41, Trit::Zero);
    trits_pos_41[0] = Trit::Pos;
    ALWAYS_ASSERT(decode_i64(trits_pos_41) == 1);

    // Corresponding negative-one case: index 0 is Neg, indices 1..40 are Zero
    std::vector<Trit> trits_neg_41(41, Trit::Zero);
    trits_neg_41[0] = Trit::Neg;
    ALWAYS_ASSERT(decode_i64(trits_neg_41) == -1);
  }

  // --- regression: cancellation-aware evaluation ---
  {
    // Positive cancellation vector:
    // Exactly 41 trits: index 40 is Pos (+3^40), indices 0..39 are Neg (-sum_{i=0}^{39} 3^i)
    // Mathematical value = 3^40 - (3^40 - 1)/2 = (3^40 + 1)/2 = 6078832729528464401
    std::vector<Trit> cancel_pos(41, Trit::Neg);
    cancel_pos[40] = Trit::Pos;
    ALWAYS_ASSERT(decode_i64(cancel_pos) == INT64_C(6078832729528464401));

    // Immediate neighbors of positive cancellation vector
    // Add +1 at index 0 (so index 0 is Zero): 6078832729528464402
    auto cancel_pos_plus1 = cancel_pos;
    cancel_pos_plus1[0] = Trit::Zero;
    ALWAYS_ASSERT(decode_i64(cancel_pos_plus1) == INT64_C(6078832729528464402));

    // Negative cancellation vector:
    // Exactly 41 trits: index 40 is Neg (-3^40), indices 0..39 are Pos (+sum_{i=0}^{39} 3^i)
    // Mathematical value = -(3^40 + 1)/2 = -6078832729528464401
    std::vector<Trit> cancel_neg(41, Trit::Pos);
    cancel_neg[40] = Trit::Neg;
    ALWAYS_ASSERT(decode_i64(cancel_neg) == -INT64_C(6078832729528464401));

    // Immediate neighbors of negative cancellation vector
    // Subtract 1 at index 0 (so index 0 is Zero): -6078832729528464402
    auto cancel_neg_minus1 = cancel_neg;
    cancel_neg_minus1[0] = Trit::Zero;
    ALWAYS_ASSERT(decode_i64(cancel_neg_minus1) == -INT64_C(6078832729528464402));
  }

  // --- regression: INT64_MIN and INT64_MAX exact roundtrips ---
  {
    const auto max_trits = encode_i64(INT64_MAX);
    ALWAYS_ASSERT(decode_i64(max_trits) == INT64_MAX);

    const auto min_trits = encode_i64(INT64_MIN);
    ALWAYS_ASSERT(decode_i64(min_trits) == INT64_MIN);

    // Boundary check: INT64_MAX - 1 and INT64_MIN + 1
    ALWAYS_ASSERT(decode_i64(encode_i64(INT64_MAX - 1)) == (INT64_MAX - 1));
    ALWAYS_ASSERT(decode_i64(encode_i64(INT64_MIN + 1)) == (INT64_MIN + 1));
  }

  // --- regression: redundant leading-zero trits (at end of LSB-first vector) ---
  {
    // Small numbers with up to 60 redundant leading zeros
    for (int64_t base_val : {0, 1, -1, 42, -42, 123456789, -123456789}) {
      auto ds = encode_i64(base_val);
      for (size_t zeros = 1; zeros <= 60; ++zeros) {
        ds.push_back(Trit::Zero);
        ALWAYS_ASSERT(decode_i64(ds) == base_val);
      }
    }

    // INT64_MAX and INT64_MIN with redundant leading zeros
    auto max_ds = encode_i64(INT64_MAX);
    for (size_t zeros = 1; zeros <= 10; ++zeros) {
      max_ds.push_back(Trit::Zero);
      ALWAYS_ASSERT(decode_i64(max_ds) == INT64_MAX);
    }

    auto min_ds = encode_i64(INT64_MIN);
    for (size_t zeros = 1; zeros <= 10; ++zeros) {
      min_ds.push_back(Trit::Zero);
      ALWAYS_ASSERT(decode_i64(min_ds) == INT64_MIN);
    }

    // All zero vector with many trits
    std::vector<Trit> many_zeros(50, Trit::Zero);
    ALWAYS_ASSERT(decode_i64(many_zeros) == 0);
  }

  // --- regression: positive and negative overflow saturation ---
  {
    // Positive overflow: values > INT64_MAX clamp to INT64_MAX
    // INT64_MAX is 9223372036854775807.
    // 3^40 = 12157665459056928801 > INT64_MAX. A single trit Pos at index 40:
    std::vector<Trit> pow40_pos(41, Trit::Zero);
    pow40_pos[40] = Trit::Pos;
    ALWAYS_ASSERT(decode_i64(pow40_pos) == INT64_MAX);

    // INT64_MAX + 1: immediately beyond positive signed 64-bit limit
    auto one_trit = encode_i64(1);
    auto max_plus_1 = add(encode_i64(INT64_MAX), one_trit);
    ALWAYS_ASSERT(decode_i64(max_plus_1) == INT64_MAX);

    // Multiple higher powers of 3
    std::vector<Trit> huge_pos(64, Trit::Pos);
    ALWAYS_ASSERT(decode_i64(huge_pos) == INT64_MAX);

    // Negative overflow: values < INT64_MIN clamp to INT64_MIN
    // -3^40 < INT64_MIN. A single trit Neg at index 40:
    std::vector<Trit> pow40_neg(41, Trit::Zero);
    pow40_neg[40] = Trit::Neg;
    ALWAYS_ASSERT(decode_i64(pow40_neg) == INT64_MIN);

    // INT64_MIN - 1: immediately beyond negative signed 64-bit limit
    auto min_minus_1 = add(encode_i64(INT64_MIN), encode_i64(-1));
    ALWAYS_ASSERT(decode_i64(min_minus_1) == INT64_MIN);

    // Multiple higher powers of -3
    std::vector<Trit> huge_neg(64, Trit::Neg);
    ALWAYS_ASSERT(decode_i64(huge_neg) == INT64_MIN);
  }

  std::cout << "ternary_arith ok\n";
  return 0;
}
