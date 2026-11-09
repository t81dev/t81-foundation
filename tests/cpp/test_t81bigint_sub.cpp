#include "t81/types/T81BigInt.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace t81::v1;
using t81::Trit;

// Strict test assertion helper
inline void require(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << std::endl;
    std::abort();
  }
}

// -----------------------------------------------------------------------------
// Independent Exact Arbitrary-Precision Integer Oracle (Base 10^9)
// Completely decoupled from T81 internal C++ types and balanced ternary limbs.
// -----------------------------------------------------------------------------
struct ExactBigIntOracle {
  static constexpr std::uint64_t BASE = 1000000000ULL;  // 10^9
  bool negative = false;
  std::vector<std::uint32_t> limbs;  // little-endian base 10^9

  ExactBigIntOracle() = default;

  explicit ExactBigIntOracle(std::int64_t v) {
    if (v < 0) {
      negative = true;
      if (v == std::numeric_limits<std::int64_t>::min()) {
        std::uint64_t uv =
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1ULL;
        while (uv > 0) {
          limbs.push_back(static_cast<std::uint32_t>(uv % BASE));
          uv /= BASE;
        }
        return;
      }
      v = -v;
    }
    if (v == 0) return;
    while (v > 0) {
      limbs.push_back(static_cast<std::uint32_t>(v % BASE));
      v /= BASE;
    }
  }

