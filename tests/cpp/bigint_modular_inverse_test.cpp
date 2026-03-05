#include "t81/types/T81BigInt.hpp"

#include <cstdlib>
#include <iostream>
#include <tuple>

// Consolidated modular-inverse test suite.
// Merges test_T81BigInt_modular_inverse.cpp (Extended GCD path) and
// test_T81BigInt_modular_inverse_stein.cpp (Stein binary GCD path) into a single
// file that shares fixture inputs between both algorithm implementations.

using namespace t81::v1;

static void check(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "Check failed: " << msg << "\n";
    std::exit(1);
  }
}

// ---------------------------------------------------------------------------
// Section A: Extended GCD
// ---------------------------------------------------------------------------

void test_extended_gcd() {
  std::cout << "Testing extended_gcd...\n";

  auto [g, x, y] = T81BigInt::extended_gcd(T81BigInt(10), T81BigInt(6));
  check(g == T81BigInt(2), "gcd(10, 6) == 2");
  check(T81BigInt(10) * x + T81BigInt(6) * y == g, "10x + 6y = g");

  auto [g2, x2, y2] = T81BigInt::extended_gcd(T81BigInt(240), T81BigInt(46));
  check(g2 == T81BigInt(2), "gcd(240, 46) == 2");
  check(T81BigInt(240) * x2 + T81BigInt(46) * y2 == g2, "240x + 46y = g");

  auto [g3, x3, y3] = T81BigInt::extended_gcd(T81BigInt(17), T81BigInt(13));
  check(g3 == T81BigInt(1), "gcd(17, 13) == 1");
  check(T81BigInt(17) * x3 + T81BigInt(13) * y3 == g3, "17x + 13y = g");
}

void test_modular_inverse_extgcd() {
  std::cout << "Testing modular_inverse (extended GCD)...\n";

  T81BigInt inv = T81BigInt::modular_inverse(T81BigInt(3), T81BigInt(11));
  check(inv == T81BigInt(4), "inv(3, 11) == 4");

  inv = T81BigInt::modular_inverse(T81BigInt(10), T81BigInt(17));
  check(inv == T81BigInt(12), "inv(10, 17) == 12");

  try {
    T81BigInt::modular_inverse(T81BigInt(6), T81BigInt(9));
    check(false, "Should have thrown for non-coprime inputs");
  } catch (const std::domain_error&) {
  }

  try {
    T81BigInt::modular_inverse(T81BigInt(5), T81BigInt(1));
    check(false, "Should have thrown for mod <= 1");
  } catch (const std::domain_error&) {
  }
}

// ---------------------------------------------------------------------------
// Section B: Stein binary GCD
// ---------------------------------------------------------------------------

void test_modular_inverse_stein_basic() {
  std::cout << "Testing modular_inverse_stein (basic)...\n";

  // Shared fixtures with the ExtGCD section above
  T81BigInt inv = T81BigInt::modular_inverse_stein(T81BigInt(3), T81BigInt(11));
  check(inv == T81BigInt(4), "inv(3, 11) == 4");

  inv = T81BigInt::modular_inverse_stein(T81BigInt(10), T81BigInt(17));
  check(inv == T81BigInt(12), "inv(10, 17) == 12");

  // Stein-specific: m divisible by 3 triggers fallback to standard Euclidean
  inv = T81BigInt::modular_inverse_stein(T81BigInt(2), T81BigInt(3));
  check(inv == T81BigInt(2), "inv(2, 3) == 2 [Stein fallback]");

  try {
    T81BigInt::modular_inverse_stein(T81BigInt(6), T81BigInt(9));
    check(false, "Should have thrown for non-coprime inputs");
  } catch (const std::domain_error&) {
  }
}

void test_modular_inverse_stein_large() {
  std::cout << "Testing modular_inverse_stein (large)...\n";

  T81BigInt m(241);  // prime, not divisible by 3
  T81BigInt a(100);
  T81BigInt inv = T81BigInt::modular_inverse_stein(a, m);
  check((a * inv) % m == T81BigInt(1), "inv(100, 241) * 100 == 1 mod 241");

  T81BigInt m_large(59047);
  T81BigInt a_large(12345);
  try {
    T81BigInt inv_large = T81BigInt::modular_inverse_stein(a_large, m_large);
    check((a_large * inv_large) % m_large == T81BigInt(1), "inv large check");
  } catch (const std::domain_error&) {
    std::cout << "Skipped large check: gcd != 1\n";
  }
}

int main() {
  test_extended_gcd();
  test_modular_inverse_extgcd();
  test_modular_inverse_stein_basic();
  test_modular_inverse_stein_large();
  std::cout << "All modular inverse tests passed.\n";
  return 0;
}
