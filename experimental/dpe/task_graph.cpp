// experimental/dpe/task_graph.cpp
//
// RFC-DPE-0002: TISC Task Graph Primitive layer — implementation.

#include "task_graph.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace t81::dpe {

// ── Serialisation helpers ─────────────────────────────────────────────────────

namespace {

void append_u8(std::vector<std::byte>& out, uint8_t v) {
  out.push_back(static_cast<std::byte>(v));
}

void append_u32_le(std::vector<std::byte>& out, uint32_t v) {
  out.push_back(static_cast<std::byte>((v >>  0) & 0xFF));
  out.push_back(static_cast<std::byte>((v >>  8) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
}

void append_u64_le(std::vector<std::byte>& out, uint64_t v) {
  out.push_back(static_cast<std::byte>((v >>  0) & 0xFF));
  out.push_back(static_cast<std::byte>((v >>  8) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 16) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 24) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 32) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 40) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 48) & 0xFF));
  out.push_back(static_cast<std::byte>((v >> 56) & 0xFF));
}

void append_hash(std::vector<std::byte>& out, const t81::hash::CanonHash81& h) {
  for (auto b : h.bytes) {
    out.push_back(static_cast<std::byte>(b));
  }
}

void append_canon_ref(std::vector<std::byte>& out, const t81::canonfs::CanonRef& ref) {
  append_hash(out, ref.hash.h);
}

void append_task_id(std::vector<std::byte>& out, const TaskId& tid) {
  append_hash(out, tid.hash);
}

void append_output_region(std::vector<std::byte>& out, const OutputRegion& r) {
  append_u64_le(out, r.base_tva);
  append_u32_le(out, r.page_count);
  append_u8(out, r.exclusive ? 1u : 0u);
}

}  // namespace

// ── serialise_task_descriptor ────────────────────────────────────────────────

std::vector<std::byte> serialise_task_descriptor(
    const TaskDescriptor& task) noexcept {
  std::vector<std::byte> out;
  out.reserve(256);

  append_u8(out, task.format_version);
  append_u64_le(out, task.epoch_id);
  append_u64_le(out, task.task_seq);
  append_canon_ref(out, task.program_ref);
  append_u64_le(out, task.entry_tva);

  // input_refs[] — length-prefixed
  append_u32_le(out, static_cast<uint32_t>(task.input_refs.size()));
  for (const auto& ref : task.input_refs) {
    append_canon_ref(out, ref);
  }

  // input_tvas[] — length-prefixed
  append_u32_le(out, static_cast<uint32_t>(task.input_tvas.size()));
  for (uint64_t tva : task.input_tvas) {
    append_u64_le(out, tva);
  }

  // output_regions[] — length-prefixed
  append_u32_le(out, static_cast<uint32_t>(task.output_regions.size()));
  for (const auto& region : task.output_regions) {
    append_output_region(out, region);
  }

  // dep_task_ids[] — length-prefixed
  append_u32_le(out, static_cast<uint32_t>(task.dep_task_ids.size()));
  for (const auto& dep : task.dep_task_ids) {
    append_task_id(out, dep);
  }

  append_u32_le(out, task.scratch_pages);

  return out;
}

// ── compute_task_id ───────────────────────────────────────────────────────────

TaskId compute_task_id(const TaskDescriptor& task) noexcept {
  const auto bytes = serialise_task_descriptor(task);
  return TaskId{t81::hash::hash_bytes(std::span<const std::byte>(bytes))};
}

// ── DeltaBuffer ───────────────────────────────────────────────────────────────

DeltaBuffer::DeltaBuffer(const TaskDescriptor& task, TaskId tid) noexcept
    : task_(task), tid_(std::move(tid)) {}

bool DeltaBuffer::tva_in_output_region(uint64_t tva) const noexcept {
  for (const auto& region : task_.output_regions) {
    const uint64_t region_end = region.base_tva +
        static_cast<uint64_t>(region.page_count) * kDpePageSize;
    if (tva >= region.base_tva && tva < region_end) {
      return true;
    }
  }
  return false;
}

bool DeltaBuffer::tva_in_scratch(uint64_t tva) const noexcept {
  // Scratch pages are allocated immediately after output_regions in the
  // conceptual address space.  For hosted tests where no real MMU maps them
  // we allow any positive scratch_pages count to accept an otherwise-unmapped
  // write — the real VM will enforce tighter bounds via the page table.
  return task_.scratch_pages > 0 && !tva_in_output_region(tva);
}

DeltaWriteResult DeltaBuffer::write(
    uint64_t tva,
    std::span<const std::byte> page_data) noexcept {
  if (faulted_) {
    return {fault_kind_, false};
  }

  if (!tva_in_output_region(tva)) {
    // Writes outside declared output regions (and scratch) are TaskFaults.
    // Scratch is a best-effort fallback: if scratch_pages == 0 every
    // non-region write is an OutOfRegionWrite.
    if (task_.scratch_pages == 0) {
      faulted_     = true;
      fault_kind_  = TaskFaultKind::OutOfRegionWrite;
      return {fault_kind_, false};
    }
    // scratch_pages > 0: silently accept (no delta record; discarded after task)
    return {TaskFaultKind::None, true};
  }

  // Accept into delta buffer — page granularity.
  DeltaRecord rec;
  rec.task_id = tid_;
  rec.tva     = tva & ~static_cast<uint64_t>(kDpePageSize - 1);  // page-align
  const std::size_t copy_n = std::min(page_data.size(), kDpePageSize);
  std::memcpy(rec.value.data(), page_data.data(), copy_n);
  records_.push_back(std::move(rec));

  return {TaskFaultKind::None, true};
}

