#ifndef T81_EXPERIMENTS_H04_R02_ADAPTIVE_EXPERIMENT_HPP_
#define T81_EXPERIMENTS_H04_R02_ADAPTIVE_EXPERIMENT_HPP_

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace t81::research::h04_r02 {

enum class Arm {
  FB, // Fixed Binary (64-bit)
  AB, // Adaptive Binary (8, 16, 32, 64-bit)
  FT, // Fixed Balanced Ternary (41 trits)
  AT  // Adaptive Balanced Ternary (6, 11, 21, 41 trits)
};

inline std::string arm_to_string(Arm arm) {
  switch (arm) {
    case Arm::FB: return "FB";
    case Arm::AB: return "AB";
    case Arm::FT: return "FT";
    case Arm::AT: return "AT";
  }
  return "UNKNOWN";
}

struct TierLimits {
  static constexpr int kNumTiers = 4;
  static constexpr int binary_bits[4] = {8, 16, 32, 64};
  static constexpr int ternary_trits[4] = {6, 11, 21, 41};

  static constexpr int64_t binary_min[4] = {
    -128LL,
    -32768LL,
    -2147483648LL,
    -9223372036854775807LL - 1LL
  };

  static constexpr int64_t binary_max[4] = {
    127LL,
    32767LL,
    2147483647LL,
    9223372036854775807LL
  };

  static constexpr int64_t ternary_min[4] = {
    -364LL,
    -88573LL,
    -5230176601LL,
    -9223372036854775807LL - 1LL // Standard 64-bit domain boundary; 41 trits covers -18236498188585393201LL
  };

  static constexpr int64_t ternary_max[4] = {
    364LL,
    88573LL,
    5230176601LL,
    9223372036854775807LL // Standard 64-bit domain boundary; 41 trits covers 18236498188585393201LL
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
  double symbol_cost_ratio = 1.0; // C3/C2
  double metadata_bits_per_block = 2.0; // 2 bits per block mode descriptor
  int block_size = 1; // block granularity
  double transition_cost_per_level = 1.0; // penalty per level switch
  double conversion_cost = 0.5; // conversion / selection work per check
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
  static TraceLedger evaluate_trace(Arm arm,
                                     const std::vector<int64_t>& trace,
                                     const CostParameters& params) {
    TraceLedger ledger;
    ledger.total_elements = static_cast<int64_t>(trace.size());
    if (trace.empty()) return ledger;

    if (arm == Arm::FB || arm == Arm::FT) {
      ledger.tier_sequence.assign(trace.size(), 3);
      for (size_t i = 0; i < trace.size(); ++i) {
        if (arm == Arm::FB) {
          ledger.symbol_slots += TierLimits::binary_bits[3];
        } else {
          ledger.symbol_slots += TierLimits::ternary_trits[3];
        }
      }
      if (arm == Arm::FB) {
        ledger.weighted_symbol_cost = static_cast<double>(ledger.symbol_slots);
      } else {
        ledger.weighted_symbol_cost = static_cast<double>(ledger.symbol_slots) * params.symbol_cost_ratio;
      }
      ledger.total_cost = ledger.weighted_symbol_cost;
      return ledger;
    }

    // Dynamic Programming for adaptive arms (AB and AT)
    size_t n = trace.size();
    struct DpNode {
      double cost = 1e18;
      std::vector<int> path;
    };

    // dp[i][t] = min cost for prefix i ending at tier t
    std::vector<std::vector<DpNode>> dp(n, std::vector<DpNode>(TierLimits::kNumTiers));

    std::vector<int> req_tiers(n);
    for (size_t i = 0; i < n; ++i) {
      if (arm == Arm::AB) {
        req_tiers[i] = TierLimits::get_binary_tier(trace[i]);
      } else {
        req_tiers[i] = TierLimits::get_ternary_tier(trace[i]);
      }
    }

    double meta_per_block = params.metadata_bits_per_block;

    // Step 0
    for (int t = req_tiers[0]; t < TierLimits::kNumTiers; ++t) {
      double sym_cost = (arm == Arm::AB) ? TierLimits::binary_bits[t] : TierLimits::ternary_trits[t] * params.symbol_cost_ratio;
      double cost = sym_cost + meta_per_block + params.conversion_cost;
      dp[0][t] = {cost, {t}};
    }

    // Steps 1..n-1
    for (size_t i = 1; i < n; ++i) {
      double meta = (i % static_cast<size_t>(params.block_size) == 0) ? meta_per_block : 0.0;
      for (int t = req_tiers[i]; t < TierLimits::kNumTiers; ++t) {
        double sym_cost = (arm == Arm::AB) ? TierLimits::binary_bits[t] : TierLimits::ternary_trits[t] * params.symbol_cost_ratio;
        double elem_cost = sym_cost + meta + params.conversion_cost;

        double best_prev_cost = 1e18;
        int best_prev_t = -1;

        for (int prev_t = req_tiers[i-1]; prev_t < TierLimits::kNumTiers; ++prev_t) {
          double trans_cost = std::abs(t - prev_t) * params.transition_cost_per_level;
          double tot = dp[i-1][prev_t].cost + elem_cost + trans_cost;
          if (tot < best_prev_cost) {
            best_prev_cost = tot;
            best_prev_t = prev_t;
          }
        }

        dp[i][t].cost = best_prev_cost;
        dp[i][t].path = dp[i-1][best_prev_t].path;
        dp[i][t].path.push_back(t);
      }
    }

    // Find best final tier
    double best_final_cost = 1e18;
    int best_final_t = -1;
    for (int t = req_tiers[n-1]; t < TierLimits::kNumTiers; ++t) {
      if (dp[n-1][t].cost < best_final_cost) {
        best_final_cost = dp[n-1][t].cost;
        best_final_t = t;
      }
    }

    ledger.tier_sequence = dp[n-1][best_final_t].path;
    ledger.selector_checks = static_cast<int64_t>(n);

    int prev_tier = -1;
    for (size_t i = 0; i < n; ++i) {
      int t = ledger.tier_sequence[i];
      if (arm == Arm::AB) {
        ledger.symbol_slots += TierLimits::binary_bits[t];
      } else {
        ledger.symbol_slots += TierLimits::ternary_trits[t];
      }

      if (i % static_cast<size_t>(params.block_size) == 0) {
        ledger.metadata_cost += meta_per_block;
      }

      if (prev_tier != -1 && t != prev_tier) {
        int delta = std::abs(t - prev_tier);
        ledger.transition_count += 1;
        ledger.transition_level_delta += delta;
        ledger.transition_cost += delta * params.transition_cost_per_level;
      }
      prev_tier = t;
    }

    if (arm == Arm::AB) {
      ledger.weighted_symbol_cost = static_cast<double>(ledger.symbol_slots);
    } else {
      ledger.weighted_symbol_cost = static_cast<double>(ledger.symbol_slots) * params.symbol_cost_ratio;
    }

    ledger.total_cost = ledger.weighted_symbol_cost +
                        ledger.metadata_cost +
                        ledger.transition_cost +
                        (ledger.selector_checks * params.conversion_cost);

    return ledger;
  }
};

class WorkloadGenerator {
 public:
  static std::vector<int64_t> constant_small(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      trace[i] = static_cast<int64_t>((i % 200) - 100);
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
    size_t phase_len = len / 4;
    for (size_t i = 0; i < len; ++i) {
      size_t phase = i / phase_len;
      if (phase == 0) trace[i] = 50;
      else if (phase == 1) trace[i] = 10000;
      else if (phase == 2) trace[i] = 500000000LL;
      else trace[i] = 2000000000000000000LL;
    }
    return trace;
  }

  static std::vector<int64_t> alternating(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      if (i % 2 == 0) trace[i] = 42;
      else trace[i] = 3000000000000000000LL;
    }
    return trace;
  }

  static std::vector<int64_t> boundary_oscillation(size_t len = 100) {
    std::vector<int64_t> trace(len);
    for (size_t i = 0; i < len; ++i) {
      if (i % 2 == 0) trace[i] = 127;
      else trace[i] = 128;
    }
    return trace;
  }

  static std::vector<int64_t> signed_extremes(size_t len = 100) {
    std::vector<int64_t> trace(len);
    int64_t targets[6] = {
      -9223372036854775807LL - 1LL,
      -2147483648LL,
      -128LL,
      0LL,
      127LL,
      9223372036854775807LL
    };
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

} // namespace t81::research::h04_r02

#endif // T81_EXPERIMENTS_H04_R02_ADAPTIVE_EXPERIMENT_HPP_
