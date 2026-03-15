// experimental/dpe/epoch_commit.cpp
//
// RFC-DPE-0003 §2–6: Epoch canonical commit implementation.

#include "epoch_commit.hpp"

#include <algorithm>
#include <cstring>

namespace t81::dpe {

// ── Internal serialisation helpers ───────────────────────────────────────────

namespace {

void append_u64_le(std::vector<std::byte>& out, uint64_t v) {
  for (int i = 0; i < 8; ++i)
    out.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xFFu));
}

void append_hash(std::vector<std::byte>& out, const t81::hash::CanonHash81& h) {
  for (auto b : h.bytes)
    out.push_back(static_cast<std::byte>(b));
}

// delta_hash(T): CanonHash81 of (tva ∥ value) for each DeltaRecord in TVA order.
// Empty buffer: hash of the zero-length span.
t81::hash::CanonHash81 compute_delta_hash(const DeltaBuffer& buf) {
  std::vector<const DeltaRecord*> sorted;
  sorted.reserve(buf.records().size());
  for (const auto& r : buf.records()) sorted.push_back(&r);
  std::sort(sorted.begin(), sorted.end(),
            [](const DeltaRecord* a, const DeltaRecord* b) {
              return a->tva < b->tva;
            });

  std::vector<std::byte> data;
  data.reserve(sorted.size() * (8 + kDpePageSize));
  for (const auto* r : sorted) {
    append_u64_le(data, r->tva);
    for (auto b : r->value) data.push_back(b);
  }
  return t81::hash::hash_bytes(std::span<const std::byte>(data));
}

// committed_deltas_hash: CanonHash81 of (TaskId ∥ delta_hash(T)) for each
// task in TaskId-ascending order.
t81::hash::CanonHash81 compute_committed_deltas_hash(
    const std::vector<std::pair<TaskId, const DeltaBuffer*>>& sorted_tasks) {
  std::vector<std::byte> data;
  for (const auto& [tid, buf] : sorted_tasks) {
    append_hash(data, tid.hash);
    append_hash(data, compute_delta_hash(*buf));
  }
  return t81::hash::hash_bytes(std::span<const std::byte>(data));
}

// EpochHash = CanonHash81(epoch_id ∥ input_snapshot ∥ committed_deltas_hash)
t81::hash::CanonHash81 compute_epoch_hash(
    uint64_t epoch_id,
    const t81::canonfs::CanonRef& input_snapshot,
    const t81::hash::CanonHash81& committed_deltas_hash) {
  std::vector<std::byte> data;
  data.reserve(8 + 32 + 32);
  append_u64_le(data, epoch_id);
  append_hash(data, input_snapshot.hash.h);
  append_hash(data, committed_deltas_hash);
  return t81::hash::hash_bytes(std::span<const std::byte>(data));
}

}  // namespace

// ── commit_epoch ─────────────────────────────────────────────────────────────

EpochCommitResult commit_epoch(
    const EpochGraph& epoch,
    const std::vector<std::pair<TaskId, const DeltaBuffer*>>& task_buffers) noexcept {

  EpochCommitResult result;

  // ── §6.1 Abort if any task faulted ───────────────────────────────────────
  for (const auto& [tid, buf] : task_buffers) {
    if (buf->faulted()) {
      result.status         = EpochCommitStatus::Aborted_TaskFault;
      result.faulting_task_id = tid;
      return result;
    }
  }

  // ── §2.1 Sort tasks in canonical TaskId order (ascending) ────────────────
  // TaskId::operator< uses lexicographic order on the 32-byte hash array,
  // which is deterministic, timing-independent, and machine-independent.
  auto sorted_tasks = task_buffers;
  std::sort(sorted_tasks.begin(), sorted_tasks.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  // ── §2.2 Apply deltas to staging area ────────────────────────────────────
  // Iterate tasks in ascending TaskId order; within each task iterate
  // DeltaRecords in ascending TVA order.  Later tasks overwrite earlier
  // tasks for the same page — this implements §3.1 last-writer-in-canonical-
  // order conflict resolution for non-exclusive regions.
  for (const auto& [tid, buf] : sorted_tasks) {
    std::vector<const DeltaRecord*> sorted_recs;
    sorted_recs.reserve(buf->records().size());
    for (const auto& r : buf->records()) sorted_recs.push_back(&r);
    std::sort(sorted_recs.begin(), sorted_recs.end(),
              [](const DeltaRecord* a, const DeltaRecord* b) {
                return a->tva < b->tva;
              });

    for (const auto* rec : sorted_recs) {
      result.committed_pages[rec->tva] = rec->value;
    }
  }

  // ── §5.1 EpochHash computation ────────────────────────────────────────────
  const auto deltas_hash = compute_committed_deltas_hash(sorted_tasks);
  result.epoch_hash = compute_epoch_hash(
      epoch.epoch_id, epoch.input_snapshot, deltas_hash);

  result.status = EpochCommitStatus::Ok;
  return result;
}

}  // namespace t81::dpe
