#pragma once
#include <stdexcept>
#include <string>
#include "t81/bigint.hpp"

namespace t81 {

// Signed rational over T243BigInt.
// Guarantees:
//  • Denominator is always positive.
//  • Value is always reduced by gcd(|num|, |den|).
//  • Zero is canonically 0/1.
class T81Fraction {
public:
  T81Fraction() : num_(), den_(one_()) {}                         // 0/1
  explicit T81Fraction(const T243BigInt& n) : num_(n), den_(one_()) { normalize_(); }
  T81Fraction(const T243BigInt& n, const T243BigInt& d) : num_(n), den_(d) {
    if (d.is_zero()) throw std::invalid_argument("denominator must be non-zero");
    normalize_();
  }

  // Convenience: construct from two ASCII strings (same placeholder encoding as T243BigInt::from_ascii).
  static T81Fraction make(const std::string& num_ascii, const std::string& den_ascii) {
    return T81Fraction(T243BigInt::from_ascii(num_ascii), T243BigInt::from_ascii(den_ascii));
  }

  // Accessors
  const T243BigInt& num() const { return num_; }
  const T243BigInt& den() const { return den_; }

  // Arithmetic
  static T81Fraction add(const T81Fraction& a, const T81Fraction& b) {
    // (a.n * b.d + b.n * a.d) / (a.d * b.d)
    T243BigInt ad = T243BigInt::mul(a.num_, b.den_);
    T243BigInt bc = T243BigInt::mul(b.num_, a.den_);
    T243BigInt n  = T243BigInt::add(ad, bc);
    T243BigInt d  = T243BigInt::mul(a.den_, b.den_);
    return T81Fraction(n, d);
  }

  static T81Fraction sub(const T81Fraction& a, const T81Fraction& b) {
    // (a.n * b.d - b.n * a.d) / (a.d * b.d)
    T243BigInt ad = T243BigInt::mul(a.num_, b.den_);
    T243BigInt bc = T243BigInt::mul(b.num_, a.den_);
    T243BigInt n  = T243BigInt::sub(ad, bc);
    T243BigInt d  = T243BigInt::mul(a.den_, b.den_);
    return T81Fraction(n, d);
  }

  static T81Fraction mul(const T81Fraction& a, const T81Fraction& b) {
    return T81Fraction(T243BigInt::mul(a.num_, b.num_), T243BigInt::mul(a.den_, b.den_));
  }

  static T81Fraction div(const T81Fraction& a, const T81Fraction& b) {
    if (b.num_.is_zero()) throw std::invalid_argument("divide by zero");
    return T81Fraction(T243BigInt::mul(a.num_, b.den_), T243BigInt::mul(a.den_, b.num_));
  }

  // Formatting (debug-oriented): NUM/DEN using T243BigInt::to_string()
  std::string to_string() const { return num_.to_string() + "/" + den_.to_string(); }

private:
  static T243BigInt one_() { return T243BigInt::from_ascii("1"); }
  static T243BigInt zero_(){ return T243BigInt(); }

  static T243BigInt abs_(T243BigInt v) {
    if (!v.is_zero() && v.sign() == T243BigInt::Sign::Neg) {
      // flip sign by subtracting from zero
      v = T243BigInt::sub(zero_(), v);
    }
    return v;
  }

  void normalize_() {
    // Move sign to numerator: ensure denominator positive.
    if (!den_.is_zero() && den_.sign() == T243BigInt::Sign::Neg) {
      den_ = T243BigInt::sub(zero_(), den_);
      num_ = T243BigInt::sub(zero_(), num_);
    }

    // If numerator is zero → canonical 0/1
    if (num_.is_zero()) {
      den_ = one_();
      return;
    }

    // Reduce by gcd(|num|, |den|)
    T243BigInt g = T243BigInt::gcd(abs_(num_), abs_(den_));
    // If gcd > 1, divide both by gcd. We don't have division; approximate via repeated subtraction modulus path:
    // Since we lack efficient division, skip actual division when gcd==1, otherwise do naive repeated subtraction.
    if (!g.is_zero() && g.to_string() != "001") { // "001" corresponds to base243 value 1 in our placeholder
      // Naive divide-by-g via repeated subtraction on magnitudes.
      auto div_naive = [](T243BigInt x, const T243BigInt& y)->T243BigInt{
        T243BigInt sign = x;
        bool neg = (!x.is_zero() && x.sign() == T243BigInt::Sign::Neg);
        x = abs_(x);
        T243BigInt yy = abs_(y);
        T243BigInt q; // not used; we only need remainder-free path
        // Subtract until remainder < y; count isn't tracked—result is not quotient,
        // but we only apply when y divides x exactly (since g = gcd). To get x/y,
        // we repeatedly subtract y from x while counting steps into q via +1.
        // Implement +1 as from_ascii("1").
        T243BigInt one = T243BigInt::from_ascii("1");
        T243BigInt count; // starts zero
        while (!x.is_zero() && T243BigInt::cmp_abs(x, yy) >= 0) {
          x = T243BigInt::sub(x, yy);
          count = T243BigInt::add(count, one);
        }
        // Assume exact division here (since g|x). If not exact, we keep original.
        if (!x.is_zero()) return sign; // fail safe: return original (skip division)
        if (neg) count = T243BigInt::sub(T243BigInt(), count);
        return count;
      };

      T243BigInt n_div = div_naive(num_, g);
      T243BigInt d_div = div_naive(den_, g);
      // Only adopt reduced if both divisions were exact (i.e., not "failed safe")
      if (T243BigInt::cmp_abs(T243BigInt::mul(n_div, g), num_) == 0 &&
          T243BigInt::cmp_abs(T243BigInt::mul(d_div, g), den_) == 0) {
        num_ = n_div;
        den_ = d_div;
      }
    }

    // Recanonicalize zero again just in case.
    if (num_.is_zero()) den_ = one_();
  }

  T243BigInt num_;
  T243BigInt den_;
};

} // namespace t81
