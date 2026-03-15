#pragma once
// experimental/dpe/task_runner.hpp
//
// RFC-DPE-0002 §5 / [DPE-02-05]: DpeTaskRunner — run a single TISC task
// through the T81VM and collect DeltaRecords for declared output regions.
//
// Acceptance criterion [DPE-02-05]:
//   A single-task epoch with no output regions produces identical register
//   results to running the same TISC program directly through the VM.
//
// Design (hosted simulation):
//   base_tva in OutputRegion is interpreted as a flat word index into
//   State::memory (std::vector<int64_t>).  One "page" = kDpePageSize bytes
//   = kDpePageSize/8 words.  This is sufficient for [DPE-02-05] and the
//   hosted test layer; a real MMU translation layer is deferred to Slice 15+.

#include "task_graph.hpp"
#include "t81/isa/program.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace t81::dpe {

// ── DpeTaskResult ─────────────────────────────────────────────────────────────

struct DpeTaskResult {
  bool halted{false};

  /// Final 243-register file after the task completes (or after the VM traps).
  std::array<std::int64_t, 243> final_registers{};

  /// Delta records accumulated for declared output_regions.
  /// Empty when the task declares no output_regions.
  std::vector<DeltaRecord> delta_records{};
};

// ── DpeTaskRunner ─────────────────────────────────────────────────────────────

class DpeTaskRunner {
public:
  /// Execute `program` in a fresh VM instance, applying the constraints from
  /// `task.output_regions` as a post-processing diff step.
  ///
  /// Returns the final register file and any DeltaRecords produced by stores
  /// to declared output regions.  When output_regions is empty (the [DPE-02-05]
  /// case) delta_records will always be empty.
  [[nodiscard]] DpeTaskResult run_direct(
      const TaskDescriptor& task,
      const t81::tisc::Program& program) noexcept;
};

}  // namespace t81::dpe
