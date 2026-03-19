// experimental/ternaryos/tests/epoch_history_test.cpp
//
// RFC-DPE-0009 — Epoch History Ring tests.
//
// Acceptance criteria covered:
//   [DPE-10-01]  After one successful epoch, epoch_history.size() == 1;
//                record fields (epoch_id, hash, task_count, level_count) correct.
//   [DPE-10-02]  After N ≤ 8 successful epochs, epoch_history.size() == N;
//                records are ordered oldest-first.
//   [DPE-10-03]  After 9 successful epochs, epoch_history.size() == 8;
//                the oldest entry is evicted; [0].epoch_id is the second submitted.
//   [DPE-10-04]  Aborted epochs (task-fault) do not add entries to the ring.
//   [DPE-10-05]  commit_sequence is monotonically increasing across consecutive
//                records.
//   [DPE-10-06]  KernelRuntimeStatusView::epoch_history contains the same
//                records as state.epoch.epoch_history at snapshot time.

#include "../kernel/kernel_epoch.hpp"
#include "../kernel/kernel_main.hpp"
#include "../kernel/kernel_service_contract.hpp"
#include "../hal/hal.hpp"

#include "t81/dpe/task_graph.hpp"

#include "t81/isa/program.hpp"
#include "t81/isa/opcodes.hpp"

#include <cassert>
#include <cstdio>

using namespace t81::ternaryos::kernel;
using namespace t81::dpe;

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* label) {
  if (cond) {
    std::printf("  PASS  %s\n", label);
    ++g_pass;
  } else {
    std::printf("  FAIL  %s\n", label);
    ++g_fail;
  }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

static t81::ternaryos::hal::BootContext make_test_boot_ctx() {
  using namespace t81::ternaryos::hal;
  BootContext ctx;
  ctx.platform_id          = "test";
  ctx.ethics_boot_required = false;
  ctx.kernel_load_address  = 0x0000'0000'0800'0000ULL;
  MemoryRegion r;
  r.base_phys  = 0x0000'0000'0000'0000ULL;
  r.size_bytes = 0x0000'0000'1000'0000ULL;
  r.writable   = true;
  ctx.memory_map.push_back(r);
  return ctx;
}

static EpochGraph make_trivial_epoch(uint64_t epoch_id) {
  TaskDescriptor task;
  task.epoch_id = epoch_id;
  task.task_seq = 0;
  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks    = {task};
  return eg;
}

static t81::tisc::Program make_valid_program() {
  t81::tisc::Program p;
  p.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 7},
    {t81::tisc::Opcode::Halt},
  };
  return p;
}

/// Faulted program: Load from invalid TVA → BoundsFault → halted = false.
static t81::tisc::Program make_faulted_program() {
  t81::tisc::Program p;
  p.insns = {{t81::tisc::Opcode::Load, 1, 0x7FFF}};
  return p;
}

/// Submit one trivial epoch with epoch_id and return true on Ok.
static bool submit_ok(KernelRuntimeState& state, uint64_t epoch_id) {
  const auto eg  = make_trivial_epoch(epoch_id);
  const auto prg = std::vector<t81::tisc::Program>{make_valid_program()};
  return axion_kernel_submit_epoch(state, eg, prg).status == KernelEpochStatus::Ok;
}

// ── [DPE-10-01] One epoch → history size == 1, fields correct ─────────────────

static void test_single_entry(KernelRuntimeState& state) {
  std::printf("\n[DPE-10-01] One successful epoch → ring size == 1, fields correct\n");

  const auto eg       = make_trivial_epoch(1001);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};
  const auto r = axion_kernel_submit_epoch(state, eg, programs);

  check(r.status == KernelEpochStatus::Ok,
        "[DPE-10-01] epoch Ok");
  check(state.epoch.epoch_history.size() == 1,
        "[DPE-10-01] ring size == 1");

  const auto& rec = state.epoch.epoch_history.back();
  check(rec.epoch_id   == 1001,
        "[DPE-10-01] record.epoch_id == 1001");
  check(rec.task_count == 1,
        "[DPE-10-01] record.task_count == 1");
  check(rec.level_count == 1,
        "[DPE-10-01] record.level_count == 1");
  check(rec.epoch_hash == r.epoch_hash,
        "[DPE-10-01] record.epoch_hash matches result hash");
  check(rec.commit_sequence == state.epoch.epochs_committed,
        "[DPE-10-01] record.commit_sequence == epochs_committed");
}

// ── [DPE-10-02] N ≤ 8 epochs → ring grows to N, oldest-first ─────────────────

