// experimental/ternaryos/tests/epoch_syscall_test.cpp
//
// RFC-DPE-0003 §10 / RFC-DPE-0006 §4 — SubmitEpoch KernelCallKind ABI tests.
//
// Acceptance criteria covered:
//   [DPE-07-01]  SubmitEpoch with a valid single-task epoch returns Ok and
//                sets epoch_committed = true with a non-zero epoch_hash.
//   [DPE-07-02]  SubmitEpoch with epoch_graph missing → MissingEpochGraph.
//   [DPE-07-03]  SubmitEpoch with epoch_programs missing → MissingEpochPrograms.
//   [DPE-07-04]  SubmitEpoch with a task that does not halt → EpochTaskFault;
//                epoch_committed remains false.
//   [DPE-07-05]  Consecutive successful SubmitEpoch calls produce distinct
//                epoch_hash values.
//   [DPE-07-06]  Yield syscall succeeds before and after SubmitEpoch (caller
//                context is preserved across epoch boundaries).

#include "../kernel/kernel_abi.hpp"
#include "../kernel/kernel_main.hpp"
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

static t81::ternaryos::hal::BootContext make_test_boot_ctx() {
  using namespace t81::ternaryos::hal;
  BootContext ctx;
  ctx.platform_id          = "test";
  ctx.ethics_boot_required = false;
  ctx.kernel_load_address  = 0x0000'0000'0800'0000ULL;
  MemoryRegion r;
  r.base_phys  = 0x0000'0000'0000'0000ULL;
  r.size_bytes = 0x0000'0000'1000'0000ULL;  // 256 MiB simulated
  r.writable   = true;
  ctx.memory_map.push_back(r);
  return ctx;
}

/// Trivial single-task epoch: no output regions, no dependencies.
static EpochGraph make_trivial_epoch(uint64_t epoch_id) {
  TaskDescriptor task;
  task.epoch_id = epoch_id;
  task.task_seq = 0;
  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks    = {task};
  return eg;
}

/// Valid program: LoadImm R1 = 7; Halt
static t81::tisc::Program make_valid_program() {
  t81::tisc::Program p;
  p.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 7},
    {t81::tisc::Opcode::Halt},
  };
  return p;
}

/// Faulted program: Load from a far-out-of-bounds data address.
/// check_mem() returns false on the first step → BoundsFault trap →
/// run_to_halt() exits immediately with !has_value() → state_.halted stays
/// false → the epoch engine treats this as a task fault.
static t81::tisc::Program make_faulted_program() {
  t81::tisc::Program p;
  // Load R1 from data address 0x7FFF — far outside any valid data segment.
  p.insns = {
    {t81::tisc::Opcode::Load, 1, 0x7FFF},
  };
  return p;
}

// ── [DPE-07-01] Valid SubmitEpoch succeeds ───────────────────────────────────

static void test_submit_epoch_ok(KernelRuntimeState& state) {
  std::printf("\n[DPE-07-01] Valid SubmitEpoch returns Ok with epoch_committed and hash\n");

  KernelCallRequest req;
  req.kind           = KernelCallKind::SubmitEpoch;
  req.epoch_graph    = make_trivial_epoch(1001);
  req.epoch_programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto result = axion_kernel_call(state, req);

  check(result.status == KernelCallStatus::Ok,
        "[DPE-07-01] status == Ok");
  check(result.rejection == KernelCallRejection::None,
        "[DPE-07-01] rejection == None");
  check(result.action_performed,
        "[DPE-07-01] action_performed is true");
  check(result.epoch_committed,
        "[DPE-07-01] epoch_committed is true");
  check(result.epoch_hash.has_value(),
        "[DPE-07-01] epoch_hash is set");

  const t81::hash::CanonHash81 zero{};
  check(result.epoch_hash.value_or(zero) != zero,
        "[DPE-07-01] epoch_hash is non-zero");
}

// ── [DPE-07-02] Missing epoch_graph → MissingEpochGraph ──────────────────────

static void test_missing_epoch_graph(KernelRuntimeState& state) {
  std::printf("\n[DPE-07-02] SubmitEpoch without epoch_graph → MissingEpochGraph\n");

  KernelCallRequest req;
  req.kind           = KernelCallKind::SubmitEpoch;
  // epoch_graph intentionally absent
  req.epoch_programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto result = axion_kernel_call(state, req);

  check(result.status == KernelCallStatus::InvalidRequest,
        "[DPE-07-02] status == InvalidRequest");
  check(result.rejection == KernelCallRejection::MissingEpochGraph,
        "[DPE-07-02] rejection == MissingEpochGraph");
  check(!result.epoch_committed,
        "[DPE-07-02] epoch_committed is false");
}

// ── [DPE-07-03] Missing epoch_programs → MissingEpochPrograms ────────────────

