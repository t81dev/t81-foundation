#undef NDEBUG
#include "t81/types/T81Fraction.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace t81::v1;
using t81::T81Int;

// Simple test assertion helper
inline void require(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << std::endl;
    std::abort();
  }
}

// -----------------------------------------------------------------------------
// 1. F6 Regression: Exception Safety and Catchability in Overflow
// -----------------------------------------------------------------------------
void test_f6_multiplication_overflow_catchable() {
  std::cout << "Running test_f6_multiplication_overflow_catchable..." << std::endl;

  // Pre-repair F6: 3^45 * 3^45 = 3^90 overflows T81Fraction<81> (which holds at most 81 trits).
  // Under pre-repair, T81Fraction::operator* was marked noexcept, so throwing T81Int
  // caused immediate std::terminate().
  // With repair, std::overflow_error is caught gracefully without std::terminate().
  T81Int<81> num_a(1);
  for (int i = 0; i < 45; ++i) {
    num_a = num_a * T81Int<81>(3);
  }

  T81Fraction<81> fa(num_a, T81Int<81>(1));
  T81Fraction<81> fb(num_a, T81Int<81>(1));

  bool caught = false;
  try {
    [[maybe_unused]] T81Fraction<81> fc = fa * fb;
  } catch (const std::overflow_error&) {
    caught = true;
  }
  require(caught, "F6: expected std::overflow_error was not caught on multiplication");

  // Normal multiplication within capacity (3^20 * 3^20 = 3^40 <= 81 trits) must succeed.
  T81Int<81> num_small(1);
  for (int i = 0; i < 20; ++i) {
    num_small = num_small * T81Int<81>(3);
  }
  T81Fraction<81> f_s1(num_small, T81Int<81>(1));
  T81Fraction<81> f_s2(num_small, T81Int<81>(1));
  T81Fraction<81> f_prod = f_s1 * f_s2;
  require(!f_prod.num().is_zero(), "F6: valid product must be non-zero");
  require(f_prod.den() == T81Int<81>(1), "F6: denominator should remain 1");
}

void test_f6_addition_subtraction_overflow() {
  std::cout << "Running test_f6_addition_subtraction_overflow..." << std::endl;

  // Addition overflow: (3^80 / 1) + (3^80 / 1) > (3^81 - 1) / 2
  T81Int<81> big(1);
  for (int i = 0; i < 80; ++i) {
    big = big * T81Int<81>(3);
  }
  T81Fraction<81> f_big(big, T81Int<81>(1));
  bool caught_add_overflow = false;
  try {
    [[maybe_unused]] T81Fraction<81> f_sum = f_big + f_big;
  } catch (const std::overflow_error&) {
    caught_add_overflow = true;
  }
  require(caught_add_overflow, "Expected std::overflow_error on fraction addition");

  // Subtraction overflow: -(3^80 / 1) - (3^80 / 1) < -(3^81 - 1) / 2
  bool caught_sub_overflow = false;
  try {
    [[maybe_unused]] T81Fraction<81> f_diff = (-f_big) - f_big;
  } catch (const std::overflow_error&) {
    caught_sub_overflow = true;
  }
  require(caught_sub_overflow, "Expected std::overflow_error on fraction subtraction");
}

// -----------------------------------------------------------------------------
// 2. F7 Regression: Independent Exact-Integer Rational Oracle & to_double() Validation
// -----------------------------------------------------------------------------

// Independent exact arbitrary-precision integer arithmetic oracle
// Base-10^9 limb vector, entirely decoupled from T81 internal C++ types and floating-point
// accumulation.
struct ExactBigIntOracle {
  static constexpr std::uint64_t BASE = 1000000000ULL;  // 10^9
  bool negative = false;
  std::vector<std::uint32_t> limbs;  // little-endian limbs base 10^9

  ExactBigIntOracle() = default;
  explicit ExactBigIntOracle(std::int64_t v) {
    if (v < 0) {
      negative = true;
      v = -v;
    }
    if (v == 0) return;
    while (v > 0) {
      limbs.push_back(static_cast<std::uint32_t>(v % BASE));
      v /= BASE;
    }
  }

  bool is_zero() const { return limbs.empty(); }

  void trim() {
    while (!limbs.empty() && limbs.back() == 0) {
      limbs.pop_back();
    }
    if (limbs.empty()) negative = false;
  }

