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
  r.base_phys  = 0x0000'0000'0000'0000ULL;
  r.size_bytes = 0x0000'0000'1000'0000ULL;  // 256 MiB simulated
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

static EpochGraph make_three_task_epoch(uint64_t epoch_id) {
  EpochGraph eg;
  eg.epoch_id = epoch_id;
  for (int i = 0; i < 3; ++i) {
    TaskDescriptor task;
    task.epoch_id = epoch_id;
    task.task_seq = static_cast<uint64_t>(i);
    eg.tasks.push_back(task);
  }
  return eg;
}

static std::vector<t81::tisc::Program> make_three_task_programs() {
  std::vector<t81::tisc::Program> programs(3);
  for (int i = 0; i < 3; ++i) {
    programs[i].insns = {
        {t81::tisc::Opcode::LoadImm, 1, static_cast<std::int32_t>(7 + i)},
        {t81::tisc::Opcode::Halt},
    };
  }
  return programs;
}

static constexpr uint64_t kFanOutPageP = 512;
static constexpr uint64_t kFanOutPageQ = 768;
static constexpr uint64_t kFanOutPageR = 1024;

static EpochGraph make_fan_out_epoch(uint64_t epoch_id) {
  TaskDescriptor t0;
  t0.epoch_id = epoch_id;
  t0.task_seq = 0;
  t0.output_regions.push_back(OutputRegion{kFanOutPageP, 1, false});
  const TaskId t0_pid = program_identity(t0);

  TaskDescriptor t1;
  t1.epoch_id = epoch_id;
  t1.task_seq = 1;
  t1.output_regions.push_back(OutputRegion{kFanOutPageQ, 1, false});
  t1.dep_task_ids.push_back(t0_pid);

  TaskDescriptor t2;
  t2.epoch_id = epoch_id;
  t2.task_seq = 2;
  t2.output_regions.push_back(OutputRegion{kFanOutPageR, 1, false});
  t2.dep_task_ids.push_back(t0_pid);

  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks = {t0, t1, t2};
  return eg;
}

