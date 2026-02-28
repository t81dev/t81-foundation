/**
 * @file T81BigInt.hpp
 * @brief T81BigInt — high-level ternary integer wrapper backed by T81Int.
 *
 * Design notes:
 *   • Internally stores a sign bit and one or more T81Int "limbs".
 *   • Supports arbitrary-precision arithmetic via multi-limb representation.
 *   • Multiplication uses Karatsuba algorithm for large values.
 *   • Division uses Knuth's Algorithm D over normalized chunks.
 *   • Addition/Subtraction uses chunk-based carry propagation.
 */

#pragma once

#include "t81/types/T81Float.hpp"
#include "t81/types/T81Int.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81::v1 {

__extension__ typedef __int128 int128_t;

namespace detail {
inline const std::array<int16_t, 256>& get_byte_to_ternary() {
  static const auto table = []() {
    std::array<int16_t, 256> t{};
    for (int i = 0; i < 256; ++i) {
      int val = 0;
      int p3 = 1;
      for (int j = 0; j < 4; ++j) {
        int u = (i >> (j * 2)) & 0x3;
        if (u > 2) u = 1;  // Treat invalid as Zero
        val += (u - 1) * p3;
        p3 *= 3;
      }
      t[i] = static_cast<int16_t>(val);
    }
    return t;
  }();
  return table;
}

inline const std::array<uint8_t, 81>& get_ternary_to_packed() {
  static const auto table = []() {
    std::array<uint8_t, 81> t{};
    for (int i = 0; i < 81; ++i) {
      int val = 0;
      int tmp = i;
      for (int j = 0; j < 4; ++j) {
        int d = tmp % 3;
        tmp /= 3;
        val |= (d << (j * 2));
      }
      t[i] = static_cast<uint8_t>(val);
    }
    return t;
  }();
  return table;
}

inline const std::array<int32_t, 65536>& get_word_to_ternary() {
  static const auto table = []() {
    std::array<int32_t, 65536> t{};
    const auto& b2t = get_byte_to_ternary();
    for (int i = 0; i < 65536; ++i) {
      int lo = i & 0xFF;
      int hi = (i >> 8) & 0xFF;
      t[i] = static_cast<int32_t>(b2t[lo]) + static_cast<int32_t>(b2t[hi]) * 81;
    }
    return t;
  }();
  return table;
}

inline const std::vector<std::string>& base81_alphabet_vec() {
  static const std::vector<std::string> kAlphabet = {
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G",
      "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X",
      "Y", "Z", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
      "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "+", "−", "×", "÷", "=", "<",
      ">", "≤", "≥", "≠", "≈", "∞", "λ", "μ", "π", "σ", "τ", "ω", "Γ"};
  return kAlphabet;
}

inline const std::unordered_map<std::string, int>& base81_digit_map() {
  static const std::unordered_map<std::string, int> kMap = [] {
    std::unordered_map<std::string, int> m;
    const auto& alpha = base81_alphabet_vec();
    m.reserve(alpha.size());
    for (std::size_t i = 0; i < alpha.size(); ++i) {
      m.emplace(alpha[i], static_cast<int>(i));
    }
    return m;
  }();
  return kMap;
}

inline std::string next_codepoint(std::string_view s, std::size_t& offset) {
  if (offset >= s.size()) return {};
  const unsigned char c = static_cast<unsigned char>(s[offset]);
  std::size_t len = 0;
  if (c < 0x80)
    len = 1;
  else if ((c & 0xE0) == 0xC0)
    len = 2;
  else if ((c & 0xF0) == 0xE0)
    len = 3;
  else if ((c & 0xF8) == 0xF0)
    len = 4;
  else
    return {};
  if (offset + len > s.size()) return {};
  std::string cp(s.substr(offset, len));
  offset += len;
  return cp;
}
}  // namespace detail

class T81BigInt {
  friend class BigIntAllocationPathologyTest;
  friend class BigIntAllocationGuardrailTest;

public:
  using size_type = std::size_t;
  static constexpr size_type kLimbTrits = 81;

  using Limb = T81Int<kLimbTrits>;

private:
  // Invariant (current implementation):
  //   • limbs_.size() >= 1
  //   • limbs_[0] encodes the entire magnitude and is non-negative
  //   • For zero, limbs_[0] == 0 and negative_ == false
  std::vector<Limb> limbs_;
  bool negative_ = false;

  void verify_invariants() const {
#ifndef NDEBUG
    if (limbs_.empty()) {
      // Must have at least one limb
      std::terminate();
    }
    // No leading zero limbs unless it's the only one
    if (limbs_.size() > 1 && limbs_.back().is_zero()) {
      std::terminate();
    }
    // Canonical zero: {0}, positive sign
    if (limbs_.size() == 1 && limbs_[0].is_zero()) {
      if (negative_) std::terminate();
    }
    // Ensure magnitude is strictly non-negative.
    // The most significant limb determines the sign of the magnitude.
    if (!limbs_.empty() && limbs_.back().sign_trit() == Trit::N) {
      std::terminate();
    }
    // Limbs should be valid T81Ints (implicitly true by type)
#endif
  }

  void normalize() {
    if (limbs_.empty()) {
      limbs_.emplace_back(0);
      negative_ = false;
      return;
    }

    // Trim leading zero limbs (except the first one if it's the only one)
    while (limbs_.size() > 1 && limbs_.back().is_zero()) {
      limbs_.pop_back();
    }

    if (limbs_.size() == 1 && limbs_[0].is_zero()) {
      negative_ = false;
    }

    verify_invariants();
  }

  void assign_from_int64(std::int64_t v) {
    limbs_.clear();
    if (v < 0) {
      negative_ = true;
      if (v == std::numeric_limits<std::int64_t>::min()) {
        // Handle INT64_MIN: magnitude is 2^63.
        // Construct 2^63 - 1 (MAX) then add 1 to get +2^63 in T81Int.
        // This avoids the issue where casting +2^63 back to int64_t wraps to negative.
        Limb l(std::numeric_limits<std::int64_t>::max());
        l = l + Limb(1);
        limbs_.push_back(l);
      } else {
        limbs_.emplace_back(-v);
      }
    } else {
      negative_ = false;
      limbs_.emplace_back(v);
    }
    normalize();
  }

public:
  // ------------------------------------------------------------------
  // Constructors
  // ------------------------------------------------------------------

  T81BigInt() {
    limbs_.emplace_back(0);
    negative_ = false;
  }

  T81BigInt(const T81BigInt&) = default;
  T81BigInt(T81BigInt&&) noexcept = default;
  T81BigInt& operator=(const T81BigInt&) = default;
  T81BigInt& operator=(T81BigInt&&) noexcept = default;

