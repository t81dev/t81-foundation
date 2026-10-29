#pragma once
#include <cstdint>
using i64 = std::int64_t;
using u64 = std::uint64_t;
#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif
using Dispatch = u64 (*)(i64, i64);
NOINLINE u64 target_less(i64, i64);
NOINLINE u64 target_equal(i64, i64);
NOINLINE u64 target_greater(i64, i64);
using Target = u64 (*)(i64, i64);
inline const Target targets[3] = {target_less, target_equal, target_greater};
NOINLINE u64 candidate(i64, i64);
NOINLINE u64 candidate_policy(i64, i64);
NOINLINE u64 baseline1(i64, i64);
NOINLINE u64 baseline1_policy(i64, i64);
NOINLINE u64 baseline2(i64, i64);
NOINLINE u64 baseline2_policy(i64, i64);
NOINLINE u64 baseline3(i64, i64);
NOINLINE u64 baseline3_policy(i64, i64);
NOINLINE u64 baseline4(i64, i64);
NOINLINE u64 baseline4_policy(i64, i64);

// Untimed correctness probes; never used by the measured dispatch loops.
using Classifier = int (*)(i64, i64, bool);
NOINLINE int probe_candidate(i64, i64, bool);
NOINLINE int probe_baseline1(i64, i64, bool);
NOINLINE int probe_baseline2(i64, i64, bool);
NOINLINE int probe_baseline3(i64, i64, bool);
NOINLINE int probe_baseline4(i64, i64, bool);
