#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace t81::experiments::exact_rational {

struct ConversionResult {
  std::optional<std::string> value;
  std::string status;  // "exact", "rounded", or "error"
  std::optional<std::string> error;
  std::vector<std::string> flags;

  bool operator==(const ConversionResult& o) const {
    return value == o.value && status == o.status && error == o.error && flags == o.flags;
  }
};

class BigInt {
public:
  std::vector<uint64_t> limbs;  // Base 2^64, least significant limb first

  BigInt() = default;

  BigInt(uint64_t v) {
    if (v > 0) limbs.push_back(v);
  }

  static BigInt from_hex(std::string_view hex_str) {
    BigInt res(0);
    BigInt sixteen(16);
    for (char c : hex_str) {
      uint64_t d = 0;
      if (c >= '0' && c <= '9')
        d = c - '0';
      else if (c >= 'a' && c <= 'f')
        d = 10 + (c - 'a');
      else if (c >= 'A' && c <= 'F')
        d = 10 + (c - 'A');
      res = res * sixteen + BigInt(d);
    }
    return res;
  }

  static BigInt from_decimal(std::string_view dec_str) {
    BigInt res(0);
    BigInt ten(10);
    for (char c : dec_str) {
      if (c >= '0' && c <= '9') {
        res = res * ten + BigInt(static_cast<uint64_t>(c - '0'));
      }
    }
    return res;
  }

  void trim() {
    while (!limbs.empty() && limbs.back() == 0) {
      limbs.pop_back();
    }
  }

  bool is_zero() const {
    for (uint64_t l : limbs)
      if (l != 0) return false;
    return true;
  }

  size_t bit_length() const {
    if (limbs.empty()) return 0;
    size_t last = limbs.size() - 1;
    while (last > 0 && limbs[last] == 0) last--;
    if (limbs[last] == 0) return 0;
    uint64_t m = limbs[last];
    size_t b = 0;
    while (m > 0) {
      b++;
      m >>= 1;
    }
    return last * 64 + b;
  }

  static BigInt pow2(size_t exp) {
    BigInt res;
    size_t limb_idx = exp / 64;
    size_t bit_idx = exp % 64;
    res.limbs.resize(limb_idx + 1, 0);
    res.limbs[limb_idx] = (1ULL << bit_idx);
    return res;
  }

  static BigInt pow3(size_t exp) {
    BigInt res(1);
    BigInt base(3);
    size_t e = exp;
    while (e > 0) {
      if (e & 1) res = res * base;
      base = base * base;
      e >>= 1;
    }
    return res;
  }

  bool operator==(const BigInt& o) const {
    BigInt a = *this;
    a.trim();
    BigInt b = o;
    b.trim();
    return a.limbs == b.limbs;
  }

  bool operator!=(const BigInt& o) const { return !(*this == o); }

  bool operator<(const BigInt& o) const {
    BigInt a = *this;
    a.trim();
    BigInt b = o;
    b.trim();
    if (a.limbs.size() != b.limbs.size()) return a.limbs.size() < b.limbs.size();
    for (size_t i = a.limbs.size(); i-- > 0;) {
      if (a.limbs[i] != b.limbs[i]) return a.limbs[i] < b.limbs[i];
    }
    return false;
  }

  bool operator<=(const BigInt& o) const { return !(o < *this); }
  bool operator>(const BigInt& o) const { return o < *this; }
  bool operator>=(const BigInt& o) const { return !(*this < o); }

  BigInt operator+(const BigInt& o) const {
    BigInt res;
    size_t n = std::max(limbs.size(), o.limbs.size());
    res.limbs.reserve(n + 1);
    unsigned __int128 carry = 0;
    for (size_t i = 0; i < n || carry; ++i) {
      unsigned __int128 sum = carry;
      if (i < limbs.size()) sum += limbs[i];
      if (i < o.limbs.size()) sum += o.limbs[i];
      res.limbs.push_back(static_cast<uint64_t>(sum));
      carry = sum >> 64;
    }
    res.trim();
    return res;
  }