  explicit T81BigInt(std::int64_t v) { assign_from_int64(v); }

private:
  Trit lowest_trit_magnitude() const {
    if (is_zero()) return Trit::Z;
    return limbs_[0][0];
  }

  Trit lowest_trit() const {
    if (is_zero()) return Trit::Z;
    Trit t = limbs_[0][0];
    if (negative_) return (t == Trit::P) ? Trit::N : ((t == Trit::N) ? Trit::P : Trit::Z);
    return t;
  }

  // Divide magnitude by 3 (shift right 1 trit).
  void div3_magnitude() {
    if (is_zero()) return;

    Trit carry = Trit::Z;
    for (size_t i = limbs_.size(); i-- > 0;) {
      Trit next_carry = limbs_[i][0];
      limbs_[i] >>= 1;
      if (carry != Trit::Z) {
        limbs_[i][kLimbTrits - 1] = carry;
      }
      carry = next_carry;
    }
    normalize();
  }

  // Signed in-place division by 3.
  // Value must be divisible by 3 for exact results.
  void div3_inplace() { div3_magnitude(); }

public:
  template <std::size_t N>
  explicit T81BigInt(const T81Int<N>& x) {
    if constexpr (N <= 40) {
      const std::int64_t v = x.to_int64();  // may throw on overflow
      assign_from_int64(v);
      return;
    }

    if (x.is_zero()) {
      limbs_.emplace_back(0);
      negative_ = false;
      return;
    }

    // T81Int is signed balanced ternary.
    bool is_neg = (x.sign_trit() == Trit::N);
    negative_ = is_neg;

    // Use absolute magnitude for limbs
    T81Int<N> abs_x = is_neg ? -x : x;

    limbs_.clear();
    // Pack into 81-trit limbs
    for (size_t i = 0; i < N; i += kLimbTrits) {
      Limb l;
      size_t count = std::min(N - i, kLimbTrits);
      for (size_t j = 0; j < count; ++j) {
        l[j] = abs_x[i + j];
      }
      limbs_.push_back(l);
    }
    normalize();
  }

  template <std::size_t M, std::size_t E>
  T81Float<M, E> to_float() const {
    using Float = T81Float<M, E>;
    if (is_zero()) return Float::zero();

    size_t num_limbs = limbs_.size();
    size_t last_limb_trits = limbs_.back().significant_trits();
    if (last_limb_trits == 0) return Float::zero();

    size_t total_trits = (num_limbs - 1) * kLimbTrits + last_limb_trits;
    size_t msb_index = total_trits - 1;

    constexpr size_t Guard = 4;
    T81Int<M + Guard> mant;

    size_t S = 0;
    if (total_trits > M + Guard) {
      S = total_trits - (M + Guard);
    }

    for (size_t i = 0; i < M + Guard; ++i) {
      size_t bit_idx = i + S;
      if (bit_idx > msb_index) break;

      size_t limb_idx = bit_idx / kLimbTrits;
      size_t offset = bit_idx % kLimbTrits;
      if (limb_idx < limbs_.size()) {
        mant[i] = limbs_[limb_idx][offset];
      }
    }

    // normalize expects exp such that val = mant * 3^(exp - (M-1))
    // Our mant matches V >> S
    // val ≈ mant * 3^S
    // exp - M + 1 = S => exp = S + M - 1
    return Float::normalize(negative_ ? Trit::N : Trit::P, static_cast<int64_t>(S + M - 1), mant);
  }

  template <std::size_t N>
  std::optional<T81Int<N>> try_to_int() const {
    if (is_zero()) return T81Int<N>();

    // Check magnitude size
    size_t sig_trits = 0;
    if (!limbs_.empty()) {
      size_t last = limbs_.back().significant_trits();
      if (last > 0) sig_trits = (limbs_.size() - 1) * kLimbTrits + last;
    }

    if (sig_trits > N) {
      return std::nullopt;
    }

    T81Int<N> res;
    // Copy trits
    for (size_t i = 0; i < N; ++i) {
      size_t limb_idx = i / kLimbTrits;
      size_t offset = i % kLimbTrits;
      Trit t = Trit::Z;
      if (limb_idx < limbs_.size()) {
        t = limbs_[limb_idx][offset];
      }
      res[i] = t;
    }

    return negative_ ? -res : res;
  }

  template <std::size_t N>
  T81Int<N> to_int() const {
    auto res = try_to_int<N>();
    if (!res) {
      t81::axion::trap_overflow("T81BigInt::to_int: value too large for T81Int<N>");
    }
    return *res;
  }

  template <std::size_t M, std::size_t E>
  static T81BigInt from_float(const T81Float<M, E>& f) {
    if (f.is_zero()) return T81BigInt::zero();
    if (f.is_nae()) throw std::domain_error("from_float: NaE");
    if (f.is_inf()) throw std::domain_error("from_float: Inf");

    T81Int<M> m = f.get_mantissa();
    std::int64_t e = f.get_exp();

    // Value = m * 3^(e - M + 1)
    std::int64_t shift = e - static_cast<std::int64_t>(M) + 1;

    // m is positive magnitude from Float (sign is separate)
    T81BigInt bm(m);

    if (shift >= 0) {
      return (f.is_negative() ? -bm : bm) * T81BigInt::pow(T81BigInt(3), T81BigInt(shift));
    } else {
      T81BigInt den = T81BigInt::pow(T81BigInt(3), T81BigInt(-shift));
      auto [q, r] = div_mod(bm, den);

      // Round to nearest (half up)
      if (T81BigInt(2) * r >= den) {
        q = q + T81BigInt(1);
      }
      return f.is_negative() ? -q : q;
    }
  }

  // Factory helpers
  static T81BigInt zero() { return T81BigInt(0); }

  static T81BigInt one() { return T81BigInt(1); }

  static T81BigInt from_int64(std::int64_t v) { return T81BigInt(v); }

  static T81BigInt from_i64(std::int64_t v) { return from_int64(v); }

  // ------------------------------------------------------------------
  // Int64 conversion
  // ------------------------------------------------------------------

  [[nodiscard]] std::int64_t to_int64() const {
    if (limbs_.empty()) {
      throw std::logic_error("T81BigInt::to_int64: no limbs");
    }
    if (limbs_.size() > 1) {
      t81::axion::trap_overflow("T81BigInt::to_int64: value too large");
    }

    try {
      const std::int64_t mag = limbs_[0].to_int64();  // magnitude
      if (negative_) {
        return -mag;
      }
      return mag;
    } catch (const std::overflow_error&) {
      // Check for INT64_MIN case: magnitude is 2^63.
      // T81Int(INT64_MIN) creates the magnitude 2^63 representation.
      if (negative_ && limbs_[0] == -Limb(std::numeric_limits<std::int64_t>::min())) {
        return std::numeric_limits<std::int64_t>::min();
      }
      t81::axion::trap_overflow("T81BigInt::to_int64: overflow");
    }
  }