// ── accept_epoch ─────────────────────────────────────────────────────────────

namespace {

// Compute a "program identity" TaskId for cycle detection: the TaskId of the
// task with its dep_task_ids cleared.  This makes the identity stable
// regardless of how deps are set, which is required for dep_task_ids to
// resolve to same-epoch tasks in the adjacency graph.
//
// Workflow invariant: callers build tasks without deps, record their program
// IDs, then set dep_task_ids using those program IDs.  accept_epoch strips
// deps before hashing so that the index matches.
TaskId program_identity(const TaskDescriptor& task) noexcept {
  TaskDescriptor stripped = task;
  stripped.dep_task_ids.clear();
  return compute_task_id(stripped);
}

// Assign a stable index 0..N-1 to each task's program identity.
std::unordered_map<std::string, std::size_t> build_task_index(
    const EpochGraph& epoch) {
  std::unordered_map<std::string, std::size_t> idx;
  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    idx.emplace(program_identity(epoch.tasks[i]).hash.to_string(), i);
  }
  return idx;
}

// Kahn's algorithm — returns false iff the graph contains a cycle.
// The index maps program-identity TaskId strings to task indices.
// dep_task_ids are looked up against the same program-identity index.
bool is_dag(const std::unordered_map<std::string, std::size_t>& idx,
            const EpochGraph& epoch) {
  const std::size_t N = epoch.tasks.size();
  std::vector<std::size_t> in_degree(N, 0);
  std::vector<std::vector<std::size_t>> adj(N);

  for (std::size_t i = 0; i < N; ++i) {
    for (const auto& dep : epoch.tasks[i].dep_task_ids) {
      auto it = idx.find(dep.hash.to_string());
      if (it == idx.end()) {
        // Dependency references a task not in this epoch — treat as external
        // (no-op for cycle detection within the epoch).
        continue;
      }
      const std::size_t dep_idx = it->second;
      // dep_idx → i (dep must complete before i runs)
      adj[dep_idx].push_back(i);
      ++in_degree[i];
    }
  }

  std::vector<std::size_t> queue;
  queue.reserve(N);
  for (std::size_t i = 0; i < N; ++i) {
    if (in_degree[i] == 0) queue.push_back(i);
  }

  std::size_t visited = 0;
  while (!queue.empty()) {
    const std::size_t node = queue.back();
    queue.pop_back();
    ++visited;
    for (std::size_t nbr : adj[node]) {
      if (--in_degree[nbr] == 0) {
        queue.push_back(nbr);
      }
    }
  }

  return visited == N;  // true iff no cycle
}

// Returns true iff two OutputRegions overlap (both page-granular intervals).
bool regions_overlap(const OutputRegion& a, const OutputRegion& b) noexcept {
  const uint64_t a_end = a.base_tva + static_cast<uint64_t>(a.page_count) * kDpePageSize;
  const uint64_t b_end = b.base_tva + static_cast<uint64_t>(b.page_count) * kDpePageSize;
  return a.base_tva < b_end && b.base_tva < a_end;
}

}  // namespace

EpochAcceptResult accept_epoch(const EpochGraph& epoch) noexcept {
  // ── Empty epoch ───────────────────────────────────────────────────────────
  if (epoch.tasks.empty()) {
    return {EpochAcceptStatus::EmptyEpoch, std::nullopt};
  }

  // ── Check for duplicate task_seq ─────────────────────────────────────────
  std::unordered_set<uint64_t> seen_seq;
  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    if (!seen_seq.insert(epoch.tasks[i].task_seq).second) {
      return {EpochAcceptStatus::DuplicateTaskSeq, i};
    }
  }

  // ── Build program-identity index and check for cycles (§5.4) ────────────
  // Program identity = TaskId with dep_task_ids stripped.  This lets dep
  // references (which use program IDs of dependency tasks) resolve to their
  // targets in the index regardless of the dependee's own deps.
  const auto idx = build_task_index(epoch);
  if (!is_dag(idx, epoch)) {
    return {EpochAcceptStatus::MalformedEpochGraph, std::nullopt};
  }

  // ── Exclusive output region conflicts (§5.5) ─────────────────────────────
  // Collect all exclusive regions across all tasks; check pairwise for overlap.
  struct TaggedRegion {
    OutputRegion region;
    std::size_t  task_index;
  };
  std::vector<TaggedRegion> exclusive_regions;
  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    for (const auto& r : epoch.tasks[i].output_regions) {
      if (r.exclusive) {
        exclusive_regions.push_back({r, i});
      }
    }
  }
  for (std::size_t i = 0; i < exclusive_regions.size(); ++i) {
    for (std::size_t j = i + 1; j < exclusive_regions.size(); ++j) {
      if (regions_overlap(exclusive_regions[i].region, exclusive_regions[j].region)) {
        return {EpochAcceptStatus::ExclusiveRegionConflict,
                exclusive_regions[j].task_index};
      }
    }
  }

  return {EpochAcceptStatus::Ok, std::nullopt};
}

}  // namespace t81::dpe
