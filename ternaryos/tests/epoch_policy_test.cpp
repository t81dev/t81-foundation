// experimental/ternaryos/tests/epoch_policy_test.cpp
//
// RFC-DPE-0003 §6.1 — [DPE-03-06] Policy fault during task execution aborts
// the epoch and records the event in the Axion audit log.
//
// Acceptance criteria covered:
//   [DPE-03-06-01]  A KernelEpochPolicyGate that denies a task causes
//                   axion_kernel_submit_epoch() to return Aborted_PolicyFault.
//   [DPE-03-06-02]  The audit log contains an EpochAbortedPolicyFault record
//                   after a policy-denied epoch.
//   [DPE-03-06-03]  state.counters.policy_faults is incremented.
//   [DPE-03-06-04]  epoch_commits is NOT incremented on a policy abort.
//   [DPE-03-06-05]  A gate that allows all tasks does not affect normal execution.
//   [DPE-03-06-06]  Gate is evaluated once per task; first-task denial aborts
//                   immediately without running later tasks.

#include "../hal/hal.hpp"
#include "../kernel/kernel_epoch.hpp"
#include "../kernel/kernel_main.hpp"
#include "../kernel/kernel_service_contract.hpp"

#include "t81/dpe/task_graph.hpp"

#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

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
  ctx.platform_id = "test";
  ctx.ethics_boot_required = false;
  ctx.kernel_load_address = 0x0000'0000'0800'0000ULL;
  MemoryRegion r;
  r.base_phys = 0;
  r.size_bytes = 0x0000'0000'1000'0000ULL;
  r.writable = true;
  ctx.memory_map.push_back(r);
  return ctx;
}

static EpochGraph make_two_task_epoch(uint64_t epoch_id) {
  TaskDescriptor t0;
  t0.epoch_id = epoch_id;
  t0.task_seq = 0;
  TaskDescriptor t1;
  t1.epoch_id = epoch_id;
  t1.task_seq = 1;
  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks = {t0, t1};
  return eg;
}

static EpochGraph make_reversed_dependency_epoch(uint64_t epoch_id) {
  TaskDescriptor t0;
  t0.epoch_id = epoch_id;
  t0.task_seq = 0;

  TaskDescriptor t1;
  t1.epoch_id = epoch_id;
  t1.task_seq = 1;
  t1.dep_task_ids.push_back(program_identity(t0));

  EpochGraph eg;
  eg.epoch_id = epoch_id;
  // Intentionally reversed storage order; scheduling must still respect dependency order.
  eg.tasks = {t1, t0};
  return eg;
}

static t81::tisc::Program make_trivial_program() {
  t81::tisc::Program p;
  p.insns = {
      {t81::tisc::Opcode::LoadImm, 1, 99},
      {t81::tisc::Opcode::Halt},
  };
  return p;
}

// Gate context: track how many times the gate was called.
struct GateContext {
  int calls{0};
  int deny_task_seq{-1};  ///< task_seq to deny; -1 = allow all
  std::vector<uint64_t> seen_task_seq{};
};

static bool counting_gate_fn(const TaskDescriptor& task, const t81::tisc::Program& /*prog*/,
                             void* user_data) noexcept {
  auto* ctx = static_cast<GateContext*>(user_data);
  ++ctx->calls;
  ctx->seen_task_seq.push_back(task.task_seq);
  if (ctx->deny_task_seq >= 0 && static_cast<int>(task.task_seq) == ctx->deny_task_seq) {
    return false;  // deny
  }
  return true;  // allow
}

// ── [DPE-03-06-01/02/03/04] Policy-denied first task ─────────────────────────

static void test_policy_denial_aborts_epoch(KernelRuntimeState& state) {
  std::printf("\n[DPE-03-06-01/02/03/04] Policy gate denial aborts epoch and records audit\n");

  const uint64_t epoch_id = 500;
  const auto epoch = make_two_task_epoch(epoch_id);
  const auto program = make_trivial_program();

  GateContext ctx;
  ctx.deny_task_seq = 0;  // deny first task

  KernelEpochPolicyGate gate;
  gate.fn = counting_gate_fn;
  gate.user_data = &ctx;

  const uint64_t commits_before = state.epoch.epochs_committed;
  const uint64_t policy_faults_before = state.counters.policy_faults;
  const std::size_t audit_log_before = state.audit_log.size();

  const auto result = axion_kernel_submit_epoch(state, epoch, {program, program}, gate);

  check(result.status == KernelEpochStatus::Aborted_PolicyFault,
        "[DPE-03-06-01] status == Aborted_PolicyFault");
  check(!result.ok(), "[DPE-03-06-01] result.ok() == false");

  // Audit log records both EpochSubmitted and EpochAbortedPolicyFault.
  check(state.audit_log.size() == audit_log_before + 2, "[DPE-03-06-02] audit_log grew by 2");
  check(state.audit_log[state.audit_log.size() - 2].kind == KernelAuditEventKind::EpochSubmitted,
        "[DPE-03-06-02] penultimate audit record is EpochSubmitted");
  const bool audit_kind_correct =
      !state.audit_log.empty() &&
      state.audit_log.back().kind == KernelAuditEventKind::EpochAbortedPolicyFault;
  check(audit_kind_correct, "[DPE-03-06-02] audit_log.back().kind == EpochAbortedPolicyFault");

  // last_audit_event must reflect the policy fault.
  check(state.last_audit_event.has_value(), "[DPE-03-06-02] last_audit_event is set");
  check(state.last_audit_event.value_or(KernelAuditRecord{}).kind ==
            KernelAuditEventKind::EpochAbortedPolicyFault,
        "[DPE-03-06-02] last_audit_event.kind == EpochAbortedPolicyFault");

  check(state.counters.policy_faults == policy_faults_before + 1,
        "[DPE-03-06-03] counters.policy_faults incremented");

  check(state.epoch.epochs_committed == commits_before,
        "[DPE-03-06-04] epochs_committed unchanged on policy abort");
}

