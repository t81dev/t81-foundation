# RFC-DPE-0005: Level-Parallel Epoch Execution

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, DpeTaskRunner, axion_kernel_submit_epoch
**Created:** 2026-03-14
**Updated:** 2026-03-14
**Author:** @t81dev
**Depends on:** RFC-DPE-0004 (DAG-Ordered Multi-Task Epoch Execution)
**Blocks:** multi-core AI inference pipelines, distributed epoch sharding

---

## 1. Summary

RFC-DPE-0004 establishes DAG-ordered execution via `topological_sort_epoch()`,
but tasks at the same topological level still run sequentially.  This RFC
specifies:

- the **topological level** concept: a group of tasks whose predecessor
  dependencies are all satisfied by earlier levels (§2)
- `topological_levels_epoch()`: the primitive that partitions an `EpochGraph`
  into an ordered sequence of levels (§3)
- the **level-parallel dispatch** algorithm: tasks within each level execute
  concurrently in separate `std::thread` instances (§4)
- input snapshot construction at level boundaries (§5)
- policy gate thread-safety contract (§6)
- thread-creation fallback to sequential execution (§7)
- interaction with the canonical commit algorithm (§8)
- acceptance criteria (§9)

---

## 2. Topological Level

### 2.1 Definition

A **topological level** `L(k)` for an epoch is defined inductively:

```
L(0) = { T ∈ epoch.tasks | T.dep_task_ids = ∅ }
L(k) = { T ∈ epoch.tasks | ∀ dep ∈ T.dep_task_ids: dep ∈ L(0) ∪ … ∪ L(k-1) }
      \ L(0) ∪ … ∪ L(k-1)
```

Every task belongs to exactly one level.  A valid `EpochGraph` (accepted by
`accept_epoch()`) always produces a finite level sequence — the DAG guarantees
termination.

### 2.2 Within-Level Ordering

Tasks within a level have no dependency relationship with each other.  The
canonical ordering within a level (for determinism in test output and audit
logs) is ascending canonical TaskId, matching RFC-DPE-0004 §2.2.

### 2.3 Relationship to topological_sort_epoch()

`topological_sort_epoch()` produces a valid linear execution order.
`topological_levels_epoch()` is its structured variant: the concatenation of
all levels (in within-level canonical order) is a valid topological sort
consistent with RFC-DPE-0004 §2.

---

## 3. topological_levels_epoch()

```cpp
/// Partition an EpochGraph into topological levels.
/// Returns a vector of vectors: result[k] contains the task indices at level k,
/// in ascending canonical TaskId order.
///
/// Returns an empty outer vector if the graph contains a cycle (should not
/// occur after accept_epoch()).
[[nodiscard]] std::vector<std::vector<std::size_t>>
topological_levels_epoch(const EpochGraph& epoch) noexcept;
```

**Algorithm:** Modified BFS (level-aware Kahn):

1. Compute program-identity index and in-degree array (same as
   `topological_sort_epoch()`).
2. Seed level 0 with all zero-in-degree tasks; sort by canonical TaskId.
3. For each level: emit the current level, decrement in-degrees of successors,
   collect newly zero-in-degree tasks as the next level (sorted canonically).
4. If the total task count processed ≠ N, return `{}` (cycle).

---

## 4. Level-Parallel Dispatch

### 4.1 Rule

Tasks within the same topological level **may execute concurrently**.  The
execution engine spawns one `std::thread` per task in the level and joins all
threads before advancing to the next level.

### 4.2 Ordering guarantees preserved

- A task at level `k` never begins until all tasks at levels `0..k-1` have
  completed and their delta records are available.
- Tasks within the same level have no dependency relationship (by §2.1), so
  no ordering constraint applies between them.

### 4.3 VM instance isolation

Each concurrent task receives its own fresh `IVirtualMachine` instance
(created inside `DpeTaskRunner::run_direct()`).  No VM state is shared between
concurrent tasks.

### 4.4 delta_sets indexing

