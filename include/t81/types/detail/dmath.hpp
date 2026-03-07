#pragma once

#include "t81/types/detail/dmath_hyper.hpp"
#include "t81/types/detail/dmath_logexp.hpp"
#include "t81/types/detail/dmath_trig.hpp"
#include "t81/types/detail/dmath_types.hpp"

namespace t81::core::detail {

// Facade for DFixed based math

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> sin(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return v1::T81Float<M, E>::nae();
  if (x.is_zero()) return x;

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::sin(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> cos(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return v1::T81Float<M, E>::nae();
  if (x.is_zero()) return v1::T81Float<M, E>::from_double(1.0);

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::cos(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> tan(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return v1::T81Float<M, E>::nae();
  if (x.is_zero()) return x;

  DFixed val = DFixed::from_float(x);
  DFixed s = detail::sin(val);
  DFixed c = detail::cos(val);

  if (c.is_zero()) return v1::T81Float<M, E>::inf(s.v.sign_trit() == t81::Trit::P);

  DFixed res = s / c;  // Deterministic Fixed division
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> exp(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) {
    return x.is_negative() ? v1::T81Float<M, E>::zero() : x;
  }
  if (x.is_zero()) return v1::T81Float<M, E>::from_double(1.0);

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::exp(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> log(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) {
    return x.is_negative() ? v1::T81Float<M, E>::nae() : x;
  }
  if (x.is_negative() || x.is_zero()) return v1::T81Float<M, E>::nae();

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::log(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> sqrt(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_negative()) return v1::T81Float<M, E>::nae();
  if (x.is_inf()) return x;  // sqrt(inf) = inf
  if (x.is_zero()) return x;

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::sqrt(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> sinh(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return x;
  if (x.is_zero()) return x;

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::sinh(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> cosh(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return v1::T81Float<M, E>::inf();
  if (x.is_zero()) return v1::T81Float<M, E>::from_double(1.0);

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::cosh(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> tanh(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) {
    return x.is_negative() ? v1::T81Float<M, E>::from_double(-1.0)
                           : v1::T81Float<M, E>::from_double(1.0);
  }
  if (x.is_zero()) return x;

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::tanh(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> asin(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return v1::T81Float<M, E>::nae();
  if (x.is_zero()) return x;

  // Domain check [-1, 1]
  if (x > v1::T81Float<M, E>::from_double(1.0) || x < v1::T81Float<M, E>::from_double(-1.0)) {
    return v1::T81Float<M, E>::nae();
  }

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::asin(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> acos(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) return v1::T81Float<M, E>::nae();

  // Domain check [-1, 1]
  if (x > v1::T81Float<M, E>::from_double(1.0) || x < v1::T81Float<M, E>::from_double(-1.0)) {
    return v1::T81Float<M, E>::nae();
  }

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::acos(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> atan(const v1::T81Float<M, E>& x) {
  if (x.is_nae()) return x;
  if (x.is_inf()) {
    // atan(inf) = pi/2, atan(-inf) = -pi/2
    DFixed pi2_fixed = constants::pi_2<DFixed>();
    auto pi2 = pi2_fixed.to_float<M, E>();
    return x.is_negative() ? -pi2 : pi2;
  }
  if (x.is_zero()) return x;

  DFixed val = DFixed::from_float(x);
  DFixed res = detail::atan(val);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> pow(const v1::T81Float<M, E>& x, const v1::T81Float<M, E>& y) {
  if (x.is_nae() || y.is_nae()) return v1::T81Float<M, E>::nae();

  if (x.is_inf()) {
    if (y.is_zero()) return v1::T81Float<M, E>::from_double(1.0);
    if (y.is_negative()) return v1::T81Float<M, E>::zero();
    // inf^pos = inf (with sign handling?)
    // inf^1 = inf. (-inf)^1 = -inf.
    // inf^2 = inf. (-inf)^2 = inf.
    // T81Float sign?
    // For simplicity return positive inf for now unless x is neg and y is odd integer...
    // But we don't have integer check easily.
    // Host pow(inf, y) -> inf.
    return v1::T81Float<M, E>::inf();
  }
  if (y.is_inf()) {
    if (x.is_zero()) return v1::T81Float<M, E>::zero();
    v1::T81Float<M, E> one = v1::T81Float<M, E>::from_double(1.0);
    v1::T81Float<M, E> abs_x = x.abs();
    if (abs_x > one) {
      return y.is_negative() ? v1::T81Float<M, E>::zero() : v1::T81Float<M, E>::inf();
    }
    if (abs_x < one) {
      return y.is_negative() ? v1::T81Float<M, E>::inf() : v1::T81Float<M, E>::zero();
    }
    return one;
  }

  if (x.is_zero()) {
    if (y.is_zero()) return v1::T81Float<M, E>::from_double(1.0);
    if (y.is_negative()) return v1::T81Float<M, E>::inf();
    return v1::T81Float<M, E>::zero();
  }

  if (x.is_negative()) {
    // If y is integer, defined. Else NaE.
    // Currently dmath::pow returns 0 for neg x.
    // We return NaE here to be safe and consistent with current float behavior.
    return v1::T81Float<M, E>::nae();
  }

  DFixed vx = DFixed::from_float(x);
  DFixed vy = DFixed::from_float(y);
  DFixed res = detail::pow(vx, vy);
  return res.to_float<M, E>();
}

template <std::size_t M, std::size_t E>
v1::T81Float<M, E> div(const v1::T81Float<M, E>& a, const v1::T81Float<M, E>& b) {
  if (a.is_nae() || b.is_nae()) return v1::T81Float<M, E>::nae();
  if (b.is_zero()) {
    if (a.is_zero() || a.is_inf()) return v1::T81Float<M, E>::nae();
    return v1::T81Float<M, E>::inf(a.is_negative() == b.is_negative());
  }
  if (a.is_inf()) {
    if (b.is_inf()) return v1::T81Float<M, E>::nae();
    return v1::T81Float<M, E>::inf(a.is_negative() == b.is_negative());
  }
  if (b.is_inf()) return v1::T81Float<M, E>::zero();
  if (a.is_zero()) return v1::T81Float<M, E>::zero();

  DFixed val_a = DFixed::from_float(a);
  DFixed val_b = DFixed::from_float(b);
  DFixed res = val_a / val_b;  // Deterministic Fixed division
  return res.to_float<M, E>();
}

}  // namespace t81::core::detail
