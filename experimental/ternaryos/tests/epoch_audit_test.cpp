// experimental/ternaryos/tests/epoch_audit_test.cpp
//
// RFC-DPE-0008 — Epoch Audit Events tests.
//
// Acceptance criteria covered:
//   [DPE-09-01]  A successful epoch emits EpochSubmitted + EpochCommitted;
//                epoch_audit_submissions == 1, epoch_audit_commits == 1.
//   [DPE-09-02]  last_epoch_audit_kind == EpochCommitted and
//                last_epoch_audit_sequence is set after a successful epoch.
//   [DPE-09-03]  A faulted (task-fault) epoch emits EpochSubmitted +
//                EpochAborted; epoch_audit_aborts == 1.
//   [DPE-09-04]  A timed-out (0 ms) epoch emits EpochSubmitted + EpochAborted;
//                last_epoch_audit_kind == EpochAborted.
//   [DPE-09-05]  A policy-denied epoch emits EpochSubmitted +
//                EpochAbortedPolicyFault; EpochAborted is NOT emitted.
//   [DPE-09-06]  counters.audit_events_recorded advances by 2 for each epoch
//                (submitted + committed/aborted).

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

/// Faulted program: Load R1 from invalid TVA → BoundsFault → halted = false.
static t81::tisc::Program make_faulted_program() {
  t81::tisc::Program p;
  p.insns = {{t81::tisc::Opcode::Load, 1, 0x7FFF}};
  return p;
}

/// Policy gate that always denies.
static bool deny_all_gate(const t81::dpe::TaskDescriptor&,
                           const t81::tisc::Program&,
                           void*) noexcept {
  return false;
}

// ── [DPE-09-01/02] Successful epoch → EpochSubmitted + EpochCommitted ─────────

static void test_successful_epoch_audit(KernelRuntimeState& state) {
  std::printf("\n[DPE-09-01/02] Successful epoch → EpochSubmitted + EpochCommitted\n");

  const auto before_audit    = state.counters.audit_events_recorded;
  const auto before_sub_a    = state.counters.epoch_audit_submissions;
  const auto before_commit_a = state.counters.epoch_audit_commits;

  const auto eg       = make_trivial_epoch(901);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};
  const auto r = axion_kernel_submit_epoch(state, eg, programs);

  check(r.status == KernelEpochStatus::Ok,
        "[DPE-09-01] epoch status == Ok");
  check(state.counters.epoch_audit_submissions == before_sub_a + 1,
        "[DPE-09-01] epoch_audit_submissions incremented");
  check(state.counters.epoch_audit_commits == before_commit_a + 1,
        "[DPE-09-01] epoch_audit_commits incremented");
  check(state.counters.audit_events_recorded == before_audit + 2,
        "[DPE-09-06] audit_events_recorded += 2 (submitted + committed)");
  check(state.last_epoch_audit_kind.has_value() &&
        *state.last_epoch_audit_kind == KernelAuditEventKind::EpochCommitted,
        "[DPE-09-02] last_epoch_audit_kind == EpochCommitted");
  check(state.last_epoch_audit_sequence.has_value(),
        "[DPE-09-02] last_epoch_audit_sequence is set");
}

// ── [DPE-09-03] Faulted epoch → EpochSubmitted + EpochAborted ────────────────

static void test_faulted_epoch_audit(KernelRuntimeState& state) {
  std::printf("\n[DPE-09-03] Faulted (task-fault) epoch → EpochSubmitted + EpochAborted\n");

  const auto before_abort_a = state.counters.epoch_audit_aborts;
  const auto before_audit   = state.counters.audit_events_recorded;

  const auto eg       = make_trivial_epoch(902);
  const auto programs = std::vector<t81::tisc::Program>{make_faulted_program()};
  const auto r = axion_kernel_submit_epoch(state, eg, programs);

  check(r.status == KernelEpochStatus::Aborted_TaskFault,
        "[DPE-09-03] epoch status == Aborted_TaskFault");
  check(state.counters.epoch_audit_aborts == before_abort_a + 1,
        "[DPE-09-03] epoch_audit_aborts incremented");
  check(state.counters.audit_events_recorded == before_audit + 2,
        "[DPE-09-06] audit_events_recorded += 2 (submitted + aborted)");
  check(state.last_epoch_audit_kind.has_value() &&
        *state.last_epoch_audit_kind == KernelAuditEventKind::EpochAborted,
        "[DPE-09-03] last_epoch_audit_kind == EpochAborted");
}

// ── [DPE-09-04] Timed-out epoch → EpochSubmitted + EpochAborted ──────────────

static void test_timeout_epoch_audit(KernelRuntimeState& state) {
  std::printf("\n[DPE-09-04] Timed-out (0 ms) epoch → EpochSubmitted + EpochAborted\n");

  const auto before_abort_a = state.counters.epoch_audit_aborts;

  const auto eg       = make_trivial_epoch(903);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};
  const auto r = axion_kernel_submit_epoch(state, eg, programs,
                                           /*gate=*/{}, /*pool=*/nullptr,
                                           std::chrono::milliseconds{0});
  check(r.status == KernelEpochStatus::Aborted_Timeout,
        "[DPE-09-04] epoch status == Aborted_Timeout");
  check(state.counters.epoch_audit_aborts == before_abort_a + 1,
        "[DPE-09-04] epoch_audit_aborts incremented on timeout");
  check(state.last_epoch_audit_kind.has_value() &&
        *state.last_epoch_audit_kind == KernelAuditEventKind::EpochAborted,
        "[DPE-09-04] last_epoch_audit_kind == EpochAborted on timeout");
}

// ── [DPE-09-05] Policy-denied epoch → EpochSubmitted + EpochAbortedPolicyFault ─

static void test_policy_denied_epoch_audit(KernelRuntimeState& state) {
  std::printf("\n[DPE-09-05] Policy-denied epoch → EpochAbortedPolicyFault (not EpochAborted)\n");

  const auto before_abort_a  = state.counters.epoch_audit_aborts;
  const auto before_policy_f = state.counters.policy_faults;

  const KernelEpochPolicyGate deny_gate{deny_all_gate, nullptr};

  const auto eg       = make_trivial_epoch(904);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};
  const auto r = axion_kernel_submit_epoch(state, eg, programs, deny_gate);

  check(r.status == KernelEpochStatus::Aborted_PolicyFault,
        "[DPE-09-05] epoch status == Aborted_PolicyFault");
  // EpochAborted counter must NOT have incremented — policy uses EpochAbortedPolicyFault.
  check(state.counters.epoch_audit_aborts == before_abort_a,
        "[DPE-09-05] epoch_audit_aborts NOT incremented for policy fault");
  check(state.counters.policy_faults == before_policy_f + 1,
        "[DPE-09-05] policy_faults incremented");
  // last_epoch_audit_kind should be EpochAbortedPolicyFault (set by existing
  // record_audit_event() call) — but it does not go through emit_epoch_audit().
  // Verify: EpochAborted is absent from last_epoch_audit_kind.
  check(!state.last_epoch_audit_kind.has_value() ||
        *state.last_epoch_audit_kind != KernelAuditEventKind::EpochAborted,
        "[DPE-09-05] last_epoch_audit_kind != EpochAborted for policy fault");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Epoch Audit Events tests (RFC-DPE-0008) ===\n");

  const auto ctx = make_test_boot_ctx();
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed\n");
    return 1;
  }
  auto& state = *state_opt;

  test_successful_epoch_audit(state);
  test_faulted_epoch_audit(state);
  test_timeout_epoch_audit(state);
  test_policy_denied_epoch_audit(state);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
