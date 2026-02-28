#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include "t81/types/T81Float.hpp"

using namespace t81;

// Use a standard size for testing
using F = T81Float<27, 9>;

void check(bool condition, const char* msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << std::endl;
    std::exit(1);
  }
}

void test_special_values_creation_and_properties() {
  std::cout << "Testing special values creation and properties...\n";

  // Infinity (Positive)
  auto inf_pos = F::inf(true);
  check(inf_pos.is_inf(), "inf(true) should be inf");
  check(!inf_pos.is_zero(), "inf(true) should not be zero");
  check(!inf_pos.is_nae(), "inf(true) should not be NaE");
  check(!inf_pos.is_negative(), "inf(true) should be positive");

  auto inf_pos_fields = inf_pos.debug_get_fields();
  check(inf_pos_fields.biased_exp == F::kInfExponent, "inf exp should be kInfExponent");
  check(inf_pos_fields.mantissa.is_zero(), "inf mantissa should be zero");
  check(inf_pos_fields.sign == Trit::P, "inf(true) sign should be P");

  // Infinity (Negative)
  auto inf_neg = F::inf(false);
  check(inf_neg.is_inf(), "inf(false) should be inf");
  check(inf_neg.is_negative(), "inf(false) should be negative");

  auto inf_neg_fields = inf_neg.debug_get_fields();
  check(inf_neg_fields.biased_exp == F::kInfExponent, "inf(neg) exp should be kInfExponent");
  check(inf_neg_fields.mantissa.is_zero(), "inf(neg) mantissa should be zero");
  check(inf_neg_fields.sign == Trit::N, "inf(neg) sign should be N");

  // NaE (Not-an-Entity)
  auto nae = F::nae();
  check(nae.is_nae(), "nae() should be NaE");
  check(!nae.is_inf(), "nae() should not be inf");
  check(!nae.is_zero(), "nae() should not be zero");

  auto nae_fields = nae.debug_get_fields();
  check(nae_fields.biased_exp == F::kInfExponent, "nae exp should be kInfExponent");
  check(!nae_fields.mantissa.is_zero(), "nae mantissa should not be zero");

  // Zero (Positive)
  auto zero_pos = F::zero(true);
  check(zero_pos.is_zero(), "zero(true) should be zero");
  check(!zero_pos.is_inf(), "zero should not be inf");
  check(!zero_pos.is_nae(), "zero should not be NaE");
  check(!zero_pos.is_negative(), "zero(true) should be positive");

  auto zero_pos_fields = zero_pos.debug_get_fields();
  check(zero_pos_fields.biased_exp == 0, "zero exp should be 0");
  check(zero_pos_fields.mantissa.is_zero(), "zero mantissa should be zero");
  check(zero_pos_fields.sign == Trit::P, "zero(true) sign should be P");

  // Zero (Negative)
  auto zero_neg = F::zero(false);
  check(zero_neg.is_zero(), "zero(false) should be zero");
  check(zero_neg.is_negative(), "zero(false) should be negative");

  auto zero_neg_fields = zero_neg.debug_get_fields();
  check(zero_neg_fields.biased_exp == 0, "zero(neg) exp should be 0");
  check(zero_neg_fields.mantissa.is_zero(), "zero(neg) mantissa should be zero");
  check(zero_neg_fields.sign == Trit::N, "zero(neg) sign should be N");
}

void test_conversions() {
  std::cout << "Testing conversions...\n";

  auto inf = F::inf();
  auto neg_inf = F::inf(false);
  auto nae = F::nae();
  auto zero = F::zero();

  double d_inf = inf.to_double();
  check(std::isinf(d_inf) && d_inf > 0, "inf -> double should be +inf");

  double d_neg_inf = neg_inf.to_double();
  check(std::isinf(d_neg_inf) && d_neg_inf < 0, "neg_inf -> double should be -inf");

  double d_nae = nae.to_double();
  check(std::isnan(d_nae), "nae -> double should be NaN");

  double d_zero = zero.to_double();
  check(d_zero == 0.0, "zero -> double should be 0.0");

  // Round trip checks
  check(F::from_double(d_inf).is_inf() && !F::from_double(d_inf).is_negative(),
        "double(+inf) -> T81Float should be +inf");
  check(F::from_double(d_neg_inf).is_inf() && F::from_double(d_neg_inf).is_negative(),
        "double(-inf) -> T81Float should be -inf");
  check(F::from_double(d_nae).is_nae(), "double(NaN) -> T81Float should be NaE");
  check(F::from_double(0.0).is_zero(), "double(0.0) -> T81Float should be zero");
  check(F::from_double(-0.0).is_zero(), "double(-0.0) -> T81Float should be zero");
}

