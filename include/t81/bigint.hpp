#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include "t81/config.hpp"

namespace t81 {

struct DivModResult;

// Signed big integer in base 81 with canonical digits (0..80).
// Extremely small, dependency-free, "good enough" reference impl.
// Digits are LSB-first, each in [0..80].
class T81BigInt {
public:
  static constexpr std::uint16_t kRadix = 81;
  enum class Sign { Neg=-1, Zero=0, Pos=1 };
  friend struct DivModResult;
  friend DivModResult divmod(const T81BigInt& a, const T81BigInt& b);

  // --- ctors ---
  T81BigInt() : sign_(Sign::Zero) {}
  explicit T81BigInt(int64_t v) { *this = from_i64(v); }

  static T81BigInt zero() { return T81BigInt(); }
  static T81BigInt one()  { return from_i64(1); }

  static T81BigInt from_i64(int64_t v) {
    T81BigInt z;
    if (v == 0) { z.sign_ = Sign::Zero; return z; }
    z.sign_ = (v < 0) ? Sign::Neg : Sign::Pos;
    uint64_t n = (v < 0) ? uint64_t(-(v+1)) + 1ull : uint64_t(v);
    while (n) {
      z.d_.push_back(static_cast<uint8_t>(n % kRadix));
      n /= kRadix;
    }
    z.trim_();
    return z;
  }

  // Placeholder ASCII mapping (not canonical base-81 alphabet):
  // Optional sign, digits 0..80 separated by '.'; no leading zeros except "0".
  static T81BigInt from_ascii(std::string_view s) { return from_base81_digit_string(s); }

  static T81BigInt from_base81_digit_string(std::string_view s) {
    if (s.empty()) throw std::invalid_argument("T81BigInt::from_base81_digit_string: empty input");

    bool neg = false;
    std::size_t pos = 0;
    if (s[pos] == '+' || s[pos] == '-') {
      neg = (s[pos] == '-');
      ++pos;
      if (pos >= s.size()) throw std::invalid_argument("T81BigInt::from_base81_digit_string: sign only");
    }

    std::vector<int> digits;
    int current = 0;
    bool have_digit = false;

    for (; pos <= s.size(); ++pos) {
      if (pos == s.size() || s[pos] == '.') {
        if (!have_digit) throw std::invalid_argument("T81BigInt::from_base81_digit_string: empty digit");
        if (current < 0 || current >= kRadix) throw std::invalid_argument("T81BigInt::from_base81_digit_string: digit out of range");
        digits.push_back(current);
        current = 0;
        have_digit = false;
      } else if (s[pos] >= '0' && s[pos] <= '9') {
        have_digit = true;
        current = current * 10 + (s[pos] - '0');
        if (current >= kRadix) throw std::invalid_argument("T81BigInt::from_base81_digit_string: digit overflow");
      } else {
        throw std::invalid_argument("T81BigInt::from_base81_digit_string: invalid character");
      }
    }

    // Normalize leading zeros (except lone zero)
    std::size_t first_nonzero = 0;
    while (first_nonzero < digits.size() - 1 && digits[first_nonzero] == 0) ++first_nonzero;

    T81BigInt out;
    out.sign_ = Sign::Pos;
    out.d_.clear();

    const int base = kRadix;
    for (std::size_t i = first_nonzero; i < digits.size(); ++i) {
      mul_small_(out.d_, base, out.d_);
      add_small_(out.d_, static_cast<uint16_t>(digits[i]), out.d_);
    }

    out.trim_();
    if (out.is_zero()) {
      return zero();
    }
    if (neg) out.sign_ = Sign::Neg;
    return out;
  }

  // --- observers ---
  static bool is_zero(const T81BigInt& a) { return a.sign_ == Sign::Zero; }
  static bool is_one (const T81BigInt& a) { return a.sign_ == Sign::Pos && a.d_.size()==1 && a.d_[0]==1; }
  static bool is_neg (const T81BigInt& a) { return a.sign_ == Sign::Neg; }

