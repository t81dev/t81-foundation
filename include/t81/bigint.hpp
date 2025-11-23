#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>
#include "t81/config.hpp"

namespace t81 {

// Signed big integer in base 243 (balanced-ternary friendly radix).
// Extremely small, dependency-free, "good enough" reference impl.
// Digits are LSB-first, each in [0..242].
class T243BigInt {
public:
  enum class Sign { Neg=-1, Zero=0, Pos=1 };

  // --- ctors ---
  T243BigInt() : sign_(Sign::Zero) {}
  explicit T243BigInt(int64_t v) { *this = from_i64(v); }

  static T243BigInt zero() { return T243BigInt(); }
  static T243BigInt one()  { return from_i64(1); }

  static T243BigInt from_i64(int64_t v) {
    T243BigInt z;
    if (v == 0) { z.sign_ = Sign::Zero; return z; }
    z.sign_ = (v < 0) ? Sign::Neg : Sign::Pos;
    uint64_t n = (v < 0) ? uint64_t(-(v+1)) + 1ull : uint64_t(v);
    while (n) {
      z.d_.push_back(static_cast<uint8_t>(n % 243ull));
      n /= 243ull;
    }
    z.trim_();
    return z;
  }

  // Placeholder ASCII mapping (not canonical Base-243!):
  // encodes each char c as a single digit = c % 243, sign is Pos unless string starts with '-'.
  static T243BigInt from_ascii(const std::string& s) {
    if (s.empty()) return zero();
    T243BigInt z;
    size_t i = 0;
    if (s[0] == '-') { z.sign_ = Sign::Neg; i = 1; }
    else             { z.sign_ = Sign::Pos; }
    z.d_.reserve(s.size());
    for (; i < s.size(); ++i) z.d_.push_back(static_cast<uint8_t>(static_cast<unsigned char>(s[i]) % 243));
    if (z.d_.empty()) z.sign_ = Sign::Zero;
    z.trim_();
    return z;
  }

  // --- observers ---
  static bool is_zero(const T243BigInt& a) { return a.sign_ == Sign::Zero; }
  static bool is_one (const T243BigInt& a) { return a.sign_ == Sign::Pos && a.d_.size()==1 && a.d_[0]==1; }
  static bool is_neg (const T243BigInt& a) { return a.sign_ == Sign::Neg; }

  static T243BigInt abs(const T243BigInt& a) { T243BigInt r=a; if (!is_zero(r)) r.sign_ = Sign::Pos; return r; }
  static T243BigInt neg(const T243BigInt& a) { T243BigInt r=a; if (!is_zero(r)) r.sign_ = (a.sign_==Sign::Pos?Sign::Neg:Sign::Pos); return r; }

  static int cmp(const T243BigInt& a, const T243BigInt& b) {
    if (a.sign_ != b.sign_) return (int)a.sign_ < (int)b.sign_ ? -1 : 1;
    if (a.sign_ == Sign::Zero) return 0;
    int m = cmp_mag_(a, b);
    return (a.sign_ == Sign::Pos) ? m : -m;
  }

  // --- arithmetic ---
  static T243BigInt add(const T243BigInt& a, const T243BigInt& b) {
    if (is_zero(a)) return b;
    if (is_zero(b)) return a;
    if (a.sign_ == b.sign_) {
      T243BigInt r; r.sign_ = a.sign_; add_mag_(a.d_, b.d_, r.d_); r.trim_(); return r;
    }
    // different signs → subtract magnitudes
    int m = cmp_mag_(a,b);
    if (m == 0) return zero();
    T243BigInt r;
    if (m > 0) { r.sign_ = a.sign_; sub_mag_(a.d_, b.d_, r.d_); }
    else       { r.sign_ = b.sign_; sub_mag_(b.d_, a.d_, r.d_); }
    r.trim_(); return r;
  }

  static T243BigInt sub(const T243BigInt& a, const T243BigInt& b) {
    return add(a, neg(b));
  }

  static T243BigInt mul(const T243BigInt& a, const T243BigInt& b) {
    if (is_zero(a) || is_zero(b)) return zero();
    T243BigInt r;
    r.sign_ = (a.sign_ == b.sign_) ? Sign::Pos : Sign::Neg;
    schoolbook_mul_(a.d_, b.d_, r.d_);
    r.trim_(); return r;
  }

  // Very naive modulus: repeated subtraction by shifted divisor (long division skeleton).
  // Works but is slow; only intended for tiny values / tests.
  static T243BigInt mod(const T243BigInt& a, const T243BigInt& b) {
    if (is_zero(b)) throw std::domain_error("BigInt mod by zero");
    if (is_zero(a)) return zero();
    T243BigInt A = abs(a);
    T243BigInt B = abs(b);
    if (cmp_mag_(A,B) < 0) return A;

    T243BigInt R = zero();
    // long-division style using base 243
    for (int i = (int)A.d_.size()-1; i >= 0; --i) {
      // R = R * 243 + A.d_[i]
      mul_small_(R.d_, 243, R.d_);
      add_small_(R.d_, A.d_[(size_t)i], R.d_);
      // subtract B as many times as fits (naive)
      while (cmp_mag_digits_(R.d_, B.d_) >= 0) {
        sub_mag_(R.d_, B.d_, R.d_);
      }
    }
    R.trim_();
    if (!is_zero(R)) R.sign_ = Sign::Pos;
    return R;
  }

