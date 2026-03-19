#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "t81/canonfs/canon_types.hpp"

namespace t81::hanoi {
using SnapshotRef = t81::canonfs::CanonRef;
using Pid = std::uint64_t;

struct RegionHandle {
  std::uint64_t id{0};
};

// ─── Command Surface Types (RFC-0000 §7) ─────────────────────────────────────

struct KernelStatus {
  bool booted{false};
  std::uint64_t active_snapshots{0};
  std::uint64_t total_processes{0};
  std::string current_root_hash;
  std::string last_axion_verdict;
};

struct OptimizationParams {
  std::string target{"performance"};
  std::vector<std::string> constraints;
};

struct OptimizationResult {
  bool success{false};
  std::string applied_optimization;
  std::string performance_delta;
};

struct SimulationParams {
  std::vector<std::string> operations;
  std::uint64_t max_steps{1000};
};

struct SimulationResult {
  bool completed{false};
  std::uint64_t steps_executed{0};
  std::string final_state_hash;
  std::vector<std::string> trace;
};

}  // namespace t81::hanoi
