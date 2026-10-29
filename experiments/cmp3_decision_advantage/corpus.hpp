#pragma once
#include "pcg64.hpp"
#include "resource_budget.hpp"
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
using i64 = std::int64_t;
using u64 = std::uint64_t;

struct Pair {
  i64 a;
  i64 b;
};

// Exact integer comparison oracle
static int oracle_pattern_ac(i64 a, i64 b) { return (a > b) - (a < b); }

// Exact policy threshold check oracle for Pattern B (L = -100, U = +100)
static int oracle_pattern_b(i64 v) {
  if (v < -100)
    return -1;
  if (v <= 100)
    return 0;
  return 1;
}

static std::array<Pair, 20> mandatory_boundaries() {
  using L = std::numeric_limits<i64>;
  return {{{0, 0},
           {42, 42},
           {-42, -42},
           {L::max(), L::max()},
           {L::min(), L::min()},
           {L::min(), L::max()},
           {L::max(), L::min()},
           {100, 101},
           {101, 100},
           {-101, -100},
           {-100, -101},
           {-1, 0},
           {0, -1},
           {0, 1},
           {1, 0},
           {-1, 1},
           {1, -1},
           {9007199254740993LL, 9007199254740994LL},
           {9007199254740994LL, 9007199254740993LL},
           {L::max() - 1, L::max()}}};
}

static std::vector<Pair> generate_corpus(int pattern, std::size_t n) {
  if (pattern < 0 || pattern > 2)
    throw std::invalid_argument("INVALID_PATTERN");
  PCG64 g;
  std::vector<Pair> v;
  v.reserve(n);
  const i64 low = std::numeric_limits<i64>::min() + (1LL << 30);
  constexpr u64 span = std::numeric_limits<u64>::max() - (1ULL << 31) + 1;

  for (std::size_t i = 0; i < n; ++i) {
    if (i % 100000 == 0)
      check_resource_budget();
    double u = g.prob();

    if (pattern == 1) {
      // Pattern B: Policy Verdict Threshold Evaluation (V vs L=-100, U=+100)
      i64 V = 0;
      u64 kb = g.bounded(1000) + 1;
      if (u < 0.20) {
        V = -100 - static_cast<i64>(kb); // V < L (-100) -> DENY (-1)
      } else if (u < 0.30) {
        V = -100 + static_cast<i64>(g.bounded(201)); // L <= V <= U -> AUDIT (0)
      } else {
        V = 100 + static_cast<i64>(kb); // V > U (+100) -> ALLOW (+1)
      }
      v.push_back({V, 0});
      continue;
    }

    // Pattern A and C
    const u64 offset = g.bounded(span);
    const i64 a = offset <= static_cast<u64>(std::numeric_limits<i64>::max())
                      ? low + static_cast<i64>(offset)
                      : static_cast<i64>(offset - (1ULL << 63)) + (1LL << 30);
    const u64 k = g.bounded(1ULL << 30) + 1;
    double cut = (pattern == 0) ? 0.475 : (1.0 / 3.0);
    double upper = (pattern == 0) ? 0.525 : (2.0 / 3.0);
    i64 b = (u < cut) ? (a + static_cast<i64>(k))
                      : ((u < upper) ? a : (a - static_cast<i64>(k)));
    v.push_back({a, b});
  }
  return v;
}
