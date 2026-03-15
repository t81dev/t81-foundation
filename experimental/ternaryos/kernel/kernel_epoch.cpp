// experimental/ternaryos/kernel/kernel_epoch.cpp
//
// RFC-DPE-0003 §10 — Kernel-side EpochRuntimeState wiring.

#include "kernel_epoch.hpp"
#include "kernel_main.hpp"

#include "experimental/dpe/task_graph.hpp"
#include "experimental/dpe/task_runner.hpp"

#include <map>
#include <thread>
#include <unordered_map>

namespace t81::ternaryos::kernel {

KernelEpochResult axion_kernel_submit_epoch(
    KernelRuntimeState&                     state,
    const t81::dpe::EpochGraph&             epoch,
    const std::vector<t81::tisc::Program>&  programs,
    KernelEpochPolicyGate                   gate) noexcept {

  // ── §1: Validate epoch graph ──────────────────────────────────────────────
  const auto accept = t81::dpe::accept_epoch(epoch);
  if (accept.status != t81::dpe::EpochAcceptStatus::Ok) {
    state.epoch.epochs_submitted++;
    state.epoch.epochs_aborted++;
    state.counters.epoch_submissions++;
    state.counters.epoch_aborts++;
    return KernelEpochResult{KernelEpochStatus::Rejected_AcceptFailed};
  }

  state.epoch.epochs_submitted++;
  state.counters.epoch_submissions++;

  // ── §2: Execute tasks level-by-level; tasks within a level run in parallel
  //        (RFC-DPE-0005 §4).
  const auto levels = t81::dpe::topological_levels_epoch(epoch);

  t81::dpe::DpeTaskRunner runner;

  // delta_sets indexed by task array index.
  std::vector<t81::dpe::TaskDeltaSet> delta_sets(epoch.tasks.size());

  // program-identity index for dep resolution: prog_id_str → array index.
  std::unordered_map<std::string, std::size_t> pid_index;
  pid_index.reserve(epoch.tasks.size());
  for (std::size_t i = 0; i < epoch.tasks.size(); ++i) {
    pid_index.emplace(
        t81::dpe::program_identity(epoch.tasks[i]).hash.to_string(), i);
  }

  for (const auto& level_tasks : levels) {
    // ── Pre-flight: policy gate checks + input snapshot construction ─────────
    // Both are sequential (gate checks mutate state counters; snapshots read
    // already-completed delta_sets from prior levels).

    struct LevelTask {
      std::size_t                 idx{0};
      t81::dpe::DpeTaskInputSnapshot snapshot{};
      bool                        skip{false};  // no program supplied
    };

    std::vector<LevelTask> prepared;
    prepared.reserve(level_tasks.size());
    bool policy_abort = false;

    for (const std::size_t i : level_tasks) {
      const auto& task = epoch.tasks[i];
      const auto  tid  = t81::dpe::compute_task_id(task);
      delta_sets[i].id = tid;

      LevelTask lt;
      lt.idx = i;

      if (i >= programs.size()) {
        lt.skip = true;
        delta_sets[i].faulted = true;
        prepared.push_back(std::move(lt));
        continue;
      }

      // Policy gate (RFC-DPE-0005 §6 — sequential, before thread launch).
      if (!gate.check(task, programs[i])) {
        state.epoch.epochs_aborted++;
        state.counters.epoch_aborts++;
        ++state.counters.policy_faults;
        record_audit_event(state,
                           KernelAuditEventKind::EpochAbortedPolicyFault,
                           KernelRuntimeState::kKernelTid,
                           KernelRuntimeState::kKernelProcessGroup);
        policy_abort = true;
        break;
      }

      // Build input snapshot from direct predecessors (RFC-DPE-0005 §5).
      std::map<uint64_t, t81::dpe::TaskId> snapshot_owner;
      for (const auto& dep_id : task.dep_task_ids) {
        auto it = pid_index.find(dep_id.hash.to_string());
        if (it == pid_index.end()) continue;
        const std::size_t dep_idx = it->second;
        const auto& dep_ds = delta_sets[dep_idx];
        for (const auto& rec : dep_ds.records) {
          auto own_it = snapshot_owner.find(rec.tva);
          if (own_it == snapshot_owner.end()) {
            lt.snapshot.pages.emplace(rec.tva, rec.value);
            snapshot_owner.emplace(rec.tva, dep_ds.id);
          } else if (own_it->second < dep_ds.id) {
            lt.snapshot.pages[rec.tva] = rec.value;
            own_it->second = dep_ds.id;
          }
        }
      }

      prepared.push_back(std::move(lt));
    }

    if (policy_abort) {
      return KernelEpochResult{KernelEpochStatus::Aborted_PolicyFault};
    }

    // ── Parallel dispatch: one thread per prepared task ───────────────────────
    // Each thread writes only to delta_sets[i] — no shared mutable state.
    // Thread-creation failure falls back to inline execution (§7).

    const std::size_t n = prepared.size();
    std::vector<t81::dpe::DpeTaskResult> level_results(n);

    std::vector<std::thread> threads;
    threads.reserve(n);

    for (std::size_t t = 0; t < n; ++t) {
      if (prepared[t].skip) continue;
      const std::size_t  task_idx  = prepared[t].idx;
      const auto&        snap      = prepared[t].snapshot;
      const auto&        task      = epoch.tasks[task_idx];
      const auto&        prog      = programs[task_idx];

      try {
        threads.emplace_back(
            [&runner, &task, &prog, &snap, &level_results, t]() {
              level_results[t] = runner.run_direct(task, prog, snap);
            });
      } catch (...) {
        // Fallback: run inline if thread creation fails (RFC-DPE-0005 §7).
        level_results[t] = runner.run_direct(task, prog, snap);
      }
    }

    for (auto& th : threads) {
      if (th.joinable()) th.join();
    }

    // ── Collect results ───────────────────────────────────────────────────────
    for (std::size_t t = 0; t < n; ++t) {
      if (prepared[t].skip) continue;
      const std::size_t i = prepared[t].idx;
      delta_sets[i].faulted = !level_results[t].halted;
      delta_sets[i].records = level_results[t].delta_records;
      state.counters.epoch_task_executions++;
      state.epoch.epoch_task_executions++;
    }
  }

  // ── §3: Canonical commit ──────────────────────────────────────────────────
  const auto commit = t81::dpe::commit_epoch(epoch, delta_sets);

  if (!commit.ok()) {
    state.epoch.epochs_aborted++;
    state.counters.epoch_aborts++;

    KernelEpochResult res;
    res.status = (commit.status == t81::dpe::EpochCommitStatus::Aborted_ExclusiveConflict)
                     ? KernelEpochStatus::Aborted_ExclusiveConflict
                     : KernelEpochStatus::Aborted_TaskFault;
    return res;
  }

  // ── §4: Record committed state ────────────────────────────────────────────
  state.epoch.epochs_committed++;
  state.counters.epoch_commits++;
  state.epoch.last_committed_epoch_id   = epoch.epoch_id;
  state.epoch.last_committed_epoch_hash = commit.epoch_hash;

  return KernelEpochResult{KernelEpochStatus::Ok, commit.epoch_hash};
}

}  // namespace t81::ternaryos::kernel
