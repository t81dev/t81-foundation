#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include "t81/types/T81BigInt.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/cell.hpp"

// Minimal test runner macro
#define TEST_CHECK(cond)                                                                      \
  do {                                                                                        \
    if (!(cond)) {                                                                            \
      std::cerr << "FAILED: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
      std::exit(1);                                                                           \
    }                                                                                         \
  } while (0)

using namespace t81::core;
using namespace t81::v1;
using namespace t81;

// --- Cell Tests ---
void test_cell_determinism() {
  std::cout << "Testing Cell determinism..." << std::endl;

  // 1. Boundary values
  Cell min = Cell::from_int(-121);
  Cell max = Cell::from_int(121);
  TEST_CHECK(min.to_int() == -121);
  TEST_CHECK(max.to_int() == 121);

  // 2. Arithmetic determinism
  Cell a = Cell::from_int(40);
  Cell b = Cell::from_int(2);
  Cell sum = a + b;
  TEST_CHECK(sum.to_int() == 42);
  Cell diff = a - b;
  TEST_CHECK(diff.to_int() == 38);
  Cell prod = a * b;
  TEST_CHECK(prod.to_int() == 80);
  Cell quot = a / b;
  TEST_CHECK(quot.to_int() == 20);

  // 3. Overflow behavior (should throw deterministically)
  bool caught1 = false;
  try {
    Cell ov = max + Cell::from_int(1);
  } catch (const std::overflow_error& e) {
    caught1 = true;
  }
  TEST_CHECK(caught1);

  // 4. Shift Overflow
  bool caught2 = false;
  try {
    Cell high = Cell::from_int(81);
    [[maybe_unused]] Cell ov = high << 1;
  } catch (const std::overflow_error& e) {
    caught2 = true;
  }
  TEST_CHECK(caught2);

  // 5. Comparison
  Cell c10 = Cell::from_int(10);
  Cell c20 = Cell::from_int(20);
  Cell c20b = Cell::from_int(20);
  Cell cn10 = Cell::from_int(-10);

  TEST_CHECK(c10 < c20);
  TEST_CHECK(cn10 < c10);
  TEST_CHECK(c20 > c10);
  TEST_CHECK(c20 >= c20b);
  TEST_CHECK(c20 <= c20b);
  TEST_CHECK(!(c20 < c20b));
  TEST_CHECK(cn10 < c20);

  // 6. Roundtrip
  for (int i = -121; i <= 121; ++i) {
    Cell c = Cell::from_int(i);
    TEST_CHECK(c.to_int() == i);
  }
}

// --- T81Int Tests ---
void test_t81int_determinism() {
  std::cout << "Testing T81Int determinism..." << std::endl;

  using Int27 = T81Int<27>;

  // 1. Zero and basic values
  Int27 zero;
  TEST_CHECK(zero.to_int64() == 0);
  TEST_CHECK(zero.is_zero());

  Int27 one(1);
  TEST_CHECK(one.to_int64() == 1);

  Int27 neg_one(-1);
  TEST_CHECK(neg_one.to_int64() == -1);

  // 2. Arithmetic
  Int27 sum = one + neg_one;
  TEST_CHECK(sum.is_zero());

  Int27 prod = Int27(10) * Int27(10);
  TEST_CHECK(prod.to_int64() == 100);

  // 3. String canonicalization (trit string)
  std::string s = Int27(13).to_string();  // 13 = 9 + 3 + 1 = 111 (base 3) -> +++
  TEST_CHECK(s == "111");

  // 4. Canonical string (trit string: +, 0, -)
  std::string can = Int27(13).to_canonical_string();
  TEST_CHECK(can.length() == 27);
  TEST_CHECK(can.substr(24) == "+++");
}

// --- T81BigInt Tests ---
void test_bigint_determinism() {
  std::cout << "Testing T81BigInt determinism..." << std::endl;

  // 1. Basic construction
  T81BigInt a(123456789);
  T81BigInt c = T81BigInt::from_int64(987654321);

  // 2. Arithmetic
  T81BigInt sum = a + c;
  TEST_CHECK(sum.to_int64() == 123456789 + 987654321);

  T81BigInt diff = c - a;
  TEST_CHECK(diff.to_int64() == 987654321 - 123456789);

  // 3. Multiplication (Karatsuba threshold check)
  T81BigInt base(3);
  T81BigInt exp(50);
  T81BigInt huge = T81BigInt::pow(base, exp);  // 3^50

  // 4. Canonical String (Base81)
  std::string s81 = huge.to_base81_string();
  // Ensure roundtrip
  T81BigInt recovered = T81BigInt::from_base81_string(s81);
  TEST_CHECK(recovered == huge);

  // 5. Division / Modulo
  auto [q, r] = T81BigInt::div_mod(c, a);
  TEST_CHECK(q.to_int64() == 987654321 / 123456789);
  TEST_CHECK(r.to_int64() == 987654321 % 123456789);
}

int main() {
  test_cell_determinism();
  test_t81int_determinism();
  test_bigint_determinism();
  std::cout << "All primitive determinism tests passed." << std::endl;
  return 0;
}