static void test_missing_epoch_programs(KernelRuntimeState& state) {
  std::printf("\n[DPE-07-03] SubmitEpoch without epoch_programs → MissingEpochPrograms\n");

  KernelCallRequest req;
  req.kind        = KernelCallKind::SubmitEpoch;
  req.epoch_graph = make_trivial_epoch(1002);
  // epoch_programs intentionally absent

  const auto result = axion_kernel_call(state, req);

  check(result.status == KernelCallStatus::InvalidRequest,
        "[DPE-07-03] status == InvalidRequest");
  check(result.rejection == KernelCallRejection::MissingEpochPrograms,
        "[DPE-07-03] rejection == MissingEpochPrograms");
  check(!result.epoch_committed,
        "[DPE-07-03] epoch_committed is false");
}

// ── [DPE-07-04] Faulted task → EpochTaskFault ────────────────────────────────

static void test_faulted_task_yields_epoch_task_fault(KernelRuntimeState& state) {
  std::printf("\n[DPE-07-04] SubmitEpoch with non-halting task → EpochTaskFault\n");

  KernelCallRequest req;
  req.kind           = KernelCallKind::SubmitEpoch;
  req.epoch_graph    = make_trivial_epoch(1003);
  req.epoch_programs = std::vector<t81::tisc::Program>{make_faulted_program()};

  const auto result = axion_kernel_call(state, req);

  check(result.status == KernelCallStatus::InvalidRequest,
        "[DPE-07-04] status == InvalidRequest");
  check(result.rejection == KernelCallRejection::EpochTaskFault,
        "[DPE-07-04] rejection == EpochTaskFault");
  check(!result.epoch_committed,
        "[DPE-07-04] epoch_committed is false on task fault");
  check(!result.epoch_hash.has_value(),
        "[DPE-07-04] epoch_hash absent on task fault");
}

// ── [DPE-07-05] Consecutive epochs produce distinct hashes ───────────────────

static void test_consecutive_epochs_distinct_hashes(KernelRuntimeState& state) {
  std::printf("\n[DPE-07-05] Consecutive SubmitEpoch calls produce distinct epoch hashes\n");

  std::optional<t81::hash::CanonHash81> prev_hash;

  for (uint64_t eid : {2001ULL, 2002ULL, 2003ULL}) {
    KernelCallRequest req;
    req.kind           = KernelCallKind::SubmitEpoch;
    req.epoch_graph    = make_trivial_epoch(eid);
    req.epoch_programs = std::vector<t81::tisc::Program>{make_valid_program()};

    const auto result = axion_kernel_call(state, req);

    check(result.status == KernelCallStatus::Ok,
          "[DPE-07-05] consecutive epoch Ok");
    check(result.epoch_committed,
          "[DPE-07-05] consecutive epoch_committed");

    if (prev_hash.has_value() && result.epoch_hash.has_value()) {
      check(*result.epoch_hash != *prev_hash,
            "[DPE-07-05] consecutive epoch hashes are distinct");
    }
    prev_hash = result.epoch_hash;
  }
}

// ── [DPE-07-06] Yield works before and after SubmitEpoch ─────────────────────

static void test_yield_around_submit_epoch(KernelRuntimeState& state) {
  std::printf("\n[DPE-07-06] Yield syscall succeeds before and after SubmitEpoch\n");

  // Yield before epoch submission.
  {
    KernelCallRequest req;
    req.kind = KernelCallKind::Yield;
    const auto result = axion_kernel_call(state, req);
    check(result.status == KernelCallStatus::Ok,
          "[DPE-07-06] Yield before SubmitEpoch returns Ok");
  }

  // Submit an epoch.
  {
    KernelCallRequest req;
    req.kind           = KernelCallKind::SubmitEpoch;
    req.epoch_graph    = make_trivial_epoch(3001);
    req.epoch_programs = std::vector<t81::tisc::Program>{make_valid_program()};
    const auto result = axion_kernel_call(state, req);
    check(result.epoch_committed,
          "[DPE-07-06] SubmitEpoch epoch_committed");
  }

  // Yield after epoch submission — caller context must still be valid.
  {
    KernelCallRequest req;
    req.kind = KernelCallKind::Yield;
    const auto result = axion_kernel_call(state, req);
    check(result.status == KernelCallStatus::Ok,
          "[DPE-07-06] Yield after SubmitEpoch returns Ok");
  }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== SubmitEpoch KernelCallKind ABI tests (RFC-DPE-0003/0006) ===\n");

  const auto ctx     = make_test_boot_ctx();
  auto state_opt     = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed\n");
    return 1;
  }
  auto& state = *state_opt;

  // Establish a current caller context: one tick makes kKernelTid current in
  // the scheduler so axion_kernel_call() can find the caller thread.
  axion_kernel_tick(state);

  test_submit_epoch_ok(state);
  test_missing_epoch_graph(state);
  test_missing_epoch_programs(state);
  test_faulted_task_yields_epoch_task_fault(state);
  test_consecutive_epochs_distinct_hashes(state);
  test_yield_around_submit_epoch(state);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