  static ExactBigIntOracle mul_uint(const ExactBigIntOracle& a, std::uint32_t m) {
    if (a.is_zero() || m == 0) return ExactBigIntOracle();
    ExactBigIntOracle res;
    res.negative = a.negative;
    std::uint64_t carry = 0;
    for (std::uint32_t limb : a.limbs) {
      std::uint64_t cur = static_cast<std::uint64_t>(limb) * m + carry;
      res.limbs.push_back(static_cast<std::uint32_t>(cur % BASE));
      carry = cur / BASE;
    }
    while (carry > 0) {
      res.limbs.push_back(static_cast<std::uint32_t>(carry % BASE));
      carry /= BASE;
    }
    res.trim();
    return res;
  }

  static int cmp_mag(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.limbs.size() != b.limbs.size()) {
      return a.limbs.size() < b.limbs.size() ? -1 : 1;
    }
    for (std::size_t i = a.limbs.size(); i-- > 0;) {
      if (a.limbs[i] != b.limbs[i]) {
        return a.limbs[i] < b.limbs[i] ? -1 : 1;
      }
    }
    return 0;
  }

  static ExactBigIntOracle add(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.negative == b.negative) {
      ExactBigIntOracle res;
      res.negative = a.negative;
      std::uint64_t carry = 0;
      std::size_t n = std::max(a.limbs.size(), b.limbs.size());
      for (std::size_t i = 0; i < n || carry; ++i) {
        std::uint64_t sum = carry;
        if (i < a.limbs.size()) sum += a.limbs[i];
        if (i < b.limbs.size()) sum += b.limbs[i];
        res.limbs.push_back(static_cast<std::uint32_t>(sum % BASE));
        carry = sum / BASE;
      }
      res.trim();
      return res;
    }

    int cmp = cmp_mag(a, b);
    if (cmp == 0) return ExactBigIntOracle();
    const ExactBigIntOracle& larger = (cmp > 0) ? a : b;
    const ExactBigIntOracle& smaller = (cmp > 0) ? b : a;

    ExactBigIntOracle res;
    res.negative = larger.negative;
    std::int64_t borrow = 0;
    for (std::size_t i = 0; i < larger.limbs.size(); ++i) {
      std::int64_t diff = static_cast<std::int64_t>(larger.limbs[i]) - borrow;
      if (i < smaller.limbs.size()) diff -= smaller.limbs[i];
      if (diff < 0) {
        diff += BASE;
        borrow = 1;
      } else {
        borrow = 0;
      }
      res.limbs.push_back(static_cast<std::uint32_t>(diff));
    }
    res.trim();
    return res;
  }

  // Convert ExactBigIntOracle to binary64 double independently via base-10^9 decomposition
  double to_double() const {
    if (is_zero()) return 0.0;
    double d = 0.0;
    double factor = 1.0;
    for (std::uint32_t limb : limbs) {
      d += static_cast<double>(limb) * factor;
      factor *= 1e9;
    }
    return negative ? -d : d;
  }
};

template <std::size_t N>
ExactBigIntOracle exact_bigint_from_t81int(const T81Int<N>& val) {
  // Exact Horner evaluation from MSB
  ExactBigIntOracle acc;
  for (std::size_t i = N; i-- > 0;) {
    acc = ExactBigIntOracle::mul_uint(acc, 3);
    int t = t81::trit_to_int(val[i]);
    if (t != 0) {
      acc = ExactBigIntOracle::add(acc, ExactBigIntOracle(t));
    }
  }
  return acc;
}

template <std::size_t N>
double compute_exact_rational_oracle(const T81Fraction<N>& frac) {
  ExactBigIntOracle n = exact_bigint_from_t81int(frac.num());
  ExactBigIntOracle d = exact_bigint_from_t81int(frac.den());
  return n.to_double() / d.to_double();
}

template <std::size_t N>
void verify_to_double_case(const T81Fraction<N>& frac, double expected_val, double tol = 1e-12) {
  double actual = frac.to_double();
  if (std::abs(expected_val) < 1e-15) {
    require(std::abs(actual) < 1e-15, "to_double near zero mismatch");
  } else {
    double rel_err = std::abs(actual - expected_val) / std::abs(expected_val);
    require(rel_err <= tol, "to_double relative error exceeded tolerance");
  }

  // Also verify against independent exact-integer rational oracle
  double oracle_val = compute_exact_rational_oracle(frac);
  if (std::abs(oracle_val) < 1e-15) {
    require(std::abs(actual) < 1e-15, "oracle near zero mismatch");
  } else {
    double oracle_rel_err = std::abs(actual - oracle_val) / std::abs(oracle_val);
    require(oracle_rel_err <= tol, "oracle relative error exceeded tolerance");
  }
}

