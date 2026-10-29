#include "dispatch.hpp"
static std::int32_t cmp3_s64(i64 a, i64 b) { return (a > b) - (a < b); }
template <bool policy> static inline u64 impl(i64 a, i64 b) {
  const int c = policy ? (cmp3_s64(a, -100) < 0  ? -1
                          : cmp3_s64(a, 100) > 0 ? 1
                                                 : 0)
                       : cmp3_s64(a, b);
  return targets[c + 1](a, b);
}

NOINLINE u64 candidate(i64 a, i64 b) { return impl<false>(a, b); }
NOINLINE u64 candidate_policy(i64 a, i64 b) { return impl<true>(a, b); }

NOINLINE int probe_candidate(i64 a, i64 b, bool policy) {
  return policy ? (cmp3_s64(a, -100) < 0  ? -1
                   : cmp3_s64(a, 100) > 0 ? 1
                                          : 0)
                : cmp3_s64(a, b);
}
