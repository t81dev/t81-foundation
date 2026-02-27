#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include "t81/types/T81Fixed.hpp"
#include "t81/types/T81Fraction.hpp"
#include "t81/types/T81Complex.hpp"

// Minimal test runner macro
#define TEST_CHECK(cond) \
  do { \
    if (!(cond)) { \
      std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::exit(1); \
    } \
  } while (0)

using namespace t81::v1;
using namespace t81;

// --- T81Fixed Tests ---
void test_fixed_determinism() {
  std::cout << "Testing T81Fixed determinism..." << std::endl;

  using Fixed = T81Fixed<18, 9>;

  // 1. Roundtrip
  double d = 123.456;
  Fixed f(d);
  double d2 = f.to_double();
  TEST_CHECK(std::abs(d - d2) < 1e-4); // Precision limited by 9 fractional trits (3^9 ~ 19683)

  // 2. Arithmetic
  Fixed a(1.5);
  Fixed b(2.5);
  Fixed sum = a + b;
  TEST_CHECK(std::abs(sum.to_double() - 4.0) < 1e-4);

  Fixed prod = a * b; // 3.75
  TEST_CHECK(std::abs(prod.to_double() - 3.75) < 1e-4);

  // 3. Overflow behavior (should throw or trap)
  // T81Fixed<18, 9> uses T81Int<27>. Max int ~ 3.8e12.
  // Let's try to overflow.
  // Fixed max = Fixed::from_raw(T81Int<27>::max_value());
}

// --- T81Fraction Tests ---
void test_fraction_determinism() {
  std::cout << "Testing T81Fraction determinism..." << std::endl;

  using Frac = T81Fraction<81>;

  // 1. Canonicalization
  Frac f1(Frac::Int(2), Frac::Int(4)); // 1/2
  TEST_CHECK(f1.num().to_int64() == 1);
  TEST_CHECK(f1.den().to_int64() == 2);

  Frac f2(Frac::Int(3), Frac::Int(9)); // 1/3
  TEST_CHECK(f2.num().to_int64() == 1);
  TEST_CHECK(f2.den().to_int64() == 3);

  // 2. Sign Normalization
  Frac f3(Frac::Int(1), Frac::Int(-2)); // -1/2
  TEST_CHECK(f3.num().to_int64() == -1);
  TEST_CHECK(f3.den().to_int64() == 2);

  Frac f4(Frac::Int(-1), Frac::Int(-2)); // 1/2
  TEST_CHECK(f4.num().to_int64() == 1);
  TEST_CHECK(f4.den().to_int64() == 2);

  // 3. Arithmetic
  Frac sum = f1 + f2; // 1/2 + 1/3 = 5/6
  TEST_CHECK(sum.num().to_int64() == 5);
  TEST_CHECK(sum.den().to_int64() == 6);
}

// --- T81Complex Tests ---
void test_complex_determinism() {
  std::cout << "Testing T81Complex determinism..." << std::endl;

  using Complex = T81Complex<18>;
  using Float = Complex::Float;

  // 1. Arithmetic
  Complex z1(Float::from_double(1.0), Float::from_double(2.0)); // 1 + 2i
  Complex z2(Float::from_double(3.0), Float::from_double(4.0)); // 3 + 4i

  Complex sum = z1 + z2; // 4 + 6i
  TEST_CHECK(std::abs(sum.real().to_double() - 4.0) < 1e-5);
  TEST_CHECK(std::abs(sum.imag().to_double() - 6.0) < 1e-5);

  Complex prod = z1 * z2; // (1*3 - 2*4) + (1*4 + 2*3)i = (3-8) + (4+6)i = -5 + 10i
  TEST_CHECK(std::abs(prod.real().to_double() - (-5.0)) < 1e-5);
  TEST_CHECK(std::abs(prod.imag().to_double() - 10.0) < 1e-5);

  // 2. Magnitude Squared
  Float mag2 = z1.mag2(); // 1^2 + 2^2 = 5
  TEST_CHECK(std::abs(mag2.to_double() - 5.0) < 1e-5);
}

int main() {
  test_fixed_determinism();
  test_fraction_determinism();
  test_complex_determinism();
  std::cout << "All math determinism tests passed." << std::endl;
  return 0;
}
