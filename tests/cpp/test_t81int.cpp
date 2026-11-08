#undef NDEBUG
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include "t81/types/T81Int.hpp"

using namespace t81;

// Helper function to test the INT64_MIN edge case
void test_int64_min_edge_case() {
  std::cout << "Testing INT64_MIN edge case...\n";
  // INT64_MIN = -9223372036854775808
  // Requires 41 trits.
  // Trit string (most significant first): -+-+---00---00+-0-++--+0-0-0+0+0--+0+-00+
  const std::string expected_trits = "-+-+---00---00+-0-++--+0-0-0+0+0--+0+-00+";

  // Case 1: Sufficient capacity (N=42). Should succeed now.
  {
    T81Int<42> v(std::numeric_limits<std::int64_t>::min());
    std::string s = v.to_trit_string();
    assert(s == "0" + expected_trits);

    // Should NOT throw now as index 41 is 0 and index 40 is handled.
    try {
      std::int64_t res = v.to_int64();
      assert(res == std::numeric_limits<std::int64_t>::min());
    } catch (const std::overflow_error& e) {
      std::cerr << "Unexpected overflow for N=42: " << e.what() << "\n";
      assert(false);
    }
  }

  // Case 2: Exact capacity (N=41). Should succeed.
  {
    T81Int<41> v(std::numeric_limits<std::int64_t>::min());
    assert(v.to_trit_string() == expected_trits);
    std::int64_t res = v.to_int64();
    assert(res == std::numeric_limits<std::int64_t>::min());
  }

  // Case 3: Insufficient capacity (N=40). Constructor throws.
  // INT64_MIN requires 41 trits.
  {
    bool threw = false;
    try {
      T81Int<40> v(std::numeric_limits<std::int64_t>::min());
    } catch (const std::overflow_error&) {
      threw = true;
    }
    assert(threw);
  }
}

void test_int64_max_edge_case() {
  std::cout << "Testing INT64_MAX edge case...\n";

  // Case 1: N=41. Should succeed.
  {
    T81Int<41> v(std::numeric_limits<std::int64_t>::max());
    std::int64_t res = v.to_int64();
    assert(res == std::numeric_limits<std::int64_t>::max());
  }

  // Case 2: N=40. Constructor throws.
  // INT64_MAX requires 41 trits.
  {
    bool threw = false;
    try {
      T81Int<40> v(std::numeric_limits<std::int64_t>::max());
    } catch (const std::overflow_error&) {
      threw = true;
    }
    assert(threw);
  }
}

int main() {
  std::cout << "T81Int tests...\n";

  test_int64_min_edge_case();
  test_int64_max_edge_case();

  T81Int<8> a{5}, b{-3};
  assert((a + b).to_binary<int>() == 2);
  assert((a - b).to_binary<int>() == 8);

  T81Int<32> x{10}, y{3};
  assert(x / y == T81Int<32>(3));
  assert(x % y == T81Int<32>(1));

  [[maybe_unused]] bool threw = false;
  try {
    T81Int<32>(1) / T81Int<32>(0);
  } catch (...) {
    threw = true;
  }
  assert(threw);

  bool overflow_trapped = false;
  try {
    [[maybe_unused]] auto v = T81Int<8>::kMaxValue;
    [[maybe_unused]] auto backup = T81Int<8>(1);
    [[maybe_unused]] auto _ = v + backup;
  } catch (const std::overflow_error&) {
    overflow_trapped = true;
  }
  assert(overflow_trapped);

  // SCG-03 / C05 Regression coverage:
  // 1. T81Int<2>(2) * T81Int<2>(2) == 4 (representable, must not throw or corrupt)
  {
    T81Int<2> two(2);
    T81Int<2> prod = two * two;
    assert(prod.to_int64() == 4);

    // Negative operands
    T81Int<2> neg_two(-2);
    assert((neg_two * two).to_int64() == -4);
    assert((two * neg_two).to_int64() == -4);
    assert((neg_two * neg_two).to_int64() == 4);
  }

  // 2. T81Int<2>(3) * T81Int<2>(3) == 9 (unrepresentable in N=2 [-4, 4], must throw std::overflow_error)
  {
    T81Int<2> three(3);
    bool threw_overflow = false;
    try {
      [[maybe_unused]] auto _ = three * three;
    } catch (const std::overflow_error&) {
      threw_overflow = true;
    }
    assert(threw_overflow);

    // Negative unrepresentable cases
    T81Int<2> neg_three(-3);
    threw_overflow = false;
    try {
      [[maybe_unused]] auto _ = neg_three * neg_three;
    } catch (const std::overflow_error&) {
      threw_overflow = true;
    }
    assert(threw_overflow);

    threw_overflow = false;
    try {
      [[maybe_unused]] auto _ = three * neg_three;
    } catch (const std::overflow_error&) {
      threw_overflow = true;
    }
    assert(threw_overflow);
  }

  // 3. Width boundary cases and representable products with 3
  {
    T81Int<2> three(3);
    T81Int<2> one(1);
    T81Int<2> neg_one(-1);
    T81Int<2> zero(0);

    assert((three * one).to_int64() == 3);
    assert((three * neg_one).to_int64() == -3);
    assert((three * zero).to_int64() == 0);
    assert((zero * three).to_int64() == 0);

    // 4 is boundary of N=2
    T81Int<2> four(4);
    assert((four * one).to_int64() == 4);
    assert((four * neg_one).to_int64() == -4);
    assert((-four * neg_one).to_int64() == 4);

    // 4 * 2 = 8 overflows N=2
    bool threw_overflow = false;
    try {
      [[maybe_unused]] auto _ = four * T81Int<2>(2);
    } catch (const std::overflow_error&) {
      threw_overflow = true;
    }
    assert(threw_overflow);
  }

  // 4. Exhaustive small-width verification against independent exact-integer oracle (N=1..4)
  {
    auto test_width = []<std::size_t N>() {
      std::int64_t max_val = 1;
      for (std::size_t i = 0; i < N; ++i) max_val *= 3;
      max_val = (max_val - 1) / 2;

      for (std::int64_t a = -max_val; a <= max_val; ++a) {
        for (std::int64_t b = -max_val; b <= max_val; ++b) {
          const std::int64_t expected_prod = a * b;
          const bool should_fit = (expected_prod >= -max_val && expected_prod <= max_val);
          T81Int<N> ta(a);
          T81Int<N> tb(b);

          if (should_fit) {
            T81Int<N> actual_prod = ta * tb;
            assert(actual_prod.to_int64() == expected_prod);
          } else {
            bool threw = false;
            try {
              [[maybe_unused]] auto _ = ta * tb;
            } catch (const std::overflow_error&) {
              threw = true;
            }
            assert(threw);
          }
        }
      }
    };

    test_width.template operator()<1>();
    test_width.template operator()<2>();
    test_width.template operator()<3>();
    test_width.template operator()<4>();
  }

  // 5. Constexpr multiplication test
  {
    constexpr auto test_constexpr_mul = []() consteval {
      T81Int<2> a;
      a[0] = Trit::N;
      a[1] = Trit::P; // 2
      T81Int<2> b;
      b[0] = Trit::N;
      b[1] = Trit::P; // 2
      T81Int<2> c = a * b; // 4
      return (c[0] == Trit::P && c[1] == Trit::P);
    };
    static_assert(test_constexpr_mul(), "constexpr multiplication failed");
  }

  std::cout << "All T81Int tests passed!\n";
}