  BigInt operator-(const BigInt& o) const {
    BigInt res;
    res.limbs.reserve(limbs.size());
    unsigned __int128 borrow = 0;
    for (size_t i = 0; i < limbs.size(); ++i) {
      unsigned __int128 sub = limbs[i];
      unsigned __int128 b = borrow + (i < o.limbs.size() ? o.limbs[i] : 0);
      if (sub < b) {
        sub += (static_cast<unsigned __int128>(1) << 64);
        borrow = 1;
      } else {
        borrow = 0;
      }
      res.limbs.push_back(static_cast<uint64_t>(sub - b));
    }
    res.trim();
    return res;
  }

  BigInt operator*(const BigInt& o) const {
    if (is_zero() || o.is_zero()) return BigInt(0);
    BigInt res;
    res.limbs.resize(limbs.size() + o.limbs.size(), 0);
    for (size_t i = 0; i < limbs.size(); ++i) {
      unsigned __int128 carry = 0;
      for (size_t j = 0; j < o.limbs.size() || carry; ++j) {
        unsigned __int128 cur = res.limbs[i + j] + carry;
        if (j < o.limbs.size()) {
          cur += static_cast<unsigned __int128>(limbs[i]) * o.limbs[j];
        }
        res.limbs[i + j] = static_cast<uint64_t>(cur);
        carry = cur >> 64;
      }
    }
    res.trim();
    return res;
  }

  BigInt operator*(uint64_t v) const {
    if (v == 0 || is_zero()) return BigInt(0);
    BigInt res;
    res.limbs.reserve(limbs.size() + 1);
    unsigned __int128 carry = 0;
    for (size_t i = 0; i < limbs.size() || carry; ++i) {
      unsigned __int128 cur = carry;
      if (i < limbs.size()) cur += static_cast<unsigned __int128>(limbs[i]) * v;
      res.limbs.push_back(static_cast<uint64_t>(cur));
      carry = cur >> 64;
    }
    res.trim();
    return res;
  }

  static std::pair<BigInt, BigInt> divmod(BigInt u, BigInt v) {
    u.trim();
    v.trim();
    if (v.is_zero()) throw std::domain_error("BigInt divmod: division by zero");
    if (u < v) return {BigInt(0), u};
    if (v.limbs.size() == 1) {
      uint64_t divisor = v.limbs[0];
      BigInt q;
      q.limbs.resize(u.limbs.size());
      unsigned __int128 rem = 0;
      for (size_t i = u.limbs.size(); i-- > 0;) {
        unsigned __int128 cur = (rem << 64) | u.limbs[i];
        q.limbs[i] = static_cast<uint64_t>(cur / divisor);
        rem = cur % divisor;
      }
      q.trim();
      return {q, BigInt(static_cast<uint64_t>(rem))};
    }

    size_t n = v.limbs.size();
    size_t m = u.limbs.size() - n;

    uint64_t v_msb = v.limbs.back();
    size_t norm_shift = 0;
    while ((v_msb & (1ULL << 63)) == 0) {
      v_msb <<= 1;
      norm_shift++;
    }

    BigInt un = u << norm_shift;
    BigInt vn = v << norm_shift;
    un.trim();
    vn.trim();

    while (un.limbs.size() < n + m + 1) un.limbs.push_back(0);

    BigInt q;
    q.limbs.resize(m + 1, 0);

    uint64_t vn1 = vn.limbs[n - 1];
    uint64_t vn2 = vn.limbs[n - 2];

    const unsigned __int128 b64 = static_cast<unsigned __int128>(1) << 64;

    for (size_t j = m + 1; j-- > 0;) {
      unsigned __int128 un_2 =
          (static_cast<unsigned __int128>(un.limbs[j + n]) << 64) | un.limbs[j + n - 1];
      unsigned __int128 qhat = un_2 / vn1;
      unsigned __int128 rhat = un_2 % vn1;

      while (qhat >= b64 || (qhat * vn2 > ((rhat << 64) | un.limbs[j + n - 2]))) {
        qhat--;
        rhat += vn1;
        if (rhat >= b64) break;
      }

      unsigned __int128 borrow = 0;
      unsigned __int128 carry = 0;
      for (size_t i = 0; i < n; ++i) {
        unsigned __int128 prod = qhat * vn.limbs[i] + carry;
        uint64_t p_lo = static_cast<uint64_t>(prod);
        carry = prod >> 64;

        unsigned __int128 sub = static_cast<unsigned __int128>(un.limbs[j + i]);
        unsigned __int128 sub_val = p_lo + borrow;
        if (sub < sub_val) {
          sub += b64;
          borrow = 1;
        } else {
          borrow = 0;
        }
        un.limbs[j + i] = static_cast<uint64_t>(sub - sub_val);
      }
      unsigned __int128 sub = static_cast<unsigned __int128>(un.limbs[j + n]);
      unsigned __int128 sub_val = carry + borrow;
      if (sub < sub_val) {
        un.limbs[j + n] = static_cast<uint64_t>(sub + b64 - sub_val);
        qhat--;
        unsigned __int128 add_carry = 0;
        for (size_t i = 0; i < n; ++i) {
          unsigned __int128 sum =
              static_cast<unsigned __int128>(un.limbs[j + i]) + vn.limbs[i] + add_carry;
          un.limbs[j + i] = static_cast<uint64_t>(sum);
          add_carry = sum >> 64;
        }
        un.limbs[j + n] += static_cast<uint64_t>(add_carry);
      } else {
        un.limbs[j + n] = static_cast<uint64_t>(sub - sub_val);
      }

      q.limbs[j] = static_cast<uint64_t>(qhat);
    }

    q.trim();
    BigInt r = un >> norm_shift;
    r.trim();
    return {q, r};
  }

