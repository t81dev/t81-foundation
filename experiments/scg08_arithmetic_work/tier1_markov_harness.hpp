#pragma once

#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#include "tier2_work_harness.hpp"

namespace scg08 {

// ============================================================================
// Tier 1: Primary Mathematical Carry-Propagation Evaluation Harness
// Instruments scalar digit addition across independent uniform digit pairs.
// Decoupled event counters for 3-state balanced ternary and 2-state binary.
// Counts each sequence trial against the cumulative budget.
// ============================================================================

struct Tier1Counters {
  uint64_t total_digits{0};
  uint64_t quiescent_to_active{0};   // c_in == 0 && |c_out| == 1
  uint64_t digits_cin_zero{0};       // c_in == 0
  uint64_t active_cout{0};           // |c_out| == 1
  uint64_t active_continuations{0};  // |c_in| == 1 && c_out == c_in
  uint64_t digits_cin_active{0};     // |c_in| == 1
  uint64_t carry_runs{0};
  uint64_t total_run_length{0};

  // Empirical frequencies
  [[nodiscard]] double f_gen_given_0() const noexcept {
    return digits_cin_zero > 0 ? static_cast<double>(quiescent_to_active) / digits_cin_zero : 0.0;
  }

  [[nodiscard]] double f_trans() const noexcept {
    return total_digits > 0 ? static_cast<double>(quiescent_to_active) / total_digits : 0.0;
  }

  [[nodiscard]] double f_occ() const noexcept {
    return total_digits > 0 ? static_cast<double>(active_cout) / total_digits : 0.0;
  }

  [[nodiscard]] double f_cont_given_act() const noexcept {
    return digits_cin_active > 0 ? static_cast<double>(active_continuations) / digits_cin_active
                                 : 0.0;
  }

  [[nodiscard]] double mean_run_length() const noexcept {
    return carry_runs > 0 ? static_cast<double>(total_run_length) / carry_runs : 0.0;
  }
};

class Tier1MarkovHarness {
public:
  static constexpr uint64_t DEFAULT_SEED = 0x5C608001ULL;

  static std::pair<int8_t, int8_t> ternary_step(int8_t a, int8_t b, int8_t c_in) noexcept {
    int sum = static_cast<int>(a) + static_cast<int>(b) + static_cast<int>(c_in);
    int8_t c_out = 0;
    int8_t r = 0;
    if (sum > 1) {
      c_out = 1;
      r = static_cast<int8_t>(sum - 3);
    } else if (sum < -1) {
      c_out = -1;
      r = static_cast<int8_t>(sum + 3);
    } else {
      c_out = 0;
      r = static_cast<int8_t>(sum);
    }
    return {r, c_out};
  }

  static std::pair<uint8_t, uint8_t> binary_step(uint8_t a, uint8_t b, uint8_t c_in) noexcept {
    uint8_t sum = a + b + c_in;
    uint8_t c_out = sum >> 1;
    uint8_t r = sum & 1;
    return {r, c_out};
  }

  static Tier1Counters simulate_ternary(size_t num_sequences, size_t sequence_length,
                                        uint64_t seed = DEFAULT_SEED) {
    Tier1Counters counters;
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> dist(-1, 1);

    for (size_t s = 0; s < num_sequences; ++s) {
      TrialBudgetEnforcer::instance().record_trial();
      int8_t c_in = 0;  // Initial carry c_0 = 0
      uint64_t current_run = 0;

      for (size_t k = 0; k < sequence_length; ++k) {
        int8_t a = static_cast<int8_t>(dist(rng));
        int8_t b = static_cast<int8_t>(dist(rng));

        counters.total_digits++;
        if (c_in == 0) {
          counters.digits_cin_zero++;
        } else {
          counters.digits_cin_active++;
        }

        auto [r, c_out] = ternary_step(a, b, c_in);

        if (c_in == 0 && (c_out != 0)) {
          counters.quiescent_to_active++;
        }
        if (c_out != 0) {
          counters.active_cout++;
        }
        if (c_in != 0 && c_out == c_in) {
          counters.active_continuations++;
        }

        if (c_out != 0) {
          current_run++;
        } else {
          if (current_run > 0) {
            counters.carry_runs++;
            counters.total_run_length += current_run;
            current_run = 0;
          }
        }

        c_in = c_out;
      }

      if (current_run > 0) {
        counters.carry_runs++;
        counters.total_run_length += current_run;
      }
    }

    return counters;
  }

  static Tier1Counters simulate_binary(size_t num_sequences, size_t sequence_length,
                                       uint64_t seed = DEFAULT_SEED) {
    Tier1Counters counters;
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> dist(0, 1);

    for (size_t s = 0; s < num_sequences; ++s) {
      TrialBudgetEnforcer::instance().record_trial();
      uint8_t c_in = 0;  // Initial carry c_0 = 0
      uint64_t current_run = 0;

      for (size_t k = 0; k < sequence_length; ++k) {
        uint8_t a = static_cast<uint8_t>(dist(rng));
        uint8_t b = static_cast<uint8_t>(dist(rng));

        counters.total_digits++;
        if (c_in == 0) {
          counters.digits_cin_zero++;
        } else {
          counters.digits_cin_active++;
        }

        auto [r, c_out] = binary_step(a, b, c_in);

        if (c_in == 0 && c_out == 1) {
          counters.quiescent_to_active++;
        }
        if (c_out == 1) {
          counters.active_cout++;
        }
        if (c_in == 1 && c_out == 1) {
          counters.active_continuations++;
        }

        if (c_out == 1) {
          current_run++;
        } else {
          if (current_run > 0) {
            counters.carry_runs++;
            counters.total_run_length += current_run;
            current_run = 0;
          }
        }

        c_in = c_out;
      }

      if (current_run > 0) {
        counters.carry_runs++;
        counters.total_run_length += current_run;
      }
    }

    return counters;
  }
};

}  // namespace scg08
