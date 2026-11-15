#include "test_runtime_check.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include "t81/experimental/cog/tier.hpp"
#include "vm/internal/tier_limits.hpp"
#include "vm/internal/value_ops.hpp"

using t81::vm::internal::add_int;
using t81::vm::internal::div_int;
using t81::vm::internal::mod_int;
using t81::vm::internal::mul_int;
using t81::vm::internal::neg_int;
using t81::vm::internal::sub_int;

namespace {
void check_tier_rank_mapping() {
  using t81::cog::TierId;
  using namespace t81::vm::internal;
  constexpr TierId tiers[] = {TierId::Tier0, TierId::Tier1, TierId::Tier2, TierId::Tier3,
                             TierId::Tier4, TierId::Tier5, TierId::Tier6};
  for (int rank = 0; rank <= 6; ++rank) {
    T81_TEST_CHECK(tier_from_rank(rank) == tiers[rank]);
    T81_TEST_CHECK(tier_rank(tier_from_rank(rank)) == rank);
  }
  // Invalid internal ranks clamp to the nearest endpoint; they do not grant promotion.
  T81_TEST_CHECK(tier_from_rank(-1) == TierId::Tier0);
  T81_TEST_CHECK(tier_from_rank(std::numeric_limits<int>::min()) == TierId::Tier0);
  T81_TEST_CHECK(tier_from_rank(7) == TierId::Tier6);
  T81_TEST_CHECK(tier_from_rank(std::numeric_limits<int>::max()) == TierId::Tier6);
  const auto top = tier_from_rank(6);
  T81_TEST_CHECK(recursion_limit_for_tier(top) == 2187);
  T81_TEST_CHECK(max_shape_complexity_for_tier(top) == 81 * 81 * 11);
  T81_TEST_CHECK(max_tensor_rank_for_tier(top) == 11);
  T81_TEST_CHECK(max_symbolic_complexity_for_tier(top) == 19683);
  T81_TEST_CHECK(max_branch_entropy_for_tier(top) == 486.0);
}
}  // namespace

int main() {
  check_tier_rank_mapping();
  // These wrapping helpers are separate from the VM's arbitrary-precision opcode path.
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
  // Evaluating it in C++ is undefined behavior and may cause a host exception
  // (for example, x86 IDIV #DE). div_int must return false on every architecture.
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
  // The C++ expression is undefined; x86 IDIV may raise #DE. Intercept it before evaluation.
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
