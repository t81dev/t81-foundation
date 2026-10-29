#include "dispatch.hpp"
template <bool policy> static inline u64 impl(i64 a, i64 b) {
  const int c = policy ? (a < -100  ? -1
                          : a > 100 ? 1
                                    : 0)
                       : (a == b  ? 0
                          : a < b ? -1
                                  : 1);
  switch (c) {
  case -1:
    return target_less(a, b);
  case 0:
    return target_equal(a, b);
  default:
    return target_greater(a, b);
  }
}

NOINLINE u64 baseline4(i64 a, i64 b) { return impl<false>(a, b); }
NOINLINE u64 baseline4_policy(i64 a, i64 b) { return impl<true>(a, b); }

NOINLINE int probe_baseline4(i64 a, i64 b, bool policy) {
  return policy ? (a < -100  ? -1
                   : a > 100 ? 1
                             : 0)
                : (a == b  ? 0
                   : a < b ? -1
                           : 1);
}