  // ------------------------------------------------------------------
  // Basic predicates and helpers
  // ------------------------------------------------------------------

  [[nodiscard]] bool is_zero() const noexcept {
    return limbs_.size() == 1 && limbs_[0].is_zero() && !negative_;
  }

  [[nodiscard]] bool is_negative() const noexcept { return negative_ && !is_zero(); }

  [[nodiscard]] T81BigInt abs() const {
    T81BigInt r = *this;
    r.negative_ = false;
    return r;
  }

  // Balanced-ternary string representation.
  // Digits: '-', '0', '+'
  [[nodiscard]] std::string str() const {
    if (is_zero()) {
      return "0";
    }

    std::string s;
    for (size_t i = 0; i < limbs_.size(); ++i) {
      for (size_t t = 0; t < kLimbTrits; ++t) {
        Trit tr = limbs_[i][t];
        // Magnitude trits are always non-negative in our canonical form
        // after addition/subtraction adjustment.
        if (tr == Trit::P)
          s.push_back('+');
        else if (tr == Trit::N)
          s.push_back('-');
        else
          s.push_back('0');
      }
    }

    // Trim leading zeros (at the end of the string before reverse)
    while (s.size() > 1 && s.back() == '0') s.pop_back();

    if (negative_) {
      s.push_back('-');
    }

    std::reverse(s.begin(), s.end());
    return s;
  }

  // ------------------------------------------------------------------
  // Compatibility helpers
  // ------------------------------------------------------------------
  static T81BigInt add(const T81BigInt& a, const T81BigInt& b) { return a + b; }
  static T81BigInt sub(const T81BigInt& a, const T81BigInt& b) { return a - b; }
  static T81BigInt mul(const T81BigInt& a, const T81BigInt& b) { return a * b; }
  static T81BigInt div(const T81BigInt& a, const T81BigInt& b) { return a / b; }
  static T81BigInt mod(const T81BigInt& a, const T81BigInt& b) { return a % b; }
  static T81BigInt abs(const T81BigInt& a) { return a.abs(); }
  static T81BigInt neg(const T81BigInt& a) { return -a; }

  static bool is_zero(const T81BigInt& a) { return a.is_zero(); }
  static bool is_neg(const T81BigInt& a) { return a.is_negative(); }
  static bool is_one(const T81BigInt& a) { return a == T81BigInt(1); }

  static int cmp(const T81BigInt& a, const T81BigInt& b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
  }

  // ------------------------------------------------------------------
  // Comparison (multi-limb balanced ternary)
  // ------------------------------------------------------------------

  static int compare_magnitudes(const std::vector<Limb>& a, const std::vector<Limb>& b) {
    if (a.size() != b.size()) {
      return (a.size() < b.size()) ? -1 : 1;
    }
    for (size_t i = a.size(); i-- > 0;) {
      if (a[i] < b[i]) return -1;
      if (a[i] > b[i]) return 1;
    }
    return 0;
  }

  [[nodiscard]] bool operator==(const T81BigInt& other) const {
    if (is_zero() && other.is_zero()) {
      return true;
    }
    return negative_ == other.negative_ && limbs_ == other.limbs_;
  }

  [[nodiscard]] bool operator!=(const T81BigInt& other) const { return !(*this == other); }

  [[nodiscard]] bool operator<(const T81BigInt& other) const {
    if (negative_ != other.negative_) {
      return negative_;
    }
    if (is_zero()) return !other.is_zero() && !other.negative_;
    if (other.is_zero()) return negative_;

    int cmp = compare_magnitudes(limbs_, other.limbs_);
    if (negative_) return cmp > 0;
    return cmp < 0;
  }

  [[nodiscard]] bool operator>(const T81BigInt& other) const { return other < *this; }

  [[nodiscard]] bool operator<=(const T81BigInt& other) const { return !(*this > other); }

  [[nodiscard]] bool operator>=(const T81BigInt& other) const { return !(*this < other); }

  // ------------------------------------------------------------------
  // Arithmetic (multi-limb balanced ternary)
  // ------------------------------------------------------------------

  static constexpr int64_t B = 7625597484987LL;  // 3^27
  static constexpr int64_t halfB = (B - 1) / 2;

  static std::vector<int64_t> to_chunks(const T81BigInt& x) {
    const auto& wtable = detail::get_word_to_ternary();
    std::vector<int64_t> chunks;
    // Reserve extra space to avoid reallocation in to_std_chunks
    chunks.reserve(x.limbs_.size() * 3 + 8);

    static constexpr int64_t p3_8[] = {1LL, 6561LL, 43046721LL, 282429536481LL};

    for (const auto& limb : x.limbs_) {
      const auto& data = limb.raw_data();
      const uint8_t* d = data.data();

      auto read_u16 = [](const uint8_t* p) {
        uint16_t val;
        std::memcpy(&val, p, 2);
        return val;
      };

      // Chunk 0: trits 0-26
      int64_t c0 = (int64_t)wtable[read_u16(&d[0])] * p3_8[0] +
                   (int64_t)wtable[read_u16(&d[2])] * p3_8[1] +
                   (int64_t)wtable[read_u16(&d[4])] * p3_8[2];
      int b6 = d[6] & 0x3F;
      int64_t v24_26 = 0;
      int p3 = 1;
      for (int j = 0; j < 3; ++j) {
        int u = (b6 >> (j * 2)) & 0x3;
        if (u > 2) u = 1;
        v24_26 += (u - 1) * p3;
        p3 *= 3;
      }
      c0 += v24_26 * 282429536481LL;
      if (x.negative_) c0 = -c0;
      chunks.push_back(c0);

      // Chunk 1: trits 27-53
      int t27 = (d[6] >> 6) & 0x3;
      if (t27 > 2) t27 = 1;
      int64_t c1 = (int64_t)(t27 - 1);
      c1 += (int64_t)wtable[read_u16(&d[7])] * 3LL;
      c1 += (int64_t)wtable[read_u16(&d[9])] * 19683LL;
      c1 += (int64_t)wtable[read_u16(&d[11])] * 129140163LL;
      int b13_0_3 = d[13] & 0x0F;
      int t52 = (b13_0_3 & 0x3);
      if (t52 > 2) t52 = 1;
      int t53 = (b13_0_3 >> 2) & 0x3;
      if (t53 > 2) t53 = 1;
      c1 += (int64_t)(t52 - 1) * 847288609443LL;
      c1 += (int64_t)(t53 - 1) * 2541865828329LL;
      if (x.negative_) c1 = -c1;
      chunks.push_back(c1);

      // Chunk 2: trits 54-80
      int b13_4_7 = (d[13] >> 4) & 0x0F;
      int t54 = (b13_4_7 & 0x3);
      if (t54 > 2) t54 = 1;
      int t55 = (b13_4_7 >> 2) & 0x3;
      if (t55 > 2) t55 = 1;
      int64_t c2 = (int64_t)(t54 - 1);
      c2 += (int64_t)(t55 - 1) * 3LL;
      c2 += (int64_t)wtable[read_u16(&d[14])] * 9LL;
      c2 += (int64_t)wtable[read_u16(&d[16])] * 59049LL;
      c2 += (int64_t)wtable[read_u16(&d[18])] * 387420489LL;
      int t80 = d[20] & 0x03;
      if (t80 > 2) t80 = 1;
      c2 += (int64_t)(t80 - 1) * 2541865828329LL;
      if (x.negative_) c2 = -c2;
      chunks.push_back(c2);
    }
    return chunks;
  }

