#pragma once
#include <stdexcept>
#include <string>
#include "t81/bigint.hpp"

namespace t81 {

// Minimal rational built over T243BigInt.
// Notes:
//  - Non-negative values only (until T243BigInt adds signed ops).
//  - No reduction/gcd yet; normalization only enforces den != 0 and num==0 => den=1.
//  - to_string encodes "NUM/DEN" using T243BigInt::to_string().
class T81Fraction {
public:
  T81Fraction() : num_(), den_(from_uint_(1)) {}
  explicit T81Fraction(const T243BigInt& n) : num_(n), den_(from_uint_(1)) {}
  T81Fraction(const T243BigInt& n, const T243BigInt& d) : num_(n), den_(d) {
    if (is_zero_(den_)) throw std::invalid_argument("denominator must be non-zero");
    normalize_zero_();
  }

  static T81Fraction make(const std::string& num_ascii, const std::string& den_ascii) {
    return T81Fraction(T243BigInt::from_ascii(num_ascii), T243BigInt::from_ascii(den_ascii));
  }

  const T243BigInt& num() const { return num_; }
  const T243BigInt& den() const { return den_; }

  // r = a + b = (a.num*b.den + b.num*a.den) / (a.den*b.den)
  static T81Fraction add(const T81Fraction& a, const T81Fraction& b) {
    T243BigInt ad = T243BigInt::mul(a.num_, b.den_);
    T243BigInt bc = T243BigInt::mul(b.num_, a.den_);
    T243BigInt n  = T243BigInt::add(ad, bc);
    T243BigInt d  = T243BigInt::mul(a.den_, b.den_);
    return T81Fraction(n, d);
  }

  // r = a * b
  static T81Fraction mul(const T81Fraction& a, const T81Fraction& b) {
    return T81Fraction(T243BigInt::mul(a.num_, b.num_), T243BigInt::mul(a.den_, b.den_));
  }

  std::string to_string() const {
    return num_.to_string() + "/" + den_.to_string();
  }

private:
  static bool is_zero_(const T243BigInt& x) { return x.digits().empty(); }
  static T243BigInt from_uint_(unsigned v) {
    // Represent small integers by ASCII mapping of 'v' (deterministic placeholder).
    // For canonical builds, replace with true base-243 encoding.
    std::string s = std::to_string(v);
    return T243BigInt::from_ascii(s);
  }
  void normalize_zero_() {
    if (is_zero_(num_)) {
      den_ = from_uint_(1);
    }
  }

  T243BigInt num_;
  T243BigInt den_;
};

} // namespace t81
