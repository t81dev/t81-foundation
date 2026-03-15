#pragma once
// experimental/dpe/epoch_commit.hpp
//
// RFC-DPE-0003 §2–6: Epoch canonical commit engine.
//
// Implements:
//   §2  Canonical commit ordering (TaskId-ascending, then TVA-ascending)
//   §3  Delta conflict resolution (last-writer-in-canonical-order for
//       non-exclusive overlapping pages; exclusive conflicts abort the epoch)
//   §5  EpochHash = CanonHash81(epoch_id ∥ input_snapshot ∥ committed_deltas_hash)
//   §6  Epoch abort on TaskFault — canonical state is never mutated on abort

#include "task_graph.hpp"

#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace t81::dpe {

// ── EpochCommitStatus ────────────────────────────────────────────────────────

enum class EpochCommitStatus : uint8_t {
  Ok = 0,
  Aborted_TaskFault,          ///< a task's DeltaBuffer is in faulted state (§6.1)
  Aborted_ExclusiveConflict,  ///< exclusive region conflict detected at commit time (§3.2)
};

// ── EpochCommitResult ────────────────────────────────────────────────────────

struct EpochCommitResult {
  EpochCommitStatus status{EpochCommitStatus::Ok};

  /// The faulting TaskId when status == Aborted_TaskFault.
  std::optional<TaskId> faulting_task_id{};

  /// Canonical epoch verification hash (RFC-DPE-0003 §5).
  /// Valid only when status == Ok.
  t81::hash::CanonHash81 epoch_hash{};

  /// Final committed page map: TVA → page content.
  /// Populated only when status == Ok.
  /// Represents the post-commit canonical state: deltas applied in
  /// TaskId-ascending / TVA-ascending order (last-writer-in-canonical-order
  /// for non-exclusive overlapping pages).
  std::map<uint64_t, std::array<std::byte, kDpePageSize>> committed_pages{};

  [[nodiscard]] bool ok() const noexcept {
    return status == EpochCommitStatus::Ok;
  }
};

// ── commit_epoch ─────────────────────────────────────────────────────────────
//
// Executes the canonical commit algorithm for a completed epoch
// (RFC-DPE-0003 §2–5).
//
// Parameters:
//   epoch        — the EpochGraph that produced these buffers; supplies
//                  epoch_id and input_snapshot for EpochHash computation.
//   task_buffers — one (TaskId, DeltaBuffer*) pair per completed task.
//                  The TaskId must be the content-addressed identity used
//                  when the DeltaBuffer was constructed.
//
// Abort conditions (§6.1):
//   • Any DeltaBuffer::faulted() == true → Aborted_TaskFault
//
// On success:
//   • Deltas committed in TaskId-ascending / TVA-ascending order.
//   • last-writer-in-canonical-order wins for non-exclusive overlaps.
//   • EpochHash computed and returned in result.epoch_hash.
[[nodiscard]] EpochCommitResult commit_epoch(
    const EpochGraph& epoch,
    const std::vector<std::pair<TaskId, const DeltaBuffer*>>& task_buffers) noexcept;

}  // namespace t81::dpe
