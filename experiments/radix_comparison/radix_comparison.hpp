/**
 * @file radix_comparison.hpp
 * @brief Standalone reference implementations and algorithms for Radix Comparison Experiment.
 *
 * Compares:
 *   1. Binary 64-bit integers and multi-limb binary BigInt.
 *   2. Canonical Balanced Ternary (2-bit packed trits, digit set {-1, 0, 1}).
 *   3. Redundant Signed-Digit Ternary (Avizienis carry-free addition, digit set {-2, -1, 0, 1, 2}).
 *   4. Grouped Radix-9 (2 trits, digit set {-4..4}).
 *   5. Grouped Radix-27 (3 trits, digit set {-13..13}).
 *   6. Grouped Radix-81 (4 trits / tryte, digit set {-40..40}).
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace t81::experiments::radix {

// ============================================================================
// Data Types and Definitions
// ============================================================================

enum class Trit : int8_t { N = -1, Z = 0, P = 1 };

inline int trit_to_int(Trit t) noexcept { return static_cast<int>(t); }
inline Trit int_to_trit(int v) noexcept {
  return (v > 0) ? Trit::P : (v < 0) ? Trit::N : Trit::Z;
}

// ----------------------------------------------------------------------------
// 1. Binary Integer Baseline
// ----------------------------------------------------------------------------
struct Binary64 {
  int64_t value = 0;

  Binary64() = default;
  explicit Binary64(int64_t v) : value(v) {}

  static Binary64 add(const Binary64& a, const Binary64& b) {
    return Binary64(a.value + b.value);
  }

  static Binary64 sub(const Binary64& a, const Binary64& b) {
    return Binary64(a.value - b.value);
  }

  static Binary64 mul(const Binary64& a, const Binary64& b) {
    return Binary64(a.value * b.value);
  }

  int64_t to_int64() const { return value; }
  size_t byte_size() const { return sizeof(int64_t); }
};

struct BinaryBigInt {
  std::vector<uint64_t> limbs; // 64-bit limbs, unsigned
  bool negative = false;

  BinaryBigInt() { limbs.push_back(0); }
  explicit BinaryBigInt(int64_t v) {
    if (v < 0) {
      negative = true;
      limbs.push_back(static_cast<uint64_t>(-(v + 1)) + 1);
    } else {
      negative = false;
      limbs.push_back(static_cast<uint64_t>(v));
    }
  }

  size_t byte_size() const { return limbs.size() * sizeof(uint64_t) + sizeof(bool); }
};

// ----------------------------------------------------------------------------
// 2. Canonical Balanced Ternary
// ----------------------------------------------------------------------------
struct CanonicalTernary {
  std::vector<Trit> trits; // LSB first

  CanonicalTernary() : trits({Trit::Z}) {}
  explicit CanonicalTernary(std::vector<Trit> t) : trits(std::move(t)) {
    normalize();
  }

  void normalize() {
    while (trits.size() > 1 && trits.back() == Trit::Z) {
      trits.pop_back();
    }
    if (trits.empty()) trits.push_back(Trit::Z);
  }

  static CanonicalTernary from_int64(int64_t x) {
    if (x == 0) return CanonicalTernary({Trit::Z});

    bool neg = x < 0;
    uint64_t abs_x = neg ? static_cast<uint64_t>(-(x + 1)) + 1 : static_cast<uint64_t>(x);

    std::vector<Trit> res;
    res.reserve(64);

    while (abs_x > 0) {
      uint64_t rem = abs_x % 3;
      abs_x /= 3;
      if (rem == 2) {
        res.push_back(Trit::N);
        abs_x += 1;
      } else {
        res.push_back(rem == 1 ? Trit::P : Trit::Z);
      }
    }

    if (neg) {
      for (auto& t : res) {
        t = (t == Trit::P) ? Trit::N : (t == Trit::N) ? Trit::P : Trit::Z;
      }
    }

    return CanonicalTernary(res);
  }

  int64_t to_int64() const {
    int64_t val = 0;
    int64_t pow3 = 1;
    for (size_t i = 0; i < trits.size(); ++i) {
      int t = trit_to_int(trits[i]);
      val += static_cast<int64_t>(t) * pow3;
      if (i + 1 < trits.size()) {
        pow3 *= 3;
      }
    }
    return val;
  }

  // Returns {result, max_carry_distance, total_carry_ops}
  static std::tuple<CanonicalTernary, size_t, size_t> add_with_stats(
      const CanonicalTernary& a, const CanonicalTernary& b) {
    size_t max_size = std::max(a.trits.size(), b.trits.size());
    std::vector<Trit> res;
    res.reserve(max_size + 2);

    int carry = 0;
    size_t cur_carry_chain = 0;
    size_t max_carry_chain = 0;
    size_t total_carry_ops = 0;

    for (size_t i = 0; i < max_size || carry != 0; ++i) {
      int sum = carry;
      if (i < a.trits.size()) sum += trit_to_int(a.trits[i]);
      if (i < b.trits.size()) sum += trit_to_int(b.trits[i]);

      if (carry != 0) total_carry_ops++;

      if (sum >= 2) {
        res.push_back(int_to_trit(sum - 3));
        carry = 1;
        cur_carry_chain++;
      } else if (sum <= -2) {
        res.push_back(int_to_trit(sum + 3));
        carry = -1;
        cur_carry_chain++;
      } else {
        res.push_back(int_to_trit(sum));
        carry = 0;
        cur_carry_chain = 0;
      }

      max_carry_chain = std::max(max_carry_chain, cur_carry_chain);
    }

    return {CanonicalTernary(res), max_carry_chain, total_carry_ops};
  }

  static CanonicalTernary add(const CanonicalTernary& a, const CanonicalTernary& b) {
    return std::get<0>(add_with_stats(a, b));
  }

  static CanonicalTernary negate(const CanonicalTernary& a) {
    std::vector<Trit> res = a.trits;
    for (auto& t : res) {
      t = (t == Trit::P) ? Trit::N : (t == Trit::N) ? Trit::P : Trit::Z;
    }
    return CanonicalTernary(res);
  }

  static CanonicalTernary sub(const CanonicalTernary& a, const CanonicalTernary& b) {
    return add(a, negate(b));
  }

  size_t byte_size_packed() const {
    // 2 bits per trit -> 4 trits per byte
    return (trits.size() + 3) / 4;
  }
};

// ----------------------------------------------------------------------------
// 3. Redundant Signed-Digit Ternary (Avizienis Carry-Free)
// ----------------------------------------------------------------------------
struct RedundantTernary {
  // Digit set: {-2, -1, 0, 1, 2}
  std::vector<int8_t> digits; // LSB first

  RedundantTernary() : digits({0}) {}
  explicit RedundantTernary(std::vector<int8_t> d) : digits(std::move(d)) {}

  static RedundantTernary from_canonical(const CanonicalTernary& ct) {
    std::vector<int8_t> d(ct.trits.size());
    for (size_t i = 0; i < ct.trits.size(); ++i) {
      d[i] = static_cast<int8_t>(trit_to_int(ct.trits[i]));
    }
    return RedundantTernary(d);
  }

  static RedundantTernary from_int64(int64_t x) {
    return from_canonical(CanonicalTernary::from_int64(x));
  }

  int64_t to_int64() const {
    return to_canonical().to_int64();
  }

  // Normalizes Redundant Signed-Digit to Canonical Balanced Ternary
  CanonicalTernary to_canonical() const {
    std::vector<Trit> res;
    res.reserve(digits.size() + 2);

    int carry = 0;
    for (size_t i = 0; i < digits.size() || carry != 0; ++i) {
      int sum = carry;
      if (i < digits.size()) sum += digits[i];

      if (sum >= 2) {
        res.push_back(int_to_trit(sum - 3));
        carry = 1;
      } else if (sum <= -2) {
        res.push_back(int_to_trit(sum + 3));
        carry = -1;
      } else {
        res.push_back(int_to_trit(sum));
        carry = 0;
      }
    }
    return CanonicalTernary(res);
  }

  // Avizienis Radix-3 Carry-Free Addition (Local carry propagation = 1)
  static RedundantTernary add_carry_free(const RedundantTernary& a, const RedundantTernary& b) {
    size_t n = std::max(a.digits.size(), b.digits.size()) + 2;
    std::vector<int8_t> da(n, 0);
    std::vector<int8_t> db(n, 0);

    for (size_t i = 0; i < a.digits.size(); ++i) da[i] = a.digits[i];
    for (size_t i = 0; i < b.digits.size(); ++i) db[i] = b.digits[i];

    // Step 1: Compute interim sum w_i = a_i + b_i in {-4..4}
    std::vector<int8_t> w(n, 0);
    for (size_t i = 0; i < n; ++i) {
      w[i] = da[i] + db[i];
    }

    // Step 2: Decompose w_i = 3 * t_{i+1} + q_i
    std::vector<int8_t> t(n + 1, 0);
    std::vector<int8_t> q(n, 0);

    for (size_t i = 0; i < n; ++i) {
      int8_t wi = w[i];
      int8_t prev_w = (i > 0) ? w[i - 1] : 0;

      if (wi >= 2) {
        if (wi == 2 && prev_w < 0) {
          t[i + 1] = 0;
          q[i] = 2;
        } else {
          t[i + 1] = 1;
          q[i] = static_cast<int8_t>(wi - 3);
        }
      } else if (wi <= -2) {
        if (wi == -2 && prev_w > 0) {
          t[i + 1] = 0;
          q[i] = -2;
        } else {
          t[i + 1] = -1;
          q[i] = static_cast<int8_t>(wi + 3);
        }
      } else {
        t[i + 1] = 0;
        q[i] = wi;
      }
    }

    // Step 3: Final sum digit s_i = q_i + t_i in {-2..2}
    std::vector<int8_t> s(n + 1, 0);
    for (size_t i = 0; i < n; ++i) {
      s[i] = static_cast<int8_t>(q[i] + t[i]);
    }
    s[n] = t[n];

    // Trim leading zeros
    while (s.size() > 1 && s.back() == 0) s.pop_back();

    return RedundantTernary(s);
  }

  size_t byte_size() const { return digits.size() * sizeof(int8_t); }
};

// ----------------------------------------------------------------------------
// 4. Grouped Radix-9 Integer (2 trits, digit set {-4..4})
// ----------------------------------------------------------------------------
struct Radix9Integer {
  std::vector<int8_t> digits; // LSB first, digits in {-4..4}

  Radix9Integer() : digits({0}) {}
  explicit Radix9Integer(std::vector<int8_t> d) : digits(std::move(d)) {}

  static Radix9Integer from_int64(int64_t x) {
    CanonicalTernary ct = CanonicalTernary::from_int64(x);
    std::vector<int8_t> r9;
    for (size_t i = 0; i < ct.trits.size(); i += 2) {
      int d0 = trit_to_int(ct.trits[i]);
      int d1 = (i + 1 < ct.trits.size()) ? trit_to_int(ct.trits[i + 1]) : 0;
      r9.push_back(static_cast<int8_t>(d0 + d1 * 3));
    }
    if (r9.empty()) r9.push_back(0);
    return Radix9Integer(r9);
  }

  int64_t to_int64() const {
    int64_t val = 0;
    int64_t pow9 = 1;
    for (size_t i = 0; i < digits.size(); ++i) {
      val += static_cast<int64_t>(digits[i]) * pow9;
      if (i + 1 < digits.size()) pow9 *= 9;
    }
    return val;
  }

  static Radix9Integer add(const Radix9Integer& a, const Radix9Integer& b) {
    size_t max_s = std::max(a.digits.size(), b.digits.size());
    std::vector<int8_t> res;
    res.reserve(max_s + 2);

    int carry = 0;
    for (size_t i = 0; i < max_s || carry != 0; ++i) {
      int sum = carry;
      if (i < a.digits.size()) sum += a.digits[i];
      if (i < b.digits.size()) sum += b.digits[i];

      if (sum > 4) {
        res.push_back(static_cast<int8_t>(sum - 9));
        carry = 1;
      } else if (sum < -4) {
        res.push_back(static_cast<int8_t>(sum + 9));
        carry = -1;
      } else {
        res.push_back(static_cast<int8_t>(sum));
        carry = 0;
      }
    }

    while (res.size() > 1 && res.back() == 0) res.pop_back();
    return Radix9Integer(res);
  }

  size_t byte_size() const { return digits.size() * sizeof(int8_t); }
};

// ----------------------------------------------------------------------------
// 5. Grouped Radix-27 Integer (3 trits, digit set {-13..13})
// ----------------------------------------------------------------------------
struct Radix27Integer {
  std::vector<int8_t> digits; // LSB first, digits in {-13..13}

  Radix27Integer() : digits({0}) {}
  explicit Radix27Integer(std::vector<int8_t> d) : digits(std::move(d)) {}

  static Radix27Integer from_int64(int64_t x) {
    CanonicalTernary ct = CanonicalTernary::from_int64(x);
    std::vector<int8_t> r27;
    for (size_t i = 0; i < ct.trits.size(); i += 3) {
      int d0 = trit_to_int(ct.trits[i]);
      int d1 = (i + 1 < ct.trits.size()) ? trit_to_int(ct.trits[i + 1]) : 0;
      int d2 = (i + 2 < ct.trits.size()) ? trit_to_int(ct.trits[i + 2]) : 0;
      r27.push_back(static_cast<int8_t>(d0 + d1 * 3 + d2 * 9));
    }
    if (r27.empty()) r27.push_back(0);
    return Radix27Integer(r27);
  }

  int64_t to_int64() const {
    int64_t val = 0;
    int64_t pow27 = 1;
    for (size_t i = 0; i < digits.size(); ++i) {
      val += static_cast<int64_t>(digits[i]) * pow27;
      if (i + 1 < digits.size()) pow27 *= 27;
    }
    return val;
  }

  static Radix27Integer add(const Radix27Integer& a, const Radix27Integer& b) {
    size_t max_s = std::max(a.digits.size(), b.digits.size());
    std::vector<int8_t> res;
    res.reserve(max_s + 2);

    int carry = 0;
    for (size_t i = 0; i < max_s || carry != 0; ++i) {
      int sum = carry;
      if (i < a.digits.size()) sum += a.digits[i];
      if (i < b.digits.size()) sum += b.digits[i];

      if (sum > 13) {
        res.push_back(static_cast<int8_t>(sum - 27));
        carry = 1;
      } else if (sum < -13) {
        res.push_back(static_cast<int8_t>(sum + 27));
        carry = -1;
      } else {
        res.push_back(static_cast<int8_t>(sum));
        carry = 0;
      }
    }

    while (res.size() > 1 && res.back() == 0) res.pop_back();
    return Radix27Integer(res);
  }

  size_t byte_size() const { return digits.size() * sizeof(int8_t); }
};

// ----------------------------------------------------------------------------
// 6. Grouped Radix-81 Integer (4 trits / tryte, digit set {-40..40})
// ----------------------------------------------------------------------------
struct Radix81Integer {
  std::vector<int8_t> digits; // LSB first, digits in {-40..40}

  Radix81Integer() : digits({0}) {}
  explicit Radix81Integer(std::vector<int8_t> d) : digits(std::move(d)) {}

  static Radix81Integer from_int64(int64_t x) {
    CanonicalTernary ct = CanonicalTernary::from_int64(x);
    std::vector<int8_t> r81;
    for (size_t i = 0; i < ct.trits.size(); i += 4) {
      int d0 = trit_to_int(ct.trits[i]);
      int d1 = (i + 1 < ct.trits.size()) ? trit_to_int(ct.trits[i + 1]) : 0;
      int d2 = (i + 2 < ct.trits.size()) ? trit_to_int(ct.trits[i + 2]) : 0;
      int d3 = (i + 3 < ct.trits.size()) ? trit_to_int(ct.trits[i + 3]) : 0;
      r81.push_back(static_cast<int8_t>(d0 + d1 * 3 + d2 * 9 + d3 * 27));
    }
    if (r81.empty()) r81.push_back(0);
    return Radix81Integer(r81);
  }

  int64_t to_int64() const {
    int64_t val = 0;
    int64_t pow81 = 1;
    for (size_t i = 0; i < digits.size(); ++i) {
      val += static_cast<int64_t>(digits[i]) * pow81;
      if (i + 1 < digits.size()) pow81 *= 81;
    }
    return val;
  }

  static Radix81Integer add(const Radix81Integer& a, const Radix81Integer& b) {
    size_t max_s = std::max(a.digits.size(), b.digits.size());
    std::vector<int8_t> res;
    res.reserve(max_s + 2);

    int carry = 0;
    for (size_t i = 0; i < max_s || carry != 0; ++i) {
      int sum = carry;
      if (i < a.digits.size()) sum += a.digits[i];
      if (i < b.digits.size()) sum += b.digits[i];

      if (sum > 40) {
        res.push_back(static_cast<int8_t>(sum - 81));
        carry = 1;
      } else if (sum < -40) {
        res.push_back(static_cast<int8_t>(sum + 81));
        carry = -1;
      } else {
        res.push_back(static_cast<int8_t>(sum));
        carry = 0;
      }
    }

    while (res.size() > 1 && res.back() == 0) res.pop_back();
    return Radix81Integer(res);
  }

  size_t byte_size() const { return digits.size() * sizeof(int8_t); }
};

} // namespace t81::experiments::radix