static void test_ring_growth(KernelRuntimeState& state) {
  std::printf("\n[DPE-10-02] 4 more epochs → ring grows, oldest-first\n");

  // Already have 1 from previous test; submit 4 more (epoch_ids 1002..1005).
  for (uint64_t id = 1002; id <= 1005; ++id) {
    check(submit_ok(state, id), "[DPE-10-02] epoch Ok");
  }

  check(state.epoch.epoch_history.size() == 5,
        "[DPE-10-02] ring size == 5 after 5 total commits");
  check(state.epoch.epoch_history.front().epoch_id == 1001,
        "[DPE-10-02] oldest entry is epoch_id 1001");
  check(state.epoch.epoch_history.back().epoch_id == 1005,
        "[DPE-10-02] newest entry is epoch_id 1005");
}

// ── [DPE-10-03] 9 epochs total → ring capped at 8, oldest evicted ─────────────

static void test_ring_eviction(KernelRuntimeState& state) {
  std::printf("\n[DPE-10-03] Fill to 8 then add one more → oldest evicted\n");

  // Ring currently has 5 entries (1001..1005).  Submit 3 more to reach 8.
  for (uint64_t id = 1006; id <= 1008; ++id) {
    check(submit_ok(state, id), "[DPE-10-03] epoch Ok (fill to 8)");
  }
  check(state.epoch.epoch_history.size() == 8,
        "[DPE-10-03] ring size == 8 at capacity");
  check(state.epoch.epoch_history.front().epoch_id == 1001,
        "[DPE-10-03] oldest is still 1001 before eviction");

  // Submit one more → 1001 evicted.
  check(submit_ok(state, 1009), "[DPE-10-03] 9th epoch Ok");
  check(state.epoch.epoch_history.size() == 8,
        "[DPE-10-03] ring size stays == 8 after eviction");
  check(state.epoch.epoch_history.front().epoch_id == 1002,
        "[DPE-10-03] oldest is now 1002 (1001 evicted)");
  check(state.epoch.epoch_history.back().epoch_id == 1009,
        "[DPE-10-03] newest is 1009");
}

// ── [DPE-10-04] Aborted epoch → ring unchanged ────────────────────────────────

static void test_abort_not_recorded(KernelRuntimeState& state) {
  std::printf("\n[DPE-10-04] Aborted (task-fault) epoch → ring unchanged\n");

  const auto before_size = state.epoch.epoch_history.size();
  const auto eg          = make_trivial_epoch(1099);
  const auto programs    = std::vector<t81::tisc::Program>{make_faulted_program()};
  const auto r = axion_kernel_submit_epoch(state, eg, programs);

  check(r.status == KernelEpochStatus::Aborted_TaskFault,
        "[DPE-10-04] epoch Aborted_TaskFault");
  check(state.epoch.epoch_history.size() == before_size,
        "[DPE-10-04] ring size unchanged after abort");
}

// ── [DPE-10-05] commit_sequence monotonically increasing ──────────────────────

static void test_commit_sequence_monotonic(const KernelRuntimeState& state) {
  std::printf("\n[DPE-10-05] commit_sequence is monotonically increasing\n");

  bool monotonic = true;
  uint64_t prev_seq = 0;
  for (const auto& rec : state.epoch.epoch_history) {
    if (rec.commit_sequence <= prev_seq && prev_seq != 0) {
      monotonic = false;
      break;
    }
    prev_seq = rec.commit_sequence;
  }
  check(monotonic, "[DPE-10-05] commit_sequence monotonically increasing across all ring entries");
}

// ── [DPE-10-06] KernelRuntimeStatusView::epoch_history matches state ──────────

static void test_view_snapshot(const KernelRuntimeState& state) {
  std::printf("\n[DPE-10-06] View snapshot matches state.epoch.epoch_history\n");

  const auto view = make_runtime_view(state);

  check(view.epoch_history.size() == state.epoch.epoch_history.size(),
        "[DPE-10-06] view.epoch_history.size() matches state ring size");

  bool ids_match = true;
  for (std::size_t i = 0; i < view.epoch_history.size(); ++i) {
    if (view.epoch_history[i].epoch_id != state.epoch.epoch_history[i].epoch_id ||
        view.epoch_history[i].commit_sequence != state.epoch.epoch_history[i].commit_sequence) {
      ids_match = false;
      break;
    }
  }
  check(ids_match, "[DPE-10-06] view epoch_id + commit_sequence match state ring entries");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Epoch History Ring tests (RFC-DPE-0009) ===\n");

  const auto ctx = make_test_boot_ctx();
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed\n");
    return 1;
  }
  auto& state = *state_opt;

  test_single_entry(state);
  test_ring_growth(state);
  test_ring_eviction(state);
  test_abort_not_recorded(state);
  test_commit_sequence_monotonic(state);
  test_view_snapshot(state);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
