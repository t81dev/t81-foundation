#include "t81/types/T81Int128.hpp"

#include <cstdint>
#include <iostream>
#include <limits>

using t81::v1::detail::int128_t;

namespace {

int g_failures = 0;

void check_eq(const char* name, int128_t actual, int128_t expected) {
  if (actual != expected) {
    std::cerr << "FAIL: " << name << "\n";
    ++g_failures;
  }
}

void check_i64(const char* name, int128_t actual, std::int64_t expected) {
  check_eq(name, actual, int128_t(expected));
}

}  // namespace

int main() {
  constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
  constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();

  // Negation & carry/borrow boundaries
  check_eq("neg_zero", -int128_t(0), int128_t(0));
  check_eq("add_max_plus_1_sub_1", int128_t(kMax) + int128_t(1) - int128_t(1), int128_t(kMax));
  check_eq("sub_min_minus_1_add_1", int128_t(kMin) - int128_t(1) + int128_t(1), int128_t(kMin));
  check_eq("neg_min_plus_min", -int128_t(kMin) + int128_t(kMin), int128_t(0));

  // Signed multiplication (int64_t and int128_t)
  check_i64("mul_zero", int128_t(kMin) * std::int64_t{0}, 0);
  check_eq("mul_min_neg1", int128_t(kMin) * std::int64_t{-1}, -int128_t(kMin));
  check_eq("mul_pos_min", int128_t(2) * kMin, int128_t(kMin) + int128_t(kMin));
  check_eq("mul_neg_neg_128", int128_t(-3) * int128_t(kMin),
           -(int128_t(kMin) + int128_t(kMin) + int128_t(kMin)));

  const int128_t pow2_40 = int128_t(std::int64_t{1} << 40);
  const int128_t pow2_80 = pow2_40 * pow2_40;
  check_eq("mul_128_pow2_80_div", pow2_80 / pow2_40, pow2_40);
  check_i64("mul_128_pow2_80_rem", pow2_80 % pow2_40, 0);

  // Signed division & remainder across signs and INT64_MIN boundaries
  const int128_t big_pos = int128_t(kMax) * std::int64_t{10} + int128_t(7);
  check_i64("div_pos_pos", big_pos / int128_t(10), kMax);
  check_i64("rem_pos_pos", big_pos % int128_t(10), 7);
  check_i64("div_pos_neg", big_pos / int128_t(-10), -kMax);
  check_i64("rem_pos_neg", big_pos % int128_t(-10), 7);

  const int128_t big_neg = -big_pos;
  check_i64("div_neg_pos", big_neg / int128_t(10), -kMax);
  check_i64("rem_neg_pos", big_neg % int128_t(10), -7);
  check_i64("div_neg_neg", big_neg / int128_t(-10), kMax);
  check_i64("rem_neg_neg", big_neg % int128_t(-10), -7);

  check_eq("div_min_neg1", int128_t(kMin) / int128_t(-1), -int128_t(kMin));
  check_i64("rem_min_neg1", int128_t(kMin) % int128_t(-1), 0);
  check_i64("div_min_2", int128_t(kMin) / int128_t(2), kMin / 2);
  check_i64("rem_min_3", int128_t(kMin) % int128_t(3), kMin % 3);
  check_i64("div_min_min", int128_t(kMin) / int128_t(kMin), 1);
  check_i64("rem_min_min", int128_t(kMin) % int128_t(kMin), 0);
  check_i64("div_small_by_min", int128_t(123) / int128_t(kMin), 0);
  check_i64("rem_small_by_min", int128_t(123) % int128_t(kMin), 123);

  if (g_failures != 0) {
    std::cerr << g_failures << " t81_int128_test check(s) failed.\n";
    return 1;
  }
  return 0;
}