static std::vector<t81::tisc::Program> make_fan_out_programs() {
  std::vector<t81::tisc::Program> programs(3);
  programs[0].insns = {
      {t81::tisc::Opcode::LoadImm, 1, 100},
      {t81::tisc::Opcode::Store, static_cast<std::int32_t>(kFanOutPageP), 1},
      {t81::tisc::Opcode::Halt},
  };
  programs[1].insns = {
      {t81::tisc::Opcode::Load, 2, static_cast<std::int32_t>(kFanOutPageP)},
      {t81::tisc::Opcode::LoadImm, 3, 1},
      {t81::tisc::Opcode::Add, 4, 2, 3},
      {t81::tisc::Opcode::Store, static_cast<std::int32_t>(kFanOutPageQ), 4},
      {t81::tisc::Opcode::Halt},
  };
  programs[2].insns = {
      {t81::tisc::Opcode::Load, 5, static_cast<std::int32_t>(kFanOutPageP)},
      {t81::tisc::Opcode::LoadImm, 6, 2},
      {t81::tisc::Opcode::Add, 7, 5, 6},
      {t81::tisc::Opcode::Store, static_cast<std::int32_t>(kFanOutPageR), 7},
      {t81::tisc::Opcode::Halt},
  };
  return programs;
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

  // A program that triggers an immediate BoundsFault trap (Load from invalid
  // address 0x7FFF). run_to_halt() exits via the trap path before setting
  // state_.halted, so the task runner sees halted=false → faulted task.
  // axion_kernel_submit_epoch() treats !halted as faulted.
  const uint64_t epoch_id = 200;
  const auto epoch = make_trivial_epoch(epoch_id);
  t81::tisc::Program bad_program;
  bad_program.insns = {{t81::tisc::Opcode::Load, 1, 0x7FFF}};

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

// ── RFC-0046 / DPE-06 kernel equivalence: bounded pool == unbounded ─────────

static void test_pool_dispatch_matches_unbounded_epoch_hash() {
  std::printf("\n[RFC-0046/DPE-06] Kernel submit: bounded pool matches unbounded dispatch\n");

  const auto ctx = make_test_boot_ctx();
  auto unbounded_state_opt = axion_kernel_bootstrap(ctx);
  auto pooled_state_opt = axion_kernel_bootstrap(ctx);
  check(unbounded_state_opt.has_value(), "[RFC-0046-06] bootstrap unbounded state");
  check(pooled_state_opt.has_value(), "[RFC-0046-07] bootstrap pooled state");
  if (!unbounded_state_opt.has_value() || !pooled_state_opt.has_value()) {
    return;
  }

  auto& unbounded_state = *unbounded_state_opt;
  auto& pooled_state = *pooled_state_opt;

  const auto epoch = make_three_task_epoch(400);
  const auto programs = make_three_task_programs();

  const auto unbounded_result = axion_kernel_submit_epoch(unbounded_state, epoch, programs);

  t81::dpe::DpeThreadPool pool(2);
  const auto pooled_result = axion_kernel_submit_epoch(
      pooled_state, epoch, programs, /*gate=*/{}, &pool, /*timeout_ms=*/std::nullopt);

  check(unbounded_result.ok(), "[RFC-0046-08] unbounded dispatch result ok");
  check(pooled_result.ok(), "[RFC-0046-09] bounded pool dispatch result ok");
  if (!unbounded_result.ok() || !pooled_result.ok()) {
    return;
  }

  check(unbounded_result.epoch_hash == pooled_result.epoch_hash,
        "[RFC-0046-10] epoch hash identical for bounded vs unbounded dispatch");
  check(unbounded_state.epoch.last_committed_epoch_hash == pooled_state.epoch.last_committed_epoch_hash,
        "[RFC-0046-11] retained last committed epoch hash identical");
  check(unbounded_state.epoch.epoch_task_executions == pooled_state.epoch.epoch_task_executions,
        "[RFC-0046-12] task execution counts identical");
  check(unbounded_state.epoch.epochs_committed == pooled_state.epoch.epochs_committed,
        "[RFC-0046-13] commit counters identical");
}

static void test_pool_dispatch_matches_unbounded_on_fan_out_epoch() {
  std::printf("\n[RFC-0046/DPE-06] Kernel submit: bounded pool matches unbounded on fan-out epoch\n");

  const auto ctx = make_test_boot_ctx();
  auto unbounded_state_opt = axion_kernel_bootstrap(ctx);
  auto pooled_state_opt = axion_kernel_bootstrap(ctx);
  check(unbounded_state_opt.has_value(), "[RFC-0046-14] bootstrap unbounded fan-out state");
  check(pooled_state_opt.has_value(), "[RFC-0046-15] bootstrap pooled fan-out state");
  if (!unbounded_state_opt.has_value() || !pooled_state_opt.has_value()) {
    return;
  }

  auto& unbounded_state = *unbounded_state_opt;
  auto& pooled_state = *pooled_state_opt;

  const auto epoch = make_fan_out_epoch(401);
  const auto levels = topological_levels_epoch(epoch);
  check(levels.size() == 2, "[RFC-0046-16] fan-out epoch has two dependency levels");
  check(levels.size() >= 2 && levels[0].size() == 1 && levels[1].size() == 2,
        "[RFC-0046-17] fan-out epoch levels are {1,2}");

  const auto programs = make_fan_out_programs();
  const auto unbounded_result = axion_kernel_submit_epoch(unbounded_state, epoch, programs);

  t81::dpe::DpeThreadPool pool(2);
  const auto pooled_result = axion_kernel_submit_epoch(
      pooled_state, epoch, programs, /*gate=*/{}, &pool, /*timeout_ms=*/std::nullopt);

  check(unbounded_result.ok(), "[RFC-0046-18] unbounded fan-out dispatch result ok");
  check(pooled_result.ok(), "[RFC-0046-19] bounded pool fan-out dispatch result ok");
  if (!unbounded_result.ok() || !pooled_result.ok()) {
    return;
  }

  check(unbounded_result.epoch_hash == pooled_result.epoch_hash,
        "[RFC-0046-20] fan-out epoch hash identical for bounded vs unbounded dispatch");
  check(unbounded_state.epoch.last_committed_epoch_hash ==
            pooled_state.epoch.last_committed_epoch_hash,
        "[RFC-0046-21] retained fan-out committed epoch hash identical");
  check(unbounded_state.epoch.epoch_task_executions == pooled_state.epoch.epoch_task_executions,
        "[RFC-0046-22] fan-out task execution counts identical");
  check(unbounded_state.epoch.epochs_committed == pooled_state.epoch.epochs_committed,
        "[RFC-0046-23] fan-out commit counters identical");
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
  test_pool_dispatch_matches_unbounded_epoch_hash();
  test_pool_dispatch_matches_unbounded_on_fan_out_epoch();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
