#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "vm_mixed_workload_conformance_matrix_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::uint64_t mix(std::uint64_t seed, std::uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

t81::tisc::Program mixed_program(const std::string& policy) {
  t81::tisc::Program p;
  p.axion_policy_text = policy;

  // Branch + memory loop.
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 42, 5, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 40, 40, 41});        // pc=3
  p.insns.push_back({t81::tisc::Opcode::Less, 43, 40, 42});       // pc=4
  p.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 3, 43, 0});  // pc=5
  p.insns.push_back({t81::tisc::Opcode::Store, 150, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::Load, 44, 150, 0});

  // Sum-type path.
  p.insns.push_back({t81::tisc::Opcode::MakeOptionSome, 45, 44, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionUnwrap, 46, 45, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeResultOk, 47, 46, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultUnwrapOk, 48, 47, 0});

  // Tensor path.
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 49, 4, 0});
  p.insns.push_back({t81::tisc::Opcode::TNew, 50, 49, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 51, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 52, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 50, 51, 52});
  p.insns.push_back({t81::tisc::Opcode::TGet, 53, 50, 51});

  // Policy-visible event + output.
  p.insns.push_back({t81::tisc::Opcode::AxReport, 53, 7, 0});
  p.insns.push_back({t81::tisc::Opcode::Print, 48, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Print, 53, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

struct RunSummary {
  bool ok{false};
  t81::vm::Trap trap{t81::vm::Trap::None};
  std::uint64_t signature{0};
};

RunSummary run_once(const t81::tisc::Program& program) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto run = vm->run_to_halt(4096);

  RunSummary out;
  out.ok = run.has_value();
  out.trap = run.has_value() ? t81::vm::Trap::None : run.error();
  if (!run.has_value()) return out;

  const auto& st = vm->state();
  const auto& ctx = st.contexts[0];
  std::uint64_t sig = 1469598103934665603ULL;
  sig = mix(sig, st.trace.size());
  sig = mix(sig, st.axion_log.size());
  sig = mix(sig, static_cast<std::uint64_t>(ctx.pc));
  sig = mix(sig, static_cast<std::uint64_t>(ctx.sp));
  for (std::size_t i = 0; i < 64; ++i) {
    sig = mix(sig, static_cast<std::uint64_t>(ctx.registers[i]));
    sig = mix(sig, static_cast<std::uint64_t>(ctx.register_tags[i]));
  }
  for (const auto& tr : st.trace) {
    sig = mix(sig, tr.pc);
    sig = mix(sig, static_cast<std::uint64_t>(tr.opcode));
    sig = mix(sig, tr.trap.has_value() ? static_cast<std::uint64_t>(tr.trap.value()) : 0ULL);
  }
  for (const auto& ev : st.axion_log) {
    sig = mix(sig, static_cast<std::uint64_t>(ev.opcode));
    sig = mix(sig, static_cast<std::uint64_t>(ev.tag));
    sig = mix(sig, static_cast<std::uint64_t>(ev.value));
    for (unsigned char c : ev.verdict.reason) sig = mix(sig, c);
  }
  for (const auto& line : st.printed_output) {
    for (unsigned char c : line) sig = mix(sig, c);
  }
  out.signature = sig;
  return out;
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
      {"allow-mixed-workload",
       "(policy (tier 1) (max-instructions 2048) (max-stack 128) (max-meta-writes 64))",
       true,
       t81::vm::Trap::None},
      {"deny-branch-loop-via-instruction-budget",
       "(policy (tier 1) (max-instructions 5) (max-stack 128) (max-meta-writes 64))",
       false,
       t81::vm::Trap::SecurityFault},
  };

  for (const auto& c : cases) {
    const auto program = mixed_program(c.policy);
    RunSummary baseline = run_once(program);
    if (!expect(baseline.ok == c.expect_ok, c.id + ": baseline outcome mismatch")) return 1;
    if (!expect(baseline.trap == c.expect_trap, c.id + ": baseline trap mismatch")) return 1;

    for (int i = 0; i < 10; ++i) {
      RunSummary repeat = run_once(program);
      if (!expect(repeat.ok == baseline.ok, c.id + ": outcome drift")) return 1;
      if (!expect(repeat.trap == baseline.trap, c.id + ": trap drift")) return 1;
      if (!expect(repeat.signature == baseline.signature, c.id + ": signature drift")) return 1;
    }
  }
  return 0;
}