  BigInt operator/(const BigInt& o) const { return divmod(*this, o).first; }
  BigInt operator%(const BigInt& o) const { return divmod(*this, o).second; }

  BigInt operator<<(size_t shift) const {
    if (is_zero() || shift == 0) return *this;
    size_t limb_shift = shift / 64;
    size_t bit_shift = shift % 64;
    BigInt res;
    res.limbs.resize(limbs.size() + limb_shift + 1, 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < limbs.size(); ++i) {
      if (bit_shift == 0) {
        res.limbs[i + limb_shift] = limbs[i];
      } else {
        res.limbs[i + limb_shift] = (limbs[i] << bit_shift) | carry;
        carry = limbs[i] >> (64 - bit_shift);
      }
    }
    if (carry && bit_shift != 0) {
      res.limbs[limbs.size() + limb_shift] = carry;
    }
    res.trim();
    return res;
  }

  BigInt operator>>(size_t shift) const {
    if (is_zero() || shift == 0) return *this;
    size_t limb_shift = shift / 64;
    size_t bit_shift = shift % 64;
    if (limb_shift >= limbs.size()) return BigInt(0);
    BigInt res;
    res.limbs.resize(limbs.size() - limb_shift, 0);
    uint64_t carry = 0;
    for (size_t i = limbs.size(); i-- > limb_shift;) {
      if (bit_shift == 0) {
        res.limbs[i - limb_shift] = limbs[i];
      } else {
        uint64_t next_carry = limbs[i] << (64 - bit_shift);
        res.limbs[i - limb_shift] = (limbs[i] >> bit_shift) | carry;
        carry = next_carry;
      }
    }
    res.trim();
    return res;
  }

  std::string to_decimal() const {
    BigInt temp = *this;
    temp.trim();
    if (temp.is_zero()) return "0";
    std::string res;
    BigInt ten(10);
    while (!temp.is_zero()) {
      auto [q, r] = divmod(temp, ten);
      uint64_t d = r.limbs.empty() ? 0 : r.limbs[0];
      res.push_back(static_cast<char>('0' + d));
      temp = q;
    }
    std::reverse(res.begin(), res.end());
    return res;
  }

