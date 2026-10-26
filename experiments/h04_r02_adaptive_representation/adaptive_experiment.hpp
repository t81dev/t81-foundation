#ifndef T81_EXPERIMENTS_H04_R02_ADAPTIVE_EXPERIMENT_HPP_
#define T81_EXPERIMENTS_H04_R02_ADAPTIVE_EXPERIMENT_HPP_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace t81::research::h04_r02 {

enum class Arm {
  FB,  // Fixed Binary (64-bit)
  AB,  // Adaptive Binary (8, 16, 32, 64-bit)
  FT,  // Fixed Balanced Ternary (41 trits)
  AT   // Adaptive Balanced Ternary (6, 11, 21, 41 trits)
};

inline std::string arm_to_string(Arm arm) {
  switch (arm) {
    case Arm::FB:
      return "FB";
    case Arm::AB:
      return "AB";
    case Arm::FT:
      return "FT";
    case Arm::AT:
      return "AT";
  }
  return "UNKNOWN";
}

struct TierLimits {
  static constexpr int kNumTiers = 4;
  static constexpr int binary_bits[4] = {8, 16, 32, 64};
  static constexpr int ternary_trits[4] = {6, 11, 21, 41};

  static constexpr int64_t binary_min[4] = {-128LL, -32768LL, -2147483648LL,
                                            -9223372036854775807LL - 1LL};

  static constexpr int64_t binary_max[4] = {127LL, 32767LL, 2147483647LL, 9223372036854775807LL};

  static constexpr int64_t ternary_min[4] = {
      -364LL, -88573LL, -5230176601LL,
      -9223372036854775807LL -
          1LL  // Standard 64-bit domain boundary; 41 trits covers -18236498188585393201LL
  };

  static constexpr int64_t ternary_max[4] = {
      364LL, 88573LL, 5230176601LL,
      9223372036854775807LL  // Standard 64-bit domain boundary; 41 trits covers
                             // 18236498188585393201LL
  };

  static int get_binary_tier(int64_t val) {
    for (int t = 0; t < kNumTiers; ++t) {
      if (val >= binary_min[t] && val <= binary_max[t]) {
        return t;
      }
    }
    return 3;
  }

  static int get_ternary_tier(int64_t val) {
    for (int t = 0; t < kNumTiers; ++t) {
      if (val >= ternary_min[t] && val <= ternary_max[t]) {
        return t;
      }
    }
    return 3;
  }
};

struct CostParameters {
  double symbol_cost_ratio = 1.0;          // C3/C2
  double metadata_bits_per_block = 2.0;    // 2 bits per block mode descriptor
  int block_size = 1;                      // one shared mode per block
  int binary_tier0_bits = 8;               // sensitivity control: 8 or 10
  double transition_cost_per_level = 1.0;  // penalty per level switch
  double conversion_cost = 0.5;            // conversion / selection work per check
};

struct TraceLedger {
  int64_t total_elements = 0;
  int64_t symbol_slots = 0;
  double weighted_symbol_cost = 0.0;
  double metadata_cost = 0.0;
  int64_t transition_count = 0;
  int64_t transition_level_delta = 0;
  double transition_cost = 0.0;
  int64_t selector_checks = 0;
  double total_cost = 0.0;

  std::vector<int> tier_sequence;
};

class Evaluator {
public:
  static TraceLedger evaluate_trace(Arm arm, const std::vector<int64_t>& trace,
                                    const CostParameters& params) {
    if (params.block_size <= 0 ||
        (params.binary_tier0_bits != 8 && params.binary_tier0_bits != 10) ||
        !std::isfinite(params.symbol_cost_ratio) || params.symbol_cost_ratio < 0 ||
        !std::isfinite(params.metadata_bits_per_block) || params.metadata_bits_per_block < 0 ||
        !std::isfinite(params.transition_cost_per_level) || params.transition_cost_per_level < 0 ||
        !std::isfinite(params.conversion_cost) || params.conversion_cost < 0) {
      throw std::invalid_argument(
          "finite nonnegative costs, positive block size, and 8/10-bit tier 0 required");
    }
    TraceLedger ledger;
    ledger.total_elements = static_cast<int64_t>(trace.size());
    if (trace.empty()) return ledger;
    const bool binary = arm == Arm::FB || arm == Arm::AB;
    const bool adaptive = arm == Arm::AB || arm == Arm::AT;
    auto width = [&](int t) {
      return binary ? (t == 0 ? params.binary_tier0_bits : TierLimits::binary_bits[t])
                    : TierLimits::ternary_trits[t];
    };
    const double ratio = binary ? 1.0 : params.symbol_cost_ratio;
    const size_t n = trace.size();
    ledger.tier_sequence.assign(n, 3);
    if (adaptive) {
      const size_t block = static_cast<size_t>(params.block_size);
      const size_t count = 1 + (n - 1) / block;
      const double inf = std::numeric_limits<double>::infinity();
      std::array<double, 4> prev = {0, 0, 0, 0};
      std::vector<std::array<int, 4>> parent(count);
      for (size_t j = 0; j < count; ++j) {
        const size_t begin = j * block;
        const size_t end = begin + std::min(block, n - begin);
        int required = 0;
        for (size_t i = begin; i < end; ++i) {
          int t = binary ? TierLimits::get_binary_tier(trace[i])
                         : TierLimits::get_ternary_tier(trace[i]);
          if (binary && params.binary_tier0_bits == 10 && trace[i] >= -512 && trace[i] <= 511)
            t = 0;
          required = std::max(required, t);
        }
        std::array<double, 4> next = {inf, inf, inf, inf};
        for (int t = required; t < 4; ++t) {
          const double local = (end - begin) * (width(t) * ratio + params.conversion_cost) +
                               params.metadata_bits_per_block;
          for (int u = 0; u < 4; ++u) {
            const double candidate =
                prev[u] + local + (j == 0 ? 0 : std::abs(t - u) * params.transition_cost_per_level);
            if (candidate < next[t]) {
              next[t] = candidate;
              parent[j][t] = u;
            }
          }
        }
        prev = next;
      }
      int t = static_cast<int>(std::min_element(prev.begin(), prev.end()) - prev.begin());
      if (!std::isfinite(prev[t])) throw std::overflow_error("cost exceeds floating point range");
      for (size_t j = count; j-- > 0;) {
        const size_t begin = j * block;
        const size_t end = begin + std::min(block, n - begin);
        std::fill(ledger.tier_sequence.begin() + begin, ledger.tier_sequence.begin() + end, t);
        t = parent[j][t];
      }
      ledger.selector_checks = static_cast<int64_t>(n);
      ledger.metadata_cost = count * params.metadata_bits_per_block;
    }
    for (size_t i = 0; i < n; ++i) {
      const int t = ledger.tier_sequence[i];
      ledger.symbol_slots += width(t);
      if (adaptive && i && t != ledger.tier_sequence[i - 1]) {
        ++ledger.transition_count;
        ledger.transition_level_delta += std::abs(t - ledger.tier_sequence[i - 1]);
      }
    }
    ledger.weighted_symbol_cost = ledger.symbol_slots * ratio;
    ledger.transition_cost = ledger.transition_level_delta * params.transition_cost_per_level;
    ledger.total_cost = ledger.weighted_symbol_cost + ledger.metadata_cost +
                        ledger.transition_cost + ledger.selector_checks * params.conversion_cost;
    if (!std::isfinite(ledger.total_cost))
      throw std::overflow_error("cost exceeds floating point range");
    return ledger;
  }
};

