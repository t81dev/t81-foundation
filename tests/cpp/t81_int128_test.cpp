#include "t81/types/T81Int128.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

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

int128_t from_words(std::uint64_t lo, std::int64_t hi) {
#if T81_INT128_BACKEND_PORTABLE
  return {lo, hi};
#else
  const int128_t two64 = (int128_t(std::numeric_limits<std::int64_t>::max()) + 1) * 2;
  return int128_t(hi) * two64 + int128_t(lo);
#endif
}

void check_divmod(const char* name, int128_t a, int128_t b, int128_t q, int128_t r) {
  check_eq(name, a / b, q);
  check_eq(name, a % b, r);
  check_eq("divmod recomposition", (a / b) * b + a % b, a);
}

void check_boundaries() {
  constexpr auto kMin = std::numeric_limits<std::int64_t>::min();
  check_divmod("python integer vector 0", from_words(0x0000000000000000ULL, kMin),
               from_words(0x0000000000000003ULL, 0LL),
               from_words(0x5555555555555556ULL, -3074457345618258603LL),
               from_words(0xfffffffffffffffeULL, -1LL));
  check_divmod("python integer vector 1", from_words(0xffffffffffffffffULL, 9223372036854775807LL),
               from_words(0xfffffffffffffffdULL, -1LL),
               from_words(0x5555555555555556ULL, -3074457345618258603LL),
               from_words(0x0000000000000001ULL, 0LL));
  check_divmod("python integer vector 2", from_words(0x000000000000007bULL, 68719476736LL),
               from_words(0x0000000000000007ULL, 64LL), from_words(0x000000003fffffffULL, 0LL),
               from_words(0xfffffffe40000082ULL, 63LL));
  check_divmod("python integer vector 3", from_words(0xffffffffffffff85ULL, -68719476737LL),
               from_words(0x0000000000000007ULL, 64LL), from_words(0xffffffffc0000001ULL, -1LL),
               from_words(0x00000001bfffff7eULL, -64LL));
  check_divmod("python integer vector 4", from_words(0x000000000000007bULL, 68719476736LL),
               from_words(0xfffffffffffffff9ULL, -65LL), from_words(0xffffffffc0000001ULL, -1LL),
               from_words(0xfffffffe40000082ULL, 63LL));
  check_divmod("python integer vector 5", from_words(0x0000000000000025ULL, 65536LL),
               from_words(0x0000000000000007ULL, 0LL), from_words(0x4924924924924929ULL, 9362LL),
               from_words(0x0000000000000006ULL, 0LL));
  check_divmod("python integer vector 6", from_words(0x0000000000000025ULL, 65536LL),
               from_words(0xfffffffffffffff9ULL, -1LL), from_words(0xb6db6db6db6db6d7ULL, -9363LL),
               from_words(0x0000000000000006ULL, 0LL));
  check_divmod("python integer vector 7", from_words(0xffffffffffffffdbULL, -65537LL),
               from_words(0x0000000000000007ULL, 0LL), from_words(0xb6db6db6db6db6d7ULL, -9363LL),
               from_words(0xfffffffffffffffaULL, -1LL));
  check_divmod("python integer vector 8", from_words(0xffffffffffffffdbULL, -65537LL),
               from_words(0xfffffffffffffff9ULL, -1LL), from_words(0x4924924924924929ULL, 9362LL),
               from_words(0xfffffffffffffffaULL, -1LL));
  check_divmod("python integer vector 9", from_words(0x0000000000000000ULL, kMin),
               from_words(0x0000000000000000ULL, kMin), from_words(0x0000000000000001ULL, 0LL),
               from_words(0x0000000000000000ULL, 0LL));
  check_divmod("python integer vector 10", from_words(0xffffffffffffffffULL, 9223372036854775807LL),
               from_words(0x0000000000000000ULL, kMin), from_words(0x0000000000000000ULL, 0LL),
               from_words(0xffffffffffffffffULL, 9223372036854775807LL));
  check_divmod("python integer vector 11", from_words(0x0000000000000000ULL, kMin),
               from_words(0x0000000000000002ULL, 0LL),
               from_words(0x0000000000000000ULL, -4611686018427387904LL),
               from_words(0x0000000000000000ULL, 0LL));
  check_divmod("python integer vector 12", from_words(0x0000000000000000ULL, kMin),
               from_words(0x0000000000000001ULL, 0LL), from_words(0x0000000000000000ULL, kMin),
               from_words(0x0000000000000000ULL, 0LL));
  check_divmod("python integer vector 13", from_words(0x0000000000000000ULL, kMin),
               from_words(0xfffffffffffffffeULL, -1LL),
               from_words(0x0000000000000000ULL, 4611686018427387904LL),
               from_words(0x0000000000000000ULL, 0LL));
  check_divmod("python integer vector 14", from_words(0xffffffffffffffffULL, 9223372036854775807LL),
               from_words(0x0000000000000001ULL, 0LL),
               from_words(0xffffffffffffffffULL, 9223372036854775807LL),
               from_words(0x0000000000000000ULL, 0LL));
  check_divmod("python integer vector 15", from_words(0x000000000000007bULL, 0LL),
               from_words(0x8000000000000000ULL, -1LL), from_words(0x0000000000000000ULL, 0LL),
               from_words(0x000000000000007bULL, 0LL));
  check_divmod("python integer vector 16", from_words(0xffffffffffffff85ULL, -1LL),
               from_words(0x8000000000000000ULL, 0LL), from_words(0x0000000000000000ULL, 0LL),
               from_words(0xffffffffffffff85ULL, -1LL));
  check_eq("python product vector 0",
           from_words(0x8000000000000000ULL, -1LL) * from_words(0x8000000000000000ULL, -1LL),
           from_words(0x0000000000000000ULL, 4611686018427387904LL));
  check_eq("python product vector 1",
           from_words(0x1000000000000013ULL, 0LL) * from_words(0x0ffffffffffffff9ULL, 0LL),
           from_words(0xbfffffffffffff7bULL, 72057594037927936LL));
  check_eq("python product vector 2",
           from_words(0xffffffffffffffdbULL, -65537LL) * from_words(0x0000010000000013ULL, 0LL),
           from_words(0xffffdafffffffd41ULL, -72057594039173121LL));
  check_eq("python product vector 3",
           from_words(0x0000000000000000ULL, kMin) * from_words(0x0000000000000001ULL, 0LL),
           from_words(0x0000000000000000ULL, kMin));
#if T81_INT128_BACKEND_PORTABLE
  for (bool remainder : {false, true}) {
    bool zero_rejected = false, overflow_rejected = false;
    try {
      (void)(remainder ? int128_t(1) % int128_t(0) : int128_t(1) / int128_t(0));
    } catch (const std::domain_error&) {
      zero_rejected = true;
    }
    try {
      auto minimum = from_words(0, kMin);
      (void)(remainder ? minimum % int128_t(-1) : minimum / int128_t(-1));
    } catch (const std::overflow_error&) {
      overflow_rejected = true;
    }
    if (!zero_rejected || !overflow_rejected) ++g_failures;
  }
#endif
}

