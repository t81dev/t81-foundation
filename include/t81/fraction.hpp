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
  T81BigInt num;
  T81BigInt den;

  // --- ctors ---
  T81Fraction() : num(T81BigInt::zero()), den(T81BigInt::one()) {}
  T81Fraction(const T81BigInt& n, const T81BigInt& d) : num(n), den(d) { normalize_(); }
  T81Fraction(T81BigInt&& n, T81BigInt&& d) : num(std::move(n)), den(std::move(d)) { normalize_(); }

  static T81Fraction from_int(int64_t v) {
    return T81Fraction(T81BigInt::from_i64(v), T81BigInt::one());
  }

  // --- arithmetic ---
  static T81Fraction add(const T81Fraction& a, const T81Fraction& b) {
    // (a.num/a.den) + (b.num/b.den) = (a.num*b.den + b.num*a.den) / (a.den*b.den)
    T81BigInt ad = T81BigInt::mul(a.num, b.den);
    T81BigInt bc = T81BigInt::mul(b.num, a.den);
    T81BigInt n  = T81BigInt::add(ad, bc);
    T81BigInt d  = T81BigInt::mul(a.den, b.den);
    return T81Fraction(std::move(n), std::move(d));
  }

  static T81Fraction sub(const T81Fraction& a, const T81Fraction& b) {
    T81BigInt ad = T81BigInt::mul(a.num, b.den);
    T81BigInt bc = T81BigInt::mul(b.num, a.den);
    T81BigInt n  = T81BigInt::sub(ad, bc);
    T81BigInt d  = T81BigInt::mul(a.den, b.den);
    return T81Fraction(std::move(n), std::move(d));
  }

  static T81Fraction mul(const T81Fraction& a, const T81Fraction& b) {
    T81BigInt n = T81BigInt::mul(a.num, b.num);
    T81BigInt d = T81BigInt::mul(a.den, b.den);
    return T81Fraction(std::move(n), std::move(d));
  }

  static T81Fraction div(const T81Fraction& a, const T81Fraction& b) {
    if (T81BigInt::is_zero(b.num)) throw std::domain_error("fraction div by zero");
    T81BigInt n = T81BigInt::mul(a.num, b.den);
    T81BigInt d = T81BigInt::mul(a.den, b.num);
    return T81Fraction(std::move(n), std::move(d));
  }

  // unary negation
  static T81Fraction neg(const T81Fraction& x) {
    return T81Fraction(T81BigInt::neg(x.num), x.den);
  }

  // --- comparison (total order) ---
  static int cmp(const T81Fraction& a, const T81Fraction& b) {
    // Compare a.num*b.den ? b.num*a.den
    T81BigInt lhs = T81BigInt::mul(a.num, b.den);
    T81BigInt rhs = T81BigInt::mul(b.num, a.den);
    return T81BigInt::cmp(lhs, rhs); // -1,0,1
  }

  // --- formatting ---
  std::string to_string() const {
    return num.to_string() + "/" + den.to_string();
  }

private:
  void normalize_() {
    if (T81BigInt::is_zero(den)) throw std::invalid_argument("fraction: denominator is zero");

    // Move sign to numerator: make den > 0
    if (T81BigInt::is_neg(den)) {
      num = T81BigInt::neg(num);
      den = T81BigInt::neg(den);
    }

    // If numerator is zero → canonical zero
    if (T81BigInt::is_zero(num)) {
      den = T81BigInt::one();
      return;
    }

    // Reduce by gcd(|num|, den)
    T81BigInt g = T81BigInt::gcd(T81BigInt::abs(num), den);
    if (!T81BigInt::is_one(g)) {
      num = T81BigInt::div(num, g); // requires BigInt::div by smallish g; if not present, use exact division helper
      den = T81BigInt::div(den, g);
    }
  }
};

} // namespace t81