  std::string trit_text(size_t width) const {
    BigInt temp = *this;
    std::string digits;
    BigInt three(3);
    for (size_t i = 0; i < width; ++i) {
      auto [q, r] = divmod(temp, three);
      uint64_t digit = r.limbs.empty() ? 0 : r.limbs[0];
      if (digit == 2) {
        q = q + BigInt(1);
        digits.push_back('-');
      } else if (digit == 1) {
        digits.push_back('+');
      } else {
        digits.push_back('0');
      }
      temp = q;
    }
    std::reverse(digits.begin(), digits.end());
    return digits;
  }
};

class BigRational {
public:
  BigInt num;
  BigInt den;
  bool negative = false;

  BigRational() : num(0), den(1), negative(false) {}

  BigRational(BigInt n, BigInt d, bool neg = false) : num(n), den(d), negative(neg) {
    if (num.is_zero()) {
      den = BigInt(1);
      negative = false;
    }
  }

  static BigRational from_strings(std::string_view num_str, std::string_view den_str) {
    bool neg = false;
    if (!num_str.empty() && num_str[0] == '-') {
      neg = true;
      num_str.remove_prefix(1);
    } else if (!num_str.empty() && num_str[0] == '+') {
      num_str.remove_prefix(1);
    }
    BigInt n = BigInt::from_decimal(num_str);
    BigInt d = BigInt::from_decimal(den_str);
    return BigRational(n, d, neg);
  }

  bool is_zero() const { return num.is_zero(); }

  BigRational abs() const { return BigRational(num, den, false); }

  bool operator==(const BigRational& o) const {
    if (is_zero() && o.is_zero()) return true;
    if (negative != o.negative) return false;
    return num * o.den == o.num * den;
  }

  bool operator<(const BigRational& o) const {
    if (negative != o.negative) return negative;
    BigInt lhs = num * o.den;
    BigInt rhs = o.num * den;
    if (negative) return rhs < lhs;
    return lhs < rhs;
  }

  bool operator<=(const BigRational& o) const { return *this < o || *this == o; }
  bool operator>(const BigRational& o) const { return o < *this; }
  bool operator>=(const BigRational& o) const { return !(*this < o); }

  BigRational operator-(const BigRational& o) const {
    BigInt lhs = num * o.den;
    BigInt rhs = o.num * den;
    BigInt d = den * o.den;
    if (lhs >= rhs) {
      return BigRational(lhs - rhs, d, false);
    } else {
      return BigRational(rhs - lhs, d, true);
    }
  }
};

inline ConversionResult make_result(std::optional<std::string> val = std::nullopt,
                                    std::string status = "error",
                                    std::optional<std::string> err = std::nullopt,
                                    std::vector<std::string> flags = {}) {
  return ConversionResult{val, status, err, flags};
}

inline std::optional<std::string> validate(int m, int e, std::string_view mode) {
  if (m < 4 || m > 72 || e < 4 || e > 9) return "unsupported_format";
  if (mode != "exact_only" && mode != "nearest_even") return "unsupported_mode";
  return std::nullopt;
}

inline std::tuple<std::optional<BigRational>, bool, std::optional<std::string>> decode_binary64(
    std::string_view bits) {
  if (bits.length() != 16) return {std::nullopt, false, "invalid_encoding"};
  for (char c : bits) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return {std::nullopt, false, "invalid_encoding"};
    }
  }
  uint64_t raw = 0;
  std::stringstream ss;
  ss << std::hex << bits;
  ss >> raw;

  bool neg = (raw >> 63) != 0;
  uint64_t exp_bits = (raw >> 52) & 2047;
  uint64_t fraction = raw & ((1ULL << 52) - 1);

  if (exp_bits == 2047) {
    return {std::nullopt, false, "nonfinite_input"};
  }

  uint64_t significand = (exp_bits == 0) ? fraction : (fraction + (1ULL << 52));
  bool neg_zero = neg && (significand == 0);

  if (significand == 0) {
    return {BigRational(BigInt(0), BigInt(1), false), neg_zero, std::nullopt};
  }

  int shift = (exp_bits == 0) ? -1074 : static_cast<int>(exp_bits) - 1075;
  BigInt num(significand);
  BigInt den(1);

  if (shift >= 0) {
    num = num * BigInt::pow2(static_cast<size_t>(shift));
  } else {
    den = BigInt::pow2(static_cast<size_t>(-shift));
  }

  return {BigRational(num, den, neg), neg_zero, std::nullopt};
}

