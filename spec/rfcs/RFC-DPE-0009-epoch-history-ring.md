# RFC-DPE-0009: Epoch History Ring

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, KernelRuntimeState::EpochRuntimeState, axion_kernel_submit_epoch
**Created:** 2026-03-15
**Updated:** 2026-03-15
**Author:** @t81dev
**Depends on:** RFC-DPE-0003 (Epoch Execution), RFC-DPE-0008 (Epoch Audit Events)
**Blocks:** epoch workload characterisation, post-mortem correlation without log scanning

---

## 1. Summary

After a successful commit, the kernel retains only `last_committed_epoch_id`
and `last_committed_epoch_hash` — a single-record view.  Diagnosing epoch
pipeline behaviour (e.g. "did the hash change after the last 8 submissions?",
"how many tasks ran in the previous few epochs?") requires scanning the full
audit log.

This RFC adds a **fixed-capacity ring buffer** of committed epoch records to
`EpochRuntimeState`.  Each successful `commit_epoch()` pushes a new
`EpochHistoryRecord`; when the ring is full the oldest entry is evicted.  The
ring is exposed read-only through `KernelRuntimeStatusView`.

---

## 2. EpochHistoryRecord

```cpp
struct EpochHistoryRecord {
  uint64_t               epoch_id{0};
  t81::hash::CanonHash81 epoch_hash{};
  std::size_t            task_count{0};        ///< epoch.tasks.size()
  std::size_t            level_count{0};       ///< topological levels executed
  std::size_t            total_delta_records{0}; ///< sum of DeltaRecord counts across all tasks
  uint64_t               commit_sequence{0};   ///< monotonic commit ordinal (== epochs_committed after increment)
};
```

---

## 3. Ring buffer in EpochRuntimeState

```cpp
static constexpr std::size_t kEpochHistoryCapacity = 8;
std::deque<EpochHistoryRecord> epoch_history{};
```

- `epoch_history.size() <= kEpochHistoryCapacity` at all times.
- On commit: `push_back` new record; if `size() > kEpochHistoryCapacity`, `pop_front`.
- Order: `[0]` = oldest retained commit, `[size-1]` = most recent.
- Aborted or rejected epochs are **not** recorded in the ring.

---

## 4. Insertion point in axion_kernel_submit_epoch()

After `§4: Record committed state` (existing) and before emitting
`EpochCommitted` audit event:

```text
record.epoch_id            = epoch.epoch_id
record.epoch_hash          = commit.epoch_hash
record.task_count          = epoch.tasks.size()
record.level_count         = levels.size()
record.total_delta_records = sum of delta_sets[i].records.size() for all i
record.commit_sequence     = state.epoch.epochs_committed   // after increment
push_back to state.epoch.epoch_history
if size > kEpochHistoryCapacity: pop_front
```

---

## 5. KernelRuntimeStatusView additions

```cpp
std::vector<EpochHistoryRecord> epoch_history{};
```

Populated by copying `state.epoch.epoch_history` into the view.  The copy is
a snapshot — callers receive a stable slice, not a live reference.

---

## 6. Acceptance criteria

| ID | Criterion |
| :--- | :--- |
| [DPE-10-01] | After one successful epoch, `epoch_history.size() == 1`; record fields (epoch_id, hash, task_count, level_count) are correct. |
| [DPE-10-02] | After N ≤ 8 successful epochs, `epoch_history.size() == N`; records are ordered oldest-first. |
| [DPE-10-03] | After 9 successful epochs, `epoch_history.size() == 8`; the oldest (epoch_id 0) is evicted; `[0].epoch_id` is the second submitted. |
| [DPE-10-04] | Aborted epochs (task-fault) do not add entries to the ring. |
| [DPE-10-05] | `commit_sequence` is monotonically increasing across consecutive records. |
| [DPE-10-06] | `KernelRuntimeStatusView::epoch_history` contains the same records as `state.epoch.epoch_history` at the time of the view snapshot. |
