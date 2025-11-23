#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace t81 {

// Base-243 big integer with sign.
// Digits are LSB-first, each in [0..242].
class T243BigInt {
public:
  using digit_t = uint8_t; // 0..242
  enum class Sign : int8_t { Neg = -1, Zero = 0, Pos = 1 };

  // --- ctors ---
  T243BigInt() : sign_(Sign::Zero) {}
  explicit T243BigInt(std::vector<digit_t> mag, Sign s = Sign::Pos)
      : sign_(s), digits_(std::move(mag)) { normalize_(); }

  // Very simple ASCII-to-base243 placeholder (same as before).
  static T243BigInt from_ascii(const std::string& s) {
    std::vector<digit_t> d(s.size());
    bool any = false;
    for (size_t i=0;i<s.size();++i) {
      d[i] = static_cast<digit_t>(static_cast<unsigned char>(s[i]) % 243);
      any = any || (d[i] != 0);
    }
    return T243BigInt(std::move(d), any ? Sign::Pos : Sign::Zero);
  }

  // --- queries ---
  Sign sign() const { return sign_; }
  bool is_zero() const { return sign_ == Sign::Zero; }
  const std::vector<digit_t>& digits() const { return digits_; }

  // --- formatting ---
  std::string to_string() const {
    if (is_zero()) return "000";
    std::ostringstream oss;
    if (sign_ == Sign::Neg) oss << "-";
    for (size_t i = digits_.size(); i-- > 0;) {
      oss.width(3); oss.fill('0');
      oss << static_cast<unsigned>(digits_[i]);
    }
    return oss.str();
  }

  // --- comparison (absolute) ---
  static int cmp_abs(const T243BigInt& a, const T243BigInt& b) {
    if (a.digits_.size() != b.digits_.size())
      return (a.digits_.size() < b.digits_.size()) ? -1 : 1;
    for (size_t i = a.digits_.size(); i-- > 0;) {
      if (a.digits_[i] != b.digits_[i])
        return (a.digits_[i] < b.digits_[i]) ? -1 : 1;
    }
    return 0;
  }

  // --- arithmetic ---
  static T243BigInt add(const T243BigInt& a, const T243BigInt& b) {
    if (a.is_zero()) return b;
    if (b.is_zero()) return a;

    if (a.sign_ == b.sign_) {
      T243BigInt r(add_abs_(a, b), a.sign_);
      r.normalize_();
      return r;
    } else {
      int c = cmp_abs(a, b);
      if (c == 0) return T243BigInt(); // zero
      if (c > 0) {
        T243BigInt r(sub_abs_(a, b), a.sign_);
        r.normalize_();
        return r;
      } else {
        T243BigInt r(sub_abs_(b, a), b.sign_);
        r.normalize_();
        return r;
      }
    }
  }

  static T243BigInt sub(const T243BigInt& a, const T243BigInt& b) {
    T243BigInt nb = b;
    if (!nb.is_zero()) nb.sign_ = (nb.sign_ == Sign::Pos ? Sign::Neg :
                                   nb.sign_ == Sign::Neg ? Sign::Pos : Sign::Zero);
    return add(a, nb);
  }

  static T243BigInt mul(const T243BigInt& a, const T243BigInt& b) {
    if (a.is_zero() || b.is_zero()) return T243BigInt();
    std::vector<uint16_t> tmp(a.digits_.size() + b.digits_.size(), 0);
    for (size_t i=0;i<a.digits_.size();++i)
      for (size_t j=0;j<b.digits_.size();++j)
        tmp[i+j] += static_cast<uint16_t>(a.digits_[i]) * b.digits_[j];

    std::vector<digit_t> r(tmp.size(), 0);
    uint32_t carry = 0;
    for (size_t k=0;k<tmp.size();++k) {
      uint32_t v = tmp[k] + carry;
      r[k] = static_cast<digit_t>(v % 243);
      carry = v / 243;
    }
    if (carry) r.push_back(static_cast<digit_t>(carry % 243));

    T243BigInt out(std::move(r),
                   (a.sign_ == b.sign_) ? Sign::Pos : Sign::Neg);
    out.normalize_();
    return out;
  }