  static T81BigInt from_chunks(std::vector<int64_t> chunks) {
    // 1. Normalize carries
    int64_t carry = 0;
    for (size_t j = 0; j < chunks.size() || carry != 0; ++j) {
      if (j >= chunks.size()) chunks.push_back(0);
      int64_t sum = chunks[j] + carry;
      if (sum > halfB) {
        chunks[j] = sum - B;
        carry = 1;
      } else if (sum < -halfB) {
        chunks[j] = sum + B;
        carry = -1;
      } else {
        chunks[j] = sum;
        carry = 0;
      }
    }

    // 2. Check sign
    T81BigInt res;
    res.limbs_.clear();
    int64_t last = 0;
    for (size_t i = chunks.size(); i-- > 0;)
      if (chunks[i] != 0) {
        last = chunks[i];
        break;
      }

    if (last < 0) {
      res.negative_ = true;
      // Negate value in balanced ternary
      int64_t c_neg = 0;
      for (auto& v : chunks) {
        int64_t val = -v + c_neg;
        if (val > halfB) {
          v = val - B;
          c_neg = 1;
        } else if (val < -halfB) {
          v = val + B;
          c_neg = -1;
        } else {
          v = val;
          c_neg = 0;
        }
      }
    } else {
      res.negative_ = false;
    }

    // 3. Pack into limbs
    const auto& packed_table = detail::get_ternary_to_packed();
    for (size_t i = 0; i < chunks.size(); i += 3) {
      Limb l;
      auto& ldata = const_cast<std::array<uint8_t, Limb::kNumBytes>&>(l.raw_data());
      std::fill(ldata.begin(), ldata.end(), 0x55u);

      auto set_chunk_in_limb = [&](int c, int64_t v) {
        static constexpr int64_t offset27 = 3812798742493LL;
        uint64_t uv = static_cast<uint64_t>(v + offset27);
        int start_trit = c * 27;
        for (int j = 0; j < 6; ++j) {
          int r = uv % 81;
          uv /= 81;
          uint8_t packed = packed_table[r];
          for (int t = 0; t < 4; ++t) {
            int u = (packed >> (t * 2)) & 0x3;
            l[start_trit + j * 4 + t] = static_cast<Trit>(u - 1);
          }
        }
        for (int t = 24; t < 27; ++t) {
          int r = uv % 3;
          uv /= 3;
          l[start_trit + t] = static_cast<Trit>(r - 1);
        }
      };

      for (int c = 0; c < 3; ++c) {
        int64_t v = (i + c < chunks.size()) ? chunks[i + c] : 0;
        // chunks already represent positive magnitude here, so no negation needed
        set_chunk_in_limb(c, v);
      }
      res.limbs_.push_back(l);
    }
    res.normalize();
    return res;
  }

  // Converts balanced chunks to standard non-negative chunks [0, B-1].
  static std::vector<int64_t> to_std_chunks(std::vector<int64_t> chunks) {
    // Reserve space to avoid reallocation during push_back
    chunks.reserve(chunks.size() + 4);

    // Safety limit to prevent infinite loops on effectively negative inputs
    const size_t limit = chunks.size() + 4;
    for (size_t i = 0; i < chunks.size(); ++i) {
      if (chunks.size() > limit) {
        throw std::logic_error("to_std_chunks: borrow beyond MSB (input effectively negative)");
      }
      while (chunks[i] < 0) {
        chunks[i] += B;
        if (i + 1 >= chunks.size()) {
          chunks.push_back(0);
        }
        chunks[i + 1] -= 1;
      }
      // Also normalize carry-out if chunk >= B (though typical balanced conversion only has
      // negatives) But if we have 0, -1 -> B, -1-1 = -2. If we have {B+1}, we need to carry over.
      while (chunks[i] >= B) {
        chunks[i] -= B;
        if (i + 1 >= chunks.size()) {
          chunks.push_back(0);
        }
        chunks[i + 1] += 1;
      }
    }
    // Trim leading zeros
    // But for to_std_chunks, we are preparing for division logic that expects
    // non-negative chunks in standard base B.
    // The last chunk MUST be non-zero if size > 1.
    while (chunks.size() > 1 && chunks.back() == 0) chunks.pop_back();

    // Ensure no negative chunks remain (this should have been handled by the borrow loop,
    // but if the MSB was negative, we might have an issue).
    // Since input is abs(), it should be positive.
    // If MSB is 0 after borrow loop (e.g. from {1, -1} -> {0, B-1}), we trim.
    // If MSB is negative, it means the number was negative, which violates abs() input
    // precondition.

    if (chunks.empty()) {
      chunks.push_back(0);
    }

    return chunks;
  }

