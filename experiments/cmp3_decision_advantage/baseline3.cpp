#include "dispatch.hpp"
#include <compare>
template <bool policy> static inline u64 impl(i64 a, i64 b) {
  if (policy) {
    if ((a <=> -100) < 0)
      return target_less(a, b);
    if ((a <=> 100) > 0)
      return target_greater(a, b);
    return target_equal(a, b);
  }
  const auto c = a <=> b;
  if (c < 0)
    return target_less(a, b);
  if (c == 0)
    return target_equal(a, b);
  return target_greater(a, b);
}

NOINLINE u64 baseline3(i64 a, i64 b) { return impl<false>(a, b); }
NOINLINE u64 baseline3_policy(i64 a, i64 b) { return impl<true>(a, b); }

NOINLINE int probe_baseline3(i64 a, i64 b, bool policy) {
  if (policy) {
    if ((a <=> -100) < 0)
      return -1;
    if ((a <=> 100) > 0)
      return 1;
    return 0;
  }
  auto c = a <=> b;
  if (c < 0)
    return -1;
  if (c == 0)
    return 0;
  return 1;
}
