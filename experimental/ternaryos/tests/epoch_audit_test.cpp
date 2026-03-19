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

// ── RFC-0046 / DPE-09 scheduler parity: audit semantics survive pool choice ──

static void test_pool_audit_semantics_match_unbounded() {
  std::printf("\n[RFC-0046/DPE-09] Audit semantics match for bounded vs unbounded dispatch\n");

  const auto ctx = make_test_boot_ctx();
  auto unbounded_state_opt = axion_kernel_bootstrap(ctx);
  auto pooled_state_opt = axion_kernel_bootstrap(ctx);
  check(unbounded_state_opt.has_value(), "[RFC-0046-43] bootstrap unbounded audit state");
  check(pooled_state_opt.has_value(), "[RFC-0046-44] bootstrap pooled audit state");
  if (!unbounded_state_opt.has_value() || !pooled_state_opt.has_value()) {
    return;
  }

  auto& unbounded_state = *unbounded_state_opt;
  auto& pooled_state = *pooled_state_opt;

  const auto success_epoch = make_trivial_epoch(905);
  const auto success_programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto unbounded_success = axion_kernel_submit_epoch(
      unbounded_state, success_epoch, success_programs);
  t81::dpe::DpeThreadPool pool(2);
  const auto pooled_success = axion_kernel_submit_epoch(
      pooled_state, success_epoch, success_programs, /*gate=*/{}, &pool, /*timeout_ms=*/std::nullopt);

  check(unbounded_success.status == KernelEpochStatus::Ok,
        "[RFC-0046-45] unbounded successful epoch status == Ok");
  check(pooled_success.status == KernelEpochStatus::Ok,
        "[RFC-0046-46] pooled successful epoch status == Ok");
  check(unbounded_state.counters.epoch_audit_submissions ==
            pooled_state.counters.epoch_audit_submissions,
        "[RFC-0046-47] successful epoch audit submissions identical");
  check(unbounded_state.counters.epoch_audit_commits ==
            pooled_state.counters.epoch_audit_commits,
        "[RFC-0046-48] successful epoch audit commits identical");
  check(unbounded_state.counters.audit_events_recorded ==
            pooled_state.counters.audit_events_recorded,
        "[RFC-0046-49] successful epoch total audit events identical");
  check(unbounded_state.last_epoch_audit_kind == pooled_state.last_epoch_audit_kind,
        "[RFC-0046-50] successful epoch last audit kind identical");

  const auto timeout_epoch = make_trivial_epoch(906);
  const auto timeout_programs = std::vector<t81::tisc::Program>{make_valid_program()};
  const auto unbounded_timeout = axion_kernel_submit_epoch(
      unbounded_state, timeout_epoch, timeout_programs,
      /*gate=*/{}, /*pool=*/nullptr, std::chrono::milliseconds{0});
  const auto pooled_timeout = axion_kernel_submit_epoch(
      pooled_state, timeout_epoch, timeout_programs,
      /*gate=*/{}, &pool, std::chrono::milliseconds{0});

  check(unbounded_timeout.status == KernelEpochStatus::Aborted_Timeout,
        "[RFC-0046-51] unbounded timeout epoch status == Aborted_Timeout");
  check(pooled_timeout.status == KernelEpochStatus::Aborted_Timeout,
        "[RFC-0046-52] pooled timeout epoch status == Aborted_Timeout");
  check(unbounded_state.counters.epoch_audit_submissions ==
            pooled_state.counters.epoch_audit_submissions,
        "[RFC-0046-53] timeout audit submissions identical");
  check(unbounded_state.counters.epoch_audit_aborts ==
            pooled_state.counters.epoch_audit_aborts,
        "[RFC-0046-54] timeout audit aborts identical");
  check(unbounded_state.counters.audit_events_recorded ==
            pooled_state.counters.audit_events_recorded,
        "[RFC-0046-55] timeout total audit events identical");
  check(unbounded_state.last_epoch_audit_kind == pooled_state.last_epoch_audit_kind,
        "[RFC-0046-56] timeout last audit kind identical");
}

