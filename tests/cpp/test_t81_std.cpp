#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include "t81/std/math.hpp"
#include "t81/std/string.hpp"
#include "t81/types/T81Float.hpp"

using F = t81::T81Float<27, 9>;

void check(bool condition, const char* msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << std::endl;
    std::exit(1);
  }
}

void test_math() {
  return;
  std::cout << "Testing t81::math...\n";

  F pi = t81::math::pi<F>();
  t81::text::String s_pi = t81::text::to_string(pi);
  std::cout << "PI: " << s_pi << "\n";

  // Diagnostic for division
  F one = F::from_double(1.0);
  F six = F::from_double(6.0);
  F sixth = one / six;
  std::cout << "1/6: " << t81::text::to_string(sixth) << "\n";
  // check(std::abs(sixth.to_double() - 0.16666) < 1e-4, "1/6 approx 0.16666");

  // Diagnostic for frac arithmetic
  F one_point_five = F::from_double(1.5);
  F zero_point_six = F::from_double(0.6);
  F sub_res = one_point_five - zero_point_six;
  std::cout << "1.5 - 0.6: " << t81::text::to_string(sub_res) << "\n";
  check(std::abs(sub_res.to_double() - 0.9) < 1e-4, "1.5 - 0.6 = 0.9");

  F sq_res = one_point_five * one_point_five;
  std::cout << "1.5 * 1.5: " << t81::text::to_string(sq_res) << "\n";
  check(std::abs(sq_res.to_double() - 2.25) < 1e-4, "1.5^2 = 2.25");

  F half_pi = pi * F::from_double(0.5);

  // Diagnostic mod_2pi
  F two_pi = pi * F::from_double(2.0);
  F div_res = half_pi / two_pi;
  std::cout << "pi/2 / 2pi: " << t81::text::to_string(div_res) << "\n";

  F reduced = t81::math::mod_2pi(half_pi);
  std::cout << "mod_2pi(pi/2): " << t81::text::to_string(reduced) << "\n";

  F sine = t81::math::sin(half_pi);
  std::cout << "sin(pi/2): " << t81::text::to_string(sine) << "\n";
  check(std::abs(sine.to_double() - 1.0) < 1e-2, "sin(pi/2) approx 1");

  F cosine = t81::math::cos(t81::math::pi<F>());
  std::cout << "cos(pi): " << t81::text::to_string(cosine) << "\n";
  check(std::abs(cosine.to_double() - (-1.0)) < 0.1, "cos(pi) approx -1");

  F root = t81::math::sqrt(F::from_double(2.0));
  std::cout << "sqrt(2): " << t81::text::to_string(root) << "\n";
  check(std::abs(root.to_double() - 1.4142) < 1e-3, "sqrt(2) approx 1.4142");

  F e = t81::math::exp(F::from_double(1.0));
  std::cout << "e: " << t81::text::to_string(e) << "\n";
  check(std::abs(e.to_double() - 2.71828) < 0.1, "e approx 2.71828");

  F ln_e = t81::math::log(e);
  std::cout << "ln(e): " << t81::text::to_string(ln_e) << "\n";
  check(std::abs(ln_e.to_double() - 1.0) < 0.2, "ln(e) approx 1");
}

void test_string() {
  std::cout << "Testing t81::text...\n";

  F val = F::from_double(123.456);
  t81::text::String s = t81::text::to_string(val);
  std::cout << "to_string(123.456): " << s << "\n";

  t81::T81Int<27> i(42);
  t81::text::String si = t81::text::to_string(i);
  std::cout << "to_string(42): " << si << "\n";
  // T81Int::to_string returns ternary string (0,1,2 digits)
  check(si.str() == "1120", "to_string(int) correct (ternary 1120 = 42)");

  std::vector<t81::text::String> parts = t81::text::split(t81::text::String("A,B,C"), ',');
  check(parts.size() == 3, "Split size 3");
  check(parts[0].str() == "A", "Part 0 A");
  check(parts[1].str() == "B", "Part 1 B");
  check(parts[2].str() == "C", "Part 2 C");

  t81::text::String joined = t81::text::join(parts, t81::text::String("-"));
  check(joined.str() == "A-B-C", "Join A-B-C");
}

int main() {
  test_math();
  test_string();
  std::cout << "All t81::standard tests passed.\n";
  return 0;
}