inline double total_crossover(const std::vector<int64_t>& trace, CostParameters params) {
  if (trace.empty()) return 0.0;  // no unique crossing: explicit empty-trace convention
  const double target = Evaluator::evaluate_trace(Arm::AB, trace, params).total_cost;
  double lo = 0.0, hi = 1.0;
  params.symbol_cost_ratio = hi;
  while (Evaluator::evaluate_trace(Arm::AT, trace, params).total_cost < target) {
    hi *= 2;
    params.symbol_cost_ratio = hi;
  }
  for (int i = 0; i < 80; ++i) {
    params.symbol_cost_ratio = lo + (hi - lo) / 2;
    if (Evaluator::evaluate_trace(Arm::AT, trace, params).total_cost < target)
      lo = params.symbol_cost_ratio;
    else
      hi = params.symbol_cost_ratio;
  }
  return lo + (hi - lo) / 2;
}

class WorkloadGenerator {
public:
  static std::vector<int64_t> constant_small(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      trace[i] = static_cast<int64_t>(i % 200) - 100;
    }
    return trace;
  }

  static std::vector<int64_t> constant_large(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      trace[i] = 1000000000000000000LL + static_cast<int64_t>(i);
    }
    return trace;
  }

  static std::vector<int64_t> phased(size_t len = 100) {
    std::vector<int64_t> trace(len);
    size_t phase_len = std::max(size_t{1}, len / 4);
    for (size_t i = 0; i < len; ++i) {
      size_t phase = i / phase_len;
      if (phase == 0)
        trace[i] = 50;
      else if (phase == 1)
        trace[i] = 10000;
      else if (phase == 2)
        trace[i] = 500000000LL;
      else
        trace[i] = 2000000000000000000LL;
    }
    return trace;
  }

  static std::vector<int64_t> alternating(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      if (i % 2 == 0)
        trace[i] = 42;
      else
        trace[i] = 3000000000000000000LL;
    }
    return trace;
  }

  static std::vector<int64_t> boundary_oscillation(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      if (i % 2 == 0)
        trace[i] = 127;
      else
        trace[i] = 128;
    }
    return trace;
  }

  static std::vector<int64_t> signed_extremes(size_t len = 100) {
    std::vector<int64_t> trace(len);
    int64_t targets[6] = {-9223372036854775807LL - 1LL, -2147483648LL, -128LL, 0LL, 127LL,
                          9223372036854775807LL};
    for (size_t i = 0; i < len; ++i) {
      trace[i] = targets[i % 6];
    }
    return trace;
  }

  static std::vector<int64_t> sparse_large(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      if (i % 50 == 0) {
        trace[i] = 5000000000000000000LL;
      } else {
        trace[i] = static_cast<int64_t>(i % 50);
      }
    }
    return trace;
  }

  static std::vector<int64_t> adversarial_transition(size_t len = 100) {
    std::vector<int64_t> trace(len);
    int64_t tiers_val[4] = {10, 10000, 1000000000LL, 5000000000000000000LL};
    for (size_t i = 0; i < len; ++i) {
      trace[i] = tiers_val[i % 4];
    }
    return trace;
  }
};

}  // namespace t81::research::h04_r02

#endif  // T81_EXPERIMENTS_H04_R02_ADAPTIVE_EXPERIMENT_HPP_