  // Instance helpers
  bool is_zero() const { return is_zero(*this); }
  bool is_one () const { return is_one(*this); }
  bool is_negative() const { return sign_ == Sign::Neg; }
  bool is_positive() const { return sign_ == Sign::Pos; }

  T81BigInt abs() const { return abs(*this); }
  T81BigInt neg() const { return neg(*this); }

  static T81BigInt abs(const T81BigInt& a) { T81BigInt r=a; if (!is_zero(r)) r.sign_ = Sign::Pos; return r; }
  static T81BigInt neg(const T81BigInt& a) { T81BigInt r=a; if (!is_zero(r)) r.sign_ = (a.sign_==Sign::Pos?Sign::Neg:Sign::Pos); return r; }

  static int cmp(const T81BigInt& a, const T81BigInt& b) {
    if (a.sign_ != b.sign_) return (int)a.sign_ < (int)b.sign_ ? -1 : 1;
    if (a.sign_ == Sign::Zero) return 0;
    int m = cmp_mag_(a, b);
    return (a.sign_ == Sign::Pos) ? m : -m;
  }

  // --- operators (thin wrappers) ---
  T81BigInt operator-() const { return neg(*this); }
  T81BigInt operator+(const T81BigInt& o) const { return add(*this, o); }
  T81BigInt operator-(const T81BigInt& o) const { return sub(*this, o); }
  T81BigInt operator*(const T81BigInt& o) const { return mul(*this, o); }

  T81BigInt& operator+=(const T81BigInt& o) { *this = add(*this, o); return *this; }
  T81BigInt& operator-=(const T81BigInt& o) { *this = sub(*this, o); return *this; }
  T81BigInt& operator*=(const T81BigInt& o) { *this = mul(*this, o); return *this; }

  bool operator==(const T81BigInt& o) const { return cmp(*this, o) == 0; }
  bool operator!=(const T81BigInt& o) const { return !(*this == o); }
  bool operator<(const T81BigInt& o)  const { return cmp(*this, o) < 0; }
  bool operator>(const T81BigInt& o)  const { return cmp(*this, o) > 0; }
  bool operator<=(const T81BigInt& o) const { return cmp(*this, o) <= 0; }
  bool operator>=(const T81BigInt& o) const { return cmp(*this, o) >= 0; }
  // Convert to int64_t when representable; throws on overflow.
  std::int64_t to_int64() const {
    if (sign_ == Sign::Zero) return 0;

    uint64_t magnitude = 0;
    const uint64_t positive_limit = static_cast<uint64_t>(INT64_MAX);
    const uint64_t negative_limit = static_cast<uint64_t>(INT64_MAX) + 1;
    const uint64_t limit = (sign_ == Sign::Pos) ? positive_limit : negative_limit;

    for (int i = static_cast<int>(d_.size()) - 1; i >= 0; --i) {
      const uint8_t digit = d_[static_cast<size_t>(i)];

      // Check for overflow before multiplication and addition
      if (magnitude > (limit - digit) / kRadix) {
        throw std::overflow_error("T81BigInt too large for int64_t");
      }
      magnitude = magnitude * kRadix + digit;
    }

    if (sign_ == Sign::Pos) {
      return static_cast<int64_t>(magnitude);
    } else { // sign_ == Sign::Neg
      if (magnitude == negative_limit) {
        return INT64_MIN;
      }
      return -static_cast<int64_t>(magnitude);
    }
  }

  // --- arithmetic ---
  static T81BigInt add(const T81BigInt& a, const T81BigInt& b) {
    if (is_zero(a)) return b;
    if (is_zero(b)) return a;
    if (a.sign_ == b.sign_) {
      T81BigInt r; r.sign_ = a.sign_; add_mag_(a.d_, b.d_, r.d_); r.trim_(); return r;
    }
    // different signs → subtract magnitudes
    int m = cmp_mag_(a,b);
    if (m == 0) return zero();
    T81BigInt r;
    if (m > 0) { r.sign_ = a.sign_; sub_mag_(a.d_, b.d_, r.d_); }
    else       { r.sign_ = b.sign_; sub_mag_(b.d_, a.d_, r.d_); }
    r.trim_(); return r;
  }

