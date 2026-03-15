# RFC-DPE-0006: Bounded Thread Pool for Epoch Execution

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, DpeThreadPool, axion_kernel_submit_epoch
**Created:** 2026-03-14
**Updated:** 2026-03-14
**Author:** @t81dev
**Depends on:** RFC-DPE-0005 (Level-Parallel Epoch Execution)
**Blocks:** production-grade multi-core epoch dispatch, distributed epoch sharding

---

## 1. Summary

RFC-DPE-0005 spawns one `std::thread` per task per level.  On an epoch with M
independent tasks, this creates M threads simultaneously — unbounded thread
creation is unacceptable in a production runtime where M may be O(1 000) or
more.  This RFC specifies:

- `DpeThreadPool`: a fixed-size worker pool with a task queue (§2)
- the pool lifecycle: construction, submission, idle-wait, shutdown (§3)
- `axion_kernel_submit_epoch()` pool integration: optional `DpeThreadPool*`
  parameter; default nullptr preserves RFC-DPE-0005 one-thread-per-task
  behavior (§4)
- noexcept fallback when pool submission fails (§5)
- determinism guarantee: pool-based and unbounded dispatch produce identical
  EpochHash (§6)
- acceptance criteria (§7)

---

## 2. DpeThreadPool

```cpp
class DpeThreadPool {
public:
  /// Construct a pool with `worker_count` worker threads.
  /// worker_count == 0 is treated as 1 (minimum one worker).
  explicit DpeThreadPool(std::size_t worker_count) noexcept;
  ~DpeThreadPool();  // calls shutdown()

  // Non-copyable, non-movable.
  DpeThreadPool(const DpeThreadPool&)            = delete;
  DpeThreadPool& operator=(const DpeThreadPool&) = delete;

  /// Submit a callable to the pool queue.
  /// Returns true if the task was accepted; false if the pool is stopped.
  /// Never throws (pool submission failure falls back to inline execution).
  bool submit(std::function<void()> task) noexcept;

  /// Block until all previously submitted tasks have completed.
  void wait_idle() noexcept;

  /// Stop accepting tasks and join all worker threads.  Idempotent.
  void shutdown() noexcept;

  [[nodiscard]] std::size_t worker_count() const noexcept;
};
```

### 2.1 Internal invariant

`pending_` counts tasks that have been accepted into the queue or are actively
executing.  It is incremented in `submit()` under the queue mutex and
decremented by each worker after its task completes.  `wait_idle()` blocks
until `pending_ == 0`.

### 2.2 Worker loop

Each worker repeatedly:
1. Waits for a task in the queue (condition variable, mutex-protected).
2. Dequeues and executes the task (lock released during execution).
3. Decrements `pending_`; notifies `wait_idle()` if `pending_` reaches 0.

Worker exceptions (from the task callable) are caught and suppressed; the
`DpeTaskResult` for a faulted task is the responsibility of the task itself
(which is always `noexcept` — `DpeTaskRunner::run_direct()` is `noexcept`).

---

## 3. Pool Lifecycle

| Phase | Trigger | Effect |
| :--- | :--- | :--- |
| Construction | `DpeThreadPool(N)` | Spawns N worker threads; thread-creation failure reduces worker count silently |
| Submission | `submit(fn)` | Enqueues fn; returns false if pool is stopped |
| Idle-wait | `wait_idle()` | Blocks caller until all submitted tasks complete |
| Shutdown | `shutdown()` or destructor | Sets stopped flag; notifies workers; joins all threads |

Shutdown is idempotent: calling it multiple times (or via destructor after an
explicit `shutdown()`) is safe.

---

## 4. axion_kernel_submit_epoch() Pool Integration

```cpp
[[nodiscard]] KernelEpochResult axion_kernel_submit_epoch(
    KernelRuntimeState&                    state,
    const t81::dpe::EpochGraph&            epoch,
    const std::vector<t81::tisc::Program>& programs,
    KernelEpochPolicyGate                  gate = {},
    t81::dpe::DpeThreadPool*               pool = nullptr) noexcept;
```

When `pool != nullptr`, the level-parallel dispatch submits each task's
dispatch closure to the pool and calls `pool->wait_idle()` after all
submissions for the level are complete.

When `pool == nullptr`, behavior is identical to RFC-DPE-0005: one
`std::thread` per task per level (unbounded).

This preserves full backwards-compatibility: callers that do not pass a pool
are unaffected.

---

## 5. Noexcept Fallback

If `pool->submit()` returns false (pool stopped) or throws, the task is
executed inline on the calling thread.  This preserves the `noexcept`
contract and ensures no tasks are silently dropped.

---

## 6. Determinism Guarantee

The canonical commit algorithm (RFC-DPE-0003 §2) commits deltas in
TaskId-ascending order regardless of execution order or scheduling.  A pool
with N workers may execute the M tasks of a level in any order; the resulting
delta_sets are identical in content, and commit_epoch() produces the same
EpochHash.  This is proved by `[DPE-06-02]`.

---

## 7. Acceptance Criteria

- `[DPE-06-01]` A pool with 2 workers correctly executes an epoch level with
  4 independent tasks — all 4 tasks complete and produce correct committed
  values.
- `[DPE-06-02]` An epoch run with a 2-worker pool produces the same
  EpochHash as the same epoch run with the unbounded one-thread dispatch.
- `[DPE-06-03]` A pool with 1 worker executes all level tasks correctly
  (serialised on the single worker thread).
- `[DPE-06-04]` `DpeThreadPool` destructor cleanly joins all worker threads;
  no crash, no hang.

---

## 8. Open Questions

- Work-stealing between levels (currently: all tasks for a level are
  enqueued before workers for the prior level are drained — no cross-level
  stealing).
- Priority scheduling (deferred to a future RFC).
- Pool sharing across concurrent `axion_kernel_submit_epoch()` invocations
  (currently: caller serialises epoch submissions).

---

## 9. Acceptance Notes (2026-03-14)

`[DPE-06-01]` through `[DPE-06-04]` proved by `t81_dpe_thread_pool_test`
(Slice 20).

| Criterion | Evidence | Status |
| :--- | :--- | :--- |
| `[DPE-06-01]` 4 tasks, 2-worker pool — all committed | `test_pool_more_tasks_than_workers` — 4 independent tasks each write distinct values; all 4 delta records present after commit | met |
| `[DPE-06-02]` EpochHash identical pool vs unbounded | `test_pool_epoch_hash_matches_unbounded` — 3-task epoch hash with 2-worker pool equals hash from one-thread-per-task dispatch | met |
| `[DPE-06-03]` 1-worker pool correct | `test_single_worker_pool` — 3 independent tasks serialised on 1 worker; all results correct | met |
| `[DPE-06-04]` Destructor clean shutdown | `test_pool_destructor_clean_shutdown` — pool goes out of scope with no pending tasks; no crash | met |
