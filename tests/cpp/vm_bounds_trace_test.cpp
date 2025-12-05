#include "t81/tisc/opcodes.hpp"
#include "t81/vm/vm.hpp"
#include "t81/tisc/program.hpp"

#include <iostream>
#include <string>
#include <vector>

static bool contains_reason(const t81::vm::State& state, std::string_view substring) {
  for (const auto& entry : state.axion_log) {
    if (entry.verdict.reason.find(substring) != std::string::npos) {
      return true;
    }
  }
  return false;
}

static int run_and_expect(std::vector<t81::tisc::Insn> insns,
                          t81::vm::Trap expected,
                          std::string_view reason_substr) {
  t81::tisc::Program program;
  program.insns = std::move(insns);
  auto vm = t81::vm::make_interpreter_vm();
  vm->load_program(program);
  auto result = vm->run_to_halt();
  if (!result) {
    if (result.error() != expected) {
      std::cerr << "Unexpected trap: " << static_cast<int>(result.error()) << "\n";
      return 1;
    }
    if (!contains_reason(vm->state(), reason_substr)) {
      std::cerr << "Missing Axion reason '" << reason_substr << "'\n";
      return 1;
    }
    return 0;
  }
  std::cerr << "Expected trap but execution succeeded\n";
  return 1;
}

int main() {
  std::vector<t81::tisc::Insn> stack_program;
  {
    t81::tisc::Insn stack_alloc{};
    stack_alloc.opcode = t81::tisc::Opcode::StackAlloc;
    stack_alloc.a = 0;
    stack_alloc.b = 0x7fffffff;
    stack_program.push_back(stack_alloc);
    t81::tisc::Insn halt{};
    halt.opcode = t81::tisc::Opcode::Halt;
    stack_program.push_back(halt);
  }
  if (run_and_expect(stack_program, t81::vm::Trap::BoundsFault, "bounds fault segment=stack") != 0) {
    return 1;
  }

  std::vector<t81::tisc::Insn> heap_program;
  {
    t81::tisc::Insn heap_alloc{};
    heap_alloc.opcode = t81::tisc::Opcode::HeapAlloc;
    heap_alloc.a = 0;
    heap_alloc.b = 0x7fffffff;
    heap_program.push_back(heap_alloc);
    t81::tisc::Insn halt{};
    halt.opcode = t81::tisc::Opcode::Halt;
    heap_program.push_back(halt);
  }
  if (run_and_expect(heap_program, t81::vm::Trap::BoundsFault, "bounds fault segment=heap") != 0) {
    return 1;
  }

  std::vector<t81::tisc::Insn> tensor_program;
  {
    t81::tisc::Insn load0{};
    load0.opcode = t81::tisc::Opcode::LoadImm;
    load0.a = 0;
    load0.b = 999;
    load0.literal_kind = t81::tisc::LiteralKind::Int;
    t81::tisc::Insn load1 = load0;
    load1.a = 1;
    t81::tisc::Insn tensordot{};
    tensordot.opcode = t81::tisc::Opcode::TTenDot;
    tensordot.a = 2;
    tensordot.b = 0;
    tensordot.c = 1;
    tensor_program.push_back(load0);
    tensor_program.push_back(load1);
    tensor_program.push_back(tensordot);
    t81::tisc::Insn halt{};
    halt.opcode = t81::tisc::Opcode::Halt;
    tensor_program.push_back(halt);
  }
  if (run_and_expect(tensor_program, t81::vm::Trap::IllegalInstruction, "bounds fault segment=tensor") != 0) {
    return 1;
  }

  return 0;
}
