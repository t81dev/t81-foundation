#include "dispatch.hpp"
template <bool policy> static inline u64 impl(i64 a, i64 b) {
  const int c = policy ? (a > 100) - (a < -100) : (a > b) - (a < b);
  return targets[c + 1](a, b);
}

NOINLINE u64 baseline2(i64 a, i64 b) { return impl<false>(a, b); }
NOINLINE u64 baseline2_policy(i64 a, i64 b) { return impl<true>(a, b); }

NOINLINE int probe_baseline2(i64 a, i64 b, bool policy) {
  return policy ? (a > 100) - (a < -100) : (a > b) - (a < b);
}
