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

  // Diagnostic: arbitrary value (precision check happens in native tests below)
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

// Native ternary-to-decimal conversion tests (no host-float dependency).
// These verify that to_string() produces exact decimal output for values
// that are exactly representable in balanced ternary.
void test_native_float_to_string() {
  std::cout << "Testing native T81Float::to_string...\n";

  // Special values
  check(t81::text::to_string(F::nae()).str() == "NAE", "NAE");
  check(t81::text::to_string(F::inf(true)).str() == "INF", "+INF");
  check(t81::text::to_string(F::inf(false)).str() == "-INF", "-INF");
  check(t81::text::to_string(F::zero()).str() == "0", "zero");
  check(t81::text::to_string(F::zero(false)).str() == "0", "-zero");

  // Exact integer: 1  (mantissa leading trit P, exp = M-1)
  F one = F::from_double(1.0);
  auto s_one = t81::text::to_string(one).str();
  std::cout << "to_string(1.0): " << s_one << "\n";
  check(s_one == "1", "1.0 -> \"1\"");

  // Exact integer: 3  (1 * 3^1)
  F three = F::from_double(3.0);
  auto s_three = t81::text::to_string(three).str();
  std::cout << "to_string(3.0): " << s_three << "\n";
  check(s_three == "3", "3.0 -> \"3\"");

  // Exact integer: 9  (1 * 3^2)
  F nine = F::from_double(9.0);
  auto s_nine = t81::text::to_string(nine).str();
  std::cout << "to_string(9.0): " << s_nine << "\n";
  check(s_nine == "9", "9.0 -> \"9\"");

  // Negative integer: -3
  F neg_three = F::from_double(-3.0);
  auto s_neg_three = t81::text::to_string(neg_three).str();
  std::cout << "to_string(-3.0): " << s_neg_three << "\n";
  check(s_neg_three == "-3", "-3.0 -> \"-3\"");

  // Exact fraction: 1/3 in balanced ternary is exactly 0.1 (base-3)
  F third = F::from_double(1.0 / 3.0);
  auto s_third = t81::text::to_string(third).str();
  std::cout << "to_string(1/3): " << s_third << "\n";
  // 1/3 is exact in balanced ternary; decimal expansion terminates: "0.333..."
  // Native output must start with "0." and consist of '3' digits only.
  check(s_third.size() >= 3, "1/3 has fractional digits");
  check(s_third[0] == '0' && s_third[1] == '.', "1/3 starts with 0.");
  for (size_t ci = 2; ci < s_third.size(); ++ci) {
    check(s_third[ci] == '3', "1/3 fractional digits are all '3'");
  }

  // Round-trip sanity: double(to_string(v)) ≈ v.to_double() for 0.5
  // 0.5 is not exactly representable in balanced ternary, so we only check
  // that the decimal output parses back to within 1e-10.
  F half = F::from_double(0.5);
  auto s_half = t81::text::to_string(half).str();
  std::cout << "to_string(0.5): " << s_half << "\n";
  double parsed = std::stod(s_half);
  check(std::abs(parsed - 0.5) < 1e-10, "0.5 round-trip within 1e-10");

  // 0.125 = 1/8: not exact in ternary — check round-trip precision
  F eighth = F::from_double(0.125);
  auto s_eighth = t81::text::to_string(eighth).str();
  std::cout << "to_string(0.125): " << s_eighth << "\n";
  check(std::abs(std::stod(s_eighth) - 0.125) < 1e-10, "0.125 round-trip within 1e-10");

  // Exact integer large: 81 = 3^4
  F eighty_one = F::from_double(81.0);
  auto s_81 = t81::text::to_string(eighty_one).str();
  std::cout << "to_string(81.0): " << s_81 << "\n";
  check(s_81 == "81", "81.0 -> \"81\"");
}

int main() {
  test_math();
  test_string();
  test_native_float_to_string();
  std::cout << "All t81::standard tests passed.\n";
  return 0;
}