void test_arithmetic_special_values() {
  std::cout << "Testing arithmetic with special values...\n";

  auto inf = F::inf();           // +inf
  auto neg_inf = F::inf(false);  // -inf
  auto zero = F::zero();         // +0
  auto one = F::from_double(1.0);
  auto neg_one = F::from_double(-1.0);
  auto nae = F::nae();

  // Addition
  check((inf + inf).is_inf() && !(inf + inf).is_negative(), "inf + inf = inf");
  check((neg_inf + neg_inf).is_inf() && (neg_inf + neg_inf).is_negative(), "-inf + -inf = -inf");
  check((inf + neg_inf).is_nae(), "inf + (-inf) = NaE");
  check((inf + one).is_inf(), "inf + 1 = inf");
  check((neg_inf + one).is_inf() && (neg_inf + one).is_negative(), "-inf + 1 = -inf");
  check((nae + one).is_nae(), "NaE + 1 = NaE");
  check((one + nae).is_nae(), "1 + NaE = NaE");
  check((inf + nae).is_nae(), "inf + NaE = NaE");

  // Subtraction
  check((inf - neg_inf).is_inf() && !(inf - neg_inf).is_negative(), "inf - (-inf) = inf");
  check((neg_inf - inf).is_inf() && (neg_inf - inf).is_negative(), "-inf - inf = -inf");
  check((inf - inf).is_nae(), "inf - inf = NaE");
  check((neg_inf - neg_inf).is_nae(), "-inf - (-inf) = NaE");

  // Multiplication
  check((inf * one).is_inf() && !(inf * one).is_negative(), "inf * 1 = inf");
  check((inf * neg_one).is_inf() && (inf * neg_one).is_negative(), "inf * -1 = -inf");
  check((neg_inf * one).is_inf() && (neg_inf * one).is_negative(), "-inf * 1 = -inf");
  check((neg_inf * neg_one).is_inf() && !(neg_inf * neg_one).is_negative(), "-inf * -1 = inf");

  check((inf * zero).is_nae(), "inf * 0 = NaE");
  check((zero * inf).is_nae(), "0 * inf = NaE");
  check((inf * inf).is_inf() && !(inf * inf).is_negative(), "inf * inf = inf");
  check((inf * neg_inf).is_inf() && (inf * neg_inf).is_negative(), "inf * -inf = -inf");
  check((nae * one).is_nae(), "NaE * 1 = NaE");

  // Division
  check((inf / one).is_inf(), "inf / 1 = inf");
  check((one / inf).is_zero(), "1 / inf = 0");
  check((inf / inf).is_nae(), "inf / inf = NaE");
  check((neg_inf / neg_inf).is_nae(), "-inf / -inf = NaE");
  check((one / zero).is_inf(), "1 / 0 = inf");
  check((zero / zero).is_nae(), "0 / 0 = NaE");
  check((nae / one).is_nae(), "NaE / 1 = NaE");
}

void test_functions_special_values() {
  std::cout << "Testing functions with special values...\n";

  auto inf = F::inf();
  auto neg_inf = F::inf(false);
  auto nae = F::nae();
  auto zero = F::zero();
  auto neg_one = F::from_double(-1.0);

  // Sqrt
  check(inf.sqrt().is_inf(), "sqrt(inf) = inf");
  check(zero.sqrt().is_zero(), "sqrt(0) = 0");
  check(neg_one.sqrt().is_nae(), "sqrt(-1) = NaE");
  check(nae.sqrt().is_nae(), "sqrt(NaE) = NaE");
  check(neg_inf.sqrt().is_nae(), "sqrt(-inf) = NaE");

  // Log
  check(inf.log().is_inf(), "log(inf) = inf");
  check(zero.log().is_nae(), "log(0) = NaE");  // Implementation returns NaE for <= 0
  check(neg_one.log().is_nae(), "log(-1) = NaE");
  check(nae.log().is_nae(), "log(NaE) = NaE");
  check(neg_inf.log().is_nae(), "log(-inf) = NaE");

  // Exp
  check(zero.exp().to_double() == 1.0, "exp(0) = 1");
  check(inf.exp().is_inf(), "exp(inf) = inf");
  check(neg_inf.exp().is_zero(), "exp(-inf) = 0");
  check(nae.exp().is_nae(), "exp(NaE) = NaE");

  // Sin
  check(inf.sin().is_nae(), "sin(inf) = NaE");
  check(neg_inf.sin().is_nae(), "sin(-inf) = NaE");
  check(nae.sin().is_nae(), "sin(NaE) = NaE");
  check(zero.sin().is_zero(), "sin(0) = 0");

  // Cos
  check(inf.cos().is_nae(), "cos(inf) = NaE");
  check(neg_inf.cos().is_nae(), "cos(-inf) = NaE");
  check(nae.cos().is_nae(), "cos(NaE) = NaE");
  check(zero.cos().to_double() == 1.0, "cos(0) = 1");
}

