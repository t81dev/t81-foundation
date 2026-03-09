/**
 * @file dmath_c_wrappers.cpp
 * @brief C-linkage wrappers for deterministic math functions.
 *
 * These symbols back the `func.call @t81_dmath_*` external declarations
 * emitted by the MLIR frontend when `--mode=dcp` is active.  Any binary
 * produced from DCP-mode MLIR output must be linked against this translation
 * unit (or the t81_mlir library) to obtain bit-exact, DCP-compatible float
 * math that routes through the T81 deterministic fixed-point pipeline
 * (DFixed<192,80>) rather than the host libm.
 *
 * Interface: all functions have signature  double fn(double)  except
 * t81_dmath_pow which is  double fn(double, double).
 *
 * No MLIR headers are required here — these are pure C++ wrapping dmath.hpp.
 */

#include "t81/mlir/dmath_runtime.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/detail/dmath.hpp"

namespace {

// Canonical VM float precision: 72 mantissa trits, 9 exponent trits.
using VMFloat = t81::v1::T81Float<72, 9>;

inline double wrap1(VMFloat (*fn)(const VMFloat&), double x) {
  return fn(VMFloat::from_double(x)).to_double();
}

inline double wrap2(VMFloat (*fn)(const VMFloat&, const VMFloat&),
                    double x, double y) {
  return fn(VMFloat::from_double(x), VMFloat::from_double(y)).to_double();
}

}  // namespace

extern "C" {

double t81_dmath_sin(double x) {
  return wrap1(t81::core::detail::sin<72, 9>, x);
}

double t81_dmath_cos(double x) {
  return wrap1(t81::core::detail::cos<72, 9>, x);
}

double t81_dmath_tan(double x) {
  return wrap1(t81::core::detail::tan<72, 9>, x);
}

double t81_dmath_exp(double x) {
  return wrap1(t81::core::detail::exp<72, 9>, x);
}

double t81_dmath_log(double x) {
  return wrap1(t81::core::detail::log<72, 9>, x);
}

double t81_dmath_sqrt(double x) {
  return wrap1(t81::core::detail::sqrt<72, 9>, x);
}

double t81_dmath_asin(double x) {
  return wrap1(t81::core::detail::asin<72, 9>, x);
}

double t81_dmath_acos(double x) {
  return wrap1(t81::core::detail::acos<72, 9>, x);
}

double t81_dmath_atan(double x) {
  return wrap1(t81::core::detail::atan<72, 9>, x);
}

double t81_dmath_sinh(double x) {
  return wrap1(t81::core::detail::sinh<72, 9>, x);
}

double t81_dmath_cosh(double x) {
  return wrap1(t81::core::detail::cosh<72, 9>, x);
}

double t81_dmath_tanh(double x) {
  return wrap1(t81::core::detail::tanh<72, 9>, x);
}

double t81_dmath_pow(double x, double y) {
  return wrap2(t81::core::detail::pow<72, 9>, x, y);
}

}  // extern "C"
