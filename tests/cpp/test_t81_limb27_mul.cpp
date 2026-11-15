/**
 * @file test_t81_limb27_mul.cpp
 * @brief Focused regression suite for T81Limb27 multiplication correctness (SCG-07 Phase 3, F1
 * repair).
 *
 * Verifies:
 * - Original F1 reproducer: 1 * 2 = 2.
 * - Small values: 0, +-1, +-2, +-3, +-9, +-27.
 * - Complete sign combinations: (+*+, +*-, -*+, -*-).
 * - Multiplication by powers of three.
 * - Boundary values near 27-trit representable limits.
 * - Alternating balanced-ternary digit patterns (+1, -1, +1, -1...).
 * - Carry propagation and partial-product alignment across digit positions.
 * - Commutativity: a * b == b * a.
 * - Identity and zero properties: a * 0 == 0, a * 1 == a, a * (-1) == -a.
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

using t81::core::T81Limb;
using t81::core::T81Limb27;
using t81::core::T81Limb54;

namespace {

// ============================================================================
// Independent Exact Integer Oracle (Base 10^9 BigInt)
// ============================================================================
struct ExactBigIntOracle {
  bool negative = false;
  std::vector<uint32_t> chunks;  // base 10^9

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

T81Limb27 make_limb27_from_trits(const std::array<int8_t, 27>& trits) {
  std::array<int8_t, T81Limb::TRITS> trits48{};
  std::copy(trits.begin(), trits.end(), trits48.begin());
  return T81Limb27::from_trits_window(trits48, 0);
}

std::array<int8_t, 27> int_to_trits27(int64_t val) {
  std::array<int8_t, 27> trits{};
  int64_t rem = val;
  for (int i = 0; i < 27; ++i) {
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

void verify_product_from_trits(const std::array<int8_t, 27>& ta, const std::array<int8_t, 27>& tb,
                               const char* context_label) {
  auto a = make_limb27_from_trits(ta);
  auto b = make_limb27_from_trits(tb);
  T81Limb54 prod = a * b;

  auto oa = trits_to_oracle(ta);
  auto ob = trits_to_oracle(tb);
  auto expected = ExactBigIntOracle::mul(oa, ob);
  auto observed = trits_to_oracle(prod.to_trits());

  if (!(expected == observed)) {
    std::cerr << "Assertion failed in " << context_label << ": product mismatch with oracle!"
              << std::endl;
    std::abort();
  }

  // Verify commutativity: a * b == b * a
  T81Limb54 prod_comm = b * a;
  if (prod.to_trits() != prod_comm.to_trits()) {
    std::cerr << "Assertion failed in " << context_label << ": commutativity violation!"
              << std::endl;
    std::abort();
  }
}

// ============================================================================
// Test Cohorts
// ============================================================================

void test_original_f1_reproducer() {
  std::cout << "Running test_original_f1_reproducer..." << std::endl;
  T81Limb54 la{}, lb{};
  la.set_tryte(0, 1);  // a = 1
  lb.set_tryte(0, 2);  // b = 2 (in balanced ternary [-1, 1, 0])
  auto a = T81Limb27::from_low_27(la);
  auto b = T81Limb27::from_low_27(lb);

  auto prod = a * b;
  auto trits = prod.to_trits();

  // 1 * 2 = 2, which in balanced ternary is 2 = 3^1 - 1 -> trits: [-1, 1, 0, 0, ...]
  if (trits[0] != -1 || trits[1] != 1) {
    std::cerr << "F1 reproducer failed!" << std::endl;
    std::abort();
  }
  for (int i = 2; i < T81Limb54::TRITS; ++i) {
    if (trits[i] != 0) {
      std::cerr << "F1 reproducer failed at trit " << i << std::endl;
      std::abort();
    }
  }

  std::array<int8_t, 27> ta{}, tb{};
  ta[0] = 1;
  tb[0] = -1;
  tb[1] = 1;
  verify_product_from_trits(ta, tb, "F1 Reproducer (1 * 2)");
}

void test_small_values_and_sign_combinations() {
  std::cout << "Running test_small_values_and_sign_combinations..." << std::endl;
  const std::vector<int64_t> values = {0, 1, -1, 2, -2, 3, -3, 9, -9, 27, -27};
  for (int64_t va : values) {
    for (int64_t vb : values) {
      auto ta = int_to_trits27(va);
      auto tb = int_to_trits27(vb);
      verify_product_from_trits(ta, tb, "Small values / sign combinations");
    }
  }
}

void test_powers_of_three() {
  std::cout << "Running test_powers_of_three..." << std::endl;
  for (int p1 = 0; p1 < 13; ++p1) {
    for (int p2 = 0; p2 < 13; ++p2) {
      for (int s1 : {1, -1}) {
        for (int s2 : {1, -1}) {
          std::array<int8_t, 27> t1{}, t2{};
          t1[p1] = static_cast<int8_t>(s1);
          t2[p2] = static_cast<int8_t>(s2);
          auto a = make_limb27_from_trits(t1);
          auto b = make_limb27_from_trits(t2);

          auto prod = a * b;
          auto trits = prod.to_trits();

          int expected_pos = p1 + p2;
          int expected_sign = s1 * s2;
          for (int i = 0; i < T81Limb54::TRITS; ++i) {
            if (i == expected_pos) {
              if (trits[i] != expected_sign) {
                std::cerr << "Powers of three mismatch at " << i << std::endl;
                std::abort();
              }
            } else {
              if (trits[i] != 0) {
                std::cerr << "Powers of three non-zero at " << i << std::endl;
                std::abort();
              }
            }
          }
          verify_product_from_trits(t1, t2, "Powers of three");
        }
      }
    }
  }
}

void test_alternating_patterns_and_boundaries() {
  std::cout << "Running test_alternating_patterns_and_boundaries..." << std::endl;

  // Max 27-trit value: all +1 (value (3^27 - 1) / 2)
  std::array<int8_t, 27> max_val{};
  std::fill(max_val.begin(), max_val.end(), static_cast<int8_t>(1));

  // Min 27-trit value: all -1 (value -(3^27 - 1) / 2)
  std::array<int8_t, 27> min_val{};
  std::fill(min_val.begin(), min_val.end(), static_cast<int8_t>(-1));

  // Alternating pattern 1: +1, -1, +1, -1...
  std::array<int8_t, 27> alt1{};
  for (int i = 0; i < 27; ++i) {
    alt1[i] = (i % 2 == 0) ? 1 : -1;
  }

  // Alternating pattern 2: -1, +1, -1, +1...
  std::array<int8_t, 27> alt2{};
  for (int i = 0; i < 27; ++i) {
    alt2[i] = (i % 2 == 0) ? -1 : 1;
  }

  std::vector<std::array<int8_t, 27>> test_trit_arrays = {
      max_val, min_val, alt1, alt2, int_to_trits27(1), int_to_trits27(-1), int_to_trits27(0),
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
  for (int len = 2; len <= 27; ++len) {
    std::array<int8_t, 27> t1{}, t2{};
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

void test_randomized_oracle_comparison(int num_trials) {
  std::cout << "Running test_randomized_oracle_comparison (" << num_trials << " trials)..."
            << std::endl;
  std::mt19937_64 rng(0xCAFE123456789ULL);

  for (int trial = 0; trial < num_trials; ++trial) {
    std::array<int8_t, 27> at{}, bt{};
    int mode = trial % 6;
    for (int i = 0; i < 27; ++i) {
      uint64_t r = rng();
      if (mode == 0) {
        // Uniform random {-1, 0, 1}
        at[i] = static_cast<int8_t>((r % 3) - 1);
        bt[i] = static_cast<int8_t>(((r >> 4) % 3) - 1);
      } else if (mode == 1) {
        // Sparse operands
        at[i] = (r % 5 == 0) ? static_cast<int8_t>((r >> 4) % 3 - 1) : 0;
        bt[i] = ((r >> 8) % 5 == 0) ? static_cast<int8_t>((r >> 12) % 3 - 1) : 0;
      } else if (mode == 2) {
        // Dense non-zero operands {-1, 1}
        at[i] = (r & 1) ? 1 : -1;
        bt[i] = (r & 2) ? 1 : -1;
      } else if (mode == 3) {
        // Boundary biased (high-order populated, low-order zero)
        at[i] = (i >= 15) ? static_cast<int8_t>((r % 3) - 1) : 0;
        bt[i] = (i >= 15) ? static_cast<int8_t>(((r >> 4) % 3) - 1) : 0;
      } else if (mode == 4) {
        // Low-order populated, high-order zero
        at[i] = (i < 12) ? static_cast<int8_t>((r % 3) - 1) : 0;
        bt[i] = (i < 12) ? static_cast<int8_t>(((r >> 4) % 3) - 1) : 0;
      } else {
        // Mixed tries (random trytes in -13..13)
        at[i] = static_cast<int8_t>((r % 3) - 1);
        bt[i] = static_cast<int8_t>(((r >> 8) % 3) - 1);
      }
    }

    verify_product_from_trits(at, bt, "Randomized oracle comparison");
  }
  std::cout << "All " << num_trials << " randomized oracle checks passed!" << std::endl;
}

}  // namespace

int main() {
  std::cout << "=== T81Limb27 Multiplication Regression Suite (SCG-07 Phase 3, F1 Repair) ==="
            << std::endl;
  test_original_f1_reproducer();
  test_small_values_and_sign_combinations();
  test_powers_of_three();
  test_alternating_patterns_and_boundaries();
  test_carry_propagation_dense_chains();
  test_randomized_oracle_comparison(10000);
  std::cout << "=== All T81Limb27 Multiplication Tests Passed! ===" << std::endl;
  return 0;
}
