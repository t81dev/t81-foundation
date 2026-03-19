#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "t81/types/T81Int.hpp"

// Helper to check conditions and log failures
auto expect = [](bool cond, const char* msg) -> bool {
  if (!cond) {
    std::cerr << "t81int_properties_test failure: " << msg << "\n";
    return false;
  }
  return true;
};

template <std::size_t N>
bool run_properties_test(std::mt19937_64& rng) {
  using T81IntN = t81::T81Int<N>;

  // Reduce safe limit for N=9 to 10 to be absolutely sure we don't hit overflow
  // during associativity/distributivity checks.
  std::int64_t safe_limit = 0;
  if constexpr (N == 9)
    safe_limit = 10;
  else
    safe_limit = 1000;

  std::uniform_int_distribution<std::int64_t> dist(-safe_limit, safe_limit);

  const T81IntN zero = T81IntN(0);
  const T81IntN one = T81IntN(1);

  for (int i = 0; i < 1000; ++i) {
    const std::int64_t ai = dist(rng);
    const std::int64_t bi = dist(rng);
    const std::int64_t ci = dist(rng);

    T81IntN a(ai);
    T81IntN b(bi);
    T81IntN c(ci);

    try {
      // Commutativity: a + b == b + a
      if (!expect((a + b) == (b + a), "commutativity (+) failed")) return false;
      if (!expect((a * b) == (b * a), "commutativity (*) failed")) return false;

      // Associativity: (a + b) + c == a + (b + c)
      if (!expect(((a + b) + c) == (a + (b + c)), "associativity (+) failed")) return false;

      // Associativity: (a * b) * c == a * (b * c)
      if (!expect(((a * b) * c) == (a * (b * c)), "associativity (*) failed")) return false;

      // Add/sub inverse law: (a + b) - b == a
      if (!expect(((a + b) - b) == a, "add/sub inverse law failed")) return false;

      // Negation involution: -(-a) == a
      if (!expect(-(-a) == a, "double negation failed")) return false;

      // Identity: a + 0 == a, a * 1 == a
      if (!expect((a + zero) == a, "additive identity failed")) return false;
      if (!expect((a * one) == a, "multiplicative identity failed")) return false;

      // Zero property: a * 0 == 0
      if (!expect((a * zero) == zero, "multiplicative zero property failed")) return false;

      // Distributivity: a * (b + c) == a * b + a * c
      if (!expect((a * (b + c)) == ((a * b) + (a * c)), "distributivity failed")) return false;

      if (!b.is_zero()) {
        T81IntN q = a / b;
        T81IntN r = a % b;
        if (!expect((q * b + r) == a, "division identity failed")) return false;
      }
    } catch (const std::overflow_error& e) {
      std::cerr << "Unexpected overflow in property check: " << e.what() << " for inputs: " << ai
                << ", " << bi << ", " << ci << " (N=" << N << ")\n";
      // For now, fail on unexpected overflow because we set safe limits
      return false;
    }
  }

  // Test overflow behavior specifically.
  bool threw = false;
  try {
    [[maybe_unused]] auto v = T81IntN::kMaxValue + one;
  } catch (const std::overflow_error&) {
    threw = true;
  }
  if (!expect(threw, "kMaxValue + 1 did not throw overflow_error")) return false;

  threw = false;
  try {
    [[maybe_unused]] auto v = T81IntN::kMinValue - one;
  } catch (const std::overflow_error&) {
    threw = true;
  }
  if (!expect(threw, "kMinValue - 1 did not throw overflow_error")) return false;

  return true;
}

int main() {
  // Fixed seed for deterministic replay.
  std::mt19937_64 rng(0x815A5A5AULL);

  // Test with N=27 (standard)
  std::cout << "Testing T81Int<27>...\n";
  if (!run_properties_test<27>(rng)) return 1;

  // Test with N=9 (small, fits in short)
  std::cout << "Testing T81Int<9>...\n";
  if (!run_properties_test<9>(rng)) return 1;

  std::cout << "t81int_properties_test ok\n";
  return 0;
}