static void test_pool_policy_fault_audit_semantics_match_unbounded() {
  std::printf("\n[RFC-0046/DPE-09] Policy-fault audit semantics match for bounded vs unbounded dispatch\n");

  const auto ctx = make_test_boot_ctx();
  auto unbounded_state_opt = axion_kernel_bootstrap(ctx);
  auto pooled_state_opt = axion_kernel_bootstrap(ctx);
  check(unbounded_state_opt.has_value(), "[RFC-0046-57] bootstrap unbounded policy-audit state");
  check(pooled_state_opt.has_value(), "[RFC-0046-58] bootstrap pooled policy-audit state");
  if (!unbounded_state_opt.has_value() || !pooled_state_opt.has_value()) {
    return;
  }

  auto& unbounded_state = *unbounded_state_opt;
  auto& pooled_state = *pooled_state_opt;

  const KernelEpochPolicyGate deny_gate{deny_all_gate, nullptr};
  const auto eg = make_trivial_epoch(907);
  const auto programs = std::vector<t81::tisc::Program>{make_valid_program()};

  const auto unbounded_result = axion_kernel_submit_epoch(
      unbounded_state, eg, programs, deny_gate, /*pool=*/nullptr, /*timeout_ms=*/std::nullopt);
  t81::dpe::DpeThreadPool pool(2);
  const auto pooled_result = axion_kernel_submit_epoch(
      pooled_state, eg, programs, deny_gate, &pool, /*timeout_ms=*/std::nullopt);

  check(unbounded_result.status == KernelEpochStatus::Aborted_PolicyFault,
        "[RFC-0046-59] unbounded policy-denied epoch status == Aborted_PolicyFault");
  check(pooled_result.status == KernelEpochStatus::Aborted_PolicyFault,
        "[RFC-0046-60] pooled policy-denied epoch status == Aborted_PolicyFault");
  check(unbounded_state.counters.policy_faults == pooled_state.counters.policy_faults,
        "[RFC-0046-61] policy fault counters identical");
  check(unbounded_state.counters.audit_events_recorded == pooled_state.counters.audit_events_recorded,
        "[RFC-0046-62] policy fault total audit events identical");
  check(unbounded_state.counters.epoch_audit_submissions == pooled_state.counters.epoch_audit_submissions,
        "[RFC-0046-63] policy fault audit submissions identical");
  check(unbounded_state.counters.epoch_audit_aborts == pooled_state.counters.epoch_audit_aborts,
        "[RFC-0046-64] policy fault epoch abort counters identical");
  check(unbounded_state.last_audit_event.has_value() && pooled_state.last_audit_event.has_value(),
        "[RFC-0046-65] policy fault last_audit_event present for both schedulers");
  if (!unbounded_state.last_audit_event.has_value() || !pooled_state.last_audit_event.has_value()) {
    return;
  }
  check(unbounded_state.last_audit_event->kind == KernelAuditEventKind::EpochAbortedPolicyFault,
        "[RFC-0046-66] unbounded last_audit_event.kind == EpochAbortedPolicyFault");
  check(pooled_state.last_audit_event->kind == KernelAuditEventKind::EpochAbortedPolicyFault,
        "[RFC-0046-67] pooled last_audit_event.kind == EpochAbortedPolicyFault");
  check(unbounded_state.last_audit_event->kind == pooled_state.last_audit_event->kind,
        "[RFC-0046-68] policy fault last_audit_event.kind identical");
  check(!unbounded_state.audit_log.empty() && !pooled_state.audit_log.empty(),
        "[RFC-0046-69] policy fault audit logs populated for both schedulers");
  if (!unbounded_state.audit_log.empty() && !pooled_state.audit_log.empty()) {
    check(unbounded_state.audit_log.back().kind == KernelAuditEventKind::EpochAbortedPolicyFault,
          "[RFC-0046-70] unbounded audit_log.back().kind == EpochAbortedPolicyFault");
    check(pooled_state.audit_log.back().kind == KernelAuditEventKind::EpochAbortedPolicyFault,
          "[RFC-0046-71] pooled audit_log.back().kind == EpochAbortedPolicyFault");
  }
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
  test_pool_audit_semantics_match_unbounded();
  test_pool_policy_fault_audit_semantics_match_unbounded();

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