  // Euclidean GCD using modulus (very slow, but deterministic/safe)
  static T243BigInt gcd(T243BigInt a, T243BigInt b) {
    a = abs(a); b = abs(b);
    if (is_zero(a)) return b;
    if (is_zero(b)) return a;
    while (!is_zero(b)) {
      T243BigInt r = mod(a, b);
      a = std::move(b);
      b = std::move(r);
    }
    return a;
  }

  // Exact division (a / b) when remainder is zero. Throws otherwise.
  static T243BigInt div(const T243BigInt& a, const T243BigInt& b) {
    if (is_zero(b)) throw std::domain_error("BigInt div by zero");
    if (is_zero(a)) return zero();
    // Long division in base 243
    T243BigInt A = abs(a);
    T243BigInt B = abs(b);
    if (cmp_mag_(A,B) < 0) return zero();

    std::vector<uint8_t> q(A.d_.size(), 0);
    T243BigInt R = zero();

    for (int i = (int)A.d_.size()-1; i >= 0; --i) {
      // R = R*243 + A.d_[i]
      mul_small_(R.d_, 243, R.d_);
      add_small_(R.d_, A.d_[(size_t)i], R.d_);

      // Find the largest digit qd in [0..242] such that B*qd <= R (by magnitude)
      // Naive linear search (OK for tiny values). Could be improved by binary search.
      uint8_t qd = 0;
      if (!is_zero(R)) {
        // upper-bound small heuristic: compare top digits
        for (uint16_t t = 242; t > 0; --t) {
          T243BigInt tmp; mul_small_(B.d_, (uint8_t)t, tmp.d_);
          if (cmp_mag_digits_(tmp.d_, R.d_) <= 0) { qd = static_cast<uint8_t>(t); break; }
        }
      }
      q[(size_t)i] = qd;
      if (qd) {
        T243BigInt t; mul_small_(B.d_, qd, t.d_);
        sub_mag_(R.d_, t.d_, R.d_);
      }
    }

    // remainder must be zero for exact division
    if (!R.d_.empty()) {
      // Verify truly zero
      bool any=false; for (auto v: R.d_) if (v){any=true;break;}
      if (any) throw std::domain_error("BigInt div: non-zero remainder");
    }

    T243BigInt Q;
    Q.sign_ = (a.sign_ == b.sign_) ? Sign::Pos : Sign::Neg;
    Q.d_ = std::move(q);
    Q.trim_();
    return Q;
  }

  // --- formatting (debug; not canonical) ---
  // Renders sign + groups of digits (base-243) as decimal-like chunks.
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

private:
  Sign sign_{Sign::Zero};
  std::vector<uint8_t> d_; // LSB-first, base 243

  void trim_() {
    while (!d_.empty() && d_.back() == 0) d_.pop_back();
    if (d_.empty()) sign_ = Sign::Zero;
  }

  // --- magnitude helpers (no sign) ---
  static int cmp_mag_(const T243BigInt& a, const T243BigInt& b) {
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
      out[i] = static_cast<uint8_t>(s % 243);
      carry  = static_cast<uint16_t>(s / 243);
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
      if (s < 0) { s += 243; borrow = 1; } else borrow = 0;
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
        out[i+j] = static_cast<uint8_t>(s % 243u);
        carry    = static_cast<uint32_t>(s / 243u);
      }
      size_t k = i + B.size();
      while (carry) {
        uint32_t s = out[k] + carry;
        out[k] = static_cast<uint8_t>(s % 243u);
        carry  = static_cast<uint32_t>(s / 243u);
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
      out[i] = static_cast<uint8_t>(s % 243u);
      carry  = static_cast<uint32_t>(s / 243u);
    }
    while (carry) {
      out.push_back(static_cast<uint8_t>(carry % 243u));
      carry /= 243u;
    }
    while (!out.empty() && out.back()==0) out.pop_back();
  }

  static void add_small_(const std::vector<uint8_t>& A, uint16_t m, std::vector<uint8_t>& out) {
    out = A;
    uint32_t s = (out.empty() ? 0u : out[0]) + m;
    if (out.empty()) out.push_back(0);
    out[0] = static_cast<uint8_t>(s % 243u);
    uint32_t carry = s / 243u;
    size_t i = 1;
    while (carry) {
      if (i >= out.size()) out.push_back(0);
      uint32_t t = out[i] + carry;
      out[i] = static_cast<uint8_t>(t % 243u);
      carry  = t / 243u;
      ++i;
    }
    while (!out.empty() && out.back()==0) out.pop_back();
  }
};

} // namespace t81
