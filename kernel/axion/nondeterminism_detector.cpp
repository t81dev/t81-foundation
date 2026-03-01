#include "t81/axion/nondeterminism_detector.hpp"

#include <algorithm>
#include <functional>

namespace t81::axion {

// static
std::size_t DeterminismDetector::hash_event(const t81::vm::AxionEvent& ev, std::size_t seed) {
  // Avalanche mix: each contributor flips roughly half the seed bits.
  auto mix = [&](std::size_t v) {
    seed ^= v + 0x9e3779b9u + (seed << 6) + (seed >> 2);
  };
  mix(std::hash<int>{}(static_cast<int>(ev.opcode)));
  mix(std::hash<std::int32_t>{}(ev.tag));
  mix(std::hash<int>{}(static_cast<int>(ev.verdict.kind)));
  mix(std::hash<std::string>{}(ev.verdict.reason));
  return seed;
}

void DeterminismDetector::record_run(const std::vector<t81::vm::AxionEvent>& log) {
  prev_hashes_ = std::move(curr_hashes_);
  curr_hashes_.clear();
  curr_hashes_.reserve(log.size());
  std::size_t rolling = 0;
  for (const auto& ev : log) {
    rolling = hash_event(ev, rolling);
    curr_hashes_.push_back(rolling);
  }
}

DivergenceReport DeterminismDetector::check_against_previous() const {
  if (prev_hashes_.empty()) {
    return DivergenceReport{false, 0, {}};
  }

  const std::size_t min_len = std::min(curr_hashes_.size(), prev_hashes_.size());
  for (std::size_t i = 0; i < min_len; ++i) {
    if (curr_hashes_[i] != prev_hashes_[i]) {
      return DivergenceReport{
          true, i, "Nondeterministic divergence at event " + std::to_string(i)};
    }
  }

  if (curr_hashes_.size() != prev_hashes_.size()) {
    const std::size_t idx = min_len;
    return DivergenceReport{
        true, idx,
        "Nondeterministic divergence at event " + std::to_string(idx) +
            ": event count mismatch (prev=" + std::to_string(prev_hashes_.size()) +
            " curr=" + std::to_string(curr_hashes_.size()) + ")"};
  }

  return DivergenceReport{false, 0, {}};
}

}  // namespace t81::axion