  // Very simple modulus using repeated subtraction on magnitudes.
  // NOTE: This is intentionally naive and suitable only for small values / tests.
  static T243BigInt mod(T243BigInt a, const T243BigInt& b) {
    if (b.is_zero()) throw std::invalid_argument("mod by zero");
    // Work on absolute values
    a.sign_ = (a.is_zero() ? Sign::Zero : Sign::Pos);
    T243BigInt bb = b;
    bb.sign_ = Sign::Pos;
    if (cmp_abs(a, bb) < 0) return a;
    while (!a.is_zero() && cmp_abs(a, bb) >= 0) {
      a = T243BigInt(sub_abs_(a, bb), Sign::Pos);
      a.normalize_();
    }
    return a; // remainder is non-negative
  }

  // gcd(a,b) with non-negative result
  static T243BigInt gcd(T243BigInt a, T243BigInt b) {
    a.sign_ = a.is_zero() ? Sign::Zero : Sign::Pos;
    b.sign_ = b.is_zero() ? Sign::Zero : Sign::Pos;
    while (!b.is_zero()) {
      T243BigInt r = mod(a, b);
      a = b;
      b = r;
    }
    a.sign_ = a.is_zero() ? Sign::Zero : Sign::Pos;
    return a;
  }

private:
  // Normalize: carry propagation (base 243), strip leading zeros, fix sign.
  void normalize_() {
    // propagate carries (digits_ may temporarily be >242 from add/sub helpers)
    uint32_t carry = 0;
    for (size_t i=0;i<digits_.size();++i) {
      uint32_t v = static_cast<uint32_t>(digits_[i]) + carry;
      digits_[i] = static_cast<digit_t>(v % 243);
      carry = v / 243;
    }
    while (carry) {
      digits_.push_back(static_cast<digit_t>(carry % 243));
      carry /= 243;
    }
    while (!digits_.empty() && digits_.back() == 0) digits_.pop_back();
    if (digits_.empty()) { sign_ = Sign::Zero; }
    else if (sign_ == Sign::Zero) { sign_ = Sign::Pos; }
  }

  // magnitude addition: |a| + |b|
  static std::vector<digit_t> add_abs_(const T243BigInt& a, const T243BigInt& b) {
    const size_t max_len = std::max(a.digits_.size(), b.digits_.size());
    std::vector<uint16_t> tmp(max_len + 1, 0);
    for (size_t i=0;i<max_len;++i) {
      uint16_t v = tmp[i];
      if (i < a.digits_.size()) v += a.digits_[i];
      if (i < b.digits_.size()) v += b.digits_[i];
      tmp[i] = v;
    }
    // fold to base-243
    std::vector<digit_t> r(tmp.size(), 0);
    uint32_t carry = 0;
    for (size_t i=0;i<tmp.size();++i) {
      uint32_t v = tmp[i] + carry;
      r[i] = static_cast<digit_t>(v % 243);
      carry = v / 243;
    }
    if (carry) r.push_back(static_cast<digit_t>(carry % 243));
    return r;
  }

  // magnitude subtraction: |a| - |b|, requires |a| >= |b|
  static std::vector<digit_t> sub_abs_(const T243BigInt& a, const T243BigInt& b) {
    std::vector<int32_t> tmp(std::max(a.digits_.size(), b.digits_.size()) + 1, 0);
    for (size_t i=0;i<a.digits_.size();++i) tmp[i] += a.digits_[i];
    for (size_t i=0;i<b.digits_.size();++i) tmp[i] -= b.digits_[i];

    // borrow normalize in base-243
    for (size_t i=0;i+1<tmp.size();++i) {
      while (tmp[i] < 0) {
        tmp[i] += 243;
        tmp[i+1] -= 1;
      }
      if (tmp[i] >= 243) {
        int q = tmp[i] / 243;
        tmp[i] -= q * 243;
        tmp[i+1] += q;
      }
    }
    // convert back to digits
    std::vector<digit_t> r(tmp.size(), 0);
    for (size_t i=0;i<tmp.size();++i) {
      int v = tmp[i];
      if (v < 0) v = 0; // any remaining negative means higher digits will clear it
      r[i] = static_cast<digit_t>(v % 243);
    }
    // trim
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
  }

  Sign sign_;
  std::vector<digit_t> digits_; // magnitude, LSB-first
};

} // namespace t81
