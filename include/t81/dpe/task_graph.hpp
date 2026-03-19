#pragma once
// include/t81/dpe/task_graph.hpp
//
// RFC-DPE-0002 §3–6: TISC Task Graph Primitive layer.
//
// Defines the data model for deterministic parallel execution:
//   TaskId         — content-addressed 32-byte identity for a TaskDescriptor
//   OutputRegion   — a TVA range buffered for delta writes
//   TaskDescriptor — fully specifies one unit of parallel computation
//   EpochGraph     — groups a set of TaskDescriptors into a single epoch
//   DeltaRecord    — one intercepted store to an output region
//   DeltaBuffer    — per-task accumulator of DeltaRecords
//   EpochAcceptor  — validates an EpochGraph before execution begins
//
// None of this requires new TISC opcodes.  Tasks are normal TISC programs.
// The delta-buffering is a VM execution mode activated by the task descriptor.

#include "t81/canonfs/canon_types.hpp"
#include "t81/tracing/canonhash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace t81::dpe {

// ── TaskId ────────────────────────────────────────────────────────────────────
//
// A TaskId is the CanonHash81 of the canonical serialisation of a TaskDescriptor
// (RFC-DPE-0002 §4).  Two tasks with identical descriptors have identical IDs.

struct TaskId {
  t81::hash::CanonHash81 hash{};

  bool operator==(const TaskId& o) const noexcept { return hash == o.hash; }
  bool operator!=(const TaskId& o) const noexcept { return hash != o.hash; }
  bool operator<(const TaskId& o) const noexcept { return hash.bytes < o.hash.bytes; }

  [[nodiscard]] std::string to_string() const { return hash.to_string(); }
};

// ── OutputRegion ─────────────────────────────────────────────────────────────
//
// Declares a TVA range as delta-buffered by one task (RFC-DPE-0002 §3).
// base_tva must be page-aligned.

struct OutputRegion {
  uint64_t base_tva{0};   ///< first TVA in the region (page-aligned)
  uint32_t page_count{0}; ///< number of pages
  bool exclusive{false};  ///< if true no other task in this epoch may write here
};

// ── TaskDescriptor ────────────────────────────────────────────────────────────
//
// Fully specifies one unit of parallel computation (RFC-DPE-0002 §3).

struct TaskDescriptor {
  uint8_t  format_version{1};  ///< must be 1
  uint64_t epoch_id{0};        ///< epoch this task belongs to
  uint64_t task_seq{0};        ///< monotone index within epoch (0-based)

  t81::canonfs::CanonRef program_ref{};  ///< CanonRef of the TISC bytecode object
  uint64_t               entry_tva{0};   ///< TVA of the program entry point

  std::vector<t81::canonfs::CanonRef> input_refs{};  ///< ordered immutable inputs
  std::vector<uint64_t>               input_tvas{};  ///< parallel array: TVA per input

  std::vector<OutputRegion> output_regions{};  ///< delta-buffered output ranges
  std::vector<TaskId>       dep_task_ids{};    ///< prerequisite tasks

  uint32_t scratch_pages{0};  ///< scratch pages (discarded after task)
};

// ── EpochGraph ────────────────────────────────────────────────────────────────
//
// Groups a set of TaskDescriptors into a single epoch (RFC-DPE-0002 §3.1).

struct EpochGraph {
  uint8_t  format_version{1};
  uint64_t epoch_id{0};

  std::vector<TaskDescriptor> tasks{};       ///< all tasks in task_seq order
  t81::canonfs::CanonRef      input_snapshot{}; ///< CanonHash81 of epoch-start state
};

// ── TaskId computation ────────────────────────────────────────────────────────

/// Canonically serialise a TaskDescriptor to a byte vector.
/// The serialisation is deterministic and field-ordered (RFC-DPE-0002 §3).
[[nodiscard]] std::vector<std::byte> serialise_task_descriptor(
    const TaskDescriptor& task) noexcept;

/// Compute the TaskId for a TaskDescriptor (RFC-DPE-0002 §4).
[[nodiscard]] TaskId compute_task_id(const TaskDescriptor& task) noexcept;

// ── DeltaRecord ───────────────────────────────────────────────────────────────
//
// One intercepted store to an output region (RFC-DPE-0002 §5.2).
// Granularity: full page (page_size bytes starting at a page-aligned TVA).

inline constexpr std::size_t kDpePageSize    = 4096;  ///< page granularity for delta records
inline constexpr std::size_t kDpeWordsPerPage = kDpePageSize / sizeof(std::int64_t);  ///< 512

// ── DeltaRecord ───────────────────────────────────────────────────────────────
//
// One intercepted store to an output region (RFC-DPE-0002 §5.2).
// Granularity: full page (page_size bytes starting at a page-aligned TVA).
//
// word_tags: per-word type annotation (one byte per int64_t word, using the
// ValueTag enum values from t81/vm/state.hpp).  A tag of 0 (= ValueTag::Int)
// means the corresponding 8 bytes in `value` are a raw int64_t.  A tag of
// ValueTag::FloatHandle means the 8 bytes contain the IEEE 754 double
// representation of the float value (not a pool handle index), enabling
// deterministic EpochHash computation independent of handle allocation order
// (RFC-DPE-0003 §5, [DPE-03-05]).