// ── [DPE-03-06-05] Allow-all gate does not affect normal execution ────────────

static void test_allow_all_gate_normal_execution(KernelRuntimeState& state) {
  std::printf("\n[DPE-03-06-05] Allow-all gate does not affect normal execution\n");

  const uint64_t epoch_id = 501;
  const auto epoch = make_two_task_epoch(epoch_id);
  const auto program = make_trivial_program();

  GateContext ctx;
  ctx.deny_task_seq = -1;  // allow all

  KernelEpochPolicyGate gate;
  gate.fn = counting_gate_fn;
  gate.user_data = &ctx;

  const uint64_t commits_before = state.epoch.epochs_committed;

  const auto result = axion_kernel_submit_epoch(state, epoch, {program, program}, gate);

  check(result.ok(), "[DPE-03-06-05] allow-all gate: result ok");
  check(state.epoch.epochs_committed == commits_before + 1,
        "[DPE-03-06-05] allow-all gate: epochs_committed incremented");
  check(ctx.calls == 2, "[DPE-03-06-05] gate was called once per task (2 tasks)");
}

// ── [DPE-03-06-06] First-task denial aborts immediately (second not run) ──────

static void test_first_task_denial_short_circuits(KernelRuntimeState& state) {
  std::printf("\n[DPE-03-06-06] First-task denial aborts without evaluating later tasks\n");

  const uint64_t epoch_id = 502;
  const auto epoch = make_two_task_epoch(epoch_id);
  const auto program = make_trivial_program();

  GateContext ctx;
  ctx.deny_task_seq = 0;  // deny the first task

  KernelEpochPolicyGate gate;
  gate.fn = counting_gate_fn;
  gate.user_data = &ctx;

  const auto result = axion_kernel_submit_epoch(state, epoch, {program, program}, gate);

  check(result.status == KernelEpochStatus::Aborted_PolicyFault,
        "[DPE-03-06-06] aborted on first task");
  // Gate should have been called exactly once — abort fired before task 1.
  check(ctx.calls == 1, "[DPE-03-06-06] gate called exactly once (first-task short-circuit)");
}

// ── [DPE-03-06] Null gate is equivalent to allow-all ─────────────────────────

static void test_null_gate_allows_all(KernelRuntimeState& state) {
  std::printf("\n[DPE-03-06] Null gate (default) is equivalent to allow-all\n");

  const uint64_t epoch_id = 503;

  TaskDescriptor t0;
  t0.epoch_id = epoch_id;
  t0.task_seq = 0;

  EpochGraph eg;
  eg.epoch_id = epoch_id;
  eg.tasks = {t0};

  const uint64_t commits_before = state.epoch.epochs_committed;

  // No gate supplied — default KernelEpochPolicyGate{} with fn==nullptr.
  const auto result = axion_kernel_submit_epoch(state, eg, {make_trivial_program()});

  check(result.ok(), "[DPE-03-06] null gate: result ok");
  check(state.epoch.epochs_committed == commits_before + 1,
        "[DPE-03-06] null gate: epochs_committed incremented");
}

// ── RFC-0046 scheduling proof: dependency order governs gate evaluation ─────

static void test_dependency_order_governs_policy_gate(KernelRuntimeState& state) {
  std::printf(
      "\n[RFC-0046] Policy gate evaluation follows dependency order, not task array order\n");

  const uint64_t epoch_id = 504;
  const auto epoch = make_reversed_dependency_epoch(epoch_id);
  const auto program = make_trivial_program();

  GateContext ctx;
  ctx.deny_task_seq = -1;  // allow both tasks, record evaluation order

  KernelEpochPolicyGate gate;
  gate.fn = counting_gate_fn;
  gate.user_data = &ctx;

  const auto commits_before = state.epoch.epochs_committed;
  const auto result = axion_kernel_submit_epoch(state, epoch, {program, program}, gate);

  check(result.ok(), "[RFC-0046-01] reversed dependency epoch still commits successfully");
  check(state.epoch.epochs_committed == commits_before + 1,
        "[RFC-0046-02] reversed dependency epoch increments commit count");
  check(ctx.calls == 2, "[RFC-0046-03] policy gate evaluated once per task");
  check(ctx.seen_task_seq.size() == 2, "[RFC-0046-04] observed two policy-gate task visits");
  if (ctx.seen_task_seq.size() == 2) {
    check(ctx.seen_task_seq[0] == 0 && ctx.seen_task_seq[1] == 1,
          "[RFC-0046-05] dependency predecessor evaluated before dependent task");
  }
}

// ── main ─────────────────────────────────────────────────────────────────────

int main() {
  std::printf("=== Epoch Policy Gate tests (RFC-DPE-0003 §6.1 / [DPE-03-06]) ===\n");

  const auto ctx = make_test_boot_ctx();
  auto state_opt = axion_kernel_bootstrap(ctx);
  if (!state_opt.has_value()) {
    std::printf("FATAL: axion_kernel_bootstrap failed\n");
    return 1;
  }
  auto& state = *state_opt;

  test_policy_denial_aborts_epoch(state);
  test_allow_all_gate_normal_execution(state);
  test_first_task_denial_short_circuits(state);
  test_null_gate_allows_all(state);
  test_dependency_order_governs_policy_gate(state);

  std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
