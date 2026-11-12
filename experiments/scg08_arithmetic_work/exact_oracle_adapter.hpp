#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace scg08 {

// ============================================================================
// Independent Arbitrary-Precision Exact Integer Oracle (Base 10^9)
// Decoupled from all T81 arithmetic internals and binary arithmetic kernels.
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
      uv = static_cast<std::uint64_t>(v);
    }
    while (uv > 0) {
      limbs.push_back(static_cast<std::uint32_t>(uv % BASE));
      uv /= BASE;
    }
  }

  static ExactBigIntOracle from_power_of_two(size_t exp) {
    ExactBigIntOracle res(1);
    ExactBigIntOracle two(2);
    for (size_t i = 0; i < exp; ++i) {
      res = mul(res, two);
    }
    return res;
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

  static int cmp(const ExactBigIntOracle& a, const ExactBigIntOracle& b) noexcept {
    if (a.is_zero() && b.is_zero()) return 0;
    if (a.negative != b.negative) {
      return a.negative ? -1 : 1;
    }
    int m = cmp_mag(a, b);
    return a.negative ? -m : m;
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
    neg_b.negative = b.is_zero() ? false : !b.negative;
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

  [[nodiscard]] std::string to_decimal() const {
    if (is_zero()) return "0";
    std::ostringstream oss;
    if (negative) oss << '-';
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

  bool operator!=(const ExactBigIntOracle& other) const noexcept { return !(*this == other); }

  bool operator<(const ExactBigIntOracle& other) const noexcept { return cmp(*this, other) < 0; }

  bool operator<=(const ExactBigIntOracle& other) const noexcept { return cmp(*this, other) <= 0; }

  bool operator>(const ExactBigIntOracle& other) const noexcept { return cmp(*this, other) > 0; }

  bool operator>=(const ExactBigIntOracle& other) const noexcept { return cmp(*this, other) >= 0; }
};

// Converts balanced ternary digits to ExactBigIntOracle
inline ExactBigIntOracle trits_to_oracle(std::span<const int8_t> trits) {
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

// Converts ExactBigIntOracle to canonical balanced trits array
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

// Converts binary 64-bit limbs (two's complement or magnitude+sign) to ExactBigIntOracle
inline ExactBigIntOracle binary_limbs_to_oracle(std::span<const uint64_t> limbs, bool negative) {
  ExactBigIntOracle val(0);
  ExactBigIntOracle base_limb(1);
  ExactBigIntOracle two64 = ExactBigIntOracle::from_power_of_two(64);

  for (uint64_t limb : limbs) {
    ExactBigIntOracle l_oracle(0);
    uint64_t lo = limb % ExactBigIntOracle::BASE;
    uint64_t mid = (limb / ExactBigIntOracle::BASE) % ExactBigIntOracle::BASE;
    uint64_t hi = limb / (static_cast<uint64_t>(ExactBigIntOracle::BASE) * ExactBigIntOracle::BASE);
    if (lo > 0)
      l_oracle = ExactBigIntOracle::add(l_oracle, ExactBigIntOracle(static_cast<int64_t>(lo)));
    if (mid > 0) {
      ExactBigIntOracle m(static_cast<int64_t>(mid));
      m = ExactBigIntOracle::mul(m, ExactBigIntOracle(ExactBigIntOracle::BASE));
      l_oracle = ExactBigIntOracle::add(l_oracle, m);
    }
    if (hi > 0) {
      ExactBigIntOracle h(static_cast<int64_t>(hi));
      h = ExactBigIntOracle::mul(h, ExactBigIntOracle(ExactBigIntOracle::BASE));
      h = ExactBigIntOracle::mul(h, ExactBigIntOracle(ExactBigIntOracle::BASE));
      l_oracle = ExactBigIntOracle::add(l_oracle, h);
    }
    ExactBigIntOracle term = ExactBigIntOracle::mul(l_oracle, base_limb);
    val = ExactBigIntOracle::add(val, term);
    base_limb = ExactBigIntOracle::mul(base_limb, two64);
  }
  if (negative && !val.is_zero()) {
    val.negative = true;
  }
  return val;
}

// 5-Dimensional Oracle Validation Structure
struct OracleValidationReport {
  bool numerical_equality{false};
  bool overflow_classification{false};
  bool canonical_digits{false};
  bool sign_handling{false};
  bool absence_of_truncation{false};

  [[nodiscard]] bool all_passed() const noexcept {
    return numerical_equality && overflow_classification && canonical_digits && sign_handling &&
           absence_of_truncation;
  }
};

// Result classification refers to the original signed B-bit operand domain;
// out-of-domain results are valid when retained exactly in a wider accumulator.
inline bool is_in_signed_b_domain(const ExactBigIntOracle& val, size_t B);

inline OracleValidationReport validate_result(const ExactBigIntOracle& actual,
    const ExactBigIntOracle& expected, size_t B, bool reported_overflow,
    bool canonical, bool represented_negative, bool capacity_sufficient) {
  OracleValidationReport r;
  r.numerical_equality = actual == expected;
  const bool expected_overflow = !is_in_signed_b_domain(expected, B);
  r.overflow_classification = reported_overflow == expected_overflow &&
      (!is_in_signed_b_domain(actual, B)) == expected_overflow;
  r.canonical_digits = canonical;
  r.sign_handling = represented_negative == expected.negative &&
      actual.negative == expected.negative;
  r.absence_of_truncation = capacity_sufficient && r.numerical_equality;
  return r;
}

inline OracleValidationReport validate_ternary_result(std::span<const int8_t> digits,
    const ExactBigIntOracle& expected, size_t B, bool reported_overflow) {
  bool canonical = true;
  bool negative = false;
  for (int8_t d : digits) {
    canonical = canonical && d >= -1 && d <= 1;
    if (d != 0) negative = d < 0; // highest nonzero balanced digit defines sign
  }
  auto remainder = expected;
  for (size_t i = 0; i < digits.size(); ++i)
    remainder = ExactBigIntOracle::div_rem3_balanced(remainder).first;
  return validate_result(trits_to_oracle(digits), expected, B, reported_overflow,
                         canonical, negative, remainder.is_zero());
}

inline OracleValidationReport validate_binary_result(std::span<const uint64_t> limbs,
    bool negative, const ExactBigIntOracle& expected, size_t B, bool reported_overflow) {
  auto actual = binary_limbs_to_oracle(limbs, negative);
  auto magnitude = expected;
  magnitude.negative = false;
  const bool fits = magnitude < ExactBigIntOracle::from_power_of_two(limbs.size() * 64);
  // uint64_t words are canonical digits; sign-magnitude zero must have a positive sign.
  const bool canonical = !actual.is_zero() || !negative;
  return validate_result(actual, expected, B, reported_overflow, canonical, negative, fits);
}

// Fail-closed oracle validator terminating with exact mandatory diagnostic
inline void enforce_oracle_parity(bool condition, const std::string& context) {
  if (!condition) {
    std::cerr << "\nINVALID EXPERIMENT (CORRECTNESS DEFECT: ORACLE MISMATCH) - " << context
              << std::endl;
    std::exit(1);
  }
}

// Signed domain bounds checker
inline bool is_in_signed_b_domain(const ExactBigIntOracle& val, size_t B) {
  ExactBigIntOracle lower_bound = ExactBigIntOracle::from_power_of_two(B - 1);
  lower_bound.negative = true;

  ExactBigIntOracle upper_bound = ExactBigIntOracle::from_power_of_two(B - 1);
  upper_bound = ExactBigIntOracle::sub(upper_bound, ExactBigIntOracle(1));

  return (val >= lower_bound && val <= upper_bound);
}

}  // namespace scg08
