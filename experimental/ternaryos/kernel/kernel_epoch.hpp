#pragma once
// experimental/ternaryos/kernel/kernel_epoch.hpp
//
// RFC-DPE-0003 §10 — Kernel-side EpochRuntimeState wiring.
//
// axion_kernel_submit_epoch() is the kernel entry point for DPE epoch
// execution.  It:
//   1. Validates the epoch graph via accept_epoch() (RFC-DPE-0002 §6).
//   2. Executes each task through DpeTaskRunner::run_direct().
//   3. Commits the collected deltas via commit_epoch() (RFC-DPE-0003 §2–5).
//   4. Updates KernelRuntimeState::epoch and ::counters.
//
// This file is compiled only when T81_ENABLE_DPE=ON (see CMakeLists.txt).

#include "kernel_runtime_state.hpp"

#include "t81/dpe/epoch_commit.hpp"
#include "t81/dpe/task_graph.hpp"
#include "t81/dpe/thread_pool.hpp"
#include "t81/isa/program.hpp"

#include <chrono>
#include <optional>
#include <vector>

namespace t81::ternaryos::kernel {

// ── KernelEpochResult ─────────────────────────────────────────────────────────

enum class KernelEpochStatus : uint8_t {
  Ok = 0,
  Rejected_AcceptFailed,     ///< accept_epoch() rejected the EpochGraph
  Aborted_TaskFault,         ///< a task produced an out-of-region write or fault
  Aborted_ExclusiveConflict, ///< exclusive output region conflict at commit time
  Aborted_PolicyFault,       ///< a task's program was denied by the epoch policy gate (RFC-DPE-0003 §6.1)
  Aborted_Timeout,           ///< epoch exceeded the per-epoch wall-clock budget (RFC-DPE-0007)
};

struct KernelEpochResult {
  KernelEpochStatus        status{KernelEpochStatus::Ok};
  t81::hash::CanonHash81   epoch_hash{};  ///< valid only when status == Ok

  [[nodiscard]] bool ok() const noexcept {
    return status == KernelEpochStatus::Ok;
  }
};

// ── KernelEpochPolicyGate ─────────────────────────────────────────────────────
//
// Optional policy gate evaluated before each task executes (RFC-DPE-0003 §6.1).
//
// If fn is non-null it is called with the task descriptor and program before
// the task runs.  Returning false aborts the epoch with Aborted_PolicyFault,
// increments state.counters.policy_faults, and records an
// EpochAbortedPolicyFault audit event.
//
// fn must be noexcept.  user_data is forwarded unchanged.
// Default-constructed gate (fn == nullptr) allows all tasks.
struct KernelEpochPolicyGate {
  using GateFn = bool (*)(const t81::dpe::TaskDescriptor&,
                          const t81::tisc::Program&,
                          void* user_data) noexcept;
  GateFn fn{nullptr};
  void*  user_data{nullptr};

  [[nodiscard]] bool check(const t81::dpe::TaskDescriptor& task,
                           const t81::tisc::Program& program) const noexcept {
    if (!fn) return true;
    return fn(task, program, user_data);
  }
};

// ── axion_kernel_submit_epoch ─────────────────────────────────────────────────
//
// Submit an epoch graph for deterministic parallel execution.
//
// Parameters:
//   state      — kernel runtime state (counters and epoch field updated in place)
//   epoch      — the EpochGraph to execute; must satisfy accept_epoch()
//   programs   — one TISC Program per task, in the same order as epoch.tasks
//   gate       — optional policy gate evaluated before each task (default: allow all)
//   pool       — optional bounded thread pool (RFC-DPE-0006); when nullptr the
//                RFC-DPE-0005 unbounded one-thread-per-task dispatch is used
//   timeout_ms — optional wall-clock budget (RFC-DPE-0007); checked after each
//                topological level; std::nullopt = no timeout
//
// On success (KernelEpochStatus::Ok):
//   • state.epoch.epochs_committed and state.counters.epoch_commits are incremented
//   • state.epoch.last_committed_epoch_id / last_committed_epoch_hash are updated
//   • state.counters.epoch_task_executions is incremented by programs.size()
//   • result.epoch_hash carries the EpochHash from commit_epoch()
//
// On abort:
//   • state.epoch.epochs_aborted and state.counters.epoch_aborts are incremented
//   • canonical state is unchanged (no partial commits)
//   • Aborted_PolicyFault additionally increments state.counters.policy_faults
//     and records KernelAuditEventKind::EpochAbortedPolicyFault in the audit log
[[nodiscard]] KernelEpochResult axion_kernel_submit_epoch(
    KernelRuntimeState&                      state,
    const t81::dpe::EpochGraph&              epoch,
    const std::vector<t81::tisc::Program>&   programs,
    KernelEpochPolicyGate                    gate       = {},
    t81::dpe::DpeThreadPool*                 pool       = nullptr,
    std::optional<std::chrono::milliseconds> timeout_ms = std::nullopt) noexcept;

}  // namespace t81::ternaryos::kernel
