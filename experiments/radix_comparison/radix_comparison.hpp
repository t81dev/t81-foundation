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
 *   7. Fixed-width stack-backed inline arrays (std::array) to isolate C++ vector heap allocation overhead.
 *   8. Dense Radix-243 packing (5 trits/byte) vs 2-bit packing (4 trits/byte).
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

inline constexpr int trit_to_int(Trit t) noexcept { return static_cast<int>(t); }
inline constexpr Trit int_to_trit(int v) noexcept {
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
// 2. Canonical Balanced Ternary (Dynamic Vector)
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
    res.reserve(45);

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
    __int128_t val = 0;
    __int128_t pow3 = 1;
    for (size_t i = 0; i < trits.size(); ++i) {
      int t = trit_to_int(trits[i]);
      val += static_cast<__int128_t>(t) * pow3;
      pow3 *= 3;
    }
    return static_cast<int64_t>(val);
  }

  // Returns {result, max_carry_chain_length, total_carry_ops}
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

  // Avizienis Radix-3 Carry-Free Addition with stats instrumentation
  // Returns {result, max_transfer_distance, total_transfers_generated}
  static std::tuple<RedundantTernary, double, size_t> add_carry_free_with_stats(
      const RedundantTernary& a, const RedundantTernary& b) {
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
    size_t total_transfers = 0;

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
          total_transfers++;
        }
      } else if (wi <= -2) {
        if (wi == -2 && prev_w > 0) {
          t[i + 1] = 0;
          q[i] = -2;
        } else {
          t[i + 1] = -1;
          q[i] = static_cast<int8_t>(wi + 3);
          total_transfers++;
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

    while (s.size() > 1 && s.back() == 0) s.pop_back();

    double max_transfer_dist = (total_transfers > 0) ? 1.0 : 0.0;
    return {RedundantTernary(s), max_transfer_dist, total_transfers};
  }

  static RedundantTernary add_carry_free(const RedundantTernary& a, const RedundantTernary& b) {
    return std::get<0>(add_carry_free_with_stats(a, b));
  }

  size_t byte_size() const { return digits.size() * sizeof(int8_t); }
};

// ----------------------------------------------------------------------------
// 4. Grouped Radix-9 Integer (Dynamic Vector)
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
    __int128_t val = 0;
    __int128_t pow9 = 1;
    for (size_t i = 0; i < digits.size(); ++i) {
      val += static_cast<__int128_t>(digits[i]) * pow9;
      pow9 *= 9;
    }
    return static_cast<int64_t>(val);
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
// 5. Grouped Radix-27 Integer (Dynamic Vector)
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
    __int128_t val = 0;
    __int128_t pow27 = 1;
    for (size_t i = 0; i < digits.size(); ++i) {
      val += static_cast<__int128_t>(digits[i]) * pow27;
      pow27 *= 27;
    }
    return static_cast<int64_t>(val);
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
// 6. Grouped Radix-81 Integer (Dynamic Vector)
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
    __int128_t val = 0;
    __int128_t pow81 = 1;
    for (size_t i = 0; i < digits.size(); ++i) {
      val += static_cast<__int128_t>(digits[i]) * pow81;
      pow81 *= 81;
    }
    return static_cast<int64_t>(val);
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

// ============================================================================
// 7. Fixed-Width Inline Array Structures (Stack-Backed, std::array)
//    Isolates dynamic C++ heap allocation overhead from pure arithmetic latency.
//    41 trits required for full 64-bit integer domain [-2^63, 2^63 - 1].
// ============================================================================

struct CanonicalTernaryArray {
  static constexpr size_t kMaxTrits = 41;
  std::array<Trit, kMaxTrits> trits{};
  uint8_t len = 1;

  CanonicalTernaryArray() { trits[0] = Trit::Z; }

