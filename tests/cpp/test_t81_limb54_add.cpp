#include "t81/core/T81Limb.hpp"

#include <array>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <random>

namespace {
using t81::core::T81Limb54;

constexpr int kTrials = 8192;

std::array<int8_t, T81Limb54::TRITS> RandomTrits(std::mt19937_64& rng) {
  std::array<int8_t, T81Limb54::TRITS> trits{};
  std::uniform_int_distribution<int> dist(-1, 1);
  for (auto& trit : trits) {
    trit = static_cast<int8_t>(dist(rng));
  }
  return trits;
}

std::array<int8_t, T81Limb54::TRITS> AddTrits(
    const std::array<int8_t, T81Limb54::TRITS>& lhs,
    const std::array<int8_t, T81Limb54::TRITS>& rhs) {
  std::array<int8_t, T81Limb54::TRITS> sum{};
  int carry = 0;
  for (int idx = 0; idx < T81Limb54::TRITS; ++idx) {
    int total = static_cast<int>(lhs[idx]) + static_cast<int>(rhs[idx]) + carry;
    if (total > 1) {
      sum[idx] = static_cast<int8_t>(total - 3);
      carry = 1;
    } else if (total < -1) {
      sum[idx] = static_cast<int8_t>(total + 3);
      carry = -1;
    } else {
      sum[idx] = static_cast<int8_t>(total);
      carry = 0;
    }
  }
  return sum;
}

void Dump(const T81Limb54& limb, const char* label) {
  auto trits = limb.to_trits();
  std::cerr << label << ":";
  for (int idx = 0; idx < T81Limb54::TRITS; ++idx) {
    std::cerr << ' ' << static_cast<int>(trits[idx]);
  }
  std::cerr << '\n';
}

void VerifyMatch(const T81Limb54& lhs, const T81Limb54& rhs,
                 const char* label) {
  auto expected_trits = AddTrits(lhs.to_trits(), rhs.to_trits());
  T81Limb54 expected = T81Limb54::from_trits(expected_trits);
  T81Limb54 actual = lhs + rhs;
  if (expected.to_trits() != actual.to_trits()) {
    std::cerr << "Mismatch in " << label << "\n";
    Dump(lhs, "  lhs");
    Dump(rhs, "  rhs");
    Dump(expected, "expected");
    Dump(actual, "actual");
    std::exit(1);
  }
}
}  // namespace

int main() {
  std::mt19937_64 rng(0xCAFEBABE);
  for (int trial = 0; trial < kTrials; ++trial) {
    auto trits_a = RandomTrits(rng);
    auto trits_b = RandomTrits(rng);
    T81Limb54 lhs = T81Limb54::from_trits(trits_a);
    T81Limb54 rhs = T81Limb54::from_trits(trits_b);
    VerifyMatch(lhs, rhs, "random trial");
    VerifyMatch(rhs, lhs, "random trial commuted");
  }
  std::cout << "T81Limb54::operator+ matches naive trit addition\n";
  return 0;
}