  static std::pair<std::vector<int64_t>, std::vector<int64_t>> div_mod_std(std::vector<int64_t> u,
                                                                           std::vector<int64_t> v) {
    if (v.empty() || (v.size() == 1 && v[0] == 0)) throw std::domain_error("division by zero");

    // Knuth Algorithm D
    if (v.size() == 1) {
      int64_t divisor = v[0];
      int64_t rem = 0;
      for (size_t i = u.size(); i-- > 0;) {
        int128_t cur = (int128_t)rem * B + u[i];
        u[i] = (int64_t)(cur / divisor);
        rem = (int64_t)(cur % divisor);
      }
      while (u.size() > 1 && u.back() == 0) u.pop_back();
      if (u.empty()) u.push_back(0);
      return {std::move(u), {rem}};
    }

    int n = (int)v.size();
    int m = (int)u.size() - n;

    if (m < 0) return {{0}, u};

    int64_t d = B / (v.back() + 1);
    auto mul_scalar = [](std::vector<int64_t>& vec, int64_t s) {
      // Reserve space for potential carry
      vec.reserve(vec.size() + 1);
      int64_t carry = 0;
      for (auto& val : vec) {
        int128_t prod = (int128_t)val * s + carry;
        val = (int64_t)(prod % B);
        carry = (int64_t)(prod / B);
      }
      if (carry) vec.push_back(carry);
    };

    // Pre-reserve u for scaling + padding
    u.reserve(u.size() + 2);
    mul_scalar(u, d);
    mul_scalar(v, d);

    // Update sizes after scaling. v size shouldn't change if d is optimal, but u might grow.
    n = (int)v.size();
    m = (int)u.size() - n;

    // Knuth Algorithm D requires u to have size m + n + 1 (indices 0..m+n).
    // Currently u.size() == m + n. We need one extra zero at the most significant position.
    // We already reserved space for this.
    u.push_back(0);

    std::vector<int64_t> q(m + 1, 0);

    for (int j = m; j >= 0; --j) {
      int128_t num = (int128_t)u[j + n] * B + u[j + n - 1];
      int64_t den = v[n - 1];
      int64_t qhat = (den == 0) ? B - 1 : (int64_t)(num / den);
      int64_t rhat = (den == 0) ? 0 : (int64_t)(num % den);

      while (qhat == B ||
             (n > 1 && (int128_t)qhat * v[n - 2] > (int128_t)rhat * B + u[j + n - 2])) {
        qhat--;
        rhat += den;
        if (rhat >= B) break;
      }

      int64_t borrow = 0;
      int64_t carry = 0;
      for (int i = 0; i < n; ++i) {
        int128_t prod = (int128_t)qhat * v[i] + carry;
        int64_t sub = u[j + i] - (int64_t)(prod % B) + borrow;
        carry = (int64_t)(prod / B);
        if (sub < 0) {
          sub += B;
          borrow = -1;
        } else {
          borrow = 0;
        }
        u[j + i] = sub;
      }
      int64_t sub = u[j + n] - carry + borrow;
      u[j + n] = sub;

      q[j] = qhat;

      if (sub < 0) {
        q[j]--;
        carry = 0;
        for (int i = 0; i < n; ++i) {
          int64_t sum = u[j + i] + v[i] + carry;
          u[j + i] = sum % B;
          carry = sum / B;
        }
        u[j + n] += carry;
      }

      // Safety: if q[j] was negative (impossible here as qhat is positive or 0),
      // or if it was 0 and decremented (impossible unless sub<0, which implies qhat was too big)
      // qhat adjustment loop ensures qhat is close.
      // But we store q[j] in vector<int64_t>.
    }

    std::vector<int64_t> r_chunks;
    r_chunks.reserve(n);
    int64_t rem = 0;
    for (int i = n - 1; i >= 0; --i) {
      int128_t cur = (int128_t)rem * B + u[i];
      r_chunks.push_back((int64_t)(cur / d));
      rem = (int64_t)(cur % d);
    }
    std::reverse(r_chunks.begin(), r_chunks.end());

    // Safety check for remainder unnormalization
    // if rem != 0, we have an issue with the division algorithm logic (d-normalization should be
    // exact?) Algorithm D: r = u / d. The remainder is u % d. But u here is the *updated* u after
    // subtraction steps. It holds r * d. So u % d should be 0. If it's not, we might have
    // overflow/precision issues in mul_scalar or setup.

    // Final remainder check (should be zero unless precision issue, but with d=B/(v[n-1]+1) it's
    // integer division) rem should be 0 here if u was fully consumed. Wait, we are unnormalizing r.
    // u[i] holds the remainder of the division step * d.
    // We divided u by d to get r_chunks.
    // So rem should be 0.

    while (q.size() > 1 && q.back() == 0) q.pop_back();
    while (r_chunks.size() > 1 && r_chunks.back() == 0) r_chunks.pop_back();
    if (r_chunks.empty()) r_chunks.push_back(0);
    if (q.empty()) q.push_back(0);

    return {std::move(q), std::move(r_chunks)};
  }

  /**
   * @brief Performs Euclidean division.
   *
   * Computes q and r such that a = b * q + r, where 0 <= r < |b|.
   *
   * @param a Dividend
   * @param b Divisor
   * @return Pair {q, r}
   * @throws std::domain_error if b is zero.
   */
  static std::pair<T81BigInt, T81BigInt> div_mod(const T81BigInt& a, const T81BigInt& b) {
    if (b.is_zero()) throw std::domain_error("BigInt division by zero");
    if (a.is_zero()) return {zero(), zero()};

    auto u_chunks = to_chunks(a.abs());
    auto v_chunks = to_chunks(b.abs());

    auto u_std = to_std_chunks(std::move(u_chunks));
    auto v_std = to_std_chunks(std::move(v_chunks));

    // div_mod_std performs unsigned division: |a| = |b| * q' + r'
    auto p = div_mod_std(std::move(u_std), std::move(v_std));

    T81BigInt q = from_chunks(std::move(p.first));
    T81BigInt r = from_chunks(std::move(p.second));

    // Adjust for signs to satisfy Euclidean property: 0 <= r < |b|
    // Case 1: a >= 0, b > 0. q = q', r = r'.
    // Case 2: a >= 0, b < 0. q = -q', r = r'.
    // Case 3: a < 0, b > 0. a = -|a| = -( |b|*q' + r' ) = |b|*(-q') - r'.
    //         If r' != 0, -r' is negative. We need r in [0, b).
    //         -r' = b - r' - b = b * (-1) + (b - r').
    //         So a = b * (-q' - 1) + (b - r').
    //         q = -q' - 1, r = b - r'.
    // Case 4: a < 0, b < 0. a = -|a| = -( |b|*q' + r' ) = (-b)*q' + (-r').
    //         = b * q' - r'.
    //         If r' != 0, -r' is negative. We need r in [0, |b|).
    //         -r' = |b| - r' - |b| = (-b) - r' + b.
    //         Wait, b is negative. |b| = -b.
    //         -r' = -b - r' + b.
    //         So a = b * (q' + 1) + (-b - r').
    //         q = q' + 1, r = |b| - r'.

    if (a.is_negative()) {
      if (!r.is_zero()) {
        // Adjustment needed
        if (b.is_negative()) {
          // Case 4: a < 0, b < 0
          q = q + one();
          r = b.abs() - r;
        } else {
          // Case 3: a < 0, b > 0
          q = -(q + one());
          r = b - r;
        }
      } else {
        // Exact division
        if (b.is_negative()) {
          // Case 4 (r=0): q = q'
          // q is positive
        } else {
          // Case 3 (r=0): q = -q'
          q = -q;
        }
      }
    } else {
      // a >= 0
      if (b.is_negative()) {
        // Case 2: a >= 0, b < 0. q = -q'
        q = -q;
      }
      // Case 1: a >= 0, b > 0. q = q'
    }

    q.verify_invariants();
    r.verify_invariants();
    return {q, r};
  }

