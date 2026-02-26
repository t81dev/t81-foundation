#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "vm_fault_family_determinism_matrix_test failure: " << msg << "\n";
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
  std::uint64_t signature{0};
};

RunSummary run_program(const t81::tisc::Program& program, std::size_t max_steps = 256) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto run = vm->run_to_halt(max_steps);

  RunSummary out;
  out.ok = run.has_value();
  out.trap = run.has_value() ? t81::vm::Trap::None : run.error();

  const auto& st = vm->state();
  std::uint64_t sig = 1469598103934665603ULL;
  for (const auto& tr : st.trace) {
    sig = mix(sig, tr.pc);
    sig = mix(sig, static_cast<std::uint64_t>(tr.opcode));
    sig = mix(sig, tr.trap.has_value() ? static_cast<std::uint64_t>(tr.trap.value()) : 0ULL);
  }
  sig = mix(sig, st.axion_log.size());
  for (const auto& ev : st.axion_log) {
    sig = mix(sig, static_cast<std::uint64_t>(ev.opcode));
    sig = mix(sig, static_cast<std::uint64_t>(ev.tag));
    sig = mix(sig, static_cast<std::uint64_t>(ev.value));
    for (unsigned char c : ev.verdict.reason) sig = mix(sig, c);
  }
  out.signature = sig;
  return out;
}

t81::tisc::Program div_by_zero_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Div, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program bounds_fault_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::Load, 40, 999999, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program type_fault_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_bad_value{t81::tisc::Opcode::LoadImm, 3, 1, 0};
  load_bad_value.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 1, 0});
  p.insns.push_back(load_bad_value);
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program shape_fault_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {1.0f, 2.0f}));
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {3.0f, 4.0f, 5.0f}));
  t81::tisc::Insn load_a{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_a.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  t81::tisc::Insn load_b{t81::tisc::Opcode::LoadImm, 2, 2, 0};
  load_b.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_a);
  p.insns.push_back(load_b);
  p.insns.push_back({t81::tisc::Opcode::TVecAdd, 3, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program security_fault_program() {
  t81::tisc::Program p;
  p.axion_policy_text = "(policy (tier 1) (max-instructions 0))";
  p.insns.push_back({t81::tisc::Opcode::Nop, 0, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

}  // namespace

int main() {
  struct MatrixCase {
    std::string id;
    t81::tisc::Program program;
    t81::vm::Trap expected_trap;
  };

  const std::vector<MatrixCase> cases = {
      {"division-fault", div_by_zero_program(), t81::vm::Trap::DivisionFault},
      {"bounds-fault", bounds_fault_program(), t81::vm::Trap::BoundsFault},
      {"type-fault", type_fault_program(), t81::vm::Trap::TypeFault},
      {"shape-fault", shape_fault_program(), t81::vm::Trap::ShapeFault},
      {"security-fault", security_fault_program(), t81::vm::Trap::SecurityFault},
  };

  for (const auto& c : cases) {
    RunSummary baseline = run_program(c.program);
    if (!expect(!baseline.ok, c.id + ": baseline expected trap")) return 1;
    if (!expect(baseline.trap == c.expected_trap, c.id + ": baseline trap mismatch")) return 1;

    for (int i = 0; i < 8; ++i) {
      RunSummary repeat = run_program(c.program);
      if (!expect(repeat.ok == baseline.ok, c.id + ": outcome drift")) return 1;
      if (!expect(repeat.trap == baseline.trap, c.id + ": trap drift")) return 1;
      if (!expect(repeat.signature == baseline.signature, c.id + ": signature drift")) return 1;
    }
  }

  return 0;
}