struct DeltaRecord {
  TaskId   task_id{};
  uint64_t tva{0};                                         ///< page-aligned TVA
  std::array<std::byte, kDpePageSize> value{};             ///< full page content
  std::array<std::uint8_t, kDpeWordsPerPage> word_tags{};  ///< ValueTag per word (0 = Int)
};

// ── TaskFaultKind ─────────────────────────────────────────────────────────────

enum class TaskFaultKind : uint8_t {
  None = 0,
  OutOfRegionWrite,  ///< store to TVA outside declared output_regions + scratch (§5.2)
};

// ── DeltaBuffer ───────────────────────────────────────────────────────────────
//
// Per-task accumulator of DeltaRecords (RFC-DPE-0002 §5.2).
// Tracks all intercepted writes; detects out-of-region stores.

struct DeltaWriteResult {
  TaskFaultKind fault{TaskFaultKind::None};
  bool          buffered{false};  ///< true when the write was accepted into the buffer
};

class DeltaBuffer {
public:
  explicit DeltaBuffer(const TaskDescriptor& task, TaskId tid) noexcept;

  /// Intercept a page-granular store to `tva`.
  /// Returns `buffered=true` if `tva` falls within a declared output_region.
  /// Returns `fault=OutOfRegionWrite` if outside all output_regions (and scratch).
  [[nodiscard]] DeltaWriteResult write(
      uint64_t tva,
      std::span<const std::byte> page_data) noexcept;

  [[nodiscard]] const std::vector<DeltaRecord>& records() const noexcept {
    return records_;
  }

  [[nodiscard]] bool faulted() const noexcept {
    return faulted_;
  }

  [[nodiscard]] TaskFaultKind fault_kind() const noexcept {
    return fault_kind_;
  }

  [[nodiscard]] const TaskId& task_id() const noexcept {
    return tid_;
  }

private:
  const TaskDescriptor& task_;
  TaskId                tid_;
  std::vector<DeltaRecord> records_{};
  bool          faulted_{false};
  TaskFaultKind fault_kind_{TaskFaultKind::None};

  [[nodiscard]] bool tva_in_output_region(uint64_t tva) const noexcept;
  [[nodiscard]] bool tva_in_scratch(uint64_t tva) const noexcept;
};

// ── EpochAcceptor ─────────────────────────────────────────────────────────────
//
// Validates an EpochGraph before execution (RFC-DPE-0002 §5.4–5.5).

enum class EpochAcceptStatus : uint8_t {
  Ok = 0,
  MalformedEpochGraph,   ///< cyclic dependency graph (§5.4)
  ExclusiveRegionConflict, ///< two tasks claim overlapping exclusive regions (§5.5)
  DuplicateTaskSeq,      ///< two tasks share the same task_seq within the epoch
  EmptyEpoch,            ///< epoch contains no tasks
};

struct EpochAcceptResult {
  EpochAcceptStatus status{EpochAcceptStatus::Ok};

  /// Index (in EpochGraph::tasks) of the first offending task, if applicable.
  std::optional<std::size_t> offending_task_index{};

  [[nodiscard]] bool ok() const noexcept {
    return status == EpochAcceptStatus::Ok;
  }
};

/// Validate an EpochGraph before execution.
/// Checks:
///   1. epoch is non-empty
///   2. task_seq values are unique within the epoch
///   3. dependency graph is a DAG (no cycles)
///   4. no exclusive output region conflicts between tasks
[[nodiscard]] EpochAcceptResult accept_epoch(const EpochGraph& epoch) noexcept;

// ── Dependency graph utilities ─────────────────────────────────────────────────

/// Returns the program-identity TaskId of a task: the TaskId computed with
/// dep_task_ids cleared.  This is the stable identity used as the value in
/// dep_task_ids of dependent tasks (RFC-DPE-0002 §4 workflow invariant).
[[nodiscard]] TaskId program_identity(const TaskDescriptor& task) noexcept;

/// Returns task indices in a valid topological execution order
/// (RFC-DPE-0004 §2.1): every predecessor appears before all its successors.
///
/// Tie-breaking among tasks with equal in-degree is by ascending canonical
/// TaskId (RFC-DPE-0004 §2.2).  Returns an empty vector if the epoch graph
/// contains a cycle (should not occur after accept_epoch()).
[[nodiscard]] std::vector<std::size_t> topological_sort_epoch(
    const EpochGraph& epoch) noexcept;

/// Partition the epoch into topological levels (RFC-DPE-0005 §3).
/// result[k] contains the task indices at level k in ascending canonical
/// TaskId order.  Tasks within the same level have no mutual dependency and
/// may execute concurrently.
///
/// Returns an empty outer vector if the graph contains a cycle (should not
/// occur after accept_epoch()).
[[nodiscard]] std::vector<std::vector<std::size_t>> topological_levels_epoch(
    const EpochGraph& epoch) noexcept;

}  // namespace t81::dpe
