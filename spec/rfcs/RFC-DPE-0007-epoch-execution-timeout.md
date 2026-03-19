# RFC-DPE-0007: Epoch Execution Timeout

**Status:** accepted
**Type:** standards-track
**Applies-To:** T81VM runtime, axion_kernel_submit_epoch, KernelCallKind::SubmitEpoch
**Created:** 2026-03-15
**Updated:** 2026-03-15
**Author:** @t81dev
**Depends on:** RFC-DPE-0005 (Level-Parallel Epoch Execution), RFC-DPE-0006 (Bounded Thread Pool)
**Blocks:** production-grade epoch scheduling with resource budgets

---

## 1. Summary

Epoch execution currently has no wall-clock bound.  A misbehaving task
program that executes a very long loop can stall the entire epoch pipeline
indefinitely.

This RFC adds an **optional per-epoch wall-clock timeout** checked at each
topological level boundary:

- A new `std::optional<std::chrono::milliseconds>` parameter `timeout_ms`
  on `axion_kernel_submit_epoch()` (§3).
- After all tasks in a level complete (post-join / post-`wait_idle()`), the
  kernel checks elapsed wall time.  If `elapsed >= timeout_ms` the epoch is
  aborted with `KernelEpochStatus::Aborted_Timeout` (§4).
- `KernelCallKind::SubmitEpoch` surfaces the timeout via a new
  `epoch_timeout_ms` field in `KernelCallRequest` (§5).
- The new rejection code `KernelCallRejection::EpochTimedOut` is returned
  with `KernelCallStatus::RetryLater` when `Aborted_Timeout` propagates
  through the syscall boundary (§5).

---

## 2. Motivation

Production environments require resource budgets.  Epoch workloads submitted
by userland services must not monopolise the kernel scheduler.  A per-level
check is cheap (one clock read per level boundary), determinism-safe (timeout
is applied after the level's tasks complete — individual task results are not
truncated), and composable with the existing pool and policy-gate machinery.

### 2.1 Why level-boundary rather than per-task?

Checking inside a running task would require cancellation support in
`DpeTaskRunner::run_direct()` and in the TISC VM loop.  Level-boundary checks
are simpler, non-intrusive, and sufficient for the resource-bounding use case:
the worst-case overshoot is the duration of the longest task in the final
(timed-out) level.

---

## 3. axion_kernel_submit_epoch() signature change

```cpp
[[nodiscard]] KernelEpochResult axion_kernel_submit_epoch(
    KernelRuntimeState&                      state,
    const t81::dpe::EpochGraph&              epoch,
    const std::vector<t81::tisc::Program>&   programs,
    KernelEpochPolicyGate                    gate       = {},
    t81::dpe::DpeThreadPool*                 pool       = nullptr,
    std::optional<std::chrono::milliseconds> timeout_ms = std::nullopt) noexcept;
```

- All existing callers are unaffected (new parameter has a default value).
- `timeout_ms == std::nullopt` → no timeout (original behaviour preserved).
- `timeout_ms == 0ms` → the post-level check fires after the first level
  (0 ms elapsed ≥ 0 ms budget), aborting immediately.

---

## 4. Timeout check semantics

```text
start_time = steady_clock::now()   // captured before the level loop

for each level in topological_levels_epoch(epoch):
    pre-flight (policy gate + snapshots)
    dispatch tasks (pool or unbounded)
    join / wait_idle()
    collect results
    if timeout_ms.has_value():
        elapsed = steady_clock::now() - start_time
        if elapsed >= *timeout_ms:
            → state.epoch.epochs_aborted++
            → state.counters.epoch_aborts++
            → return KernelEpochResult{Aborted_Timeout}
```

Checked **after** the level's tasks complete — partial task results already
collected are discarded (the epoch is never partially committed).

---

## 5. KernelCallKind::SubmitEpoch wiring

### 5.1 KernelCallRequest

```cpp
#ifdef T81_ENABLE_DPE
  std::optional<std::chrono::milliseconds> epoch_timeout_ms{};
  ///< SubmitEpoch: per-epoch wall-clock budget; std::nullopt = no timeout
#endif
```

### 5.2 KernelCallRejection

```cpp
EpochTimedOut,  ///< SubmitEpoch: epoch exceeded wall-clock budget (RFC-DPE-0007)
```

### 5.3 Status mapping

`Aborted_Timeout` maps to `KernelCallStatus::RetryLater` + `EpochTimedOut`.
`RetryLater` is used because the work is valid — the caller may resubmit with
a larger budget or lighter workload.

### 5.4 Dispatch

The `SubmitEpoch` dispatch in `axion_kernel_call()` passes
`request.epoch_timeout_ms` to `axion_kernel_submit_epoch()` and maps the new
`Aborted_Timeout` case.

---

## 6. KernelEpochStatus addition

```cpp
Aborted_Timeout,  ///< epoch exceeded the per-epoch wall-clock budget (RFC-DPE-0007)
```

---

## 7. Determinism note

The timeout check uses `std::chrono::steady_clock`.  Steady-clock reads are
platform-specific and non-deterministic across machines; they may cause the
same epoch to succeed on a fast host and time out on a slow host.  This is
intentional: timeout is a **resource-bounding** mechanism, not part of the
canonical execution contract.  The `EpochHash` is never computed on a timed-
out epoch, so replay determinism is unaffected.

---

## 8. Acceptance criteria

| ID | Criterion |
| :--- | :--- |
| [DPE-08-01] | `axion_kernel_submit_epoch()` with `timeout_ms = std::nullopt` behaves identically to the RFC-DPE-0006 baseline (no regression). |
| [DPE-08-02] | A single-level epoch with a generous timeout (5 000 ms) completes with `KernelEpochStatus::Ok`. |
| [DPE-08-03] | A single-level epoch submitted with `timeout_ms = 0ms` returns `KernelEpochStatus::Aborted_Timeout`. |
| [DPE-08-04] | `state.epoch.epochs_aborted` and `state.counters.epoch_aborts` are each incremented by 1 on `Aborted_Timeout`. |
| [DPE-08-05] | `KernelCallKind::SubmitEpoch` with `epoch_timeout_ms = 0ms` returns `KernelCallStatus::RetryLater` with `rejection == EpochTimedOut`. |
| [DPE-08-06] | `epoch_committed` is `false` when timeout fires. |
