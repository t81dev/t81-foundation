#include "t81/core/T81Limb.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

namespace {
using t81::core::T81Limb;

constexpr int kTrials = 4096;

T81Limb RandomLimb(std::mt19937_64& rng,
                    std::uniform_int_distribution<int>& dist) {
  T81Limb limb;
  for (int idx = 0; idx < T81Limb::TRYTES; ++idx) {
    limb.set_tryte(idx, static_cast<int8_t>(dist(rng)));
  }
  return limb;
}

void Dump(const T81Limb& limb, const char* label) {
  auto trits = limb.to_trits();
  std::cerr << label << ":";
  for (int i = 0; i < T81Limb::TRITS; ++i) {
    std::cerr << " " << static_cast<int>(trits[i]);
  }
  std::cerr << "\n";
}

void VerifyMatch(const T81Limb& lhs, const T81Limb& rhs,
                 const char* label = "") {
  T81Limb expected = lhs + rhs;
  T81Limb actual = t81::core::bohemian_add(lhs, rhs);
  if (std::memcmp(&expected, &actual, sizeof(expected)) != 0) {
    std::cerr << "Mismatch in " << label << "\n";
    Dump(lhs, "  lhs");
    Dump(rhs, "  rhs");
    Dump(expected, "expected");
    Dump(actual, "actual");
#if defined(T81_BOHEMIAN_DEBUG)
    std::cerr << "[T81_BOHEMIAN_DEBUG]: enable to view stage arrays\n";
#endif
    std::exit(1);
  }
}
}  // namespace

int main() {
  std::mt19937_64 rng(0xDEADBEEF);
  std::uniform_int_distribution<int> tryte_dist(-13, 13);

  for (int trial = 0; trial < kTrials; ++trial) {
    auto lhs = RandomLimb(rng, tryte_dist);
    auto rhs = RandomLimb(rng, tryte_dist);
    VerifyMatch(lhs, rhs, "random trial");
    VerifyMatch(rhs, lhs, "random trial commutative");
  }

  std::cout << "t81::core bohemian_add matches operator+ for all checked inputs\n";
  return 0;
}
