// experimental/ternaryos/tests/epoch_submission_test.cpp
//
// RFC-DPE-0003 §10 — Kernel EpochRuntimeState wiring conformance tests.
//
// Acceptance criteria covered:
//   [AC-22s-01]  axion_kernel_submit_epoch() increments epoch_submissions
//                and epoch_commits on a successful single-task epoch.
//   [AC-22s-02]  last_committed_epoch_id and last_committed_epoch_hash are
//                set after a successful commit.
//   [AC-22s-03]  make_runtime_view() reflects epoch counters.
//   [AC-22s-04]  A faulted task aborts the epoch; epoch_aborts is incremented
//                and canonical state is unchanged (epoch_commits not incremented).
//   [AC-22s-05]  Consecutive successful epochs increment counters correctly.

#include "../kernel/kernel_epoch.hpp"
#include "../kernel/kernel_main.hpp"
#include "../kernel/kernel_service_contract.hpp"
#include "../hal/hal.hpp"

#include "experimental/dpe/task_graph.hpp"

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

/// Build a minimal valid BootContext for axion_kernel_bootstrap().
static t81::ternaryos::hal::BootContext make_test_boot_ctx() {
  using namespace t81::ternaryos::hal;
  BootContext ctx;
  ctx.platform_id          = "test";
  ctx.ethics_boot_required = false;
  ctx.kernel_load_address  = 0x0000'0000'0800'0000ULL;
  MemoryRegion r;
  r.base    = 0x0000'0000'0000'0000ULL;
  r.length  = 0x0000'0000'1000'0000ULL;  // 256 MiB simulated
  r.writable = true;
  ctx.memory_map.push_back(r);
  return ctx;
}

/// Build a trivial single-task EpochGraph: task has no output regions.
static EpochGraph make_trivial_epoch(uint64_t epoch_id) {
  TaskDescriptor task;
  task.epoch_id = epoch_id;
  task.task_seq = 0;
  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks    = {task};
  return eg;
}

/// Simple program: LoadImm R1 = 7; Halt
static t81::tisc::Program make_trivial_program() {
  t81::tisc::Program p;
  p.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 7},
    {t81::tisc::Opcode::Halt},
  };
  return p;
}

// ── [AC-22s-01/02] Single-task epoch increments counters ─────────────────────

static void test_successful_epoch_increments_counters(KernelRuntimeState& state) {
  std::printf("\n[AC-22s-01/02] Successful single-task epoch updates counters and last_committed fields\n");

  const uint64_t epoch_id = 100;
  const auto epoch   = make_trivial_epoch(epoch_id);
  const auto program = make_trivial_program();

  const auto result = axion_kernel_submit_epoch(state, epoch, {program});

  check(result.ok(), "[AC-22s-01] submit result is Ok");
  check(state.epoch.epochs_submitted == 1,
        "[AC-22s-01] epoch.epochs_submitted == 1");
  check(state.epoch.epochs_committed == 1,
        "[AC-22s-01] epoch.epochs_committed == 1");
  check(state.epoch.epochs_aborted == 0,
        "[AC-22s-01] epoch.epochs_aborted == 0");
  check(state.epoch.epoch_task_executions == 1,
        "[AC-22s-01] epoch.epoch_task_executions == 1");
  check(state.counters.epoch_submissions == 1,
        "[AC-22s-01] counters.epoch_submissions == 1");
  check(state.counters.epoch_commits == 1,
        "[AC-22s-01] counters.epoch_commits == 1");
  check(state.counters.epoch_aborts == 0,
        "[AC-22s-01] counters.epoch_aborts == 0");

  check(state.epoch.last_committed_epoch_id.has_value(),
        "[AC-22s-02] last_committed_epoch_id is set");
  check(state.epoch.last_committed_epoch_id.value_or(0) == epoch_id,
        "[AC-22s-02] last_committed_epoch_id == epoch_id");
  check(state.epoch.last_committed_epoch_hash.has_value(),
        "[AC-22s-02] last_committed_epoch_hash is set");

  // EpochHash must be non-zero.
  const t81::hash::CanonHash81 zero_hash{};
  check(state.epoch.last_committed_epoch_hash.value_or(zero_hash) != zero_hash,
        "[AC-22s-02] last_committed_epoch_hash is non-zero");
}

