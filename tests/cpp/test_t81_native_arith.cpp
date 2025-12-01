#include "t81/native.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>

namespace {
constexpr int kAddDigits = 10;
constexpr int kAddResultDigits = kAddDigits + 4;
constexpr int kMulDigits = 7;
constexpr int kMulResultDigits = kMulDigits * 2;
constexpr int kTrials = 2048;

int64_t Evaluate(const std::array<int8_t, 128>& digits, int limit) {
  int64_t value = 0;
  int64_t weight = 1;
  for (int idx = 0; idx < limit; ++idx) {
    value += static_cast<int64_t>(digits[idx]) * weight;
    weight *= 3;
  }
  return value;
}

t81::T81 MakeNative(const std::array<int8_t, 128>& digits) {
  std::array<uint8_t, 32> buffer{};
  t81::T81::PackDigits(digits, buffer);
  return t81::T81(buffer);
}

void FillRandom(std::array<int8_t, 128>& digits, int count,
                std::mt19937_64& rng,
                std::uniform_int_distribution<int>& dist) {
  std::fill(digits.begin(), digits.end(), 0);
  for (int idx = 0; idx < count; ++idx) {
    digits[idx] = static_cast<int8_t>(dist(rng));
  }
}

}  // namespace

int main() {
  std::mt19937_64 rng(0xABCD1234);
  std::uniform_int_distribution<int> trit_dist(-1, 1);

  for (int trial = 0; trial < kTrials; ++trial) {
    std::array<int8_t, 128> lhs_digits{};
    std::array<int8_t, 128> rhs_digits{};
    FillRandom(lhs_digits, kAddDigits, rng, trit_dist);
    FillRandom(rhs_digits, kAddDigits, rng, trit_dist);

    auto lhs = MakeNative(lhs_digits);
    auto rhs = MakeNative(rhs_digits);
    auto sum = lhs + rhs;

    std::array<int8_t, 128> sum_digits{};
    t81::T81::UnpackDigits(sum.data, sum_digits);

    const int64_t lhs_val = Evaluate(lhs_digits, kAddResultDigits);
    const int64_t rhs_val = Evaluate(rhs_digits, kAddResultDigits);
    const int64_t sum_val = Evaluate(sum_digits, kAddResultDigits);
    if (sum_val != (lhs_val + rhs_val)) {
      std::cerr << "Addition mismatch at trial " << trial << "\n";
      return 1;
    }
  }

  for (int trial = 0; trial < kTrials; ++trial) {
    std::array<int8_t, 128> lhs_digits{};
    std::array<int8_t, 128> rhs_digits{};
    FillRandom(lhs_digits, kMulDigits, rng, trit_dist);
    FillRandom(rhs_digits, kMulDigits, rng, trit_dist);

    auto lhs = MakeNative(lhs_digits);
    auto rhs = MakeNative(rhs_digits);
    auto product = lhs * rhs;

    std::array<int8_t, 128> product_digits{};
    t81::T81::UnpackDigits(product.data, product_digits);

    const int64_t lhs_val = Evaluate(lhs_digits, kMulResultDigits);
    const int64_t rhs_val = Evaluate(rhs_digits, kMulResultDigits);
    const int64_t product_val = Evaluate(product_digits, kMulResultDigits);
    const int64_t expect = lhs_val * rhs_val;
    if (product_val != expect) {
      std::cerr << "Multiplication mismatch at trial " << trial
                << " lhs=" << lhs_val << " rhs=" << rhs_val
                << " result=" << product_val << " expected=" << expect << "\n";
      return 1;
    }
  }

  std::cout << "t81::T81 SIMD fallback arithmetic OK\n";
  return 0;
}
