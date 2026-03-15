// experimental/ternaryos/tests/epoch_timeout_test.cpp
//
// RFC-DPE-0007 — Epoch Execution Timeout tests.
//
// Acceptance criteria covered:
//   [DPE-08-01]  axion_kernel_submit_epoch() with timeout_ms = std::nullopt
//                behaves identically to the RFC-DPE-0006 baseline (no regression).
//   [DPE-08-02]  axion_kernel_submit_epoch() with a generous timeout (5 000 ms)
//                completes with KernelEpochStatus::Ok.
//   [DPE-08-03]  axion_kernel_submit_epoch() with timeout_ms = 0ms returns
//                KernelEpochStatus::Aborted_Timeout (post-level check fires
//                because 0 ms elapsed >= 0 ms budget).
//   [DPE-08-04]  state.epoch.epochs_aborted and state.counters.epoch_aborts are
//                each incremented by 1 on Aborted_Timeout.
//   [DPE-08-05]  KernelCallKind::SubmitEpoch with epoch_timeout_ms = 0ms returns
//                KernelCallStatus::RetryLater with rejection == EpochTimedOut.
//   [DPE-08-06]  epoch_committed is false when timeout fires.

#include "../kernel/kernel_abi.hpp"
#include "../kernel/kernel_epoch.hpp"
#include "../kernel/kernel_main.hpp"
#include "../hal/hal.hpp"

#include "experimental/dpe/task_graph.hpp"

#include "t81/isa/program.hpp"
#include "t81/isa/opcodes.hpp"

#include <cassert>
#include <chrono>
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

/// Valid program: LoadImm R1 = 42; Halt.
static t81::tisc::Program make_valid_program() {
  t81::tisc::Program p;
  p.insns = {
    {t81::tisc::Opcode::LoadImm, 1, 42},
    {t81::tisc::Opcode::Halt},
  };
  return p;
}

// ── [DPE-08-01] No timeout → baseline behaviour ───────────────────────────────

static void test_no_timeout(KernelRuntimeState& state) {
  std::printf("\n[DPE-08-01] No timeout (std::nullopt) → baseline Ok\n");

  const auto eg       = make_trivial_epoch(801);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto before_commits = state.epoch.epochs_committed;
  const auto r = axion_kernel_submit_epoch(state, eg, programs,
                                           /*gate=*/{}, /*pool=*/nullptr,
                                           /*timeout_ms=*/std::nullopt);
  check(r.status == KernelEpochStatus::Ok,
        "[DPE-08-01] status == Ok (no timeout)");
  check(state.epoch.epochs_committed == before_commits + 1,
        "[DPE-08-01] epochs_committed incremented");
}

// ── [DPE-08-02] Generous timeout → completes Ok ───────────────────────────────

static void test_generous_timeout(KernelRuntimeState& state) {
  std::printf("\n[DPE-08-02] Generous timeout (5 000 ms) → Ok\n");

  const auto eg       = make_trivial_epoch(802);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto r = axion_kernel_submit_epoch(state, eg, programs,
                                           /*gate=*/{}, /*pool=*/nullptr,
                                           std::chrono::milliseconds{5000});
  check(r.status == KernelEpochStatus::Ok,
        "[DPE-08-02] status == Ok (5 000 ms budget)");
}

// ── [DPE-08-03/04] Zero timeout → Aborted_Timeout + counters ──────────────────

static void test_zero_timeout(KernelRuntimeState& state) {
  std::printf("\n[DPE-08-03/04] Zero timeout (0 ms) → Aborted_Timeout + counters\n");

  const auto eg       = make_trivial_epoch(803);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto before_aborted  = state.epoch.epochs_aborted;
  const auto before_c_aborts = state.counters.epoch_aborts;

  const auto r = axion_kernel_submit_epoch(state, eg, programs,
                                           /*gate=*/{}, /*pool=*/nullptr,
                                           std::chrono::milliseconds{0});
  check(r.status == KernelEpochStatus::Aborted_Timeout,
        "[DPE-08-03] status == Aborted_Timeout");
  check(state.epoch.epochs_aborted == before_aborted + 1,
        "[DPE-08-04] epochs_aborted incremented");
  check(state.counters.epoch_aborts == before_c_aborts + 1,
        "[DPE-08-04] counters.epoch_aborts incremented");
}

// ── [DPE-08-05/06] KernelCallKind::SubmitEpoch with 0 ms → RetryLater/EpochTimedOut

static void test_syscall_timeout(KernelRuntimeState& state) {
  std::printf("\n[DPE-08-05/06] SubmitEpoch syscall with 0 ms → RetryLater / EpochTimedOut\n");

  const auto eg       = make_trivial_epoch(804);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};

  KernelCallRequest req;
  req.kind             = KernelCallKind::SubmitEpoch;
  req.epoch_graph      = eg;
  req.epoch_programs   = programs;
  req.epoch_timeout_ms = std::chrono::milliseconds{0};

  const auto result = axion_kernel_call(state, req);

  check(result.status    == KernelCallStatus::RetryLater,
        "[DPE-08-05] status == RetryLater");
  check(result.rejection == KernelCallRejection::EpochTimedOut,
        "[DPE-08-05] rejection == EpochTimedOut");
  check(!result.epoch_committed,
        "[DPE-08-06] epoch_committed == false on timeout");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Epoch Execution Timeout tests (RFC-DPE-0007) ===\n");

  const auto ctx = make_test_boot_ctx();
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed\n");
    return 1;
  }
  auto& state = *state_opt;

  // One tick to establish kKernelTid as current caller context.
  axion_kernel_tick(state);

  test_no_timeout(state);
  test_generous_timeout(state);
  test_zero_timeout(state);
  test_syscall_timeout(state);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