inline ConversionResult quantize_rational(BigRational value, int m, int e,
                                          std::string_view mode = "exact_only",
                                          bool negative_zero = false) {
  auto err = validate(m, e, mode);
  if (err) return make_result(std::nullopt, "error", err);

  if (value.is_zero()) {
    std::vector<std::string> flags;
    if (negative_zero) flags.push_back("negative_zero_normalized");
    return make_result("+0E0", "exact", std::nullopt, flags);
  }

  BigRational x = value.abs();

  BigInt three(3);
  BigInt bias_val = (BigInt::pow3(e) - BigInt(1)) / BigInt(2);
  BigInt maximum_val = (BigInt::pow3(m) - BigInt(1)) / BigInt(2);
  BigInt minimum_val = (BigInt::pow3(m - 1) + BigInt(1)) / BigInt(2);

  int low = -static_cast<int>(bias_val.limbs.empty() ? 0 : bias_val.limbs[0]);
  int high = static_cast<int>(bias_val.limbs.empty() ? 0 : bias_val.limbs[0]) - 1;

  int min_p3_exp = low - m + 1;
  BigRational min_representable;
  if (min_p3_exp >= 0) {
    min_representable = BigRational(BigInt::pow3(min_p3_exp), BigInt(1));
  } else {
    min_representable = BigRational(BigInt(1), BigInt::pow3(-min_p3_exp));
  }

  if (x < min_representable) return make_result(std::nullopt, "error", "underflow");

  int max_p3_exp = high - m + 1;
  BigRational max_representable;
  if (max_p3_exp >= 0) {
    max_representable = BigRational(maximum_val * BigInt::pow3(max_p3_exp), BigInt(1));
  } else {
    max_representable = BigRational(maximum_val, BigInt::pow3(-max_p3_exp));
  }

  if (x > max_representable) return make_result(std::nullopt, "error", "overflow");

  int exp_val = 0;
  auto get_cand_limit = [&](int exp_idx) -> BigRational {
    int p = exp_idx - m + 1;
    if (p >= 0)
      return BigRational(maximum_val * BigInt::pow3(p), BigInt(1));
    else
      return BigRational(maximum_val, BigInt::pow3(-p));
  };

  while (x > get_cand_limit(exp_val)) exp_val++;
  while (exp_val > low && x <= get_cand_limit(exp_val - 1)) exp_val--;

  int search_start = std::max(low, exp_val - 1);
  int search_end = std::min(high, std::max(low, exp_val) + 1);

  struct Candidate {
    BigRational dist;
    uint64_t mantissa_parity;
    int exponent;
    BigInt mantissa;
  };

  std::vector<Candidate> candidates;

  for (int exponent = search_start; exponent <= search_end; ++exponent) {
    int k = exponent - m + 1;
    BigInt scaled_num = x.num;
    BigInt scaled_den = x.den;
    if (k >= 0) {
      scaled_den = scaled_den * BigInt::pow3(k);
    } else {
      scaled_num = scaled_num * BigInt::pow3(-k);
    }

    BigInt floor_m = scaled_num / scaled_den;
    BigInt lower_m = (exponent == low) ? BigInt(1) : minimum_val;

    std::vector<BigInt> test_mants = {
        lower_m, maximum_val, std::max(lower_m, std::min(maximum_val, floor_m)),
        std::max(lower_m, std::min(maximum_val, floor_m + BigInt(1)))};

    std::set<BigInt> unique_mants(test_mants.begin(), test_mants.end());

    for (const BigInt& mant : unique_mants) {
      BigRational cand_val;
      if (k >= 0) {
        cand_val = BigRational(mant * BigInt::pow3(k), BigInt(1));
      } else {
        cand_val = BigRational(mant, BigInt::pow3(-k));
      }
      BigRational dist = (x >= cand_val) ? (x - cand_val) : (cand_val - x);
      uint64_t parity = mant.limbs.empty() ? 0 : (mant.limbs[0] % 2);
      candidates.push_back(Candidate{dist, parity, exponent, mant});
    }
  }

  std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
    if (a.dist != b.dist) return a.dist < b.dist;
    if (a.mantissa_parity != b.mantissa_parity) return a.mantissa_parity < b.mantissa_parity;
    if (a.exponent != b.exponent) return a.exponent < b.exponent;
    return a.mantissa < b.mantissa;
  });

  const auto& best = candidates.front();

  if (!best.dist.is_zero() && mode == "exact_only") {
    return make_result(std::nullopt, "error", "inexact");
  }

  std::string sign_str = value.negative ? "-" : "+";
  std::string text = sign_str + best.mantissa.trit_text(static_cast<size_t>(m)) + "E" +
                     std::to_string(best.exponent);
  std::string status = best.dist.is_zero() ? "exact" : "rounded";

  return make_result(text, status, std::nullopt, {});
}

