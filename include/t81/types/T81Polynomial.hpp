#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <span>
#include "t81/types/T81Complex.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/T81List.hpp"
#include "t81/types/T81Vector.hpp"

namespace t81 {

template <typename Coeff = T81Float<72, 9>>
class T81Polynomial {
  T81List<Coeff> coeffs_;

  void trim() noexcept {
    while (coeffs_.size() > 1 && coeffs_.back().is_zero()) coeffs_.pop_back();
  }

public:
  using coefficient_type = Coeff;
  using value_type = Coeff;

  constexpr T81Polynomial() noexcept { coeffs_.push_back(Coeff(0)); }

  explicit constexpr T81Polynomial(T81List<Coeff> c) : coeffs_(std::move(c)) { trim(); }

  constexpr T81Polynomial(std::initializer_list<Coeff> c) : coeffs_(c) { trim(); }

  template <typename InputIt>
    requires std::input_iterator<InputIt>
  constexpr T81Polynomial(InputIt first, InputIt last) : coeffs_(first, last) {
    trim();
  }

  [[nodiscard]] static constexpr T81Polynomial monomial(size_t degree) {
    T81List<Coeff> c;
    for (size_t i = 0; i < degree; ++i) c.push_back(Coeff(0));
    c.push_back(Coeff(1));
    return T81Polynomial(std::move(c));
  }

  [[nodiscard]] static constexpr T81Polynomial x() { return monomial(1); }

  [[nodiscard]] constexpr size_t degree() const noexcept {
    return coeffs_.empty() ? 0 : coeffs_.size() - 1;
  }

  [[nodiscard]] constexpr Coeff operator[](size_t i) const noexcept {
    return i < coeffs_.size() ? coeffs_[i] : Coeff(0);
  }

  [[nodiscard]] constexpr Coeff& operator[](size_t i) noexcept {
    if (i >= coeffs_.size()) {
      for (size_t j = coeffs_.size(); j <= i; ++j) coeffs_.push_back(Coeff(0));
    }
    return coeffs_[i];
  }

  [[nodiscard]] constexpr const T81List<Coeff>& coefficients() const noexcept { return coeffs_; }

  [[nodiscard]] constexpr Coeff eval(const Coeff& x_val) const noexcept {
    Coeff result(0);
    for (size_t i = coeffs_.size(); i > 0; --i) result = result * x_val + coeffs_[i - 1];
    return result;
  }

  [[nodiscard]] friend constexpr T81Polynomial operator*(const T81Polynomial& p,
                                                         const Coeff& s) noexcept {
    T81List<Coeff> c = p.coeffs_;
    for (auto& coef : c) coef = coef * s;
    return T81Polynomial(std::move(c));
  }

  [[nodiscard]] friend constexpr T81Polynomial operator+(const T81Polynomial& a,
                                                         const T81Polynomial& b) noexcept {
    size_t max_deg = std::max(a.degree(), b.degree());
    T81List<Coeff> c;
    for (size_t i = 0; i <= max_deg; ++i) c.push_back(a[i] + b[i]);
    return T81Polynomial(std::move(c));
  }

  [[nodiscard]] friend constexpr T81Polynomial operator-(const T81Polynomial& a,
                                                         const T81Polynomial& b) noexcept {
    size_t max_deg = std::max(a.degree(), b.degree());
    T81List<Coeff> c;
    for (size_t i = 0; i <= max_deg; ++i) c.push_back(a[i] - b[i]);
    return T81Polynomial(std::move(c));
  }

  [[nodiscard]] friend constexpr T81Polynomial operator*(const T81Polynomial& a,
                                                         const T81Polynomial& b) noexcept {
    if (a.degree() == 0) return b * a[0];
    if (b.degree() == 0) return a * b[0];
    size_t new_deg = a.degree() + b.degree();
    T81List<Coeff> c;
    for (size_t i = 0; i <= new_deg; ++i) c.push_back(Coeff(0));
    for (size_t i = 0; i <= a.degree(); ++i)
      for (size_t j = 0; j <= b.degree(); ++j) c[i + j] = c[i + j] + a[i] * b[j];
    return T81Polynomial(std::move(c));
  }

  [[nodiscard]] constexpr T81Polynomial operator-() const noexcept {
    T81List<Coeff> c = coeffs_;
    for (auto& coef : c) coef = -coef;
    return T81Polynomial(std::move(c));
  }

  struct Division {
    T81Polynomial quotient;
    T81Polynomial remainder;
  };

  [[nodiscard]] constexpr Division div(const T81Polynomial& divisor) const {
    if (divisor.degree() == 0) {
      return {*this * (Coeff(1) / divisor[0]), T81Polynomial{}};
    }
    if (degree() < divisor.degree()) {
      return {T81Polynomial{}, *this};
    }
    auto q_work = *this;
    T81Polynomial r_acc;
    while (q_work.degree() >= divisor.degree()) {
      auto lead = q_work.coefficients().back() / divisor.coefficients().back();
      size_t deg_diff = q_work.degree() - divisor.degree();
      auto term = T81Polynomial::monomial(deg_diff) * lead;
      q_work = q_work - (divisor * term);
      r_acc = r_acc + term;
    }
    return {r_acc, q_work};
  }

  [[nodiscard]] constexpr T81Polynomial derivative() const noexcept {
    if (degree() == 0) return T81Polynomial();
    T81List<Coeff> c;
    for (size_t i = 1; i <= degree(); ++i)
      c.push_back(coeffs_[i] * Coeff(static_cast<long long>(i)));
    return T81Polynomial(std::move(c));
  }

  [[nodiscard]] constexpr T81Polynomial integral(Coeff constant = Coeff(0)) const noexcept {
    T81List<Coeff> c;
    c.push_back(constant);
    for (size_t i = 0; i <= degree(); ++i) {
      if (coeffs_.size() == 1 && coeffs_[0] == Coeff(0)) continue;
      c.push_back(coeffs_[i] / Coeff(static_cast<long long>(i + 1)));
    }
    return T81Polynomial(std::move(c));
  }

  /**
   * @brief Finds roots for linear and quadratic polynomials.
   * Higher degrees or complex roots might require specific Coeff types or iterative solvers.
   */
  [[nodiscard]] T81List<Coeff> roots() const {
    T81List<Coeff> r;
    if (degree() == 1) {
      // ax + b = 0 => x = -b/a
      r.push_back(-coeffs_[0] / coeffs_[1]);
    } else if (degree() == 2) {
      // ax^2 + bx + c = 0
      Coeff a = coeffs_[2];
      Coeff b = coeffs_[1];
      Coeff c = coeffs_[0];
      Coeff delta = b * b - Coeff(4) * a * c;

      // Note: Coeff type must support sqrt().
      // If delta < 0 and Coeff is real, sqrt() might throw or return NaN.
      // We assume caller handles domain errors or uses ComplexPoly.
      Coeff sqrt_delta = delta.sqrt();
      Coeff two_a = Coeff(2) * a;
      r.push_back((-b - sqrt_delta) / two_a);
      r.push_back((-b + sqrt_delta) / two_a);
    }
    return r;
  }

  [[nodiscard]] constexpr auto operator<=>(const T81Polynomial& o) const noexcept = default;
  [[nodiscard]] std::string serialize_canonical() const { return "Polynomial()"; }
};

using IntPoly = T81Polynomial<T81Int<81>>;
using FloatPoly = T81Polynomial<T81Float<72, 9>>;
using ComplexPoly = T81Polynomial<T81Complex<121>>;

}  // namespace t81