  T81BigInt operator-() const {
    if (is_zero()) return *this;
    T81BigInt r = *this;
    r.negative_ = !r.negative_;
    return r;
  }

  friend T81BigInt operator+(const T81BigInt& a, const T81BigInt& b) {
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;
    // Fast path: single-limb values avoid chunk expansion and carry normalization.
    if (a.limbs_.size() == 1 && b.limbs_.size() == 1) {
      size_t ta = a.limbs_[0].significant_trits();
      size_t tb = b.limbs_[0].significant_trits();
      // Addition adds at most 1 trit of magnitude.
      // If both are strictly less than max trits, result fits.
      if (std::max(ta, tb) < kLimbTrits) {
        T81BigInt res;
        res.limbs_.clear();
        // Note: sign handling is complex, but here we just do raw addition if possible.
        // Actually, if signs differ, magnitude decreases, so it's always safe.
        // If signs same, magnitude increases.
        // Simplest is just try it if potentially safe.
        // But we still need to handle the T81Int API which throws on overflow.
        // The check `max(ta, tb) < kLimbTrits` ensures NO overflow in T81Int addition.
        res.limbs_.clear();
        Limb val =
            (a.negative_ ? -a.limbs_[0] : a.limbs_[0]) + (b.negative_ ? -b.limbs_[0] : b.limbs_[0]);
        if (val.sign_trit() == Trit::N) {
          res.negative_ = true;
          res.limbs_.push_back(-val);
        } else {
          res.negative_ = false;
          res.limbs_.push_back(val);
        }
        res.normalize();
        return res;
      } else {
        // Try/catch fallback for the edge case where it MIGHT fit (e.g. cancellations)
        try {
          T81BigInt res;
          res.limbs_.clear();
          res.limbs_.push_back(a.negative_ ? -a.limbs_[0] : a.limbs_[0]);
          Limb rhs = b.negative_ ? -b.limbs_[0] : b.limbs_[0];
          res.limbs_[0] = res.limbs_[0] + rhs;
          res.negative_ = false;
          res.normalize();
          return res;
        } catch (const std::overflow_error&) {
          // Fall through
        }
      }
    }
    auto ac = to_chunks(a);
    auto bc = to_chunks(b);
    size_t n = std::max(ac.size(), bc.size());
    size_t n_padded = (n + 3) & ~size_t(3);
    ac.resize(n_padded, 0);
    bc.resize(n_padded, 0);

    std::vector<int64_t> rc(n_padded + 1, 0);
    size_t i = 0;
#if defined(__AVX2__)
    for (; i < n_padded; i += 4) {
      __m256i va = _mm256_loadu_si256((const __m256i*)&ac[i]);
      __m256i vb = _mm256_loadu_si256((const __m256i*)&bc[i]);
      __m256i vsum = _mm256_add_epi64(va, vb);
      _mm256_storeu_si256((__m256i*)&rc[i], vsum);
    }
#endif
    for (; i < n; ++i) rc[i] = ac[i] + bc[i];

    return from_chunks(std::move(rc));
  }

  friend T81BigInt operator-(const T81BigInt& a, const T81BigInt& b) {
    if (b.is_zero()) return a;
    if (a.is_zero()) {
      T81BigInt res = b;
      res.negative_ = !b.negative_;
      return res;
    }
    // For now, reuse addition logic for simplicity and correctness.
    // Optimization: implement direct subtraction with SIMD if needed.
    T81BigInt neg_b = b;
    neg_b.negative_ = !b.negative_;
    return a + neg_b;
  }

  static std::vector<int128_t> karatsuba_mul_(std::span<const int64_t> a,
                                              std::span<const int64_t> b) {
    size_t n = std::max(a.size(), b.size());
    if (n <= 32) {  // Schoolbook threshold
      std::vector<int128_t> res(a.size() + b.size(), 0);
      for (size_t i = 0; i < a.size(); ++i) {
        const int64_t val_a = a[i];
        if (val_a == 0) continue;
        const int128_t a128 = val_a;
        for (size_t j = 0; j < b.size(); ++j) {
          const int64_t val_b = b[j];
          if (val_b == 0) continue;
          res[i + j] += a128 * val_b;
        }
      }
      return res;
    }

    size_t k = n / 2;
    auto split = [k](std::span<const int64_t> v) {
      std::span<const int64_t> low, high;
      if (v.size() <= k) {
        low = v;
        high = {};
      } else {
        low = v.subspan(0, k);
        high = v.subspan(k);
      }
      return std::make_pair(low, high);
    };

    auto [a0, a1] = split(a);
    auto [b0, b1] = split(b);

    auto z0 = karatsuba_mul_(a0, b0);
    auto z2 = karatsuba_mul_(a1, b1);

    auto add_v = [](std::span<const int64_t> x, std::span<const int64_t> y) {
      size_t nx = x.size(), ny = y.size();
      size_t n = std::max(nx, ny);
      std::vector<int64_t> r(n, 0);
      size_t i = 0;
#if defined(__AVX2__)
      size_t n_simd = std::min(nx, ny) & ~size_t(3);
      for (; i < n_simd; i += 4) {
        __m256i vx = _mm256_loadu_si256((const __m256i*)&x[i]);
        __m256i vy = _mm256_loadu_si256((const __m256i*)&y[i]);
        _mm256_storeu_si256((__m256i*)&r[i], _mm256_add_epi64(vx, vy));
      }
#endif
      for (; i < n; ++i) {
        r[i] = (i < nx ? x[i] : 0) + (i < ny ? y[i] : 0);
      }

      int64_t carry = 0;
      constexpr int64_t B = 7625597484987LL;
      constexpr int64_t halfB = (B - 1) / 2;
      for (size_t j = 0; j < r.size() || carry != 0; ++j) {
        if (j >= r.size()) r.push_back(0);
        int64_t val = r[j] + carry;
        int64_t q = (val >= 0) ? (val + halfB) / B : (val - halfB) / B;
        r[j] = val - q * B;
        carry = q;
      }
      return r;
    };

    auto a01 = add_v(a0, a1);
    auto b01 = add_v(b0, b1);
    auto z1 = karatsuba_mul_(a01, b01);

    // z1 = z1 - z0 - z2
    auto sub_v_128 = [](std::vector<int128_t>& x, const std::vector<int128_t>& y) {
      size_t nx = x.size(), ny = y.size();
      if (nx < ny) x.resize(ny, 0);
      size_t i = 0;
#if defined(__AVX2__)
      // Optimization: partial SIMD for subtraction if we had 128-bit SIMD.
      // Since we don't, we'll use unrolling.
      for (; i + 3 < ny; i += 4) {
        x[i] -= y[i];
        x[i + 1] -= y[i + 1];
        x[i + 2] -= y[i + 2];
        x[i + 3] -= y[i + 3];
      }
#endif
      for (; i < ny; ++i) x[i] -= y[i];
    };

    std::vector<int128_t> middle = std::move(z1);
    sub_v_128(middle, z0);
    sub_v_128(middle, z2);

    std::vector<int128_t> res(z2.size() + 2 * k, 0);
    if (res.size() < z0.size()) res.resize(z0.size(), 0);
    if (res.size() < middle.size() + k) res.resize(middle.size() + k, 0);

    for (size_t i = 0; i < z0.size(); ++i) res[i] += z0[i];
    for (size_t i = 0; i < middle.size(); ++i) res[i + k] += middle[i];
    for (size_t i = 0; i < z2.size(); ++i) res[i + 2 * k] += z2[i];

    return res;
  }