inline ConversionResult from_binary64(std::string_view bits, int m = 27, int e = 9,
                                      std::string_view mode = "exact_only") {
  auto err = validate(m, e, mode);
  if (err) return make_result(std::nullopt, "error", err);

  auto [val_opt, neg_zero, dec_err] = decode_binary64(bits);
  if (dec_err) return make_result(std::nullopt, "error", dec_err);

  return quantize_rational(*val_opt, m, e, mode, neg_zero);
}

inline std::pair<std::optional<BigRational>, std::optional<std::string>> decode_ternary(
    std::string_view text, int m, int e) {
  if (text == "NaE" || text == "+Inf" || text == "-Inf") {
    return {std::nullopt, "nonfinite_input"};
  }
  if (text == "+0E0") {
    return {BigRational(BigInt(0), BigInt(1)), std::nullopt};
  }
  if (text == "-0E0") {
    return {std::nullopt, "noncanonical_encoding"};
  }
  if (text.empty() || (text[0] != '+' && text[0] != '-')) {
    return {std::nullopt, "invalid_encoding"};
  }

  size_t e_pos = text.find('E');
  if (e_pos == std::string_view::npos) return {std::nullopt, "invalid_encoding"};

  std::string_view sign_str = text.substr(0, 1);
  std::string_view digits = text.substr(1, e_pos - 1);
  std::string_view exp_str = text.substr(e_pos + 1);

  if (digits.length() != static_cast<size_t>(m)) return {std::nullopt, "invalid_encoding"};
  if (exp_str.empty() || exp_str.length() > 6) return {std::nullopt, "invalid_encoding"};

  int exp_val = 0;
  try {
    exp_val = std::stoi(std::string(exp_str));
  } catch (...) {
    return {std::nullopt, "invalid_encoding"};
  }

  if (std::to_string(exp_val) != exp_str) {
    return {std::nullopt, "noncanonical_encoding"};
  }

  BigInt bias_val = (BigInt::pow3(e) - BigInt(1)) / BigInt(2);
  int bias_int = static_cast<int>(bias_val.limbs.empty() ? 0 : bias_val.limbs[0]);
  if (exp_val < -bias_int || exp_val >= bias_int) {
    return {std::nullopt, "invalid_encoding"};
  }

  BigInt magnitude(0);
  BigInt three(3);
  for (char d : digits) {
    if (d != '+' && d != '0' && d != '-') {
      return {std::nullopt, "invalid_encoding"};
    }
    magnitude = magnitude * three;
    if (d == '+')
      magnitude = magnitude + BigInt(1);
    else if (d == '-') {
      if (magnitude.is_zero()) return {std::nullopt, "noncanonical_encoding"};
      magnitude = magnitude - BigInt(1);
    }
  }

  if (magnitude.is_zero() || (exp_val != -bias_int && digits[0] != '+')) {
    return {std::nullopt, "noncanonical_encoding"};
  }

  int p3 = exp_val - m + 1;
  BigInt num = magnitude;
  BigInt den(1);
  if (p3 >= 0) {
    num = num * BigInt::pow3(p3);
  } else {
    den = BigInt::pow3(-p3);
  }

  return {BigRational(num, den, sign_str == "-"), std::nullopt};
}

