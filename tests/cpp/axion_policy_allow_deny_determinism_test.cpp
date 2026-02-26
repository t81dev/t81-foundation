#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "axion_policy_allow_deny_determinism_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::uint64_t mix(std::uint64_t seed, std::uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

struct RunSummary {
  bool ok{false};
  t81::vm::Trap trap{t81::vm::Trap::None};
  std::uint64_t trace_sig{0};
  std::uint64_t axion_sig{0};
};

RunSummary run_once(const t81::tisc::Program& program, std::size_t max_steps) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto run = vm->run_to_halt(max_steps);

  RunSummary out;
  out.ok = run.has_value();
  out.trap = run.has_value() ? t81::vm::Trap::None : run.error();

  std::uint64_t ts = 1469598103934665603ULL;
  for (const auto& entry : vm->state().trace) {
    ts = mix(ts, entry.pc);
    ts = mix(ts, static_cast<std::uint64_t>(entry.opcode));
    ts = mix(ts, entry.trap.has_value() ? static_cast<std::uint64_t>(entry.trap.value()) : 0ULL);
  }
  out.trace_sig = ts;

  std::uint64_t as = 1469598103934665603ULL;
  for (const auto& event : vm->state().axion_log) {
    as = mix(as, static_cast<std::uint64_t>(event.opcode));
    as = mix(as, static_cast<std::uint64_t>(event.tag));
    as = mix(as, static_cast<std::uint64_t>(event.value));
    for (unsigned char ch : event.verdict.reason) {
      as = mix(as, static_cast<std::uint64_t>(ch));
    }
    for (unsigned char ch : event.structured.decision) {
      as = mix(as, static_cast<std::uint64_t>(ch));
    }
  }
  out.axion_sig = as;
  return out;
}

t81::tisc::Program make_policy_program(const std::string& policy) {
  t81::tisc::Program p;
  p.axion_policy_text = policy;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Push, 42, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Pop, 43, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::AxReport, 43, 7, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

}  // namespace

int main() {
  struct MatrixCase {
    std::string id;
    std::string policy;
    bool expect_ok;
    t81::vm::Trap expect_trap;
  };

  const std::vector<MatrixCase> cases = {
      {"allow-basic", R"((policy (tier 1) (max-instructions 64) (max-stack 64)))", true,
       t81::vm::Trap::None},
      {"deny-max-instructions", R"((policy (tier 1) (max-instructions 0)))", false,
       t81::vm::Trap::SecurityFault},
      {"deny-required-event", R"((policy (tier 1) (require-axion-event (reason "never-seen"))))",
       false, t81::vm::Trap::SecurityFault},
      {"invalid-clause", R"((policy (tier 1) (unknown-clause 1)))", false,
       t81::vm::Trap::SecurityFault},
  };

  for (const auto& c : cases) {
    auto program = make_policy_program(c.policy);
    RunSummary baseline = run_once(program, 256);
    if (!expect(baseline.ok == c.expect_ok, c.id + ": baseline outcome mismatch")) return 1;
    if (!expect(baseline.trap == c.expect_trap, c.id + ": baseline trap mismatch")) return 1;

    for (int i = 0; i < 8; ++i) {
      RunSummary repeat = run_once(program, 256);
      if (!expect(repeat.ok == baseline.ok, c.id + ": outcome drift")) return 1;
      if (!expect(repeat.trap == baseline.trap, c.id + ": trap drift")) return 1;
      if (!expect(repeat.trace_sig == baseline.trace_sig, c.id + ": trace signature drift"))
        return 1;
      if (!expect(repeat.axion_sig == baseline.axion_sig, c.id + ": axion signature drift"))
        return 1;
    }
  }

  return 0;
}