  friend T81BigInt operator*(const T81BigInt& a, const T81BigInt& b) {
    if (a.is_zero() || b.is_zero()) return T81BigInt::zero();
    // Fast path: single-limb products are common in language/runtime pipelines.
    if (a.limbs_.size() == 1 && b.limbs_.size() == 1) {
      size_t ta = a.limbs_[0].significant_trits();
      size_t tb = b.limbs_[0].significant_trits();
      // Multiplication requires sum of trits.
      if (ta + tb <= kLimbTrits) {
        T81BigInt res;
        res.limbs_.clear();
        res.limbs_.push_back(a.limbs_[0] * b.limbs_[0]);  // Safe
        res.negative_ = (a.negative_ != b.negative_);
        res.normalize();
        return res;
      } else if (ta + tb <= kLimbTrits + 1) {
        try {
          T81BigInt res;
          res.limbs_.clear();
          res.limbs_.push_back(a.limbs_[0] * b.limbs_[0]);
          res.negative_ = (a.negative_ != b.negative_);
          res.normalize();
          return res;
        } catch (const std::overflow_error&) {
          // Fall through
        }
      }
    }
    auto ac = to_chunks(a);
    auto bc = to_chunks(b);
    std::vector<int128_t> rc = karatsuba_mul_(ac, bc);

    int128_t carry = 0;
    const int128_t B128 = B;
    const int128_t halfB128 = halfB;
    std::vector<int64_t> final_c;
    for (size_t i = 0; i < rc.size() || carry != 0; ++i) {
      int128_t val = (i < rc.size() ? rc[i] : 0) + carry;
      int128_t q = (val >= 0) ? (val + halfB128) / B128 : (val - halfB128) / B128;
      final_c.push_back(static_cast<int64_t>(val - q * B128));
      carry = q;
    }

    return from_chunks(std::move(final_c));
  }

  T81BigInt& operator+=(const T81BigInt& rhs) {
    *this = *this + rhs;
    return *this;
  }

  T81BigInt& operator-=(const T81BigInt& rhs) {
    *this = *this - rhs;
    return *this;
  }

  T81BigInt& operator*=(const T81BigInt& rhs) {
    *this = *this * rhs;
    return *this;
  }

  friend T81BigInt operator/(const T81BigInt& a, const T81BigInt& b) { return div_mod(a, b).first; }

  friend T81BigInt operator%(const T81BigInt& a, const T81BigInt& b) {
    return div_mod(a, b).second;
  }

  T81BigInt& operator/=(const T81BigInt& rhs) {
    *this = *this / rhs;
    return *this;
  }

  T81BigInt& operator%=(const T81BigInt& rhs) {
    *this = *this % rhs;
    return *this;
  }

  static T81BigInt gcd(T81BigInt a, T81BigInt b) {
    a = a.abs();
    b = b.abs();
    while (!b.is_zero()) {
      T81BigInt r = a % b;
      a = std::move(b);
      b = std::move(r);
    }
    return a;
  }

  static T81BigInt pow(const T81BigInt& base, const T81BigInt& exp) {
    if (exp.is_negative()) throw std::domain_error("BigInt pow: negative exponent");
    if (exp.is_zero()) return one();
    if (base.is_zero()) return zero();

    T81BigInt result = one();
    T81BigInt b = base;
    T81BigInt e = exp;
    T81BigInt two(2);

    while (!e.is_zero()) {
      auto dm = div_mod(e, two);
      if (!dm.second.is_zero()) {
        result = result * b;
      }
      b = b * b;
      e = dm.first;
    }
    return result;
  }

  static T81BigInt pow_mod(const T81BigInt& base, const T81BigInt& exp, const T81BigInt& mod) {
    if (mod.is_zero()) throw std::domain_error("pow_mod: modulus is zero");
    if (exp.is_negative()) throw std::domain_error("pow_mod: negative exponent");

    T81BigInt result = one() % mod;
    T81BigInt b = base % mod;
    T81BigInt e = exp;
    T81BigInt two(2);

    while (!e.is_zero()) {
      auto dm = div_mod(e, two);
      if (!dm.second.is_zero()) {
        result = (result * b) % mod;
      }
      b = (b * b) % mod;
      e = dm.first;
    }
    return result;
  }

  static T81BigInt sqrt(const T81BigInt& x) {
    if (x.is_negative()) throw std::domain_error("sqrt of negative number");
    if (x.is_zero()) return zero();

    T81BigInt a = x;
    T81BigInt two(2);
    T81BigInt b = (x + one()) / two;

    while (b < a) {
      a = b;
      b = (a + x / a) / two;
    }
    return a;
  }

  static std::tuple<T81BigInt, T81BigInt, T81BigInt> extended_gcd(T81BigInt a, T81BigInt b) {
    T81BigInt old_r = a;
    T81BigInt r = b;
    T81BigInt old_s = one();
    T81BigInt s = zero();
    T81BigInt old_t = zero();
    T81BigInt t = one();

    while (!r.is_zero()) {
      auto qr = div_mod(old_r, r);
      T81BigInt q = qr.first;
      T81BigInt rem = qr.second;

      old_r = r;
      r = rem;

      T81BigInt tmp = old_s - q * s;
      old_s = s;
      s = tmp;

      tmp = old_t - q * t;
      old_t = t;
      t = tmp;
    }

    return std::make_tuple(old_r, old_s, old_t);
  }

  static T81BigInt modular_inverse(const T81BigInt& a, const T81BigInt& m) {
    if (m <= one()) {
      throw std::domain_error("modular_inverse: modulus must be > 1");
    }
    auto [g, x, y] = extended_gcd(a, m);

    if (!is_one(g.abs())) {
      throw std::domain_error("modular_inverse: inverse does not exist");
    }

    T81BigInt res = x % m;
    if (res.is_negative()) {
      res = res + m;
    }
    return res;
  }