inline ConversionResult binary64_from_rational(BigRational value,
                                               std::string_view mode = "exact_only") {
  if (mode != "exact_only" && mode != "nearest_even") {
    return make_result(std::nullopt, "error", "unsupported_mode");
  }

  if (value.is_zero()) {
    return make_result("0000000000000000", "exact");
  }

  BigRational x = value.abs();

  BigRational min_val(BigInt(1), BigInt::pow2(1074));
  if (x < min_val) return make_result(std::nullopt, "error", "underflow");

  BigInt max_mant = BigInt::pow2(53) - BigInt(1);
  BigRational max_val(max_mant * BigInt::pow2(971), BigInt(1));
  if (x > max_val) return make_result(std::nullopt, "error", "overflow");

  int exponent = static_cast<int>(x.num.bit_length()) - static_cast<int>(x.den.bit_length());

  auto get_p2 = [](int exp) -> BigRational {
    if (exp >= 0)
      return BigRational(BigInt::pow2(exp), BigInt(1));
    else
      return BigRational(BigInt(1), BigInt::pow2(-exp));
  };

  if (x < get_p2(exponent)) exponent--;

  bool subnormal = (exponent < -1022);
  int shift = subnormal ? -1074 : (exponent - 52);

  BigInt scaled_num = x.num;
  BigInt scaled_den = x.den;
  if (shift >= 0) {
    scaled_den = scaled_den * BigInt::pow2(shift);
  } else {
    scaled_num = scaled_num * BigInt::pow2(-shift);
  }

  auto [q, remainder] = BigInt::divmod(scaled_num, scaled_den);

  if (!remainder.is_zero() && mode == "exact_only") {
    return make_result(std::nullopt, "error", "inexact");
  }

  BigInt two_rem = remainder * BigInt(2);
  bool parity = q.limbs.empty() ? false : ((q.limbs[0] % 2) != 0);

  if (two_rem > scaled_den || (two_rem == scaled_den && parity)) {
    q = q + BigInt(1);
  }

  uint64_t raw = 0;
  if (subnormal) {
    raw = q.limbs.empty() ? 0 : q.limbs[0];
  } else {
    if (q == BigInt::pow2(53)) {
      exponent++;
      q = q / BigInt(2);
    }
    uint64_t exp_bits = static_cast<uint64_t>(exponent + 1023);
    uint64_t mant_bits = (q - BigInt::pow2(52)).limbs.empty() ? 0 : (q - BigInt::pow2(52)).limbs[0];
    raw = (exp_bits << 52) | mant_bits;
  }

  if (value.negative) raw |= (1ULL << 63);

  std::stringstream ss;
  ss << std::setfill('0') << std::setw(16) << std::hex << raw;
  std::string hex_str = ss.str();
  std::string status = remainder.is_zero() ? "exact" : "rounded";

  return make_result(hex_str, status, std::nullopt, {});
}

inline ConversionResult to_binary64(std::string_view text, int m = 27, int e = 9,
                                    std::string_view mode = "exact_only") {
  auto err = validate(m, e, mode);
  if (err) return make_result(std::nullopt, "error", err);

  auto [val_opt, dec_err] = decode_ternary(text, m, e);
  if (dec_err) return make_result(std::nullopt, "error", dec_err);

  return binary64_from_rational(*val_opt, mode);
}

}  // namespace t81::experiments::exact_rational

#pragma GCC diagnostic pop
