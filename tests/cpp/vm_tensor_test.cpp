#include <cassert>
#include <vector>

#include "t81/vm/vm.hpp"
#include "t81/tisc/program.hpp"
#include "t81/tensor.hpp"

using namespace t81;

int main() {
  tisc::Program program;
  program.insns.push_back({tisc::Opcode::TVecAdd, 3, 1, 2});
  program.insns.push_back({tisc::Opcode::TMatMul, 4, 5, 6});
  program.insns.push_back({tisc::Opcode::TTenDot, 7, 1, 2});
  program.insns.push_back({tisc::Opcode::LoadImm, 9, 3, 0});
  program.insns.push_back({tisc::Opcode::I2F, 8, 9, 0});
  program.insns.push_back({tisc::Opcode::F2I, 10, 8, 0});
  program.insns.push_back({tisc::Opcode::I2Frac, 11, 9, 0});
  program.insns.push_back({tisc::Opcode::Frac2I, 12, 11, 0});
  program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  auto vm = vm::make_interpreter_vm();
  vm->load_program(program);

  auto& mutable_state = const_cast<vm::State&>(vm->state());
  // Seed tensor pool with two vectors and two matrices.
  t81::T729Tensor vecA({3}, {1.0f, 2.0f, 3.0f});
  t81::T729Tensor vecB({3}, {4.0f, 5.0f, 6.0f});
  t81::T729Tensor matA({2, 2}, {1.0f, 2.0f, 3.0f, 4.0f});
  t81::T729Tensor matB({2, 2}, {5.0f, 6.0f, 7.0f, 8.0f});
  mutable_state.tensors.push_back(vecA); // handle 1
  mutable_state.tensors.push_back(vecB); // handle 2
  mutable_state.tensors.push_back(matA); // handle 3
  mutable_state.tensors.push_back(matB); // handle 4
  mutable_state.registers[1] = 1;
  mutable_state.registers[2] = 2;
  mutable_state.registers[5] = 3;
  mutable_state.registers[6] = 4;

  auto result = vm->run_to_halt();
  assert(result.has_value());

  // Vector addition
  auto vecHandle = vm->state().registers[3];
  assert(vecHandle == 5); // 4th tensor inserted next index
  const auto& vecRes = mutable_state.tensors[static_cast<std::size_t>(vecHandle - 1)];
  assert(vecRes.shape()[0] == 3);
  assert(vecRes.data()[0] == 5.0f && vecRes.data()[2] == 9.0f);

  // Matrix multiplication
  auto matHandle = vm->state().registers[4];
  const auto& matRes = mutable_state.tensors[static_cast<std::size_t>(matHandle - 1)];
  assert(matRes.shape()[0] == 2 && matRes.shape()[1] == 2);
  assert(static_cast<int>(matRes.data()[0]) == 19); // 1*5 + 2*7

  // Dot product (vector handles reused)
  auto dotHandle = vm->state().registers[7];
  const auto& dotRes = mutable_state.tensors[static_cast<std::size_t>(dotHandle - 1)];
  assert(dotRes.rank() == 1);
  assert(dotRes.data()[0] == 32.0f);

  // Conversion ops
  assert(vm->state().registers[10] == 3);
  assert(vm->state().registers[12] == 3);

  return 0;
}