  /**
   * @brief Computes modular inverse using Stein's algorithm (Ternary GCD).
   *
   * @warning This implementation is NOT constant-time and NOT side-channel resistant.
   * It avoids full division steps (Knuth D) but still leaks information via execution path.
   * For critical cryptographic operations requiring constant-time execution, further hardening is
   * needed.
   */
  static T81BigInt modular_inverse_stein(const T81BigInt& a, const T81BigInt& m) {
    if (m <= one()) throw std::domain_error("modular_inverse_stein: modulus must be > 1");

    // Check if m is coprime to 3. If not, fallback to Euclidean.
    T81BigInt m_abs = m.abs();
    if (m_abs.lowest_trit_magnitude() == Trit::Z) {
      return modular_inverse(a, m);
    }

    T81BigInt u = a.abs();
    T81BigInt v = m_abs;
    T81BigInt x1 = one();
    T81BigInt x2 = zero();

    // Helper to divide by 3 modulo m
    auto div3_mod = [&](T81BigInt& val) {
      Trit t = val.lowest_trit();
      if (t != Trit::Z) {
        Trit m_t = m_abs.lowest_trit_magnitude();
        if (t == Trit::P) {
          if (m_t == Trit::P)
            val -= m_abs;
          else
            val += m_abs;
        } else {
          if (m_t == Trit::P)
            val += m_abs;
          else
            val -= m_abs;
        }
      }
      val.div3_inplace();
    };

    while (!u.is_zero() && !v.is_zero()) {
      while (u.lowest_trit_magnitude() == Trit::Z) {
        u.div3_magnitude();
        div3_mod(x1);
      }
      while (v.lowest_trit_magnitude() == Trit::Z) {
        v.div3_magnitude();
        div3_mod(x2);
      }

      if (u >= v) {
        Trit u_t = u.lowest_trit_magnitude();
        Trit v_t = v.lowest_trit_magnitude();
        if (u_t == v_t) {
          u -= v;
          x1 -= x2;
        } else {
          u += v;
          x1 += x2;
        }
      } else {
        Trit u_t = u.lowest_trit_magnitude();
        Trit v_t = v.lowest_trit_magnitude();
        if (v_t == u_t) {
          v -= u;
          x2 -= x1;
        } else {
          v += u;
          x2 += x1;
        }
      }
    }

    T81BigInt g = u.is_zero() ? v : u;
    T81BigInt res = u.is_zero() ? x2 : x1;

    if (!is_one(g)) {
      throw std::domain_error("modular_inverse_stein: inverse does not exist");
    }

    if (a.is_negative()) res = -res;

    res = res % m_abs;
    if (res.is_negative()) res += m_abs;

    return res;
  }

  std::string to_string() const { return to_decimal_string(); }

  std::string to_decimal_string() const {
    if (is_zero()) return "0";

    T81BigInt base(10);
    T81BigInt v = abs();
    std::vector<int> digits;
    digits.reserve(48);

    while (!v.is_zero()) {
      auto dm = div_mod(v, base);
      int d = static_cast<int>(dm.second.to_int64());
      digits.push_back(d);
      v = dm.first;
    }

    std::string out;
    if (negative_) out.push_back('-');
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
      out += static_cast<char>('0' + *it);
    }
    return out;
  }

  std::string to_base81_string() const {
    if (is_zero()) return "0";

    T81BigInt base(81);
    T81BigInt v = abs();
    std::vector<int> digits;
    digits.reserve(48);

    while (!v.is_zero()) {
      auto dm = div_mod(v, base);
      int d = static_cast<int>(dm.second.to_int64());
      digits.push_back(d);
      v = dm.first;
    }

    std::string out;
    if (negative_) out.push_back('-');
    const auto& alpha = detail::base81_alphabet_vec();
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
      out += alpha[static_cast<std::size_t>(*it)];
    }
    return out;
  }

  static T81BigInt from_base81_string(std::string_view s) {
    if (s.empty()) throw std::invalid_argument("T81BigInt::from_base81_string: empty input");

    bool neg = false;
    std::size_t pos = 0;
    if (s[pos] == '+' || s[pos] == '-') {
      neg = (s[pos] == '-');
      ++pos;
      if (pos >= s.size()) throw std::invalid_argument("T81BigInt::from_base81_string: sign only");
    }

    std::vector<int> digits;
    const auto& map = detail::base81_digit_map();
    while (pos < s.size()) {
      std::string cp = detail::next_codepoint(s, pos);
      if (cp.empty())
        throw std::invalid_argument("T81BigInt::from_base81_string: invalid encoding");
      auto it = map.find(cp);
      if (it == map.end())
        throw std::invalid_argument("T81BigInt::from_base81_string: invalid character");
      digits.push_back(it->second);
    }
    if (digits.size() > 1 && digits.front() == 0) {
      throw std::invalid_argument("T81BigInt::from_base81_string: non-canonical leading zero");
    }

    T81BigInt base(81);
    T81BigInt v(0);
    for (int d : digits) {
      v = v * base;
      v = v + T81BigInt(d);
    }
    if (neg && !v.is_zero()) v = -v;
    return v;
  }

  static T81BigInt from_base81_digit_string(std::string_view s) {
    if (s.empty()) throw std::invalid_argument("from_base81_digit_string: empty input");
    bool neg = false;
    size_t pos = 0;
    if (s[pos] == '+' || s[pos] == '-') {
      neg = (s[pos] == '-');
      pos++;
    }

    std::vector<int> digits;
    int current = 0;
    bool have_digit = false;

    for (; pos <= s.size(); ++pos) {
      if (pos == s.size() || s[pos] == '.') {
        if (!have_digit) throw std::invalid_argument("from_base81_digit_string: empty digit");
        if (current < 0 || current >= 81) throw std::invalid_argument("digit out of range");
        digits.push_back(current);
        current = 0;
        have_digit = false;
      } else if (s[pos] >= '0' && s[pos] <= '9') {
        have_digit = true;
        current = current * 10 + (s[pos] - '0');
        if (current >= 81) throw std::invalid_argument("digit overflow");
      } else {
        throw std::invalid_argument("invalid char");
      }
    }

    T81BigInt res(0);
    T81BigInt base(81);
    for (int d : digits) {
      res = res * base + T81BigInt(d);
    }
    if (neg) res = -res;
    return res;
  }

  static T81BigInt from_ascii(std::string_view s) { return from_base81_digit_string(s); }

  void serialize(std::ostream& os) const {
    std::string s = to_base81_string();
    uint64_t len = s.size();
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    os.write(s.data(), static_cast<std::streamsize>(len));
  }

  void deserialize(std::istream& is) {
    uint64_t len;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!is) return;
    std::string s(static_cast<size_t>(len), '\0');
    is.read(s.data(), static_cast<std::streamsize>(len));
    *this = from_base81_string(s);
  }
};

// Convenience alias
using BigInt = T81BigInt;

}  // namespace t81::v1
