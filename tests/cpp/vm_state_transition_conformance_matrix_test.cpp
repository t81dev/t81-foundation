#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "vm_state_transition_conformance_matrix_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

std::uint64_t mix(std::uint64_t seed, std::uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

bool validate_layout_invariants(const t81::vm::State& st) {
  const auto& layout = st.layout;
  if (!expect(layout.code.start == 0, "code segment start must be zero")) return false;
  if (!expect(layout.code.limit <= layout.stack.start, "code/stack overlap")) return false;
  if (!expect(layout.stack.limit <= layout.heap.start, "stack/heap overlap")) return false;
  if (!expect(layout.heap.limit <= layout.tensor.start, "heap/tensor overlap")) return false;
  if (!expect(layout.tensor.limit <= layout.meta.start, "tensor/meta overlap")) return false;
  if (!expect(layout.total_size() == layout.meta.limit, "layout total mismatch")) return false;
  if (!expect(st.memory.size() == layout.total_size(), "memory size mismatch")) return false;
  if (!expect(st.memory_tags.size() == st.memory.size(), "memory tags size mismatch")) return false;
  return true;
}

struct RunSummary {
  bool ok{false};
  t81::vm::Trap trap{t81::vm::Trap::None};
  std::uint64_t signature{0};
};

RunSummary run_and_summarize(const t81::tisc::Program& program, std::size_t max_steps) {
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto run = vm->run_to_halt(max_steps);

  RunSummary out;
  out.ok = run.has_value();
  out.trap = run.has_value() ? t81::vm::Trap::None : run.error();

  const auto& st = vm->state();
  if (!validate_layout_invariants(st)) {
    out.ok = false;
    out.trap = t81::vm::Trap::TrapInstruction;
    return out;
  }
  if (!expect(!st.contexts.empty(), "state has no contexts")) {
    out.ok = false;
    out.trap = t81::vm::Trap::TrapInstruction;
    return out;
  }

  const auto& ctx = st.contexts[st.current_context];
  if (!expect(ctx.sp <= ctx.stack_base, "sp exceeds stack base")) {
    out.ok = false;
    out.trap = t81::vm::Trap::TrapInstruction;
    return out;
  }
  if (!expect(ctx.sp >= ctx.stack_limit, "sp under stack limit")) {
    out.ok = false;
    out.trap = t81::vm::Trap::TrapInstruction;
    return out;
  }
  if (!expect(ctx.call_depth == 0, "call depth non-zero at halt")) {
    out.ok = false;
    out.trap = t81::vm::Trap::TrapInstruction;
    return out;
  }

  std::uint64_t sig = 1469598103934665603ULL;
  sig = mix(sig, static_cast<std::uint64_t>(st.halted ? 1 : 0));
  sig = mix(sig, st.trace.size());
  sig = mix(sig, st.axion_log.size());
  sig = mix(sig, static_cast<std::uint64_t>(ctx.pc));
  sig = mix(sig, static_cast<std::uint64_t>(ctx.sp));
  for (std::size_t i = 0; i < 48; ++i) {
    sig = mix(sig, static_cast<std::uint64_t>(ctx.registers[i]));
    sig = mix(sig, static_cast<std::uint64_t>(ctx.register_tags[i]));
  }
  for (const auto& entry : st.trace) {
    sig = mix(sig, entry.pc);
    sig = mix(sig, static_cast<std::uint64_t>(entry.opcode));
    sig = mix(sig, entry.trap.has_value() ? static_cast<std::uint64_t>(entry.trap.value()) : 0ULL);
  }
  sig = mix(sig, st.tensors.size());
  for (const auto& slot : st.tensors) {
    sig = mix(sig, slot.has_value() ? 1ULL : 0ULL);
    if (!slot.has_value()) {
      continue;
    }
    sig = mix(sig, static_cast<std::uint64_t>(slot->numeric_class()));
    sig = mix(sig, slot->canonical_fixed_authoritative() ? 1ULL : 0ULL);
    sig = mix(sig, slot->shape().size());
    for (int dim : slot->shape()) {
      sig = mix(sig, static_cast<std::uint64_t>(dim));
    }
    sig = mix(sig, slot->data().size());
    for (float value : slot->data()) {
      std::uint32_t bits = 0;
      static_assert(sizeof(bits) == sizeof(value));
      std::memcpy(&bits, &value, sizeof(bits));
      sig = mix(sig, static_cast<std::uint64_t>(bits));
    }
  }
  out.signature = sig;
  return out;
}

t81::tisc::Program make_arith_stack_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 17, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 25, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 42, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Push, 42, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Pop, 43, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_loop_memory_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 41, 1, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 42, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::Add, 40, 40, 41});
  p.insns.push_back({t81::tisc::Opcode::Less, 43, 40, 42});
  p.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 3, 43, 0});
  p.insns.push_back({t81::tisc::Opcode::Store, 144, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::Load, 44, 144, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_option_result_program() {
  t81::tisc::Program p;
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 40, 42, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeOptionSome, 41, 40, 0});
  p.insns.push_back({t81::tisc::Opcode::OptionUnwrap, 42, 41, 0});
  p.insns.push_back({t81::tisc::Opcode::MakeResultOk, 43, 42, 0});
  p.insns.push_back({t81::tisc::Opcode::ResultUnwrapOk, 44, 43, 0});
  p.insns.push_back({t81::tisc::Opcode::Store, 156, 44, 0});
  p.insns.push_back({t81::tisc::Opcode::Load, 45, 156, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tensor_visibility_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 17, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::TGet, 4, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::F2I, 5, 4, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tensor_bounds_fault_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({2}, {8.0f, 9.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 5, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 99, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 3});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

t81::tisc::Program make_tensor_write_kind_transition_program() {
  t81::tisc::Program p;
  p.tensor_pool.push_back(t81::T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));

  t81::tisc::Insn load_tensor{t81::tisc::Opcode::LoadImm, 1, 1, 0};
  load_tensor.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  p.insns.push_back(load_tensor);

  p.insns.push_back({t81::tisc::Opcode::LoadImm, 2, 0, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 3, 6, 0});
  p.insns.push_back({t81::tisc::Opcode::I2F, 4, 3, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 2, 4});

  p.insns.push_back({t81::tisc::Opcode::LoadImm, 5, 2, 0});
  p.insns.push_back({t81::tisc::Opcode::LoadImm, 6, 8, 0});
  p.insns.push_back({t81::tisc::Opcode::TSet, 1, 5, 6});

  p.insns.push_back({t81::tisc::Opcode::TGet, 7, 1, 2});
  p.insns.push_back({t81::tisc::Opcode::F2I, 8, 7, 0});
  p.insns.push_back({t81::tisc::Opcode::TGet, 9, 1, 5});
  p.insns.push_back({t81::tisc::Opcode::F2I, 10, 9, 0});
  p.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
  return p;
}

}  // namespace

int main() {
  struct MatrixCase {
    std::string id;
    t81::tisc::Program program;
    bool expect_ok;
    t81::vm::Trap expect_trap;
  };

  const std::vector<MatrixCase> cases = {
      {"arith-stack", make_arith_stack_program(), true, t81::vm::Trap::None},
      {"loop-memory", make_loop_memory_program(), true, t81::vm::Trap::None},
      {"option-result", make_option_result_program(), true, t81::vm::Trap::None},
      {"tensor-visibility", make_tensor_visibility_program(), true, t81::vm::Trap::None},
      {"tensor-write-kind-transition", make_tensor_write_kind_transition_program(), true,
       t81::vm::Trap::None},
      {"tensor-bounds-fault", make_tensor_bounds_fault_program(), false,
       t81::vm::Trap::BoundsFault},
  };

  for (const auto& c : cases) {
    RunSummary baseline = run_and_summarize(c.program, 2048);
    if (!expect(baseline.ok == c.expect_ok, c.id + ": baseline outcome mismatch")) return 1;
    if (!c.expect_ok &&
        !expect(baseline.trap == c.expect_trap, c.id + ": baseline trap mismatch")) {
      return 1;
    }

    for (int i = 0; i < 8; ++i) {
      RunSummary repeat = run_and_summarize(c.program, 2048);
      if (!expect(repeat.ok == baseline.ok, c.id + ": repeat outcome drift")) return 1;
      if (!expect(repeat.trap == baseline.trap, c.id + ": repeat trap drift")) return 1;
      if (!expect(repeat.signature == baseline.signature, c.id + ": repeat signature drift")) {
        return 1;
      }
    }
  }

  return 0;
}