  static CanonicalTernaryArray from_int64(int64_t x) {
    CanonicalTernaryArray res;
    if (x == 0) return res;

    bool neg = x < 0;
    uint64_t abs_x = neg ? static_cast<uint64_t>(-(x + 1)) + 1 : static_cast<uint64_t>(x);

    uint8_t idx = 0;
    while (abs_x > 0 && idx < kMaxTrits) {
      uint64_t rem = abs_x % 3;
      abs_x /= 3;
      if (rem == 2) {
        res.trits[idx++] = Trit::N;
        abs_x += 1;
      } else {
        res.trits[idx++] = (rem == 1 ? Trit::P : Trit::Z);
      }
    }

    if (neg) {
      for (size_t i = 0; i < idx; ++i) {
        res.trits[i] = (res.trits[i] == Trit::P) ? Trit::N : (res.trits[i] == Trit::N) ? Trit::P : Trit::Z;
      }
    }

    res.len = (idx > 0) ? idx : 1;
    return res;
  }

  int64_t to_int64() const {
    __int128_t val = 0;
    __int128_t pow3 = 1;
    for (size_t i = 0; i < len; ++i) {
      int t = trit_to_int(trits[i]);
      val += static_cast<__int128_t>(t) * pow3;
      pow3 *= 3;
    }
    return static_cast<int64_t>(val);
  }

  static CanonicalTernaryArray add(const CanonicalTernaryArray& a, const CanonicalTernaryArray& b) {
    CanonicalTernaryArray res;
    uint8_t max_s = std::max(a.len, b.len);
    int carry = 0;
    uint8_t i = 0;

    for (; (i < max_s || carry != 0) && i < kMaxTrits; ++i) {
      int sum = carry;
      if (i < a.len) sum += trit_to_int(a.trits[i]);
      if (i < b.len) sum += trit_to_int(b.trits[i]);

      if (sum >= 2) {
        res.trits[i] = int_to_trit(sum - 3);
        carry = 1;
      } else if (sum <= -2) {
        res.trits[i] = int_to_trit(sum + 3);
        carry = -1;
      } else {
        res.trits[i] = int_to_trit(sum);
        carry = 0;
      }
    }

    res.len = (i > 0) ? i : 1;
    while (res.len > 1 && res.trits[res.len - 1] == Trit::Z) res.len--;
    return res;
  }

  size_t byte_size() const { return len * sizeof(Trit); }
};

struct Radix81Array {
  static constexpr size_t kMaxTrytes = 11; // 41 trits / 4 = 11 trytes
  std::array<int8_t, kMaxTrytes> digits{};
  uint8_t len = 1;

  Radix81Array() = default;

  static Radix81Array from_int64(int64_t x) {
    CanonicalTernaryArray ct = CanonicalTernaryArray::from_int64(x);
    Radix81Array res;
    uint8_t tryte_idx = 0;
    for (size_t i = 0; i < ct.len; i += 4) {
      int d0 = trit_to_int(ct.trits[i]);
      int d1 = (i + 1 < ct.len) ? trit_to_int(ct.trits[i + 1]) : 0;
      int d2 = (i + 2 < ct.len) ? trit_to_int(ct.trits[i + 2]) : 0;
      int d3 = (i + 3 < ct.len) ? trit_to_int(ct.trits[i + 3]) : 0;
      res.digits[tryte_idx++] = static_cast<int8_t>(d0 + d1 * 3 + d2 * 9 + d3 * 27);
    }
    res.len = (tryte_idx > 0) ? tryte_idx : 1;
    return res;
  }

  int64_t to_int64() const {
    __int128_t val = 0;
    __int128_t pow81 = 1;
    for (size_t i = 0; i < len; ++i) {
      val += static_cast<__int128_t>(digits[i]) * pow81;
      pow81 *= 81;
    }
    return static_cast<int64_t>(val);
  }

  static Radix81Array add(const Radix81Array& a, const Radix81Array& b) {
    Radix81Array res;
    uint8_t max_s = std::max(a.len, b.len);
    int carry = 0;
    uint8_t i = 0;

    for (; (i < max_s || carry != 0) && i < kMaxTrytes; ++i) {
      int sum = carry;
      if (i < a.len) sum += a.digits[i];
      if (i < b.len) sum += b.digits[i];

      if (sum > 40) {
        res.digits[i] = static_cast<int8_t>(sum - 81);
        carry = 1;
      } else if (sum < -40) {
        res.digits[i] = static_cast<int8_t>(sum + 81);
        carry = -1;
      } else {
        res.digits[i] = static_cast<int8_t>(sum);
        carry = 0;
      }
    }

    res.len = (i > 0) ? i : 1;
    while (res.len > 1 && res.digits[res.len - 1] == 0) res.len--;
    return res;
  }

