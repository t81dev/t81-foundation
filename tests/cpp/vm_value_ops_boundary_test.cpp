#include "test_runtime_check.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include "vm/internal/value_ops.hpp"

using t81::vm::internal::add_int;
using t81::vm::internal::div_int;
using t81::vm::internal::mod_int;
using t81::vm::internal::mul_int;
using t81::vm::internal::neg_int;
using t81::vm::internal::sub_int;

int main() {
  constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
  constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();

  // --- neg_int: Two's-complement negation semantics ---
  T81_TEST_CHECK(neg_int(0) == 0);
  T81_TEST_CHECK(neg_int(1) == -1);
  T81_TEST_CHECK(neg_int(-1) == 1);
  T81_TEST_CHECK(neg_int(42) == -42);
  T81_TEST_CHECK(neg_int(-42) == 42);
  T81_TEST_CHECK(neg_int(kMax) == -kMax);
  T81_TEST_CHECK(neg_int(-kMax) == kMax);

  // In two's-complement arithmetic, -INT64_MIN wraps back to INT64_MIN (modulo 2^64).
  // It must NOT saturate to INT64_MAX.
  T81_TEST_CHECK(neg_int(kMin) == kMin);

  // --- div_int: Division semantics & boundary overflow handling ---
  std::int64_t out = 0;

  // Normal division
  T81_TEST_CHECK(div_int(10, 2, &out));
  T81_TEST_CHECK(out == 5);
  T81_TEST_CHECK(div_int(-10, 2, &out));
  T81_TEST_CHECK(out == -5);
  T81_TEST_CHECK(div_int(10, -2, &out));
  T81_TEST_CHECK(out == -5);
  T81_TEST_CHECK(div_int(-10, -2, &out));
  T81_TEST_CHECK(out == 5);
  T81_TEST_CHECK(div_int(0, 7, &out));
  T81_TEST_CHECK(out == 0);

  // Divide by zero must fail closed
  T81_TEST_CHECK(!div_int(10, 0, &out));
  T81_TEST_CHECK(!div_int(0, 0, &out));
  T81_TEST_CHECK(!div_int(kMin, 0, &out));

  // Null output pointer must fail closed
  T81_TEST_CHECK(!div_int(10, 2, nullptr));

  // INT64_MIN / -1 overflows signed 64-bit int (+2^63 exceeds INT64_MAX).
  // In hardware (x86/ARM), this triggers #DE/SIGFPE. div_int must return false.
  T81_TEST_CHECK(!div_int(kMin, -1, &out));

  // Safe boundary divisions
  T81_TEST_CHECK(div_int(kMin, 1, &out));
  T81_TEST_CHECK(out == kMin);
  T81_TEST_CHECK(div_int(kMax, 1, &out));
  T81_TEST_CHECK(out == kMax);
  T81_TEST_CHECK(div_int(kMax, -1, &out));
  T81_TEST_CHECK(out == -kMax);

  // --- mod_int: Modulo semantics & hardware exception avoidance ---
  // Normal modulo (truncation towards zero)
  T81_TEST_CHECK(mod_int(10, 3, &out));
  T81_TEST_CHECK(out == 1);
  T81_TEST_CHECK(mod_int(-10, 3, &out));
  T81_TEST_CHECK(out == -1);
  T81_TEST_CHECK(mod_int(10, -3, &out));
  T81_TEST_CHECK(out == 1);
  T81_TEST_CHECK(mod_int(-10, -3, &out));
  T81_TEST_CHECK(out == -1);

  // Modulo by zero must fail closed
  T81_TEST_CHECK(!mod_int(10, 0, &out));
  T81_TEST_CHECK(!mod_int(0, 0, &out));
  T81_TEST_CHECK(!mod_int(kMin, 0, &out));

  // Null output pointer must fail closed
  T81_TEST_CHECK(!mod_int(10, 3, nullptr));

  // INT64_MIN % -1: Mathematically, (-2^63) = (-1)*(2^63) + 0, so remainder is 0.
  // Hardware idiv triggers #DE on x86 for this case. mod_int intercepts and returns 0.
  T81_TEST_CHECK(mod_int(kMin, -1, &out));
  T81_TEST_CHECK(out == 0);

  // Safe boundary modulos
  T81_TEST_CHECK(mod_int(kMin, 1, &out));
  T81_TEST_CHECK(out == 0);
  T81_TEST_CHECK(mod_int(kMin, 2, &out));
  T81_TEST_CHECK(out == 0);
  T81_TEST_CHECK(mod_int(kMax, 1, &out));
  T81_TEST_CHECK(out == 0);
  T81_TEST_CHECK(mod_int(kMax, -1, &out));
  T81_TEST_CHECK(out == 0);

  // --- add_int, sub_int, mul_int: Well-defined two's complement wrapping ---
  T81_TEST_CHECK(add_int(kMax, 1) == kMin);
  T81_TEST_CHECK(sub_int(kMin, 1) == kMax);
  T81_TEST_CHECK(mul_int(kMax, 2) == -2);

  std::cout << "All vm_value_ops_boundary_test checks passed successfully!\n";
  return 0;
}
