#include <cstdint>
#include <iostream>

#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

namespace {

bool expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "vm_state_transition_invariants_test failure: " << msg << "\n";
    return false;
  }
  return true;
}

bool check_layout_invariants(const t81::vm::State& st) {
  const auto& layout = st.layout;
  if (!expect(layout.code.start == 0, "code segment must start at zero")) return false;
  if (!expect(layout.code.limit <= layout.stack.start, "code must not overlap stack")) return false;
  if (!expect(layout.stack.limit <= layout.heap.start, "stack must not overlap heap")) return false;
  if (!expect(layout.heap.limit <= layout.tensor.start, "heap must not overlap tensor"))
    return false;
  if (!expect(layout.tensor.limit <= layout.meta.start, "tensor must not overlap meta"))
    return false;
  if (!expect(layout.total_size() == layout.meta.limit, "layout total size mismatch")) return false;
  if (!expect(st.memory.size() == layout.total_size(), "memory size must match layout"))
    return false;
  if (!expect(st.memory_tags.size() == st.memory.size(), "memory tags size mismatch")) return false;
  return true;
}

}  // namespace

int main() {
  using t81::tisc::Opcode;
  t81::tisc::Program program;
  program.insns.push_back({Opcode::LoadImm, 40, 41, 0});
  program.insns.push_back({Opcode::LoadImm, 41, 1, 0});
  program.insns.push_back({Opcode::Add, 42, 40, 41});
  program.insns.push_back({Opcode::Push, 42, 0, 0});
  program.insns.push_back({Opcode::Pop, 44, 0, 0});
  program.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);

  std::size_t last_trace_size = 0;
  for (int step = 0; step < 64; ++step) {
    const auto& before = vm->state();
    if (!check_layout_invariants(before)) return 1;
    if (!expect(before.current_context < before.contexts.size(), "current context out of range")) {
      return 1;
    }
    const auto& before_ctx = before.contexts[before.current_context];
    if (!expect(before_ctx.sp <= before_ctx.stack_base, "sp exceeds stack_base")) return 1;
    if (!expect(before_ctx.sp >= before_ctx.stack_limit, "sp under stack_limit")) return 1;
    if (!expect(before_ctx.call_depth == 0, "call depth must stay zero in non-call program")) {
      return 1;
    }
    if (!expect(before_ctx.stack_frames.empty(), "stack frame list must remain empty")) return 1;

    if (before.halted) break;
    auto result = vm->step();
    if (!expect(result.has_value(), "step unexpectedly trapped")) return 1;

    const auto& after = vm->state();
    if (!check_layout_invariants(after)) return 1;
    if (!expect(after.trace.size() == last_trace_size + 1,
                "trace size must advance by one per step")) {
      return 1;
    }
    last_trace_size = after.trace.size();
    if (!expect(after.trace.back().pc < program.insns.size(), "trace pc out of program bounds"))
      return 1;

    const auto& after_ctx = after.contexts[after.current_context];
    if (!expect(after_ctx.sp <= after_ctx.stack_base, "post-step sp exceeds stack_base")) return 1;
    if (!expect(after_ctx.sp >= after_ctx.stack_limit, "post-step sp under stack_limit")) return 1;
    if (!expect(after_ctx.call_depth == 0, "post-step call depth must remain zero")) return 1;

    if (!after.halted) {
      if (!expect(after_ctx.pc < program.insns.size(), "pc out of bounds before halt")) return 1;
    }
  }

  const auto& final_state = vm->state();
  if (!expect(final_state.halted, "program must halt")) return 1;
  if (!expect(final_state.contexts[0].registers[44] == 42,
              "register R44 must contain computed value")) {
    return 1;
  }

  return 0;
}
