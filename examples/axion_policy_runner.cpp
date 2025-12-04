#include "t81/tisc/program.hpp"
#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"

#include <iostream>
#include <string_view>
#include <vector>

int main() {
  t81::tisc::Program program;
  program.tensor_pool.emplace_back(std::vector<int>{1}, std::vector<float>{3.14f});
  program.tensor_pool.emplace_back(std::vector<int>{1}, std::vector<float>{2.72f});

  t81::tisc::Insn load_tensor0{};
  load_tensor0.opcode = t81::tisc::Opcode::LoadImm;
  load_tensor0.a = 1;
  load_tensor0.b = 1;
  load_tensor0.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn load_tensor1{};
  load_tensor1.opcode = t81::tisc::Opcode::LoadImm;
  load_tensor1.a = 2;
  load_tensor1.b = 2;
  load_tensor1.literal_kind = t81::tisc::LiteralKind::TensorHandle;

  t81::tisc::Insn vec_add{};
  vec_add.opcode = t81::tisc::Opcode::TVecAdd;
  vec_add.a = 3;
  vec_add.b = 1;
  vec_add.c = 2;

  t81::tisc::Insn stack_alloc{};
  stack_alloc.opcode = t81::tisc::Opcode::StackAlloc;
  stack_alloc.a = 4;
  stack_alloc.b = 16;

  t81::tisc::Insn heap_alloc{};
  heap_alloc.opcode = t81::tisc::Opcode::HeapAlloc;
  heap_alloc.a = 5;
  heap_alloc.b = 32;

  t81::tisc::Insn load_value{};
  load_value.opcode = t81::tisc::Opcode::LoadImm;
  load_value.a = 6;
  load_value.b = 123;

  t81::tisc::Insn ax_set{};
  ax_set.opcode = t81::tisc::Opcode::AxSet;
  ax_set.a = 4;
  ax_set.b = 6;

  t81::tisc::Insn ax_read{};
  ax_read.opcode = t81::tisc::Opcode::AxRead;
  ax_read.a = 7;
  ax_read.b = 5;

  t81::tisc::Insn heap_free{};
  heap_free.opcode = t81::tisc::Opcode::HeapFree;
  heap_free.a = 5;
  heap_free.b = 32;

  t81::tisc::Insn stack_free{};
  stack_free.opcode = t81::tisc::Opcode::StackFree;
  stack_free.a = 4;
  stack_free.b = 16;

  t81::tisc::Insn halt{};
  halt.opcode = t81::tisc::Opcode::Halt;

  program.insns = {
      load_tensor0, load_tensor1, vec_add, stack_alloc, heap_alloc, load_value,
      ax_set, ax_read, heap_free, stack_free, halt,
  };

  program.axion_policy_text =
      "(policy (tier 1)"
      " (require-segment-event (segment stack) (action \"stack frame allocated\"))"
      " (require-segment-event (segment heap) (action \"heap block allocated\"))"
      " (require-segment-event (segment tensor) (action \"tensor slot allocated\"))"
      " (require-segment-event (segment meta) (action \"meta slot axion event\"))"
      " (require-segment-event (segment stack) (action \"AxRead guard\"))"
      " (require-segment-event (segment heap) (action \"AxSet guard\")))";

  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result.has_value()) {
    std::cerr << "Axion policy runner failed with trap\n";
    return 1;
  }

  std::cout << "Axion policy runner emitted the following verdict.reason strings:\n";
  for (const auto& entry : vm->state().axion_log) {
    std::cout << "  opcode=" << static_cast<int>(entry.opcode)
              << " tag=" << entry.tag
              << " reason=\"" << entry.verdict.reason << "\"\n";
  }

  auto contains_reason = [&](std::string_view substring) {
    for (const auto& entry : vm->state().axion_log) {
      if (entry.verdict.reason.find(substring) != std::string::npos) {
        return true;
      }
    }
    return false;
  };

  struct Requirement {
    const char* label;
    std::string_view substring;
  } requirements[] = {
      {"stack trace", "stack frame allocated"},
      {"heap trace", "heap block allocated"},
      {"tensor slot trace", "tensor slot allocated"},
      {"meta slot trace", "meta slot axion event"},
      {"AxRead guard", "AxRead guard"},
      {"AxSet guard", "AxSet guard"},
  };

  bool all_ok = true;
  for (const auto& req : requirements) {
    if (!contains_reason(req.substring)) {
      std::cerr << "Missing trace entry for " << req.label << "\n";
      all_ok = false;
    }
  }
  if (!all_ok) {
    return 1;
  }

  std::cout << "Axion segment requirements satisfied per RFC-0013/RFC-0009.\n";
  return 0;
}