  size_t byte_size() const { return len * sizeof(int8_t); }
};

struct Radix27Array {
  static constexpr size_t kMaxDigits = 14; // 41 trits / 3 = 14 digits
  std::array<int8_t, kMaxDigits> digits{};
  uint8_t len = 1;

  static Radix27Array from_int64(int64_t x) {
    CanonicalTernaryArray ct = CanonicalTernaryArray::from_int64(x);
    Radix27Array res;
    uint8_t idx = 0;
    for (size_t i = 0; i < ct.len; i += 3) {
      int d0 = trit_to_int(ct.trits[i]);
      int d1 = (i + 1 < ct.len) ? trit_to_int(ct.trits[i + 1]) : 0;
      int d2 = (i + 2 < ct.len) ? trit_to_int(ct.trits[i + 2]) : 0;
      res.digits[idx++] = static_cast<int8_t>(d0 + d1 * 3 + d2 * 9);
    }
    res.len = (idx > 0) ? idx : 1;
    return res;
  }

  int64_t to_int64() const {
    __int128_t val = 0;
    __int128_t pow27 = 1;
    for (size_t i = 0; i < len; ++i) {
      val += static_cast<__int128_t>(digits[i]) * pow27;
      pow27 *= 27;
    }
    return static_cast<int64_t>(val);
  }

  static Radix27Array add(const Radix27Array& a, const Radix27Array& b) {
    Radix27Array res;
    uint8_t max_s = std::max(a.len, b.len);
    int carry = 0;
    uint8_t i = 0;

    for (; (i < max_s || carry != 0) && i < kMaxDigits; ++i) {
      int sum = carry;
      if (i < a.len) sum += a.digits[i];
      if (i < b.len) sum += b.digits[i];

      if (sum > 13) {
        res.digits[i] = static_cast<int8_t>(sum - 27);
        carry = 1;
      } else if (sum < -13) {
        res.digits[i] = static_cast<int8_t>(sum + 27);
        carry = -1;
      } else {
        res.digits[i] = static_cast<int8_t>(sum);
        carry = 0;
      }
    }

    res.len = (i > 0) ? i : 1;
    while (res.len > 1 && res.digits[res.len - 1] == 0) res.len--;
    return res;
  }

  size_t byte_size() const { return len * sizeof(int8_t); }
};

struct Radix9Array {
  static constexpr size_t kMaxDigits = 21; // 41 trits / 2 = 21 digits
  std::array<int8_t, kMaxDigits> digits{};
  uint8_t len = 1;

  static Radix9Array from_int64(int64_t x) {
    CanonicalTernaryArray ct = CanonicalTernaryArray::from_int64(x);
    Radix9Array res;
    uint8_t idx = 0;
    for (size_t i = 0; i < ct.len; i += 2) {
      int d0 = trit_to_int(ct.trits[i]);
      int d1 = (i + 1 < ct.len) ? trit_to_int(ct.trits[i + 1]) : 0;
      res.digits[idx++] = static_cast<int8_t>(d0 + d1 * 3);
    }
    res.len = (idx > 0) ? idx : 1;
    return res;
  }

  int64_t to_int64() const {
    __int128_t val = 0;
    __int128_t pow9 = 1;
    for (size_t i = 0; i < len; ++i) {
      val += static_cast<__int128_t>(digits[i]) * pow9;
      pow9 *= 9;
    }
    return static_cast<int64_t>(val);
  }

  static Radix9Array add(const Radix9Array& a, const Radix9Array& b) {
    Radix9Array res;
    uint8_t max_s = std::max(a.len, b.len);
    int carry = 0;
    uint8_t i = 0;

    for (; (i < max_s || carry != 0) && i < kMaxDigits; ++i) {
      int sum = carry;
      if (i < a.len) sum += a.digits[i];
      if (i < b.len) sum += b.digits[i];

      if (sum > 4) {
        res.digits[i] = static_cast<int8_t>(sum - 9);
        carry = 1;
      } else if (sum < -4) {
        res.digits[i] = static_cast<int8_t>(sum + 9);
        carry = -1;
      } else {
        res.digits[i] = static_cast<int8_t>(sum);
        carry = 0;
      }
    }

    res.len = (i > 0) ? i : 1;
    while (res.len > 1 && res.digits[res.len - 1] == 0) res.len--;
    return res;
  }