void test_f7_to_double_boundary_widths() {
  std::cout << "Running test_f7_to_double_boundary_widths..." << std::endl;

  // Test Width 13 (minimum supported width)
  {
    T81Fraction<13> f13_zero;
    verify_to_double_case(f13_zero, 0.0);

    T81Fraction<13> f13_pos(T81Int<13>(27), T81Int<13>(2));
    verify_to_double_case(f13_pos, 13.5);

    T81Fraction<13> f13_neg(T81Int<13>(-27), T81Int<13>(2));
    verify_to_double_case(f13_neg, -13.5);
  }

  // Test Width 40 (maximum width that fits in to_int64())
  {
    T81Int<40> p39(1);
    for (int i = 0; i < 39; ++i) p39 = p39 * T81Int<40>(3);
    T81Fraction<40> f40(p39, T81Int<40>(1));
    double ref = std::pow(3.0, 39.0);
    verify_to_double_case(f40, ref);
  }

  // Test Width 41 (exceeds to_int64() max: would throw in pre-repair)
  {
    T81Int<41> p40(1);
    for (int i = 0; i < 40; ++i) p40 = p40 * T81Int<41>(3);
    T81Fraction<41> f41(p40, T81Int<41>(1));
    double ref = std::pow(3.0, 40.0);
    verify_to_double_case(f41, ref);

    // Negative counterpart
    T81Fraction<41> f41_neg(-p40, T81Int<41>(1));
    verify_to_double_case(f41_neg, -ref);
  }

  // Test Width 81 (canonical standard width, > 40 trits)
  {
    // Pre-repair F7 failure case: 3^45
    T81Int<81> p45(1);
    for (int i = 0; i < 45; ++i) p45 = p45 * T81Int<81>(3);
    T81Fraction<81> f81_45(p45, T81Int<81>(1));
    double ref45 = std::pow(3.0, 45.0);
    verify_to_double_case(f81_45, ref45);

    // Large power up to 3^80
    T81Int<81> p80(1);
    for (int i = 0; i < 80; ++i) p80 = p80 * T81Int<81>(3);
    T81Fraction<81> f81_80(p80, T81Int<81>(1));
    double ref80 = std::pow(3.0, 80.0);
    verify_to_double_case(f81_80, ref80);

    // True fractional value: 1 / 3^45
    T81Fraction<81> f81_inv(T81Int<81>(1), p45);
    double ref_inv = 1.0 / ref45;
    verify_to_double_case(f81_inv, ref_inv);

    // Ratio of large numbers: 3^80 / 3^45 = 3^35
    T81Fraction<81> f81_ratio(p80, p45);
    double ref_ratio = std::pow(3.0, 35.0);
    verify_to_double_case(f81_ratio, ref_ratio);

    // Multi-digit mixed trits (positive and negative trits across wide representation):
    // 3^80 - 3^45 + 1
    T81Int<81> mixed = p80 - p45 + T81Int<81>(1);
    T81Fraction<81> f81_mixed(mixed, T81Int<81>(1));
    double ref_mixed = ref80 - ref45 + 1.0;
    verify_to_double_case(f81_mixed, ref_mixed);

    // Non-trivial ratio with mixed numerator and denominator:
    // (3^70 - 1) / (3^35 + 2)
    T81Int<81> p70(1);
    for (int i = 0; i < 70; ++i) p70 = p70 * T81Int<81>(3);
    T81Int<81> p35(1);
    for (int i = 0; i < 35; ++i) p35 = p35 * T81Int<81>(3);

    T81Int<81> num_nontrivial = p70 - T81Int<81>(1);
    T81Int<81> den_nontrivial = p35 + T81Int<81>(2);
    T81Fraction<81> f81_nontrivial(num_nontrivial, den_nontrivial);
    double analytical_ref = (std::pow(3.0, 70.0) - 1.0) / (std::pow(3.0, 35.0) + 2.0);
    verify_to_double_case(f81_nontrivial, analytical_ref, 1e-12);
  }

  // Test Width 324 (maximum supported width)
  {
    // Near maximum supported width powers: 3^300 / 3^100
    T81Int<324> p300(1);
    for (int i = 0; i < 300; ++i) p300 = p300 * T81Int<324>(3);
    T81Int<324> p100(1);
    for (int i = 0; i < 100; ++i) p100 = p100 * T81Int<324>(3);

    // Single large power (fits in IEEE double ~10^143 < 10^308)
    T81Int<324> p150(1);
    for (int i = 0; i < 150; ++i) p150 = p150 * T81Int<324>(3);
    T81Fraction<324> f324(p150, T81Int<324>(1));
    double ref150 = std::pow(3.0, 150.0);
    verify_to_double_case(f324, ref150);

    // Negative counterpart near 150 trits
    T81Fraction<324> f324_neg(-p150, T81Int<324>(1));
    verify_to_double_case(f324_neg, -ref150);

    // High boundary near max width: 3^300 / 3^100 = 3^200
    // Non-trivial ratio with mixed digits:
    // (3^300 - 3^100 + 1) / (3^100 + 2)
    T81Int<324> num_324 = p300 - p100 + T81Int<324>(1);
    T81Int<324> den_324 = p100 + T81Int<324>(2);
    T81Fraction<324> f324_nontrivial(num_324, den_324);
    double analytical_ref_324 =
        (std::pow(3.0, 300.0) - std::pow(3.0, 100.0) + 1.0) / (std::pow(3.0, 100.0) + 2.0);
    verify_to_double_case(f324_nontrivial, analytical_ref_324, 1e-12);

    // Negative non-trivial ratio
    T81Fraction<324> f324_nontrivial_neg(-num_324, den_324);
    verify_to_double_case(f324_nontrivial_neg, -analytical_ref_324, 1e-12);
  }
}

