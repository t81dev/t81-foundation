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
    -9223372036854775807LL - 1LL
  };

  static constexpr int64_t ternary_max[4] = {
    364LL,
    88573LL,
    5230176601LL,
    9223372036854775807LL
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

    ledger.tier_sequence.reserve(trace.size());
    int prev_tier = -1;

    for (size_t i = 0; i < trace.size(); ++i) {
      int64_t val = trace[i];
      int current_tier = 3;

      switch (arm) {
        case Arm::FB:
          current_tier = 3;
          ledger.symbol_slots += TierLimits::binary_bits[3];
          break;
        case Arm::AB:
          current_tier = TierLimits::get_binary_tier(val);
          ledger.symbol_slots += TierLimits::binary_bits[current_tier];
          ledger.selector_checks += 1;
          break;
        case Arm::FT:
          current_tier = 3;
          ledger.symbol_slots += TierLimits::ternary_trits[3];
          break;
        case Arm::AT:
          current_tier = TierLimits::get_ternary_tier(val);
          ledger.symbol_slots += TierLimits::ternary_trits[current_tier];
          ledger.selector_checks += 1;
          break;
      }

      ledger.tier_sequence.push_back(current_tier);

      // Metadata cost (charged per block)
      if (arm == Arm::AB || arm == Arm::AT) {
        if (i % static_cast<size_t>(params.block_size) == 0) {
          double meta = params.metadata_bits_per_block;
          if (arm == Arm::AT) {
            meta *= params.symbol_cost_ratio;
          }
          ledger.metadata_cost += meta;
        }
      }

      // Transition tracking
      if (prev_tier != -1 && current_tier != prev_tier) {
        int delta = std::abs(current_tier - prev_tier);
        ledger.transition_count += 1;
        ledger.transition_level_delta += delta;
        if (arm == Arm::AB || arm == Arm::AT) {
          ledger.transition_cost += delta * params.transition_cost_per_level;
        }
      }

      prev_tier = current_tier;
    }

    // Weighted symbol cost
    if (arm == Arm::FB || arm == Arm::AB) {
      ledger.weighted_symbol_cost = static_cast<double>(ledger.symbol_slots);
    } else {
      ledger.weighted_symbol_cost = static_cast<double>(ledger.symbol_slots) * params.symbol_cost_ratio;
    }

    double conversion_work = (arm == Arm::AB || arm == Arm::AT) ? (ledger.selector_checks * params.conversion_cost) : 0.0;

    ledger.total_cost = ledger.weighted_symbol_cost +
                        ledger.metadata_cost +
                        ledger.transition_cost +
                        conversion_work;

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
