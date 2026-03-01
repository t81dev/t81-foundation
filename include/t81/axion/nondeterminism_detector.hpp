#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "t81/vm/state.hpp"

namespace t81::axion {

/**
 * @struct DivergenceReport
 * @brief Result of a cross-run trace comparison.
 */
struct DivergenceReport {
  bool diverged{false};
  std::size_t event_index{0};
  std::string reason;  ///< canonical "Nondeterministic divergence at event N" or empty
};

/**
 * @class DeterminismDetector
 * @brief Active nondeterminism detector for the Axion audit trace (AX-M5).
 *
 * Usage (CI harness pattern):
 *   DeterminismDetector det;
 *   vm->set_determinism_detector(&det);
 *   vm->run_to_halt();   // run 1 — det records baseline
 *   vm->load_program(p); vm->run_to_halt();   // run 2 — det detects any divergence
 *
 * The detector stores a rolling hash fingerprint for each event position.
 * A divergence is reported when the hash at any position differs between
 * two successive runs, or when the event counts differ.
 */
class DeterminismDetector {
public:
  /// Feed the axion log from a completed run.
  /// Promotes the current hash chain to "previous" and recomputes from @p log.
  void record_run(const std::vector<t81::vm::AxionEvent>& log);

  /// Compare the most recently recorded run against the one before it.
  /// Returns diverged=false when fewer than two runs have been recorded.
  [[nodiscard]] DivergenceReport check_against_previous() const;

  [[nodiscard]] bool has_previous_run() const { return !prev_hashes_.empty(); }

private:
  /// Combine one event's identity fields into a running seed (avalanche mix).
  static std::size_t hash_event(const t81::vm::AxionEvent& ev, std::size_t seed);

  std::vector<std::size_t> prev_hashes_;  ///< rolling hash chain from run N-1
  std::vector<std::size_t> curr_hashes_;  ///< rolling hash chain from run N
};

}  // namespace t81::axion
