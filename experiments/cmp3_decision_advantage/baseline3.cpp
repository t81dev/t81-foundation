#include <compare>
#include "dispatch.hpp"
template <bool policy>
static inline u64 impl(i64 a, i64 b) {
  if (policy) {
    if ((a <=> -100) < 0) return target_less(a, b);
    if ((a <=> 100) > 0) return target_greater(a, b);
    return target_equal(a, b);
  }
  const auto c = a <=> b;
  if (c < 0) return target_less(a, b);
  if (c == 0) return target_equal(a, b);
  return target_greater(a, b);
}

NOINLINE u64 baseline3(i64 a, i64 b) { return impl<false>(a, b); }
NOINLINE u64 baseline3_policy(i64 a, i64 b) { return impl<true>(a, b); }
