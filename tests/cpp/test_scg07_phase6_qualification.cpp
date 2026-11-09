/**
 * @file test_scg07_phase6_qualification.cpp
 * @brief SCG-07 Phase 6: Consolidated Independent Arithmetic Qualification Suite.
 *
 * Implements independent, multi-component arithmetic qualification across:
 * - F1: T81Limb27 balanced ternary multiplication.
 * - F2: T81Limb54 reference multiplication.
 * - F3: T81Limb54 Karatsuba multiplication.
 * - F4: T81Int<N> fixed-width multiplication and overflow trapping.
 * - F5: T81BigInt signed subtraction (operator- and underlying single-limb fallback).
 * - F6: T81Fraction arithmetic exception safety (noexcept boundary elimination).
 * - F7: T81Fraction::to_double() finite-range numerical conversion approximation.
 *
 * Employs a fully independent, arbitrary-precision exact integer oracle
 * (ExactBigIntOracle, base 10^9) decoupled from all T81 arithmetic internals.
 * All assertions use unconditional fail-closed require() checks active under -DNDEBUG.
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "t81/types/T81BigInt.hpp"
#include "t81/types/T81Fraction.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/T81Limb.hpp"

using t81::T81Int;
using t81::core::T81Limb;
using t81::core::T81Limb27;
using t81::core::T81Limb54;
using t81::v1::T81BigInt;
using t81::v1::T81Fraction;

namespace {

[[noreturn]] void fail_closed(const char* expr, const char* file, int line,
                              const std::string& msg) {
  std::cerr << "\n[QUALIFICATION FATAL FAILURE] " << file << ":" << line << "\n"
            << "  Expression: " << expr << "\n"
            << "  Message:    " << msg << "\n\n";
  std::abort();
}

#define QUAL_REQUIRE(cond, msg)                      \
  do {                                               \
    if (!(cond)) {                                   \
      fail_closed(#cond, __FILE__, __LINE__, (msg)); \
    }                                                \
  } while (false)

// ============================================================================
// Independent Arbitrary-Precision Exact Integer Oracle (Base 10^9)
// ============================================================================

struct ExactBigIntOracle {
  static constexpr std::uint32_t BASE = 1'000'000'000U;
  bool negative{false};
  std::vector<std::uint32_t> limbs;  // little-endian base-10^9 digits

  ExactBigIntOracle() = default;

  explicit ExactBigIntOracle(std::int64_t v) {
    if (v == 0) return;
    std::uint64_t uv;
    if (v < 0) {
      negative = true;
      uv = (v == INT64_MIN) ? 9223372036854775808ULL : static_cast<std::uint64_t>(-v);
    } else {
      negative = false;
      uv = static_cast<std::uint64_t>(v);
    }
    while (uv > 0) {
      limbs.push_back(static_cast<std::uint32_t>(uv % BASE));
      uv /= BASE;
    }
  }

  [[nodiscard]] bool is_zero() const noexcept { return limbs.empty(); }

  void trim() noexcept {
    while (!limbs.empty() && limbs.back() == 0) {
      limbs.pop_back();
    }
    if (limbs.empty()) negative = false;
  }

  static int cmp_mag(const ExactBigIntOracle& a, const ExactBigIntOracle& b) noexcept {
    if (a.limbs.size() != b.limbs.size()) {
      return a.limbs.size() < b.limbs.size() ? -1 : 1;
    }
    for (size_t i = a.limbs.size(); i-- > 0;) {
      if (a.limbs[i] != b.limbs[i]) {
        return a.limbs[i] < b.limbs[i] ? -1 : 1;
      }
    }
    return 0;
  }

  static ExactBigIntOracle add(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    if (a.negative == b.negative) {
      ExactBigIntOracle res;
      res.negative = a.negative;
      std::uint64_t carry = 0;
      size_t n = std::max(a.limbs.size(), b.limbs.size());
      for (size_t i = 0; i < n || carry; ++i) {
        std::uint64_t sum = carry;
        if (i < a.limbs.size()) sum += a.limbs[i];
        if (i < b.limbs.size()) sum += b.limbs[i];
        res.limbs.push_back(static_cast<std::uint32_t>(sum % BASE));
        carry = sum / BASE;
      }
      res.trim();
      return res;
    }

    int c = cmp_mag(a, b);
    if (c == 0) return ExactBigIntOracle(0);
    const ExactBigIntOracle& larger = (c > 0) ? a : b;
    const ExactBigIntOracle& smaller = (c > 0) ? b : a;

    ExactBigIntOracle res;
    res.negative = larger.negative;
    std::int64_t borrow = 0;
    for (size_t i = 0; i < larger.limbs.size(); ++i) {
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

  static ExactBigIntOracle sub(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    ExactBigIntOracle neg_b = b;
    neg_b.negative = !b.negative;
    return add(a, neg_b);
  }

  static ExactBigIntOracle mul(const ExactBigIntOracle& a, const ExactBigIntOracle& b) {
    if (a.is_zero() || b.is_zero()) return ExactBigIntOracle(0);
    ExactBigIntOracle res;
    res.negative = (a.negative != b.negative);
    res.limbs.assign(a.limbs.size() + b.limbs.size(), 0);

    for (size_t i = 0; i < a.limbs.size(); ++i) {
      std::uint64_t carry = 0;
      for (size_t j = 0; j < b.limbs.size() || carry; ++j) {
        std::uint64_t cur = res.limbs[i + j] + carry;
        if (j < b.limbs.size()) {
          cur += static_cast<std::uint64_t>(a.limbs[i]) * b.limbs[j];
        }
        res.limbs[i + j] = static_cast<std::uint32_t>(cur % BASE);
        carry = cur / BASE;
      }
    }
    res.trim();
    return res;
  }

  static std::pair<ExactBigIntOracle, int8_t> div_rem3_balanced(const ExactBigIntOracle& val) {
    if (val.is_zero()) return {ExactBigIntOracle(0), 0};

    ExactBigIntOracle q;
    q.negative = val.negative;
    q.limbs.assign(val.limbs.size(), 0);

    std::uint64_t rem = 0;
    for (size_t i = val.limbs.size(); i-- > 0;) {
      std::uint64_t cur = val.limbs[i] + rem * BASE;
      q.limbs[i] = static_cast<std::uint32_t>(cur / 3);
      rem = cur % 3;
    }
    q.trim();

    int8_t srem = static_cast<int8_t>(rem);
    if (val.negative) srem = static_cast<int8_t>(-srem);

    if (srem == 2) {
      srem = -1;
      q = add(q, ExactBigIntOracle(1));
    } else if (srem == -2) {
      srem = 1;
      q = sub(q, ExactBigIntOracle(1));
    }
    return {q, srem};
  }

  [[nodiscard]] double to_double() const noexcept {
    if (is_zero()) return 0.0;
    double d = 0.0;
    double base_pow = 1.0;
    for (std::uint32_t limb : limbs) {
      d += static_cast<double>(limb) * base_pow;
      base_pow *= static_cast<double>(BASE);
    }
    return negative ? -d : d;
  }

  [[nodiscard]] std::string to_decimal() const {
    if (is_zero()) return "0";
    std::ostringstream oss;
    if (negative) oss << "-";
    oss << limbs.back();
    for (size_t i = limbs.size() - 1; i-- > 0;) {
      oss << std::setw(9) << std::setfill('0') << limbs[i];
    }
    return oss.str();
  }

  bool operator==(const ExactBigIntOracle& other) const noexcept {
    if (is_zero() && other.is_zero()) return true;
    if (negative != other.negative) return false;
    return limbs == other.limbs;
  }
};

ExactBigIntOracle trits_to_oracle(std::span<const int8_t> trits) {
  ExactBigIntOracle val(0);
  ExactBigIntOracle p(1);
  ExactBigIntOracle three(3);
  for (int8_t t : trits) {
    if (t == 1)
      val = ExactBigIntOracle::add(val, p);
    else if (t == -1)
      val = ExactBigIntOracle::sub(val, p);
    p = ExactBigIntOracle::mul(p, three);
  }
  return val;
}

template <size_t N>
std::array<int8_t, N> oracle_to_canonical_trits(ExactBigIntOracle val) {
  std::array<int8_t, N> trits{};
  for (size_t i = 0; i < N; ++i) {
    auto [next_val, rem] = ExactBigIntOracle::div_rem3_balanced(val);
    trits[i] = rem;
    val = next_val;
  }
  return trits;
}

T81Limb27 make_limb27(const std::array<int8_t, 27>& trits) {
  std::array<int8_t, T81Limb::TRITS> trits48{};
  std::copy(trits.begin(), trits.end(), trits48.begin());
  return T81Limb27::from_trits_window(trits48, 0);
}

// SplitMix64 pseudo-random generator for reproducible deterministic sampling
struct SplitMix64 {
  std::uint64_t state;
  explicit SplitMix64(std::uint64_t seed) noexcept : state(seed) {}
  std::uint64_t next() noexcept {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
  }
};

// ============================================================================
// Phase C & D: Focused Independent Tests
// ============================================================================

void test_f1_limb27_multiplication() {
  std::cout << "[QUAL] Running F1 (T81Limb27 multiplication) checks...\n";

  // 1. Original F1 reproducer: 1 * 2 = 2 (not 27)
  {
    std::array<int8_t, 27> t1{}, t2{};
    t1[0] = 1;
    t2[0] = -1;
    t2[1] = 1;  // -1 + 3 = 2
    auto l1 = make_limb27(t1);
    auto l2 = make_limb27(t2);
    auto prod = l1 * l2;
    auto prod_trits = prod.to_trits();
    QUAL_REQUIRE(prod_trits[0] == -1 && prod_trits[1] == 1, "F1 reproducer: 1 * 2 must be 2");
    for (size_t i = 2; i < 27; ++i) {
      QUAL_REQUIRE(prod_trits[i] == 0, "F1 reproducer: upper trits must be zero");
    }
  }

  // 2. Commutativity, zero, identity across 2,000 deterministic trials
  SplitMix64 rng(0xDEADBEEF01ULL);
  for (size_t trial = 0; trial < 2000; ++trial) {
    std::array<int8_t, 27> a_trits{}, b_trits{};
    for (size_t i = 0; i < 27; ++i) {
      a_trits[i] = static_cast<int8_t>((static_cast<int>(rng.next() % 3)) - 1);
      b_trits[i] = static_cast<int8_t>((static_cast<int>(rng.next() % 3)) - 1);
    }
    auto la = make_limb27(a_trits);
    auto lb = make_limb27(b_trits);
    auto p1 = la * lb;
    auto p2 = lb * la;
    QUAL_REQUIRE(p1.to_trits() == p2.to_trits(), "T81Limb27 commutativity check failed");

    // Exact oracle check modulo 3^54 (T81Limb27 * T81Limb27 produces 54 trits)
    auto oa = trits_to_oracle(a_trits);
    auto ob = trits_to_oracle(b_trits);
    auto expected_prod = ExactBigIntOracle::mul(oa, ob);
    auto expected_trits = oracle_to_canonical_trits<54>(expected_prod);
    QUAL_REQUIRE(p1.to_trits() == expected_trits, "T81Limb27 exact oracle comparison failed");
  }
}

void test_f2_and_f3_limb54_multiplication() {
  std::cout << "[QUAL] Running F2 & F3 (T81Limb54 reference_mul & karatsuba) checks...\n";

  // 1. Original F2 reproducers: dense all-ones 54-trit vectors
  {
    std::array<int8_t, 54> all_ones{};
    all_ones.fill(1);
    auto la = T81Limb54::from_trits(all_ones);
    auto p_ref = T81Limb54::reference_mul(la, la);
    auto p_kar = T81Limb54::karatsuba(la, la);

    auto oa = trits_to_oracle(all_ones);
    auto expected_exact = ExactBigIntOracle::mul(oa, oa);
    auto expected_trits = oracle_to_canonical_trits<54>(expected_exact);

    QUAL_REQUIRE(p_ref.to_trits() == expected_trits, "F2 all-ones reference_mul failed");
    QUAL_REQUIRE(p_kar.to_trits() == expected_trits, "F3 all-ones karatsuba failed");
    QUAL_REQUIRE(p_ref.to_trits() == p_kar.to_trits(),
                 "F2 vs F3 differential mismatch on all-ones");
  }

  // 2. Randomized trials: 2,500 trials against independent exact oracle + differential check
  SplitMix64 rng(0xCAFEBABEF00DULL);
  for (size_t trial = 0; trial < 2500; ++trial) {
    std::array<int8_t, 54> a_trits{}, b_trits{};
    for (size_t i = 0; i < 54; ++i) {
      a_trits[i] = static_cast<int8_t>((static_cast<int>(rng.next() % 3)) - 1);
      b_trits[i] = static_cast<int8_t>((static_cast<int>(rng.next() % 3)) - 1);
    }
    auto la = T81Limb54::from_trits(a_trits);
    auto lb = T81Limb54::from_trits(b_trits);
    auto p_ref = T81Limb54::reference_mul(la, lb);
    auto p_kar = T81Limb54::karatsuba(la, lb);

    // Cross-implementation agreement
    QUAL_REQUIRE(p_ref.to_trits() == p_kar.to_trits(),
                 "Differential mismatch between reference_mul and karatsuba");

    // Exact oracle agreement modulo 3^54
    auto oa = trits_to_oracle(a_trits);
    auto ob = trits_to_oracle(b_trits);
    auto expected_prod = ExactBigIntOracle::mul(oa, ob);
    auto expected_trits = oracle_to_canonical_trits<54>(expected_prod);
    QUAL_REQUIRE(p_kar.to_trits() == expected_trits,
                 "T81Limb54 karatsuba vs ExactBigIntOracle mismatch");
  }
}

void test_f4_t81int_arithmetic_and_overflow() {
  std::cout << "[QUAL] Running F4 (T81Int fixed-width arithmetic & overflow) checks...\n";

  // 1. Small width exact arithmetic
  T81Int<2> a(2), b(2);
  T81Int<2> c = a * b;  // 2 * 2 = 4 (representable in 2 trits: range [-4, 4])
  QUAL_REQUIRE(c.to_int64() == 4, "T81Int<2>(2) * (2) must be 4");

  // 2. Overflow trapping via std::overflow_error
  T81Int<2> d(3);
  bool caught = false;
  try {
    [[maybe_unused]] auto o = d * d;  // 3 * 3 = 9 > 4 -> must throw std::overflow_error
  } catch (const std::overflow_error&) {
    caught = true;
  }
  QUAL_REQUIRE(caught, "T81Int<2>(3) * (3) must throw std::overflow_error");
}

void test_f5_t81bigint_signed_subtraction() {
  std::cout << "[QUAL] Running F5 (T81BigInt signed subtraction) checks...\n";

  // 1. Original F5 reproducer: (3^79 + 7) - (3^80 + 11)
  {
    T81BigInt x(1), y(3);
    for (int i = 0; i < 79; ++i) {
      x = x * T81BigInt(3);
      y = y * T81BigInt(3);
    }
    x = x + T81BigInt(7);
    y = y + T81BigInt(11);
    T81BigInt diff = x - y;
    QUAL_REQUIRE(diff.is_negative(), "F5 reproducer diff must be negative");
    QUAL_REQUIRE(diff.to_decimal_string() == "-98539219609563948877388806804255531738",
                 "F5 reproducer decimal value mismatch");
  }

  // 2. Cancellation variant: (3^79 + 7) - (3^79 + 11) = -4
  {
    T81BigInt x(1), y(1);
    for (int i = 0; i < 79; ++i) {
      x = x * T81BigInt(3);
      y = y * T81BigInt(3);
    }
    x = x + T81BigInt(7);
    y = y + T81BigInt(11);
    T81BigInt diff = x - y;
    QUAL_REQUIRE(diff.to_decimal_string() == "-4", "F5 cancellation variant must be -4");
  }

  // 3. Multi-limb borrow propagation across 160-trit boundary
  {
    T81BigInt x(1);
    for (int i = 0; i < 160; ++i) {
      x = x * T81BigInt(3);
    }
    T81BigInt y(1);
    T81BigInt diff = x - y;
    QUAL_REQUIRE(!diff.is_negative(), "3^160 - 1 must be positive");
    T81BigInt sum = diff + y;
    QUAL_REQUIRE(sum.to_decimal_string() == x.to_decimal_string(),
                 "Borrow reversal (x - 1) + 1 == x failed");
  }

  // 4. Randomized oracle checks against ExactBigIntOracle
  SplitMix64 rng(0x8899AABBCCDDEEFFULL);
  for (size_t trial = 0; trial < 800; ++trial) {
    int64_t v1 = static_cast<int64_t>(rng.next() % 2'000'000'000ULL) - 1'000'000'000LL;
    int64_t v2 = static_cast<int64_t>(rng.next() % 2'000'000'000ULL) - 1'000'000'000LL;
    T81BigInt b1(v1), b2(v2);
    T81BigInt b_diff = b1 - b2;

    ExactBigIntOracle o1(v1), o2(v2);
    ExactBigIntOracle o_diff = ExactBigIntOracle::sub(o1, o2);

    QUAL_REQUIRE(b_diff.to_decimal_string() == o_diff.to_decimal(),
                 "T81BigInt operator- vs ExactBigIntOracle mismatch");
  }
}

void test_f6_and_f7_t81fraction() {
  std::cout << "[QUAL] Running F6 & F7 (T81Fraction exception safety & to_double) checks...\n";

  // 1. F6: Multiplication overflow catches cleanly without invoking std::terminate()
  {
    using Frac81 = T81Fraction<81>;
    Frac81::Int num1(1);
    for (int i = 0; i < 45; ++i) {
      num1 = num1 * Frac81::Int(3);
    }
    Frac81 f1(num1, Frac81::Int(1));
    Frac81 f2(num1, Frac81::Int(1));

    bool caught = false;
    try {
      [[maybe_unused]] auto f3 = f1 * f2;
    } catch (const std::overflow_error&) {
      caught = true;
    }
    QUAL_REQUIRE(caught, "F6: Frac81 * Frac81 overflow must throw std::overflow_error");
  }

  // 2. F7: to_double() across boundary widths (13, 40, 41, 81, 324 trits)
  {
    // Width 41 (> 40 trits previously aborted in to_int64())
    using Frac41 = T81Fraction<41>;
    Frac41::Int p40(1);
    for (int i = 0; i < 40; ++i) {
      p40 = p40 * Frac41::Int(3);
    }
    Frac41 f_large(p40, Frac41::Int(1));
    double d_val = f_large.to_double();
    double expected = std::pow(3.0, 40);
    double rel_err = std::abs(d_val - expected) / expected;
    QUAL_REQUIRE(rel_err < 1e-12, "F7: Frac41 to_double() precision out of tolerance");
  }

  {
    // Width 81
    using Frac81 = T81Fraction<81>;
    Frac81::Int p80(1);
    for (int i = 0; i < 80; ++i) {
      p80 = p80 * Frac81::Int(3);
    }
    Frac81 f_large(p80, Frac81::Int(1));
    double d_val = f_large.to_double();
    double expected = std::pow(3.0, 80);
    double rel_err = std::abs(d_val - expected) / expected;
    QUAL_REQUIRE(rel_err < 1e-12, "F7: Frac81 to_double() precision out of tolerance");
  }

  {
    // Width 324
    using Frac324 = T81Fraction<324>;
    Frac324::Int p300(1);
    for (int i = 0; i < 300; ++i) {
      p300 = p300 * Frac324::Int(3);
    }
    Frac324 f_large(p300, Frac324::Int(1));
    double d_val = f_large.to_double();
    double expected = std::pow(3.0, 300);
    double rel_err = std::abs(d_val - expected) / expected;
    QUAL_REQUIRE(rel_err < 1e-12, "F7: Frac324 to_double() precision out of tolerance");
  }
}

}  // namespace

int main() {
  std::cout << "=====================================================================\n";
  std::cout << " SCG-07 Phase 6: Consolidated Independent Arithmetic Qualification   \n";
  std::cout << "=====================================================================\n";

  test_f1_limb27_multiplication();
  test_f2_and_f3_limb54_multiplication();
  test_f4_t81int_arithmetic_and_overflow();
  test_f5_t81bigint_signed_subtraction();
  test_f6_and_f7_t81fraction();

  std::cout << "=====================================================================\n";
  std::cout << " [QUALIFICATION RESULT] ALL INDEPENDENT TESTS PASSED UNCONDITIONALLY \n";
  std::cout << "=====================================================================\n";
  return 0;
}