  static ExactBigIntOracle from_decimal(std::string_view s) {
    ExactBigIntOracle res;
    if (s.empty()) return res;
    std::size_t idx = 0;
    if (s[0] == '-') {
      res.negative = true;
      idx = 1;
    } else if (s[0] == '+') {
      idx = 1;
    }
    while (idx < s.size() && s[idx] == '0') ++idx;
    if (idx == s.size()) {
      res.negative = false;
      return res;
    }

    for (; idx < s.size(); ++idx) {
      res = mul_uint(res, 10);
      std::uint32_t digit = static_cast<std::uint32_t>(s[idx] - '0');
      res = add_uint(res, digit);
    }
    return res;
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

  static ExactBigIntOracle add_uint(const ExactBigIntOracle& a, std::uint32_t v) {
    if (a.negative) {
      ExactBigIntOracle b(v);
      return add(a, b);
    }
    ExactBigIntOracle res = a;
    std::uint64_t carry = v;
    for (std::size_t i = 0; i < res.limbs.size() && carry > 0; ++i) {
      std::uint64_t cur = static_cast<std::uint64_t>(res.limbs[i]) + carry;
      res.limbs[i] = static_cast<std::uint32_t>(cur % BASE);
      carry = cur / BASE;
    }
    if (carry > 0) {
      res.limbs.push_back(static_cast<std::uint32_t>(carry));
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

  static ExactBigIntOracle sub(ExactBigIntOracle a, ExactBigIntOracle b) {
    b.negative = !b.negative;
    b.trim();
    return add(a, b);
  }

  std::string to_decimal_string() const {
    if (is_zero()) return "0";
    std::string s;
    if (negative) s += "-";
    for (std::size_t i = limbs.size(); i-- > 0;) {
      std::string limb_str = std::to_string(limbs[i]);
      if (i + 1 < limbs.size()) {
        s += std::string(9 - limb_str.size(), '0') + limb_str;
      } else {
        s += limb_str;
      }
    }
    return s;
  }
};

// -----------------------------------------------------------------------------
// 1. SCG-06 Finding F5 Reproducer and Direct Variants
// -----------------------------------------------------------------------------
void test_f5_original_reproducer() {
  std::cout << "Running test_f5_original_reproducer..." << std::endl;

  // Exact F5 reproduction test case from SCG-06:
  // x = 3^79 + 7
  // y = 3^80 + 11
  // In pre-repair:
  //   Debug: std::terminate() via verify_invariants() (negative MSB limb)
  //   Release: std::logic_error("borrow beyond MSB") via to_std_chunks()
  // With Phase 2 repair:
  //   Normal subtraction evaluated, exactly matching -98539219609563948877388806804255531738.
  T81BigInt x(1), y(3);
  for (int i = 0; i < 79; ++i) {
    x = x * T81BigInt(3);
    y = y * T81BigInt(3);
  }
  x = x + T81BigInt(7);
  y = y + T81BigInt(11);

  T81BigInt diff = x - y;
  require(!diff.is_zero(), "F5: diff must not be zero");
  require(diff.is_negative(), "F5: diff must be negative");
  require(diff.to_decimal_string() == "-98539219609563948877388806804255531738",
          "F5: diff decimal value mismatch");

  // Reverse subtraction: y - x = +98539219609563948877388806804255531738
  T81BigInt rev = y - x;
  require(!rev.is_negative(), "F5: reverse diff must be positive");
  require(rev.to_decimal_string() == "98539219609563948877388806804255531738",
          "F5: reverse diff decimal value mismatch");
  require((diff + rev).is_zero(), "F5: diff + rev must equal 0");
}

void test_f5_single_limb_subtraction_cancellation() {
  std::cout << "Running test_f5_single_limb_subtraction_cancellation..." << std::endl;

  // Single-limb subtraction with 80-trit identical prefixes:
  // x = 3^79 + 7
  // y = 3^79 + 11
  // x - y = -4
  T81BigInt x(1), y(1);
  for (int i = 0; i < 79; ++i) {
    x = x * T81BigInt(3);
    y = y * T81BigInt(3);
  }
  x = x + T81BigInt(7);
  y = y + T81BigInt(11);

  T81BigInt diff = x - y;
  require(diff.to_decimal_string() == "-4", "F5 variant: (3^79+7) - (3^79+11) must be -4");
  require(diff.is_negative(), "F5 variant: -4 must be negative");

  T81BigInt rev = y - x;
  require(rev.to_decimal_string() == "4", "F5 variant: (3^79+11) - (3^79+7) must be 4");
  require(!rev.is_negative(), "F5 variant: 4 must be positive");

  T81BigInt same = x - x;
  require(same.is_zero(), "F5 variant: x - x must be zero");
  require(!same.is_negative(), "F5 variant: 0 must not be negative");
}

// -----------------------------------------------------------------------------
// 2. Comprehensive Sign-Combination Matrix
// -----------------------------------------------------------------------------
void test_sign_combination_matrix() {
  std::cout << "Running test_sign_combination_matrix..." << std::endl;

  T81BigInt p1(123456789LL);
  T81BigInt p2(987654321LL);
  T81BigInt n1(-123456789LL);
  T81BigInt n2(-987654321LL);

  // Positive - Positive (larger - smaller, smaller - larger, equal)
  require((p2 - p1).to_decimal_string() == "864197532", "P - P (larger - smaller) failed");
  require((p1 - p2).to_decimal_string() == "-864197532", "P - P (smaller - larger) failed");
  require((p1 - p1).is_zero(), "P - P (equal) failed");

  // Positive - Negative
  require((p1 - n2).to_decimal_string() == "1111111110", "P - N failed");
  require((p2 - n1).to_decimal_string() == "1111111110", "P - N failed");

  // Negative - Positive
  require((n1 - p2).to_decimal_string() == "-1111111110", "N - P failed");
  require((n2 - p1).to_decimal_string() == "-1111111110", "N - P failed");

  // Negative - Negative
  require((n1 - n2).to_decimal_string() == "864197532", "N - N failed");
  require((n2 - n1).to_decimal_string() == "-864197532", "N - N failed");
  require((n1 - n1).is_zero(), "N - N (equal) failed");
}

// -----------------------------------------------------------------------------
// 3. Zero and Sign-Boundary Cases
// -----------------------------------------------------------------------------
void test_zero_and_boundary_cases() {
  std::cout << "Running test_zero_and_boundary_cases..." << std::endl;

  T81BigInt z(0);
  T81BigInt p(500);
  T81BigInt n(-500);

  require((z - z).is_zero(), "0 - 0 == 0");
  require(!(z - z).is_negative(), "0 - 0 sign must be positive");

  require((z - p).to_decimal_string() == "-500", "0 - P == -P");
  require((z - p).is_negative(), "0 - P must be negative");

  require((p - z).to_decimal_string() == "500", "P - 0 == P");
  require(!(p - z).is_negative(), "P - 0 must be positive");

  require((z - n).to_decimal_string() == "500", "0 - N == P");
  require(!(z - n).is_negative(), "0 - N must be positive");

  require((n - z).to_decimal_string() == "-500", "N - 0 == N");
  require((n - z).is_negative(), "N - 0 must be negative");

  // Boundary widths: values near INT64_MIN and INT64_MAX
  T81BigInt i_max(std::numeric_limits<std::int64_t>::max());
  T81BigInt i_min(std::numeric_limits<std::int64_t>::min());

  require((i_max - i_max).is_zero(), "INT64_MAX - INT64_MAX == 0");
  require((i_min - i_min).is_zero(), "INT64_MIN - INT64_MIN == 0");
  require((i_max - T81BigInt(0)) == i_max, "INT64_MAX - 0 == INT64_MAX");
  require((i_min - T81BigInt(0)) == i_min, "INT64_MIN - 0 == INT64_MIN");
}

// -----------------------------------------------------------------------------
// 4. Multi-Limb Borrow Propagation Across Limb Boundaries
// -----------------------------------------------------------------------------
void test_multilimb_borrow_propagation() {
  std::cout << "Running test_multilimb_borrow_propagation..." << std::endl;

  // 160 trits spans 2 full limbs (> 81 trits).
  // x = 3^160 + 7
  // y = 3^161 + 11
  T81BigInt x(1), y(3);
  for (int i = 0; i < 160; ++i) {
    x = x * T81BigInt(3);
    y = y * T81BigInt(3);
  }
  x = x + T81BigInt(7);
  y = y + T81BigInt(11);

  T81BigInt diff = x - y;
  require(diff.is_negative(), "Multi-limb x - y must be negative");
  require(diff.to_decimal_string() ==
              "-43694900105678425248461313005980470285134100209825503761625647897325864710406",
          "Multi-limb borrow decimal mismatch");

  T81BigInt rev = y - x;
  require(!rev.is_negative(), "Multi-limb y - x must be positive");
  require(rev.to_decimal_string() ==
              "43694900105678425248461313005980470285134100209825503761625647897325864710406",
          "Multi-limb reverse decimal mismatch");
  require((diff + rev).is_zero(), "Multi-limb (diff + rev) must be zero");

  // Deep borrow: 3^243 - 1 (3 full limbs of 81 trits borrow across all limbs)
  T81BigInt p243(1);
  for (int i = 0; i < 243; ++i) {
    p243 = p243 * T81BigInt(3);
  }
  T81BigInt p243_minus_1 = p243 - T81BigInt(1);
  require(!p243_minus_1.is_negative(), "3^243 - 1 must be positive");
  require(p243_minus_1 + T81BigInt(1) == p243, "(3^243 - 1) + 1 == 3^243");
}

// -----------------------------------------------------------------------------
// 5. Randomized Stress Testing Against Independent Oracle
// -----------------------------------------------------------------------------
void test_randomized_subtraction_oracle_comparison() {
  std::cout << "Running test_randomized_subtraction_oracle_comparison (1,200 trials)..."
            << std::endl;

  std::mt19937_64 rng(0x815A5A5AULL);

  // Cohort A: 400 trials with arbitrary int64 values
  for (int i = 0; i < 400; ++i) {
    std::int64_t v1 = static_cast<std::int64_t>(rng());
    std::int64_t v2 = static_cast<std::int64_t>(rng());
    if (v1 == std::numeric_limits<std::int64_t>::min()) ++v1;
    if (v2 == std::numeric_limits<std::int64_t>::min()) ++v2;

    T81BigInt b1(v1), b2(v2);
    ExactBigIntOracle o1(v1), o2(v2);

    T81BigInt b_diff = b1 - b2;
    ExactBigIntOracle o_diff = ExactBigIntOracle::sub(o1, o2);

    require(b_diff.to_decimal_string() == o_diff.to_decimal_string(),
            "Randomized int64 subtraction mismatch against Oracle");
  }

  // Cohort B: 400 trials with large powers and limb-boundary perturbations (40..120 trits)
  for (int i = 0; i < 400; ++i) {
    int p = 40 + static_cast<int>(rng() % 80);
    std::int64_t offset1 = static_cast<std::int64_t>(rng() % 100000) - 50000;
    std::int64_t offset2 = static_cast<std::int64_t>(rng() % 100000) - 50000;

    T81BigInt b1(1);
    ExactBigIntOracle o1(1);
    for (int k = 0; k < p; ++k) {
      b1 = b1 * T81BigInt(3);
      o1 = ExactBigIntOracle::mul_uint(o1, 3);
    }
    b1 = b1 + T81BigInt(offset1);
    o1 = ExactBigIntOracle::add(o1, ExactBigIntOracle(offset1));

    T81BigInt b2(1);
    ExactBigIntOracle o2(1);
    int p2 = p + (static_cast<int>(rng() % 5) - 2);
    if (p2 < 1) p2 = 1;
    for (int k = 0; k < p2; ++k) {
      b2 = b2 * T81BigInt(3);
      o2 = ExactBigIntOracle::mul_uint(o2, 3);
    }
    b2 = b2 + T81BigInt(offset2);
    o2 = ExactBigIntOracle::add(o2, ExactBigIntOracle(offset2));

    T81BigInt b_diff = b1 - b2;
    ExactBigIntOracle o_diff = ExactBigIntOracle::sub(o1, o2);

    require(b_diff.to_decimal_string() == o_diff.to_decimal_string(),
            "Power-of-three boundary subtraction mismatch against Oracle");
  }

  // Cohort C: 400 trials of sign-inversion and involution properties
  for (int i = 0; i < 400; ++i) {
    std::int64_t val = static_cast<std::int64_t>(rng());
    if (val == std::numeric_limits<std::int64_t>::min()) ++val;

    T81BigInt b(val);
    require(-(-b) == b, "Involution: -(-b) == b failed");
    require((b - b).is_zero(), "Self-subtraction: b - b == 0 failed");
    require(b - T81BigInt(0) == b, "Identity: b - 0 == b failed");
    require(T81BigInt(0) - b == -b, "Identity: 0 - b == -b failed");
  }

  std::cout << "All 1,200 randomized oracle checks passed!" << std::endl;
}

int main() {
  std::cout << "=== T81BigInt Signed Subtraction Regression Suite (SCG-07 Phase 2) ==="
            << std::endl;

  test_f5_original_reproducer();
  test_f5_single_limb_subtraction_cancellation();
  test_sign_combination_matrix();
  test_zero_and_boundary_cases();
  test_multilimb_borrow_propagation();
  test_randomized_subtraction_oracle_comparison();

  std::cout << "=== All T81BigInt Subtraction Tests Passed! ===" << std::endl;
  return 0;
}
