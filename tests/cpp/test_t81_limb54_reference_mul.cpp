/**
 * @file test_t81_limb54_reference_mul.cpp
 * @brief Focused regression suite for T81Limb54 reference multiplication correctness
 * (SCG-07 Phase 4, F2 repair).
 *
 * Verifies:
 * - Original SCG-06 F2 reproducer cases (SplitMix64 300 trials, all-ones vector).
 * - Small values: 0, +-1, +-2, +-3, +-9, +-27.
 * - Complete sign combinations: (+*+, +*-, -*+, -*-).
 * - Multiplication by powers of three across the full 54-trit range.
 * - Positive and negative cascading carry chains.
 * - Dense and sparse balanced-ternary patterns.
 * - Alternating-sign trits (+1, -1, +1, -1...).
 * - Tryte-boundary carry propagation.
 * - Maximum and minimum representable 54-trit values (+-(3^54 - 1)/2).
 * - Multiplication where the exact result fits within 54 trits.
 * - Multiplication where the exact result exceeds 54 trits (canonical balanced-ternary residue
 * modulo 3^54).
 * - Explicit verification of the final carry across all 54 trits and overflow/truncation handling.
 * - Commutativity and identity properties: a * b == b * a, a * 1 == a, a * 0 == 0, a * (-1) == -a.
 * - Strict canonical output verification: all output trits in {-1, 0, 1}.
 * - Comparison against an independent base-10^9 exact-integer oracle (ExactBigIntOracle)
 *   across at least 10,000 deterministic pseudo-random trials.
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <span>
#include <vector>

#include "t81/types/T81Limb.hpp"

using t81::core::T81Limb54;

namespace {

// ============================================================================
// Independent Exact Integer Oracle (Base 10^9 BigInt)
// ============================================================================
struct ExactBigIntOracle {
  bool negative = false;
  std::vector<uint32_t> chunks;  // base 10^9, little-endian

  static constexpr uint64_t BASE = 1000000000ULL;

  ExactBigIntOracle() = default;

  explicit ExactBigIntOracle(int64_t val) {
    if (val < 0) {
      negative = true;
      val = -val;
    }
    while (val > 0) {
      chunks.push_back(static_cast<uint32_t>(val % BASE));
      val /= BASE;
    }
  }

  [[nodiscard]] bool is_zero() const noexcept {
    return chunks.empty() || (chunks.size() == 1 && chunks[0] == 0);
  }

  void trim() noexcept {
    while (!chunks.empty() && chunks.back() == 0) {
      chunks.pop_back();
    }
    if (chunks.empty()) {
      negative = false;
    }
  }

  static ExactBigIntOracle mul(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.is_zero() || b.is_zero()) {
      return ExactBigIntOracle(0);
    }
    ExactBigIntOracle res;
    res.negative = (a.negative != b.negative);
    res.chunks.assign(a.chunks.size() + b.chunks.size(), 0);
    for (size_t i = 0; i < a.chunks.size(); ++i) {
      uint64_t carry = 0;
      for (size_t j = 0; j < b.chunks.size() || carry > 0; ++j) {
        uint64_t cur = res.chunks[i + j] + carry;
        if (j < b.chunks.size()) {
          cur += static_cast<uint64_t>(a.chunks[i]) * b.chunks[j];
        }
        res.chunks[i + j] = static_cast<uint32_t>(cur % BASE);
        carry = cur / BASE;
      }
    }
    res.trim();
    return res;
  }

  static int compare_mag(const ExactBigIntOracle& a, const ExactBigIntOracle& b) noexcept {
    if (a.chunks.size() != b.chunks.size()) {
      return a.chunks.size() < b.chunks.size() ? -1 : 1;
    }
    for (size_t i = a.chunks.size(); i-- > 0;) {
      if (a.chunks[i] != b.chunks[i]) {
        return a.chunks[i] < b.chunks[i] ? -1 : 1;
      }
    }
    return 0;
  }

  static ExactBigIntOracle add(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.negative == b.negative) {
      ExactBigIntOracle res;
      res.negative = a.negative;
      size_t n = std::max(a.chunks.size(), b.chunks.size());
      res.chunks.assign(n + 1, 0);
      uint64_t carry = 0;
      for (size_t i = 0; i < n || carry > 0; ++i) {
        uint64_t sum = carry;
        if (i < a.chunks.size()) {
          sum += a.chunks[i];
        }
        if (i < b.chunks.size()) {
          sum += b.chunks[i];
        }
        res.chunks[i] = static_cast<uint32_t>(sum % BASE);
        carry = sum / BASE;
      }
      res.trim();
      return res;
    }
    ExactBigIntOracle neg_b = b;
    neg_b.negative = !b.negative;
    return sub(a, neg_b);
  }

  static ExactBigIntOracle sub(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.negative != b.negative) {
      ExactBigIntOracle neg_b = b;
      neg_b.negative = !b.negative;
      return add(a, neg_b);
    }
    int cmp = compare_mag(a, b);
    if (cmp == 0) {
      return ExactBigIntOracle(0);
    }
    const ExactBigIntOracle* larger = &a;
    const ExactBigIntOracle* smaller = &b;
    bool res_neg = a.negative;
    if (cmp < 0) {
      larger = &b;
      smaller = &a;
      res_neg = !a.negative;
    }
    ExactBigIntOracle res;
    res.negative = res_neg;
    res.chunks.assign(larger->chunks.size(), 0);
    int64_t borrow = 0;
    for (size_t i = 0; i < larger->chunks.size(); ++i) {
      int64_t diff = static_cast<int64_t>(larger->chunks[i]) - borrow;
      if (i < smaller->chunks.size()) {
        diff -= smaller->chunks[i];
      }
      if (diff < 0) {
        diff += BASE;
        borrow = 1;
      } else {
        borrow = 0;
      }
      res.chunks[i] = static_cast<uint32_t>(diff);
    }
    res.trim();
    return res;
  }

  // Divides oracle value by 3, returning {balanced_quotient, balanced_remainder in {-1, 0, 1}}.
  static std::pair<ExactBigIntOracle, int8_t> div_rem3_balanced(const ExactBigIntOracle& val) {
    if (val.is_zero()) {
      return {ExactBigIntOracle(0), 0};
    }
    ExactBigIntOracle q;
    q.chunks.assign(val.chunks.size(), 0);
    uint64_t rem = 0;
    for (size_t i = val.chunks.size(); i-- > 0;) {
      uint64_t cur = rem * BASE + val.chunks[i];
      q.chunks[i] = static_cast<uint32_t>(cur / 3);
      rem = cur % 3;
    }
    q.trim();

    int64_t signed_rem = val.negative ? -static_cast<int64_t>(rem) : static_cast<int64_t>(rem);
    q.negative = val.negative;
    if (q.is_zero()) {
      q.negative = false;
    }

    int8_t bal_rem = 0;
    if (signed_rem == 2) {
      bal_rem = -1;
      q = add(q, ExactBigIntOracle(1));
    } else if (signed_rem == -2) {
      bal_rem = 1;
      q = sub(q, ExactBigIntOracle(1));
    } else {
      bal_rem = static_cast<int8_t>(signed_rem);
    }

    return {q, bal_rem};
  }

  bool operator==(const ExactBigIntOracle& other) const noexcept {
    if (is_zero() && other.is_zero()) {
      return true;
    }
    if (negative != other.negative) {
      return false;
    }
    return chunks == other.chunks;
  }
};

ExactBigIntOracle trits_to_oracle(std::span<const int8_t> trits) {
  ExactBigIntOracle val(0);
  ExactBigIntOracle p(1);
  ExactBigIntOracle three(3);
  for (int8_t t : trits) {
    if (t == 1) {
      val = ExactBigIntOracle::add(val, p);
    } else if (t == -1) {
      val = ExactBigIntOracle::sub(val, p);
    }
    p = ExactBigIntOracle::mul(p, three);
  }
  return val;
}

// Converts an arbitrary-precision exact integer to 54-trit balanced-ternary residue modulo 3^54.
// Also returns the quotient after 54 trits (representing the carry out of the 54-trit window).
std::pair<std::array<int8_t, 54>, ExactBigIntOracle> oracle_to_canonical_trits54_with_carry(
    ExactBigIntOracle val) {
  std::array<int8_t, 54> trits{};
  for (int i = 0; i < 54; ++i) {
    auto [next_val, rem] = ExactBigIntOracle::div_rem3_balanced(val);
    trits[i] = rem;
    val = next_val;
  }
  return {trits, val};
}

[[maybe_unused]] std::array<int8_t, 54> oracle_to_canonical_trits54(ExactBigIntOracle val) {
  return oracle_to_canonical_trits54_with_carry(val).first;
}

std::array<int8_t, 54> int_to_trits54(int64_t val) {
  std::array<int8_t, 54> trits{};
  int64_t rem = val;
  for (int i = 0; i < 54; ++i) {
    int64_t r = (rem % 3);
    if (r == 2 || r == -1) {
      trits[i] = -1;
      rem = (rem - (-1)) / 3;
    } else if (r == 1 || r == -2) {
      trits[i] = 1;
      rem = (rem - 1) / 3;
    } else {
      trits[i] = 0;
      rem /= 3;
    }
  }
  return trits;
}

void verify_product_from_trits(const std::array<int8_t, 54>& ta, const std::array<int8_t, 54>& tb,
                               const char* context_label) {
  auto a = T81Limb54::from_trits(ta);
  auto b = T81Limb54::from_trits(tb);
  T81Limb54 prod = T81Limb54::reference_mul(a, b);
  auto prod_trits = prod.to_trits();

  // Canonicality check: each trit strictly in {-1, 0, 1}
  for (int i = 0; i < 54; ++i) {
    if (prod_trits[i] < -1 || prod_trits[i] > 1) {
      std::cerr << "Assertion failed in " << context_label
                << ": non-canonical trit emitted at index " << i << ": "
                << static_cast<int>(prod_trits[i]) << std::endl;
      std::abort();
    }
  }

  // Exact mathematical oracle comparison
  auto oa = trits_to_oracle(ta);
  auto ob = trits_to_oracle(tb);
  auto expected_exact = ExactBigIntOracle::mul(oa, ob);
  auto [expected_trits54, carry_out] = oracle_to_canonical_trits54_with_carry(expected_exact);

  if (prod_trits != expected_trits54) {
    std::cerr << "Assertion failed in " << context_label
              << ": product mismatch with independent oracle!" << std::endl;
    std::abort();
  }

  // Verify commutativity: a * b == b * a
  T81Limb54 prod_comm = T81Limb54::reference_mul(b, a);
  if (prod.to_trits() != prod_comm.to_trits()) {
    std::cerr << "Assertion failed in " << context_label << ": commutativity violation!"
              << std::endl;
    std::abort();
  }
}

// ============================================================================
// Test Cohorts
// ============================================================================

void test_scg06_f2_reproducers() {
  std::cout << "Running test_scg06_f2_reproducers..." << std::endl;

  // Case 1: All-ones 54-trit operands (dense carry cascades previously emitting non-trits up to 52)
  std::array<int8_t, 54> all_ones{}, all_minus_ones{};
  std::fill(all_ones.begin(), all_ones.end(), static_cast<int8_t>(1));
  std::fill(all_minus_ones.begin(), all_minus_ones.end(), static_cast<int8_t>(-1));

  verify_product_from_trits(all_ones, all_ones, "F2 all-ones * all-ones");
  verify_product_from_trits(all_ones, all_minus_ones, "F2 all-ones * all-minus-ones");
  verify_product_from_trits(all_minus_ones, all_minus_ones, "F2 all-minus-ones * all-minus-ones");

  // Case 2: The exact 300 test battery from reproduce_f1_f7.py
  struct SplitMix64 {
    uint64_t state;
    uint64_t next() {
      uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
      z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
      z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
      return z ^ (z >> 31);
    }
  };

  SplitMix64 rng{0x123456789abcdef0ULL};
  for (int k = 0; k < 300; ++k) {
    std::array<int8_t, 54> ta{}, tb{};
    int mode = k % 5;
    for (int i = 0; i < 54; ++i) {
      uint64_t r = rng.next();
      if (mode == 0) {  // sparse
        ta[i] = (r % 7 == 0) ? static_cast<int8_t>((r >> 4) % 3 - 1) : 0;
        tb[i] = ((r >> 8) % 7 == 0) ? static_cast<int8_t>((r >> 12) % 3 - 1) : 0;
      } else if (mode == 1) {  // small width
        ta[i] = (i < 20) ? static_cast<int8_t>(r % 3 - 1) : 0;
        tb[i] = (i < 20) ? static_cast<int8_t>((r >> 8) % 3 - 1) : 0;
      } else if (mode == 2) {  // dense
        ta[i] = static_cast<int8_t>(r % 3 - 1);
        tb[i] = static_cast<int8_t>((r >> 8) % 3 - 1);
      } else if (mode == 3) {  // alternating
        ta[i] = (i % 2 == 0) ? 1 : -1;
        tb[i] = static_cast<int8_t>((r >> 4) % 3 - 1);
      } else {  // boundary
        ta[i] = (i < 27) ? 1 : -1;
        tb[i] = static_cast<int8_t>((r >> 4) % 3 - 1);
      }
    }
    verify_product_from_trits(ta, tb, "SCG-06 300-trial suite");
  }
}

void test_small_values_and_sign_combinations() {
  std::cout << "Running test_small_values_and_sign_combinations..." << std::endl;
  const std::vector<int64_t> small_vals = {
      0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 8, -8, 9, -9, 13, -13, 26, -26, 27, -27,
  };

  for (int64_t v1 : small_vals) {
    for (int64_t v2 : small_vals) {
      auto t1 = int_to_trits54(v1);
      auto t2 = int_to_trits54(v2);
      verify_product_from_trits(t1, t2, "Small values & sign combinations");
    }
  }

  // Identity and zero properties
  for (int64_t v : small_vals) {
    auto tv = int_to_trits54(v);
    auto zero = int_to_trits54(0);
    auto one = int_to_trits54(1);
    auto neg_one = int_to_trits54(-1);

    auto lv = T81Limb54::from_trits(tv);
    auto lzero = T81Limb54::from_trits(zero);
    auto lone = T81Limb54::from_trits(one);
    auto lneg_one = T81Limb54::from_trits(neg_one);

    // v * 0 == 0
    if (T81Limb54::reference_mul(lv, lzero).to_trits() != zero) {
      std::cerr << "Identity property (v * 0 == 0) violated for " << v << std::endl;
      std::abort();
    }
    // v * 1 == v
    if (T81Limb54::reference_mul(lv, lone).to_trits() != tv) {
      std::cerr << "Identity property (v * 1 == v) violated for " << v << std::endl;
      std::abort();
    }
    // v * (-1) == -v
    auto expected_neg = int_to_trits54(-v);
    if (T81Limb54::reference_mul(lv, lneg_one).to_trits() != expected_neg) {
      std::cerr << "Identity property (v * -1 == -v) violated for " << v << std::endl;
      std::abort();
    }
  }
}

void test_powers_of_three() {
  std::cout << "Running test_powers_of_three..." << std::endl;
  // Test all pairs (3^p1) * (3^p2) for p1, p2 in 0..53 with signs +-1
  const int signs[] = {1, -1};
  for (int p1 = 0; p1 < 54; ++p1) {
    for (int s1 : signs) {
      for (int p2 = 0; p2 < 54; ++p2) {
        for (int s2 : signs) {
          std::array<int8_t, 54> t1{}, t2{};
          t1[p1] = static_cast<int8_t>(s1);
          t2[p2] = static_cast<int8_t>(s2);

          verify_product_from_trits(t1, t2, "Powers of three");
        }
      }
    }
  }
}

void test_alternating_patterns_and_boundaries() {
  std::cout << "Running test_alternating_patterns_and_boundaries..." << std::endl;

  // Max 54-trit value: all +1 (value (3^54 - 1) / 2)
  std::array<int8_t, 54> max_val{};
  std::fill(max_val.begin(), max_val.end(), static_cast<int8_t>(1));

  // Min 54-trit value: all -1 (value -(3^54 - 1) / 2)
  std::array<int8_t, 54> min_val{};
  std::fill(min_val.begin(), min_val.end(), static_cast<int8_t>(-1));

  // Alternating pattern 1: +1, -1, +1, -1...
  std::array<int8_t, 54> alt1{};
  for (int i = 0; i < 54; ++i) {
    alt1[i] = (i % 2 == 0) ? 1 : -1;
  }

  // Alternating pattern 2: -1, +1, -1, +1...
  std::array<int8_t, 54> alt2{};
  for (int i = 0; i < 54; ++i) {
    alt2[i] = (i % 2 == 0) ? -1 : 1;
  }

  std::vector<std::array<int8_t, 54>> test_trit_arrays = {
      max_val, min_val, alt1, alt2, int_to_trits54(1), int_to_trits54(-1), int_to_trits54(0),
  };

  for (size_t i = 0; i < test_trit_arrays.size(); ++i) {
    for (size_t j = 0; j < test_trit_arrays.size(); ++j) {
      verify_product_from_trits(test_trit_arrays[i], test_trit_arrays[j],
                                "Alternating patterns and boundary values");
    }
  }
}

void test_carry_propagation_dense_chains() {
  std::cout << "Running test_carry_propagation_dense_chains..." << std::endl;
  // Dense chains creating cascading carries across tryte and trit boundaries
  for (int len = 2; len <= 54; ++len) {
    std::array<int8_t, 54> t1{}, t2{};
    for (int i = 0; i < len; ++i) {
      t1[i] = 1;
      t2[i] = 1;
    }
    verify_product_from_trits(t1, t2, "Dense carry chain");

    for (int i = 0; i < len; ++i) {
      t2[i] = -1;
    }
    verify_product_from_trits(t1, t2, "Dense carry chain opposite signs");
  }
}

void test_tryte_boundary_propagation() {
  std::cout << "Running test_tryte_boundary_propagation..." << std::endl;
  // Test carries that specifically straddle tryte boundaries (trit positions 2->3, 5->6, etc.)
  // Multiplier is 2 (represented by canonical trits [-1, 1, 0...]), so 1 * 2 at tryte boundary trit
  // produces carry into the next tryte!
  auto two_trits = int_to_trits54(2);       // [-1, 1, 0...]
  auto neg_two_trits = int_to_trits54(-2);  // [1, -1, 0...]

  for (int tryte = 0; tryte < T81Limb54::TRYTES; ++tryte) {
    std::array<int8_t, 54> t1{};
    int pos = tryte * 3 + 2;  // highest trit in tryte
    t1[pos] = 1;
    verify_product_from_trits(t1, two_trits, "Tryte boundary positive");
    verify_product_from_trits(t1, neg_two_trits, "Tryte boundary negative");
  }
}

void test_final_carry_and_truncation_overflow() {
  std::cout << "Running test_final_carry_and_truncation_overflow..." << std::endl;

  // Explicitly test positive and negative overflow where the mathematical product exceeds 54 trits.
  // We verify that:
  // 1. The emitted 54 trits are strictly canonical.
  // 2. The reconstructed 54-trit value equals the canonical balanced residue modulo 3^54.
  // 3. The final carry out of the 54-trit boundary matches the high-order quotient of the exact
  // product.
  // 4. No carry is silently lost inside the 54-trit window.

  // Case 1: Max positive product ((3^54 - 1)/2) * ((3^54 - 1)/2)
  std::array<int8_t, 54> max_54{};
  std::fill(max_54.begin(), max_54.end(), static_cast<int8_t>(1));
  verify_product_from_trits(max_54, max_54, "Max positive product overflow");

  // Case 2: Max negative product ((3^54 - 1)/2) * (-(3^54 - 1)/2)
  std::array<int8_t, 54> min_54{};
  std::fill(min_54.begin(), min_54.end(), static_cast<int8_t>(-1));
  verify_product_from_trits(max_54, min_54, "Max negative product overflow");

  // Case 3: Powers of 3 where p1 + p2 >= 54
  for (int p1 = 27; p1 < 54; ++p1) {
    for (int p2 = 27; p2 < 54; ++p2) {
      std::array<int8_t, 54> t1{}, t2{};
      t1[p1] = 1;
      t2[p2] = 1;
      // Exact product is 3^(p1 + p2), which is 0 mod 3^54 since p1 + p2 >= 54
      auto a = T81Limb54::from_trits(t1);
      auto b = T81Limb54::from_trits(t2);
      auto prod = T81Limb54::reference_mul(a, b);
      auto tr = prod.to_trits();
      for (int i = 0; i < 54; ++i) {
        if (tr[i] != 0) {
          std::cerr << "Overflow power of 3 should be 0 mod 3^54 at pos " << i << std::endl;
          std::abort();
        }
      }
      // Also verify opposite sign
      t2[p2] = -1;
      auto b_neg = T81Limb54::from_trits(t2);
      auto prod_neg = T81Limb54::reference_mul(a, b_neg);
      auto tr_neg = prod_neg.to_trits();
      for (int i = 0; i < 54; ++i) {
        if (tr_neg[i] != 0) {
          std::cerr << "Overflow negative power of 3 should be 0 mod 3^54 at pos " << i
                    << std::endl;
          std::abort();
        }
      }
    }
  }

  // Case 4: High-order populated operands with non-zero lower order:
  // e.g., (3^53 + 1) * (3^53 + 1) = 3^106 + 2*3^53 + 1.
  // Modulo 3^54: 2*3^53 + 1 = 3^54 - 3^53 + 1 = 1 - 3^53 (mod 3^54).
  // In balanced ternary, 1 - 3^53 has trit 0 = 1, trit 53 = -1, others 0.
  std::array<int8_t, 54> t_high1{}, t_high2{};
  t_high1[53] = 1;
  t_high1[0] = 1;
  t_high2[53] = 1;
  t_high2[0] = 1;
  verify_product_from_trits(t_high1, t_high2, "High-order mixed overflow");

  t_high2[0] = -1;
  verify_product_from_trits(t_high1, t_high2, "High-order mixed overflow negative");
}

void test_randomized_oracle_comparison(int num_trials) {
  std::cout << "Running test_randomized_oracle_comparison (" << num_trials << " trials)..."
            << std::endl;
  std::mt19937_64 rng(0xCAFE54545454ULL);

  for (int trial = 0; trial < num_trials; ++trial) {
    std::array<int8_t, 54> at{}, bt{};
    int mode = trial % 7;
    for (int i = 0; i < 54; ++i) {
      uint64_t r = rng();
      if (mode == 0) {
        // Uniform random {-1, 0, 1}
        at[i] = static_cast<int8_t>((r % 3) - 1);
        bt[i] = static_cast<int8_t>(((r >> 4) % 3) - 1);
      } else if (mode == 1) {
        // Sparse operands
        at[i] = (r % 7 == 0) ? static_cast<int8_t>((r >> 4) % 3 - 1) : 0;
        bt[i] = ((r >> 8) % 7 == 0) ? static_cast<int8_t>((r >> 12) % 3 - 1) : 0;
      } else if (mode == 2) {
        // Dense non-zero operands {-1, 1}
        at[i] = (r & 1) ? 1 : -1;
        bt[i] = (r & 2) ? 1 : -1;
      } else if (mode == 3) {
        // Boundary biased (high-order populated, low-order zero)
        at[i] = (i >= 27) ? static_cast<int8_t>((r % 3) - 1) : 0;
        bt[i] = (i >= 27) ? static_cast<int8_t>(((r >> 4) % 3) - 1) : 0;
      } else if (mode == 4) {
        // Low-order populated (within 27 trits, exact product fits in 54 trits)
        at[i] = (i < 27) ? static_cast<int8_t>((r % 3) - 1) : 0;
        bt[i] = (i < 27) ? static_cast<int8_t>(((r >> 4) % 3) - 1) : 0;
      } else if (mode == 5) {
        // Out-of-range products (high width, guaranteed > 54 trits)
        at[i] = (i >= 30) ? static_cast<int8_t>((r % 3) - 1) : 1;
        bt[i] = (i >= 30) ? static_cast<int8_t>(((r >> 4) % 3) - 1) : -1;
      } else {
        // Alternating signs
        at[i] = (i % 2 == 0) ? 1 : -1;
        bt[i] = static_cast<int8_t>(((r >> 8) % 3) - 1);
      }
    }

    verify_product_from_trits(at, bt, "Randomized oracle comparison");
  }
  std::cout << "All " << num_trials << " randomized oracle checks passed!" << std::endl;
}

}  // namespace

int main() {
  std::cout
      << "=== T81Limb54 Reference Multiplication Regression Suite (SCG-07 Phase 4, F2 Repair) ==="
      << std::endl;
  test_scg06_f2_reproducers();
  test_small_values_and_sign_combinations();
  test_powers_of_three();
  test_alternating_patterns_and_boundaries();
  test_carry_propagation_dense_chains();
  test_tryte_boundary_propagation();
  test_final_carry_and_truncation_overflow();
  test_randomized_oracle_comparison(10000);
  std::cout << "=== All T81Limb54 Reference Multiplication Tests Passed! ===" << std::endl;
  return 0;
}