  static T81BigInt sub(const T81BigInt& a, const T81BigInt& b) {
    return add(a, neg(b));
  }

  static T81BigInt mul(const T81BigInt& a, const T81BigInt& b) {
    if (is_zero(a) || is_zero(b)) return zero();
    T81BigInt r;
    r.sign_ = (a.sign_ == b.sign_) ? Sign::Pos : Sign::Neg;
    schoolbook_mul_(a.d_, b.d_, r.d_);
    r.trim_(); return r;
  }

  // Very naive modulus: repeated subtraction by shifted divisor (long division skeleton).
  // Works but is slow; only intended for tiny values / tests.
  static T81BigInt mod(const T81BigInt& a, const T81BigInt& b) {
    if (is_zero(b)) throw std::domain_error("BigInt mod by zero");
    if (is_zero(a)) return zero();
    T81BigInt A = abs(a);
    T81BigInt B = abs(b);
    if (cmp_mag_(A,B) < 0) return A;
    T81BigInt q, r;
    divmod_nonneg_(A, B, q, r);
    if (!is_zero(r)) r.sign_ = Sign::Pos;
    return r;
  }

  // Euclidean GCD using modulus (very slow, but deterministic/safe)
  static T81BigInt gcd(T81BigInt a, T81BigInt b) {
    a = abs(a); b = abs(b);
    if (is_zero(a)) return b;
    if (is_zero(b)) return a;
    while (!is_zero(b)) {
      T81BigInt r = mod(a, b);
      a = std::move(b);
      b = std::move(r);
    }
    return a;
  }

  static T81BigInt pow(const T81BigInt& base, const T81BigInt& exp) {
    if (exp.is_negative()) {
      throw std::domain_error("BigInt pow: negative exponent");
    }
    if (is_zero(exp)) return one();
    if (is_zero(base)) return zero();

    T81BigInt result = one();
    T81BigInt b = abs(base);
    T81BigInt e = exp;
    const T81BigInt two(2);

    while (e > zero()) {
      if (!is_zero(mod(e, two))) { // if e is odd
        result = mul(result, b);
      }
      b = mul(b, b);
      T81BigInt q, r;
      divmod_nonneg_(e, two, q, r);
      e = q;
    }

    if (base.is_negative() && !is_zero(mod(exp, two))) { // if base is negative and exp is odd
      return neg(result);
    }
    return result;
  }

  // Exact division (a / b) when remainder is zero. Throws otherwise.
  static T81BigInt div(const T81BigInt& a, const T81BigInt& b) {
    if (is_zero(b)) throw std::domain_error("BigInt div by zero");
    if (is_zero(a)) return zero();
    T81BigInt A = abs(a);
    T81BigInt B = abs(b);
    if (cmp_mag_(A,B) < 0) return zero();

    T81BigInt Q, R;
    divmod_nonneg_(A, B, Q, R);
    if (!is_zero(R)) throw std::domain_error("BigInt div: non-zero remainder");
    Q.sign_ = (a.sign_ == b.sign_) ? Sign::Pos : Sign::Neg;
    Q.trim_();
    return Q;
  }

  // --- formatting (debug; not canonical) ---
  // Renders sign + groups of digits (base-81) as decimal-like chunks.
  std::string to_string() const {
    if (sign_ == Sign::Zero) return "0";
    std::ostringstream os;
    if (sign_ == Sign::Neg) os << "-";
    // Render most significant first
    for (int i = (int)d_.size()-1; i >= 0; --i) {
      if (i == (int)d_.size()-1) os << int(d_[(size_t)i]);
      else { os << '.' << int(d_[(size_t)i]); }
    }
    return os.str();
  }

  // Canonical base-81 string: optional sign + digits [0..80] separated by '.' (MSB-first).
  std::string to_base81_string() const;
  static T81BigInt from_base81_string(std::string_view s);

private:
  Sign sign_{Sign::Zero};
  std::vector<uint8_t> d_; // LSB-first, base 81