void test_fma() {
  std::cout << "Testing FMA...\n";
  F two = F::from_double(2.0);
  F three = F::from_double(3.0);
  F four = F::from_double(4.0);
  F inf = F::inf();
  F zero = F::zero();

  // 2 * 3 + 4 = 10
  F res = fma(two, three, four);
  check(std::abs(res.to_double() - 10.0) < 1e-9, "2.0 * 3.0 + 4.0 == 10.0");

  // (Inf * 2) + 4 -> Inf
  res = fma(inf, two, four);
  check(res.is_inf(), "Inf * 2.0 + 4.0 is Inf");

  // (Inf * 0) + 4 -> NaE + 4 -> NaE
  res = fma(inf, zero, four);
  check(res.is_nae(), "Inf * 0.0 + 4.0 is NaE");
}

void test_trig_functions() {
  std::cout << "Testing new trig functions...\n";

  // tan
  check(F::from_double(0.0).tan().is_zero(), "tan(0) = 0");
  check(std::abs(F::from_double(0.785398).tan().to_double() - 1.0) < 1e-3, "tan(pi/4) ~= 1");
  check(F::inf().tan().is_nae(), "tan(inf) = NaE");
  check(F::nae().tan().is_nae(), "tan(NaE) = NaE");

  // asin
  check(F::from_double(0.0).asin().is_zero(), "asin(0) = 0");
  check(std::abs(F::from_double(1.0).asin().to_double() - 1.570796) < 1e-3, "asin(1) ~= pi/2");
  check(F::from_double(2.0).asin().is_nae(), "asin(2) = NaE");
  check(F::from_double(-2.0).asin().is_nae(), "asin(-2) = NaE");
  check(F::nae().asin().is_nae(), "asin(NaE) = NaE");

  // atan
  check(F::from_double(0.0).atan().is_zero(), "atan(0) = 0");
  check(std::abs(F::from_double(1.0).atan().to_double() - 0.785398) < 1e-3, "atan(1) ~= pi/4");
  check(F::inf().atan().to_double() > 1.5, "atan(inf) ~= pi/2");
  check(F::nae().atan().is_nae(), "atan(NaE) = NaE");
}

void test_hyperbolic_functions() {
  std::cout << "Testing hyperbolic functions...\n";

  // sinh
  check(F::from_double(0.0).sinh().is_zero(), "sinh(0) = 0");
  check(std::abs(F::from_double(1.0).sinh().to_double() - 1.1752) < 1e-3, "sinh(1) ~= 1.1752");
  check(F::inf().sinh().is_inf(), "sinh(inf) = inf");
  check(F::nae().sinh().is_nae(), "sinh(NaE) = NaE");

  // cosh
  check(std::abs(F::from_double(0.0).cosh().to_double() - 1.0) < 1e-3, "cosh(0) = 1");
  check(std::abs(F::from_double(1.0).cosh().to_double() - 1.5430) < 1e-3, "cosh(1) ~= 1.5430");
  check(F::inf().cosh().is_inf(), "cosh(inf) = inf");
  check(F::nae().cosh().is_nae(), "cosh(NaE) = NaE");

  // tanh
  check(F::from_double(0.0).tanh().is_zero(), "tanh(0) = 0");
  check(std::abs(F::from_double(1.0).tanh().to_double() - 0.7615) < 1e-3, "tanh(1) ~= 0.7615");
  check(std::abs(F::inf().tanh().to_double() - 1.0) < 1e-3, "tanh(inf) = 1");
  check(F::nae().tanh().is_nae(), "tanh(NaE) = NaE");
}

int main() {
  try {
    test_special_values_creation_and_properties();
    test_conversions();
    test_arithmetic_special_values();
    test_fma();
    test_functions_special_values();
    test_trig_functions();
    test_hyperbolic_functions();

    std::cout << "All specialized T81Float tests PASSED!\n";
  } catch (const std::exception& e) {
    std::cerr << "Exception caught: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
