#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "axion_policy_conformance_matrix_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

struct RunResult {
  bool ok{false};
  t81::vm::Trap trap{t81::vm::Trap::None};
  std::uint64_t trace_sig{0};
  std::uint64_t axion_sig{0};
};

std::uint64_t mix(std::uint64_t seed, std::uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

RunResult run_program(const t81::tisc::Program& p, std::size_t max_steps) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(p);
  auto run = vm->run_to_halt(max_steps);

  RunResult out;
  out.ok = run.has_value();
  out.trap = run.has_value() ? t81::vm::Trap::None : run.error();
  std::uint64_t sig = 1469598103934665603ULL;
  for (const auto& entry : vm->state().trace) {
    sig = mix(sig, entry.pc);
    sig = mix(sig, static_cast<std::uint64_t>(entry.opcode));
    sig = mix(sig, entry.trap.has_value() ? static_cast<std::uint64_t>(entry.trap.value()) : 0ULL);
  }
  sig = mix(sig, vm->state().axion_log.size());
  out.trace_sig = sig;

  std::uint64_t ax = 1469598103934665603ULL;
  for (const auto& event : vm->state().axion_log) {
    ax = mix(ax, static_cast<std::uint64_t>(event.opcode));
    ax = mix(ax, static_cast<std::uint64_t>(event.tag));
    ax = mix(ax, static_cast<std::uint64_t>(event.value));
    for (unsigned char ch : event.verdict.reason) {
      ax = mix(ax, static_cast<std::uint64_t>(ch));
    }
  }
  out.axion_sig = ax;
  return out;
}

t81::tisc::Program make_noop_halt_program(std::string policy) {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::Nop, 0, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  p.axion_policy_text = std::move(policy);
  return p;
}

t81::tisc::Program make_stack_program(std::string policy) {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::Push, 40, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Pop, 41, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  p.axion_policy_text = std::move(policy);
  return p;
}

struct MatrixCase {
  std::string id;
  t81::tisc::Program program;
  bool expect_ok;
  t81::vm::Trap expect_trap;
};

}  // namespace

int main() {
  // Spec alignment: spec/axion-kernel.md policy fail-closed + deterministic verdict behavior.
  std::vector<MatrixCase> cases;
  cases.push_back({"allow-basic",
                   make_noop_halt_program(R"((policy (tier 1) (max-instructions 8)))"),
                   true,
                   t81::vm::Trap::None});
  cases.push_back(
      {"deny-max-instructions",
       make_noop_halt_program(R"((policy (tier 1) (max-instructions 0)))"),
       false,
       t81::vm::Trap::SecurityFault});
  cases.push_back({"deny-required-axion-event",
                   make_noop_halt_program(
                       R"((policy (tier 1) (require-axion-event (reason "unreachable"))))"),
                   false,
                   t81::vm::Trap::SecurityFault});
  cases.push_back({"max-stack-basic",
                   make_stack_program(R"((policy (tier 1) (max-stack 64)))"),
                   true,
                   t81::vm::Trap::None});
  cases.push_back({"invalid-unknown-clause",
                   make_noop_halt_program(R"((policy (tier 1) (unknown-clause 1)))"),
                   false,
                   t81::vm::Trap::SecurityFault});
  cases.push_back({"allow-clause-order-a",
                   make_stack_program(
                       R"((policy (tier 1) (max-instructions 64) (max-stack 64) (max-meta-writes 8)))"),
                   true,
                   t81::vm::Trap::None});
  cases.push_back({"allow-clause-order-b",
                   make_stack_program(
                       R"((policy (tier 1) (max-meta-writes 8) (max-stack 64) (max-instructions 64)))"),
                   true,
                   t81::vm::Trap::None});
  cases.push_back({"deny-clause-order-a",
                   make_noop_halt_program(
                       R"((policy (tier 1) (max-instructions 64) (require-axion-event (reason "never-seen"))))"),
                   false,
                   t81::vm::Trap::SecurityFault});
  cases.push_back({"deny-clause-order-b",
                   make_noop_halt_program(
                       R"((policy (tier 1) (require-axion-event (reason "never-seen")) (max-instructions 64)))"),
                   false,
                   t81::vm::Trap::SecurityFault});

  for (const auto& c : cases) {
    RunResult a = run_program(c.program, 128);
    RunResult b = run_program(c.program, 128);

    if (!expect(a.ok == c.expect_ok, c.id + ": outcome mismatch run A")) return 1;
    if (!expect(b.ok == c.expect_ok, c.id + ": outcome mismatch run B")) return 1;
    if (!c.expect_ok) {
      if (!expect(a.trap == c.expect_trap, c.id + ": trap mismatch run A")) return 1;
      if (!expect(b.trap == c.expect_trap, c.id + ": trap mismatch run B")) return 1;
    }
    if (!expect(a.ok == b.ok, c.id + ": run-to-run outcome drift")) return 1;
    if (!expect(a.trap == b.trap, c.id + ": run-to-run trap drift")) return 1;
    if (!expect(a.trace_sig == b.trace_sig, c.id + ": run-to-run trace signature drift")) return 1;
    if (!expect(a.axion_sig == b.axion_sig, c.id + ": run-to-run axion signature drift")) return 1;
  }

  // Clause ordering invariants: semantically equivalent clause sets should yield equivalent outcomes.
  const auto allow_order_a = run_program(
      make_stack_program(
          R"((policy (tier 1) (max-instructions 64) (max-stack 64) (max-meta-writes 8)))"),
      128);
  const auto allow_order_b = run_program(
      make_stack_program(
          R"((policy (tier 1) (max-meta-writes 8) (max-stack 64) (max-instructions 64)))"),
      128);
  if (!expect(allow_order_a.ok == allow_order_b.ok, "allow clause-order equivalence outcome mismatch"))
    return 1;
  if (!expect(allow_order_a.trap == allow_order_b.trap, "allow clause-order equivalence trap mismatch"))
    return 1;
  if (!expect(allow_order_a.trace_sig == allow_order_b.trace_sig,
              "allow clause-order equivalence trace signature mismatch"))
    return 1;
  if (!expect(allow_order_a.axion_sig == allow_order_b.axion_sig,
              "allow clause-order equivalence axion signature mismatch"))
    return 1;

  const auto deny_order_a = run_program(
      make_noop_halt_program(
          R"((policy (tier 1) (max-instructions 64) (require-axion-event (reason "never-seen"))))"),
      128);
  const auto deny_order_b = run_program(
      make_noop_halt_program(
          R"((policy (tier 1) (require-axion-event (reason "never-seen")) (max-instructions 64)))"),
      128);
  if (!expect(deny_order_a.ok == deny_order_b.ok, "deny clause-order equivalence outcome mismatch"))
    return 1;
  if (!expect(deny_order_a.trap == deny_order_b.trap, "deny clause-order equivalence trap mismatch"))
    return 1;
  if (!expect(deny_order_a.trace_sig == deny_order_b.trace_sig,
              "deny clause-order equivalence trace signature mismatch"))
    return 1;
  if (!expect(deny_order_a.axion_sig == deny_order_b.axion_sig,
              "deny clause-order equivalence axion signature mismatch"))
    return 1;

  return 0;
}