`delta_sets` is a vector pre-sized to `epoch.tasks.size()`, indexed by task
array index.  Each thread writes only to its own slot (`delta_sets[i]`), so no
mutex is required for result collection.

---

## 5. Input Snapshot Construction at Level Boundaries

Before dispatching level `k`, the execution engine constructs
`DpeTaskInputSnapshot` for each task in level `k` from the **already-completed**
delta records of its direct predecessors (all of which are in levels `0..k-1`).
This construction is sequential (no concurrency needed) and precedes thread
launch.

The conflict resolution rule is unchanged from RFC-DPE-0004 §3.1: if multiple
predecessors of T write to the same TVA, the value from the predecessor with
the higher canonical TaskId wins.

---

## 6. Policy Gate Thread-Safety Contract

`KernelEpochPolicyGate::fn` may be invoked from multiple threads concurrently
when tasks in the same level are dispatched in parallel.

**Contract:** The policy gate function pointer and its `user_data` **must be
thread-safe** if parallel execution is used.  The simplest way to satisfy
this contract is to use a stateless gate function (reads from immutable data
only).

Policy gate checks are performed **before** thread launch (sequentially),
so the implementation currently evaluates all gates in the dispatch loop
prior to spawning threads.  This ensures state-mutating gates (e.g., those
that count calls) remain correct without user-visible races.

---

## 7. Thread-Creation Fallback

`std::thread` construction may fail (throwing `std::system_error`).
`axion_kernel_submit_epoch()` is `noexcept`; thread-creation failures are
caught and the affected task runs synchronously as a fallback.  The fallback
is transparent to the caller — the task still executes and its result is
collected normally.

---

## 8. Interaction with Canonical Commit

The canonical commit algorithm (RFC-DPE-0003 §2) commits deltas in
TaskId-ascending order, independent of execution order or thread scheduling.
Level-parallel execution does not change the EpochHash.  The determinism of
the EpochHash is proved by `[DPE-05-04]` (identical hash for parallel vs.
sequential execution of the same epoch).

---

## 9. Acceptance Criteria

- `[DPE-05-01]` `topological_levels_epoch()` assigns all tasks to exactly one
  level; independent tasks land at level 0; dependent tasks land at the
  correct successor level.
- `[DPE-05-02]` Two independent tasks at level 0 execute concurrently and
  both produce correct committed values.
- `[DPE-05-03]` A fan-out epoch (T0→{T1,T2}): T0 at level 0; T1 and T2 at
  level 1 (parallel).  Each of T1 and T2 receives T0's output via input
  snapshot and produces the correct transformed value.
- `[DPE-05-04]` The EpochHash produced by parallel execution of a multi-task
  epoch is identical to the EpochHash produced by sequential execution of the
  same epoch — proving canonical commit determinism is preserved across
  concurrency.

---

## 10. Open Questions

- Configurable thread-pool size (currently: one thread per task per level).
- Distributed epoch sharding: levels executed on separate nodes.
- Priority scheduling within a level (currently: all tasks at a level are
  equally prioritised).

---

## 11. Acceptance Notes (2026-03-14)

`[DPE-05-01]` through `[DPE-05-04]` proved by `t81_dpe_epoch_parallel_test`
(Slice 19).

| Criterion | Evidence | Status |
| :--- | :--- | :--- |
| `[DPE-05-01]` Level assignment correct | `test_topological_levels_basic` — single task, two-task chain, independent pair, diamond each produce correct level partition | met |
| `[DPE-05-02]` Two independent tasks execute and commit correctly | `test_parallel_independent_tasks` — both tasks write distinct pages; both values committed | met |
| `[DPE-05-03]` Fan-out epoch T0→{T1,T2}: T1 and T2 receive T0's delta | `test_parallel_fan_out` — T1 and T2 each read T0's output page and produce correct transformed values | met |
| `[DPE-05-04]` EpochHash identical for parallel vs sequential execution | `test_epoch_hash_deterministic_across_parallel` — two-task epoch hash matches whether tasks run in parallel or sequentially | met |