// -----------------------------------------------------------------------------
// 3. Normal Rational Arithmetic & Canonicalization
// -----------------------------------------------------------------------------
void test_normal_rational_arithmetic() {
  std::cout << "Running test_normal_rational_arithmetic..." << std::endl;

  using Frac = T81Fraction<81>;

  // Reduction and Canonical form
  Frac f1(T81Int<81>(6), T81Int<81>(8));  // 3/4
  require(f1.num() == T81Int<81>(3), "Canonical num 3/4");
  require(f1.den() == T81Int<81>(4), "Canonical den 3/4");
  verify_to_double_case(f1, 0.75);

  // Negative denominator canonicalization
  Frac f2(T81Int<81>(3), T81Int<81>(-4));  // -3/4
  require(f2.num() == T81Int<81>(-3), "Canonical num -3/4");
  require(f2.den() == T81Int<81>(4), "Canonical den -3/4");
  verify_to_double_case(f2, -0.75);

  // Zero denominator canonicalizes to 0/1 deterministically
  Frac f_zero_den(T81Int<81>(5), T81Int<81>(0));
  require(f_zero_den.num() == T81Int<81>(0), "Zero den -> 0/1 num");
  require(f_zero_den.den() == T81Int<81>(1), "Zero den -> 0/1 den");
  verify_to_double_case(f_zero_den, 0.0);

  // Addition: 1/3 + 1/6 = 3/6 = 1/2
  Frac a(T81Int<81>(1), T81Int<81>(3));
  Frac b(T81Int<81>(1), T81Int<81>(6));
  Frac sum = a + b;
  require(sum.num() == T81Int<81>(1), "1/3 + 1/6 num");
  require(sum.den() == T81Int<81>(2), "1/3 + 1/6 den");
  verify_to_double_case(sum, 0.5);

  // Subtraction: 1/2 - 1/3 = 1/6
  Frac diff = sum - a;
  require(diff.num() == T81Int<81>(1), "1/2 - 1/3 num");
  require(diff.den() == T81Int<81>(6), "1/2 - 1/3 den");

  // Multiplication: (2/3) * (3/4) = 1/2
  Frac m1(T81Int<81>(2), T81Int<81>(3));
  Frac m2(T81Int<81>(3), T81Int<81>(4));
  Frac prod = m1 * m2;
  require(prod.num() == T81Int<81>(1), "(2/3)*(3/4) num");
  require(prod.den() == T81Int<81>(2), "(2/3)*(3/4) den");

  // Division: (1/2) / (3/4) = 2/3
  Frac quot = prod / m2;
  require(quot.num() == T81Int<81>(2), "(1/2)/(3/4) num");
  require(quot.den() == T81Int<81>(3), "(1/2)/(3/4) den");

  // Comparison
  require((a < sum), "1/3 < 1/2");
  require((sum > a), "1/2 > 1/3");
  require((prod == Frac(T81Int<81>(1), T81Int<81>(2))), "1/2 == 1/2");
}

int main() {
  std::cout << "=== T81Fraction Regression Test Suite (SCG-07 Phase 1) ===" << std::endl;
  test_f6_multiplication_overflow_catchable();
  test_f6_addition_subtraction_overflow();
  test_f7_to_double_boundary_widths();
  test_normal_rational_arithmetic();
  std::cout << "=== All T81Fraction Tests Passed! ===" << std::endl;
  return 0;
}
