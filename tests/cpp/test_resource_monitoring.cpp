#include "t81/isa/program.hpp"
#include "t81/vm/vm.hpp"

#include <cstdlib>
#include <iostream>

using namespace t81;
using namespace t81::tisc;

[[noreturn]] static void fail(const char* msg) {
  std::cerr << "test_resource_monitoring failure: " << msg << "\n";
  std::exit(1);
}

static bool expect(bool cond, const char* msg) {
  if (!cond) {
    fail(msg);
  }
  return true;
}

static void test_max_tensors() {
  std::cout << "Running test_max_tensors...\n";
  tisc::Program program;
  program.axion_policy_text = "(policy (max-tensors 1))";

  // R1 = 1
  tisc::Insn i1;
  i1.opcode = Opcode::LoadImm;
  i1.a = 1;
  i1.b = 1;
  program.insns.push_back(i1);

  // TNew R2, size=1. (Count=1)
  tisc::Insn i2;
  i2.opcode = Opcode::TNew;
  i2.a = 2;
  i2.b = 1;
  // Insn struct: opcode, a, b, c, literal_kind.
  // TNew uses R(b) as size. So we need R1 to be int 1.
  // LoadImm R1, 1 -> R1 is Int(1).
  // TNew R2, R1.
  // Wait, TNew syntax: TNew R_dst, R_size_src
  // My test code used TNew with immediate? No, VM says:
  // case Opcode::TNew:
  //   std::int64_t size = state_.registers[insn.b];
  // So operand B is register index!
  // In my previous code: {Opcode::TNew, 2, 1, 0} -> R2 = TNew(R1). R1 has value 1. Correct.

  program.insns.push_back(i2);

  // R3 = 1
  tisc::Insn i3;
  i3.opcode = Opcode::LoadImm;
  i3.a = 3;
  i3.b = 1;
  program.insns.push_back(i3);

  // TNew R4, R3 (Count=2 > 1) -> Fail
  tisc::Insn i4;
  i4.opcode = Opcode::TNew;
  i4.a = 4;
  i4.b = 3;
  program.insns.push_back(i4);

  program.insns.push_back({Opcode::Halt});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(program);

  // Step 1: LoadImm
  auto r = vm->step();
  expect(r.has_value(), "step 1 ok");

  // Step 2: TNew (1st tensor)
  r = vm->step();
  expect(r.has_value(), "step 2 ok");

  // Step 3: LoadImm
  r = vm->step();
  expect(r.has_value(), "step 3 ok");

  // Step 4: TNew (2nd tensor) -> Should fail
  r = vm->step();
  expect(!r.has_value(), "step 4 fail");
  expect(r.error() == vm::Trap::SecurityFault, "step 4 trap");

  std::cout << "test_max_tensors passed.\n";
}

static void test_max_tensor_elements() {
  std::cout << "Running test_max_tensor_elements...\n";
  tisc::Program program;
  program.axion_policy_text = "(policy (max-tensor-elements 10))";

  // R1 = 5
  tisc::Insn i1;
  i1.opcode = Opcode::LoadImm;
  i1.a = 1;
  i1.b = 5;
  program.insns.push_back(i1);

  // TNew R2, R1 (size 5). Total=5. OK.
  tisc::Insn i2;
  i2.opcode = Opcode::TNew;
  i2.a = 2;
  i2.b = 1;
  program.insns.push_back(i2);

  // R3 = 6
  tisc::Insn i3;
  i3.opcode = Opcode::LoadImm;
  i3.a = 3;
  i3.b = 6;
  program.insns.push_back(i3);

  // TNew R4, R3 (size 6). Total=11 > 10. Fail.
  tisc::Insn i4;
  i4.opcode = Opcode::TNew;
  i4.a = 4;
  i4.b = 3;
  program.insns.push_back(i4);

  program.insns.push_back({Opcode::Halt});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(program);

  // Step 1: LoadImm
  auto r = vm->step();
  expect(r.has_value(), "step 1 ok");

  // Step 2: TNew (5 elements)
  r = vm->step();
  expect(r.has_value(), "step 2 ok");

  // Step 3: LoadImm
  r = vm->step();
  expect(r.has_value(), "step 3 ok");

  // Step 4: TNew (6 elements) -> Fail
  r = vm->step();
  expect(!r.has_value(), "step 4 fail");
  expect(r.error() == vm::Trap::SecurityFault, "step 4 trap");

  std::cout << "test_max_tensor_elements passed.\n";
}

static void test_max_symbolic_nodes() {
  std::cout << "Running test_max_symbolic_nodes...\n";
  tisc::Program program;
  // Allow 1 node total
  program.axion_policy_text = "(policy (max-symbolic-nodes 1))";

  program.symbol_pool.push_back(
      "A");  // index 0? No, VM: index 0 is invalid handle. pool[0] corresponds to handle 1.
  // intern_symbol: state_.symbols.push_back... return size.
  // Program::symbol_pool is copied to state_.symbols.
  // Handle 1 -> pool[0].
  program.symbol_pool.push_back("B");  // handle 2 -> pool[1].

  // LoadImm R1, handle 1 ("A")
  tisc::Insn i1;
  i1.opcode = Opcode::LoadImm;
  i1.a = 1;
  i1.b = 1;  // handle 1
  i1.literal_kind = tisc::LiteralKind::SymbolHandle;
  program.insns.push_back(i1);

  // SymLoad R2, R1 -> Create graph with node "A" (1 node). OK.
  tisc::Insn i2;
  i2.opcode = Opcode::SymLoad;
  i2.a = 2;
  i2.b = 1;
  program.insns.push_back(i2);

  // LoadImm R3, handle 2 ("B")
  tisc::Insn i3;
  i3.opcode = Opcode::LoadImm;
  i3.a = 3;
  i3.b = 2;  // handle 2
  i3.literal_kind = tisc::LiteralKind::SymbolHandle;
  program.insns.push_back(i3);

  // SymLoad R4, R3 -> Create graph with node "B". Total nodes = 1 + 1 = 2. Fail (max 1).
  tisc::Insn i4;
  i4.opcode = Opcode::SymLoad;
  i4.a = 4;
  i4.b = 3;
  program.insns.push_back(i4);

  program.insns.push_back({Opcode::Halt});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(program);

  // Step 1: Load "A"
  auto r = vm->step();
  expect(r.has_value(), "step 1 ok");

  // Step 2: SymLoad "A" (1 node)
  r = vm->step();
  expect(r.has_value(), "step 2 ok");

  // Step 3: Load "B"
  r = vm->step();
  expect(r.has_value(), "step 3 ok");

  // Step 4: SymLoad "B" (2nd node) -> Fail
  r = vm->step();
  expect(!r.has_value(), "step 4 fail");
  expect(r.error() == vm::Trap::SecurityFault, "step 4 trap");

  std::cout << "test_max_symbolic_nodes passed.\n";
}

int main() {
  test_max_tensors();
  test_max_tensor_elements();
  test_max_symbolic_nodes();
  return 0;
}