  void trim_() {
    while (!d_.empty() && d_.back() == 0) d_.pop_back();
    if (d_.empty()) sign_ = Sign::Zero;
  }

  // --- magnitude helpers (no sign) ---
  static int cmp_mag_(const T81BigInt& a, const T81BigInt& b) {
    return cmp_mag_digits_(a.d_, b.d_);
  }
  static int cmp_mag_digits_(const std::vector<uint8_t>& A, const std::vector<uint8_t>& B) {
    if (A.size() != B.size()) return (A.size() < B.size()) ? -1 : 1;
    for (int i = (int)A.size()-1; i >= 0; --i) {
      if (A[(size_t)i] != B[(size_t)i]) return (A[(size_t)i] < B[(size_t)i]) ? -1 : 1;
    }
    return 0;
  }

  static void add_mag_(const std::vector<uint8_t>& A, const std::vector<uint8_t>& B, std::vector<uint8_t>& out) {
    const size_t n = std::max(A.size(), B.size());
    out.clear(); out.resize(n, 0);
    uint16_t carry = 0;
    for (size_t i = 0; i < n; ++i) {
      uint16_t a = (i < A.size() ? A[i] : 0);
      uint16_t b = (i < B.size() ? B[i] : 0);
      uint16_t s = a + b + carry;
      out[i] = static_cast<uint8_t>(s % kRadix);
      carry  = static_cast<uint16_t>(s / kRadix);
    }
    if (carry) out.push_back(static_cast<uint8_t>(carry));
  }

  static void sub_mag_(const std::vector<uint8_t>& A, const std::vector<uint8_t>& B, std::vector<uint8_t>& out) {
    // assumes A >= B
    out.clear(); out.resize(A.size(), 0);
    int16_t borrow = 0;
    for (size_t i = 0; i < A.size(); ++i) {
      int16_t a = A[i];
      int16_t b = (i < B.size() ? B[i] : 0);
      int16_t s = a - b - borrow;
      if (s < 0) { s += kRadix; borrow = 1; } else borrow = 0;
      out[i] = static_cast<uint8_t>(s);
    }
    // trim handled by caller
    while (!out.empty() && out.back()==0) out.pop_back();
  }

  static void schoolbook_mul_(const std::vector<uint8_t>& A, const std::vector<uint8_t>& B, std::vector<uint8_t>& out) {
    out.assign(A.size()+B.size(), 0);
    for (size_t i = 0; i < A.size(); ++i) {
      uint32_t carry = 0;
      for (size_t j = 0; j < B.size(); ++j) {
        uint32_t s = out[i+j] + uint32_t(A[i]) * uint32_t(B[j]) + carry;
        out[i+j] = static_cast<uint8_t>(s % kRadix);
        carry    = static_cast<uint32_t>(s / kRadix);
      }
      size_t k = i + B.size();
      while (carry) {
        uint32_t s = out[k] + carry;
        out[k] = static_cast<uint8_t>(s % kRadix);
        carry  = static_cast<uint32_t>(s / kRadix);
        ++k;
      }
    }
    while (!out.empty() && out.back()==0) out.pop_back();
  }

  static void mul_small_(const std::vector<uint8_t>& A, uint16_t m, std::vector<uint8_t>& out) {
    if (m == 0 || A.empty()) { out.clear(); return; }
    if (m == 1) { out = A; return; }
    out.resize(A.size());
    uint32_t carry = 0;
    for (size_t i = 0; i < A.size(); ++i) {
      uint32_t s = uint32_t(A[i]) * m + carry;
      out[i] = static_cast<uint8_t>(s % kRadix);
      carry  = static_cast<uint32_t>(s / kRadix);
    }
    while (carry) {
      out.push_back(static_cast<uint8_t>(carry % kRadix));
      carry /= kRadix;
    }
    while (!out.empty() && out.back()==0) out.pop_back();
  }