#if T81_INT128_BACKEND_PORTABLE && T81_HAS_NATIVE_INT128
void check_native_reference() {
  __extension__ typedef __int128 native;
  __extension__ typedef unsigned __int128 unative;
  const native two64 = (native(1) << 64);
  auto ref = [&](std::uint64_t lo, std::int64_t hi) { return native(hi) * two64 + native(lo); };
  auto expected = [](native v) {
    return from_words(static_cast<std::uint64_t>(v),
                      static_cast<std::int64_t>(static_cast<unative>(v) >> 64));
  };
  std::mt19937_64 rng(0x782128);
  for (int i = 0; i < 1000; ++i) {
    const auto al = rng(), bl = rng();
    // Bound the numerator to 127 magnitude bits, excluding MIN/-1 overflow.
    const auto ah = static_cast<std::int64_t>(rng() >> 1) - (std::int64_t{1} << 62);
    const auto bh =
        i % 3 == 0 ? 0 : static_cast<std::int64_t>(rng() >> 1) - (std::int64_t{1} << 62);
    const native a = ref(al, ah), b = ref(bl | 1, bh);
    const auto pa = from_words(al, ah), pb = from_words(bl | 1, bh);
    check_eq("native-reference quotient", pa / pb, expected(a / b));
    check_eq("native-reference remainder", pa % pb, expected(a % b));
    const auto x = static_cast<std::int64_t>(rng());
    const auto y = static_cast<std::int64_t>(rng());
    check_eq("native-reference product", int128_t(x) * int128_t(y), expected(native(x) * y));
  }
}
#endif

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

  check_boundaries();
#if T81_INT128_BACKEND_PORTABLE && T81_HAS_NATIVE_INT128
  check_native_reference();
#endif
  std::cout << "int128 backend=" << (T81_INT128_BACKEND_PORTABLE ? "portable" : "native")
            << "; Release-active checks; failures=" << g_failures << "\n";

  if (g_failures != 0) {
    std::cerr << g_failures << " t81_int128_test check(s) failed.\n";
    return 1;
  }
  return 0;
}
