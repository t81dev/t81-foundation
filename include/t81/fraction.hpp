#pragma once
#include <stdexcept>
#include <string>
#include "t81/bigint.hpp"

namespace t81 {

// Signed rational with canonical invariants:
//  • den > 0
//  • gcd(|num|, den) == 1
//  • zero is 0/1
struct T81Fraction {
  T243BigInt num;
  T243BigInt den;

  // --- ctors ---
  T81Fraction() : num(T243BigInt::zero()), den(T243BigInt::one()) {}
  T81Fraction(const T243BigInt& n, const T243BigInt& d) : num(n), den(d) { normalize_(); }
  T81Fraction(T243BigInt&& n, T243BigInt&& d) : num(std::move(n)), den(std::move(d)) { normalize_(); }

  static T81Fraction from_int(int64_t v) {
    return T81Fraction(T243BigInt::from_i64(v), T243BigInt::one());
  }

  // --- arithmetic ---
  static T81Fraction add(const T81Fraction& a, const T81Fraction& b) {
    // (a.num/a.den) + (b.num/b.den) = (a.num*b.den + b.num*a.den) / (a.den*b.den)
    T243BigInt ad = T243BigInt::mul(a.num, b.den);
    T243BigInt bc = T243BigInt::mul(b.num, a.den);
    T243BigInt n  = T243BigInt::add(ad, bc);
    T243BigInt d  = T243BigInt::mul(a.den, b.den);
    return T81Fraction(std::move(n), std::move(d));
  }

  static T81Fraction sub(const T81Fraction& a, const T81Fraction& b) {
    T243BigInt ad = T243BigInt::mul(a.num, b.den);
    T243BigInt bc = T243BigInt::mul(b.num, a.den);
    T243BigInt n  = T243BigInt::sub(ad, bc);
    T243BigInt d  = T243BigInt::mul(a.den, b.den);
    return T81Fraction(std::move(n), std::move(d));
  }

  static T81Fraction mul(const T81Fraction& a, const T81Fraction& b) {
    T243BigInt n = T243BigInt::mul(a.num, b.num);
    T243BigInt d = T243BigInt::mul(a.den, b.den);
    return T81Fraction(std::move(n), std::move(d));
  }

  static T81Fraction div(const T81Fraction& a, const T81Fraction& b) {
    if (T243BigInt::is_zero(b.num)) throw std::domain_error("fraction div by zero");
    T243BigInt n = T243BigInt::mul(a.num, b.den);
    T243BigInt d = T243BigInt::mul(a.den, b.num);
    return T81Fraction(std::move(n), std::move(d));
  }

  // unary negation
  static T81Fraction neg(const T81Fraction& x) {
    return T81Fraction(T243BigInt::neg(x.num), x.den);
  }

  // --- comparison (total order) ---
  static int cmp(const T81Fraction& a, const T81Fraction& b) {
    // Compare a.num*b.den ? b.num*a.den
    T243BigInt lhs = T243BigInt::mul(a.num, b.den);
    T243BigInt rhs = T243BigInt::mul(b.num, a.den);
    return T243BigInt::cmp(lhs, rhs); // -1,0,1
  }

  // --- formatting ---
  std::string to_string() const {
    return num.to_string() + "/" + den.to_string();
  }

private:
  void normalize_() {
    if (T243BigInt::is_zero(den)) throw std::invalid_argument("fraction: denominator is zero");

    // Move sign to numerator: make den > 0
    if (T243BigInt::is_neg(den)) {
      num = T243BigInt::neg(num);
      den = T243BigInt::neg(den);
    }

    // If numerator is zero → canonical zero
    if (T243BigInt::is_zero(num)) {
      den = T243BigInt::one();
      return;
    }

    // Reduce by gcd(|num|, den)
    T243BigInt g = T243BigInt::gcd(T243BigInt::abs(num), den);
    if (!T243BigInt::is_one(g)) {
      num = T243BigInt::div(num, g); // requires BigInt::div by smallish g; if not present, use exact division helper
      den = T243BigInt::div(den, g);
    }
  }
};

} // namespace t81