  size_t byte_size() const { return len * sizeof(int8_t); }
};

struct RedundantTernaryArray {
  static constexpr size_t kMaxDigits = 45;
  std::array<int8_t, kMaxDigits> digits{};
  uint8_t len = 1;

  static RedundantTernaryArray from_int64(int64_t x) {
    CanonicalTernaryArray ct = CanonicalTernaryArray::from_int64(x);
    RedundantTernaryArray res;
    for (size_t i = 0; i < ct.len && i < kMaxDigits; ++i) {
      res.digits[i] = static_cast<int8_t>(trit_to_int(ct.trits[i]));
    }
    res.len = ct.len;
    return res;
  }

  int64_t to_int64() const {
    // Convert to canonical representation
    CanonicalTernaryArray ct;
    int carry = 0;
    uint8_t i = 0;
    for (; (i < len || carry != 0) && i < CanonicalTernaryArray::kMaxTrits; ++i) {
      int sum = carry;
      if (i < len) sum += digits[i];

      if (sum >= 2) {
        ct.trits[i] = int_to_trit(sum - 3);
        carry = 1;
      } else if (sum <= -2) {
        ct.trits[i] = int_to_trit(sum + 3);
        carry = -1;
      } else {
        ct.trits[i] = int_to_trit(sum);
        carry = 0;
      }
    }
    ct.len = (i > 0) ? i : 1;
    while (ct.len > 1 && ct.trits[ct.len - 1] == Trit::Z) ct.len--;
    return ct.to_int64();
  }

  static RedundantTernaryArray add_carry_free(const RedundantTernaryArray& a, const RedundantTernaryArray& b) {
    RedundantTernaryArray res;
    uint8_t max_input = std::max(a.len, b.len);
    uint8_t n = max_input + 2;
    if (n >= kMaxDigits) n = kMaxDigits - 1;

    std::array<int8_t, kMaxDigits> w{};
    for (size_t i = 0; i < n; ++i) {
      int8_t da = (i < a.len) ? a.digits[i] : 0;
      int8_t db = (i < b.len) ? b.digits[i] : 0;
      w[i] = da + db;
    }

    std::array<int8_t, kMaxDigits> t{};
    std::array<int8_t, kMaxDigits> q{};

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

    for (size_t i = 0; i < n; ++i) {
      res.digits[i] = static_cast<int8_t>(q[i] + t[i]);
    }
    if (n < kMaxDigits) {
      res.digits[n] = t[n];
      res.len = n + 1;
    } else {
      res.len = n;
    }

    while (res.len > 1 && res.digits[res.len - 1] == 0) res.len--;
    return res;
  }

  size_t byte_size() const { return len * sizeof(int8_t); }
};

// ============================================================================
// 8. Base-243 Packing & Unpacking Analysis Functions
//    Packs 5 trits into a single byte (3^5 = 243 <= 256). State efficiency 94.92%.
// ============================================================================

inline std::vector<uint8_t> pack_base243(const std::vector<Trit>& trits) {
  std::vector<uint8_t> packed;
  packed.reserve((trits.size() + 4) / 5);

  for (size_t i = 0; i < trits.size(); i += 5) {
    int val = 0;
    int pow3 = 1;
    for (size_t j = 0; j < 5; ++j) {
      int t_val = (i + j < trits.size()) ? (trit_to_int(trits[i + j]) + 1) : 1; // Map {-1,0,1} -> {0,1,2}
      val += t_val * pow3;
      pow3 *= 3;
    }
    packed.push_back(static_cast<uint8_t>(val));
  }
  return packed;
}

inline std::vector<Trit> unpack_base243(const std::vector<uint8_t>& bytes, size_t original_trit_count) {
  std::vector<Trit> trits;
  trits.reserve(original_trit_count);

  for (uint8_t b : bytes) {
    int val = static_cast<int>(b);
    for (size_t j = 0; j < 5; ++j) {
      if (trits.size() >= original_trit_count) break;
      int rem = val % 3;
      val /= 3;
      trits.push_back(int_to_trit(rem - 1)); // Map {0,1,2} -> {-1,0,1}
    }
  }
  return trits;
}

} // namespace t81::experiments::radix