  static void add_small_(const std::vector<uint8_t>& A, uint16_t m, std::vector<uint8_t>& out) {
    out = A;
    uint32_t s = (out.empty() ? 0u : out[0]) + m;
    if (out.empty()) out.push_back(0);
    out[0] = static_cast<uint8_t>(s % kRadix);
    uint32_t carry = s / kRadix;
    size_t i = 1;
    while (carry) {
      if (i >= out.size()) out.push_back(0);
      uint32_t t = out[i] + carry;
      out[i] = static_cast<uint8_t>(t % kRadix);
      carry  = t / kRadix;
      ++i;
    }
    while (!out.empty() && out.back()==0) out.pop_back();
  }

  // Slow but simple non-negative divmod used by stubs/tests.
  static void divmod_nonneg_(const T81BigInt& ua, const T81BigInt& ub,
                             T81BigInt& uq, T81BigInt& ur) {
    if (is_zero(ub)) throw std::domain_error("BigInt div by zero");
    uq = zero();
    ur = ua;
    if (cmp(ua, ub) < 0) return;
    const T81BigInt one_val = one();

    while (cmp(ur, ub) >= 0) {
      T81BigInt multiple = ub;
      T81BigInt factor   = one_val;
      while (cmp(add(multiple, multiple), ur) <= 0) {
        multiple = add(multiple, multiple);
        factor   = add(factor, factor);
      }
      ur = sub(ur, multiple);
      uq = add(uq, factor);
    }
  }
};

struct DivModResult {
  T81BigInt q;  // quotient
  T81BigInt r;  // remainder
};

// Canonical base-81 alphabet (Unicode, spec/v1.1.0-canonical.md)
namespace detail {
inline const std::vector<std::string>& base81_alphabet_vec() {
  static const std::vector<std::string> kAlphabet = {
    "0","1","2","3","4","5","6","7","8","9",
    "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z",
    "+","−","×","÷","=","<",">","≤","≥","≠","≈","∞","λ","μ","π","σ","τ","ω","Γ"
  };
  return kAlphabet;
}

inline const std::unordered_map<std::string, int>& base81_digit_map() {
  static const std::unordered_map<std::string, int> kMap = []{
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
  if (c < 0x80) len = 1;
  else if ((c & 0xE0) == 0xC0) len = 2;
  else if ((c & 0xF0) == 0xE0) len = 3;
  else if ((c & 0xF8) == 0xF0) len = 4;
  else return {};
  if (offset + len > s.size()) return {};
  std::string cp(s.substr(offset, len));
  offset += len;
  return cp;
}
} // namespace detail

// Inline implementations that depend on DivModResult being complete.
inline std::string T81BigInt::to_base81_string() const {
  if (sign_ == Sign::Zero) return "0";

  const T81BigInt base(kRadix);
  T81BigInt v = abs(*this);
  std::vector<int> digits;
  digits.reserve(48);

  while (!is_zero(v)) {
    auto dm = divmod(v, base);
    int d = static_cast<int>(dm.r.to_int64());
    digits.push_back(d);
    v = dm.q;
  }

  std::string out;
  if (sign_ == Sign::Neg) out.push_back('-');
  const auto& alpha = detail::base81_alphabet_vec();
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
    out += alpha[static_cast<std::size_t>(*it)];
  }
  return out;
}

inline T81BigInt T81BigInt::from_base81_string(std::string_view s) {
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
    if (cp.empty()) throw std::invalid_argument("T81BigInt::from_base81_string: invalid encoding");
    auto it = map.find(cp);
    if (it == map.end()) throw std::invalid_argument("T81BigInt::from_base81_string: invalid character");
    digits.push_back(it->second);
  }
  if (digits.size() > 1 && digits.front() == 0) {
    throw std::invalid_argument("T81BigInt::from_base81_string: non-canonical leading zero");
  }

  T81BigInt base(kRadix);
  T81BigInt v(0);
  for (int d : digits) {
    v = mul(v, base);
    v = add(v, T81BigInt(d));
  }
  if (neg && !is_zero(v)) v.sign_ = Sign::Neg;
  return v;
}

// Legacy alias preserved for downstream code migrating from the old API.
using T243BigInt = T81BigInt;

} // namespace t81
