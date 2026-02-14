#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <string>

#include "t81/bigint.hpp"

namespace {

using t81::T81BigInt;

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "bigint_gcd_divmod_property_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

bool run_property_sweep() {
  std::mt19937_64 rng(0x81D10DULL);
  std::uniform_int_distribution<std::int64_t> dist(-1'000'000'000LL, 1'000'000'000LL);

  for (int i = 0; i < 2000; ++i) {
    const std::int64_t a64 = dist(rng);
    std::int64_t b64 = dist(rng);
    if (b64 == 0) {
      b64 = 1;
    }

    const T81BigInt a = T81BigInt::from_i64(a64);
    const T81BigInt b = T81BigInt::from_i64(b64);

    const auto dm = t81::divmod(a, b);

    // a = b*q + r
    if (!expect(b * dm.q + dm.r == a, "divmod recomposition failed")) return false;

    // Euclidean remainder range: 0 <= r < |b|
    const T81BigInt abs_b = T81BigInt::abs(b);
    if (!expect(!dm.r.is_negative(), "remainder is negative")) return false;
    if (!expect(dm.r < abs_b, "remainder is not strictly less than |b|")) return false;

    const T81BigInt g = T81BigInt::gcd(a, b);
    const std::int64_t expected_g = std::gcd(std::llabs(a64), std::llabs(b64));

    if (!expect(!g.is_negative(), "gcd is negative")) return false;
    if (!expect(g.to_int64() == expected_g, "gcd mismatch vs std::gcd")) return false;

    if (!T81BigInt::is_zero(g)) {
      const auto da = t81::divmod(a, g);
      const auto db = t81::divmod(b, g);
      if (!expect(T81BigInt::is_zero(da.r), "gcd does not divide first operand")) return false;
      if (!expect(T81BigInt::is_zero(db.r), "gcd does not divide second operand")) return false;
    }
  }

  return true;
}

}  // namespace

int main() {
  if (!run_property_sweep()) {
    return 1;
  }
  std::cout << "bigint gcd/divmod property test passed\n";
  return 0;
}
