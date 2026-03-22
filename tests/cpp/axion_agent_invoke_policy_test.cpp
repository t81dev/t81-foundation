// tests/cpp/axion_agent_invoke_policy_test.cpp
//
// RFC-0015 §3.2 + RFC-0048 §4.2 integration evidence test — Axion policy
// enforcement for AgentInvoke at the VM runtime boundary.
//
// This test closes the runtime-integration evidence gap identified in
// docs/status/DRIFT_DECOMPOSITION.md: the existing agent_constructs_test.cpp
// verifies compilation correctness ([RFC-0015-01..09]) but does not prove that
// the Axion policy engine is correctly wired to the AgentInvoke dispatch path.
//
// Verified criteria:
//   [AI-01]  No-policy VM: AgentInvoke + Ret runs to Halt; audit event logged.
//   [AI-02]  Policy engine via axion_policy_text (Allow): program fits within
//            instruction limit; VM halts cleanly.
//   [AI-03]  Policy engine via axion_policy_text (Deny): program exceeds
//            instruction limit; VM returns SecurityFault, not Halt.
//   [AI-04]  Policy engine injected via make_interpreter_vm(engine): enforcement
//            is active even when axion_policy_text is empty in the Program.
//   [AI-05]  Axion audit log contains an AgentInvoke entry with reason
//            "agent-invoke" after a successful dispatch.
//
// Governance reference: RFC-0015 §3.2, RFC-0048 §4.2 (runtime boundary).

#include <cassert>
#include <cstdio>
#include <string>

#include "t81/axion/engine.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/verdict.hpp"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/vm/traps.hpp"
#include "t81/vm/vm.hpp"

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

// Build a minimal 5-instruction program that performs one AgentInvoke and
// returns cleanly.
//
//   PC 0: LoadImm r2 = 4    — load behavior entry address into r2
//   PC 1: AgentInvoke r0, r2 — audit + dispatch; pushes PC 2 as return address
//   PC 2: Halt               — reached after Ret
//   PC 3: Nop                — padding
//   PC 4: Ret                — behavior body (returns to PC 2)
//
// Instruction count per step(): LoadImm(1) AgentInvoke(2) Ret(3) Halt(4).
// The Halt step_verdict check fires at count=4 before the Halt is dispatched.
static t81::tisc::Program make_agent_invoke_program() {
  using namespace t81::tisc;
  Program prog;
  prog.insns.push_back({Opcode::LoadImm,    2, 4, 0});  // PC 0
  prog.insns.push_back({Opcode::AgentInvoke, 0, 2, 0}); // PC 1
  prog.insns.push_back({Opcode::Halt,        0, 0, 0}); // PC 2
  prog.insns.push_back({Opcode::Nop,         0, 0, 0}); // PC 3
  prog.insns.push_back({Opcode::Ret,         0, 0, 0}); // PC 4
  return prog;
}

// ── [AI-01]: no-policy VM halts cleanly; audit event present ─────────────────

static void test_no_policy_allow() {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(make_agent_invoke_program());
  auto result = vm->run_to_halt();
  check(result.has_value(),
        "[AI-01] no-policy AgentInvoke program runs to Halt cleanly");
  check(!vm->state().axion_log.empty(),
        "[AI-01] axion_log is non-empty after AgentInvoke");
}

// ── [AI-02]: policy engine via axion_policy_text (allow) ─────────────────────

static void test_policy_text_allow() {
  auto prog = make_agent_invoke_program();
  // 100-instruction budget — program uses 4 steps, well within limit.
  prog.axion_policy_text = "(policy (tier 1) (max-instructions 100))";
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto result = vm->run_to_halt();
  check(result.has_value(),
        "[AI-02] policy-text allow: AgentInvoke program halts cleanly");
}

// ── [AI-03]: policy engine via axion_policy_text (deny) ──────────────────────

static void test_policy_text_deny() {
  auto prog = make_agent_invoke_program();
  // 2-instruction budget: LoadImm(count=1) and AgentInvoke(count=2) pass;
  // Ret inside the behavior body (count=3) exceeds the limit (3 > 2) and
  // causes a SecurityFault before the behavior can return.
  prog.axion_policy_text = "(policy (tier 1) (max-instructions 2))";
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(prog);
  auto result = vm->run_to_halt();
  check(!result.has_value(),
        "[AI-03] policy-text deny: run_to_halt returns an error (policy violation)");
  if (!result.has_value()) {
    check(result.error() == t81::vm::Trap::SecurityFault,
          "[AI-03] policy-text deny: trap kind is SecurityFault");
  }
}

// ── [AI-04]: policy engine injected via make_interpreter_vm(engine) ──────────

static void test_injected_engine_deny() {
  // Parse and compile the policy, then inject it at VM construction time.
  // This verifies the public make_interpreter_vm(engine) API path is wired
  // to the same per-instruction eval_axion_call gate as axion_policy_text.
  auto policy_res = t81::axion::parse_policy("(policy (tier 1) (max-instructions 2))");
  check(policy_res.has_value(), "[AI-04] policy parses successfully");
  if (!policy_res.has_value()) return;

  auto engine = t81::axion::make_policy_engine(std::move(policy_res.value()));
  auto vm = t81::vm::make_interpreter_vm(std::move(engine));
  vm->load_program(make_agent_invoke_program());
  auto result = vm->run_to_halt();
  check(!result.has_value(),
        "[AI-04] injected-engine deny: run_to_halt returns an error");
  if (!result.has_value()) {
    check(result.error() == t81::vm::Trap::SecurityFault,
          "[AI-04] injected-engine deny: trap kind is SecurityFault");
  }
}

// ── [AI-05]: audit log contains AgentInvoke entry with reason "agent-invoke" ──

static void test_audit_log_agent_invoke_entry() {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(make_agent_invoke_program());
  (void)vm->run_to_halt();

  const auto& log = vm->state().axion_log;
  bool found = false;
  for (const auto& entry : log) {
    if (entry.opcode == t81::tisc::Opcode::AgentInvoke &&
        entry.verdict.reason.find("agent-invoke") != std::string::npos) {
      found = true;
      break;
    }
  }
  check(found,
        "[AI-05] axion_log contains AgentInvoke entry with reason 'agent-invoke'");
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
  std::printf("Axion AgentInvoke Policy Integration tests\n");
  std::printf("───────────────────────────────────────────\n");

  test_no_policy_allow();
  test_policy_text_allow();
  test_policy_text_deny();
  test_injected_engine_deny();
  test_audit_log_agent_invoke_entry();

  std::printf("───────────────────────────────────────────\n");
  std::printf("Result: %d passed, %d failed\n", g_pass, g_fail);
  return g_fail == 0 ? 0 : 1;
}