// ── [AC-22s-03] make_runtime_view() reflects epoch counters ──────────────────

static void test_runtime_view_reflects_epoch_state(KernelRuntimeState& state) {
  std::printf("\n[AC-22s-03] make_runtime_view() reflects epoch counters\n");

  const auto view = make_runtime_view(state);

  check(view.epoch_submissions == state.counters.epoch_submissions,
        "[AC-22s-03] view.epoch_submissions matches counters");
  check(view.epoch_commits == state.counters.epoch_commits,
        "[AC-22s-03] view.epoch_commits matches counters");
  check(view.epoch_aborts == state.counters.epoch_aborts,
        "[AC-22s-03] view.epoch_aborts matches counters");
  check(view.epoch_task_executions == state.counters.epoch_task_executions,
        "[AC-22s-03] view.epoch_task_executions matches counters");
  check(view.last_committed_epoch_id == state.epoch.last_committed_epoch_id,
        "[AC-22s-03] view.last_committed_epoch_id matches epoch state");
  check(view.last_committed_epoch_hash == state.epoch.last_committed_epoch_hash,
        "[AC-22s-03] view.last_committed_epoch_hash matches epoch state");
}

// ── [AC-22s-04] Faulted task aborts epoch ────────────────────────────────────

static void test_faulted_task_aborts_epoch(KernelRuntimeState& state) {
  std::printf("\n[AC-22s-04] Task that does not halt aborts the epoch\n");

  // An empty program (no instructions, not even Halt) will not halt cleanly.
  // The DpeTaskRunner sets halted=false when the VM does not reach Halt.
  // axion_kernel_submit_epoch() treats !halted as faulted.
  const uint64_t epoch_id = 200;
  const auto epoch = make_trivial_epoch(epoch_id);
  t81::tisc::Program bad_program;  // empty: no instructions

  const uint64_t commits_before = state.epoch.epochs_committed;
  const uint64_t aborts_before  = state.epoch.epochs_aborted;

  const auto result = axion_kernel_submit_epoch(state, epoch, {bad_program});

  check(!result.ok(), "[AC-22s-04] submit result is not Ok for faulted task");
  check(state.epoch.epochs_aborted == aborts_before + 1,
        "[AC-22s-04] epoch.epochs_aborted incremented");
  check(state.epoch.epochs_committed == commits_before,
        "[AC-22s-04] epoch.epochs_committed unchanged on abort");
}

// ── [AC-22s-05] Consecutive epochs increment counters correctly ───────────────

static void test_consecutive_epochs_accumulate_counters(KernelRuntimeState& state) {
  std::printf("\n[AC-22s-05] Consecutive successful epochs accumulate counters\n");

  const uint64_t submissions_before = state.epoch.epochs_submitted;
  const uint64_t commits_before     = state.epoch.epochs_committed;

  // Submit two more trivial epochs.
  for (uint64_t eid : {301ULL, 302ULL}) {
    const auto epoch   = make_trivial_epoch(eid);
    const auto program = make_trivial_program();
    const auto res = axion_kernel_submit_epoch(state, epoch, {program});
    check(res.ok(), "consecutive: epoch committed");
  }

  check(state.epoch.epochs_submitted == submissions_before + 2,
        "[AC-22s-05] epochs_submitted incremented by 2");
  check(state.epoch.epochs_committed == commits_before + 2,
        "[AC-22s-05] epochs_committed incremented by 2");

  // last_committed_epoch_id should reflect the last submitted epoch.
  check(state.epoch.last_committed_epoch_id.value_or(0) == 302ULL,
        "[AC-22s-05] last_committed_epoch_id == 302 (most recent epoch)");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Kernel Epoch Submission tests (RFC-DPE-0003 §10) ===\n");

  const auto ctx = make_test_boot_ctx();
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed — cannot run epoch tests\n");
    return 1;
  }
  auto& state = *state_opt;

  test_successful_epoch_increments_counters(state);
  test_runtime_view_reflects_epoch_state(state);
  test_faulted_task_aborts_epoch(state);
  test_consecutive_epochs_accumulate_counters(state);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
