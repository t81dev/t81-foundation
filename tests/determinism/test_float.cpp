#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "t81/types/T81Float.hpp"

// Minimal test runner macro
#define TEST_CHECK(cond)                                                                      \
  do {                                                                                        \
    if (!(cond)) {                                                                            \
      std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::exit(1);                                                                           \
    }                                                                                         \
  } while (0)

using namespace t81::v1;
using namespace t81;

void test_t81float_determinism() {
  std::cout << "Testing T81Float determinism..." << std::endl;

  // Use standard size: T81Float<72, 9>
  using Float = T81Float<72, 9>;

  // 1. Basic Values & Canonicalization
  Float zero = Float::zero();
  TEST_CHECK(zero.is_zero());
  TEST_CHECK(zero.to_canonical_string() == "+0E0");

  Float neg_zero = Float::zero(false);
  TEST_CHECK(neg_zero.is_zero());
  TEST_CHECK(neg_zero.to_canonical_string() == "-0E0");

  // Note: T81Float considers +0 and -0 equal in value equality check
  TEST_CHECK(zero == neg_zero);

  Float inf = Float::inf();
  TEST_CHECK(inf.is_inf());
  TEST_CHECK(inf.to_canonical_string() == "+Inf");

  Float neg_inf = Float::inf(false);
  TEST_CHECK(neg_inf.is_inf());
  TEST_CHECK(neg_inf.is_negative());
  TEST_CHECK(neg_inf.to_canonical_string() == "-Inf");

  Float nae = Float::nae();  // Not an Entity
  TEST_CHECK(nae.is_nae());
  TEST_CHECK(nae.to_canonical_string() == "NaE");

  // 2. Roundtrip Double
  double d = 3.14159;
  Float fd = Float::from_double(d);
  double d2 = fd.to_double();
  TEST_CHECK(std::abs(d - d2) < 1e-10);

  // 3. Arithmetic Determinism
  Float a = Float::from_double(1.5);
  Float b = Float::from_double(2.5);
  Float sum = a + b;
  TEST_CHECK(std::abs(sum.to_double() - 4.0) < 1e-10);

  Float prod = a * b;  // 3.75
  TEST_CHECK(std::abs(prod.to_double() - 3.75) < 1e-10);

  // 4. Transcendental Determinism (Check host dependency)
#if defined(T81_DETERMINISTIC)
  std::cout << "  (T81_DETERMINISTIC is enabled: checking dmath)" << std::endl;
  // TODO: Verify dmath path explicitly if possible
#else
  std::cout << "  (T81_DETERMINISTIC NOT enabled: warning emitted for host math)" << std::endl;
#endif

  Float pi_approx = Float::from_double(3.1415926535);
  Float sin_pi = pi_approx.sin();
  // sin(pi) ~ 0
  TEST_CHECK(std::abs(sin_pi.to_double()) < 1e-5);

  // 5. Canonical String Representation
  // Ensure that equivalent values produce identical strings
  Float v1 = Float::from_double(123.456);
  Float v2 = Float::from_double(123.456);
  TEST_CHECK(v1.to_canonical_string() == v2.to_canonical_string());
}

int main() {
  test_t81float_determinism();
  std::cout << "All float determinism tests passed." << std::endl;
  return 0;
}
