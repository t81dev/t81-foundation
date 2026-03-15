// Tests for TISC v0.4 extensions (RFC-0005 §2.2 + §2.4):
//   ReadIsaVersion, VAdd, VFma, VLoad, VStore
//
// Acceptance criteria covered:
//   [A-0005-02] VLOAD/VSTORE: shape-validated reshape and copy, ShapeFault on mismatch
//   [A-0005-03] VADD/VFMA: elementwise ops on handles, ShapeFault on incompatible shapes
//   [A-0005-04] READ_ISA_VERSION: writes constant 4 to destination register

#include <cmath>
#include <vector>

#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/vm/vm.hpp"

using namespace t81;
using namespace t81::tisc;
using namespace t81::vm;

namespace {

// Helper: load tensor handle literal into register.
Insn load_tensor_handle(uint8_t reg, int pool_idx) {
  Insn i{Opcode::LoadImm, reg, static_cast<uint8_t>(pool_idx), 0};
  i.literal_kind = LiteralKind::TensorHandle;
  return i;
}

// Helper: load shape handle literal into register.
Insn load_shape_handle(uint8_t reg, int pool_idx) {
  Insn i{Opcode::LoadImm, reg, static_cast<uint8_t>(pool_idx), 0};
  i.literal_kind = LiteralKind::ShapeHandle;
  return i;
}

// -------------------------------------------------------------------------
// READ_ISA_VERSION [A-0005-04]
// -------------------------------------------------------------------------

int test_read_isa_version() {
  Program p;
  p.insns.push_back({Opcode::ReadIsaVersion, 1, 0, 0});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());
  T81_TEST_CHECK(vm->state().contexts[0].registers[1] == 4);
  return 0;
}

// -------------------------------------------------------------------------
// VADD [A-0005-03]
// -------------------------------------------------------------------------

int test_vadd_elementwise() {
  // [1, 2, 3] + [4, 5, 6] = [5, 7, 9]
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  p.tensor_pool.push_back(T729DynamicTensor({3}, {4.0f, 5.0f, 6.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back({Opcode::VAdd, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  auto handle = vm->state().contexts[0].registers[3];
  const auto& tensors = vm->state().tensors;
  T81_TEST_CHECK(handle >= 1 && static_cast<std::size_t>(handle) <= tensors.size());
  const auto& t = tensors[static_cast<std::size_t>(handle) - 1];
  T81_TEST_CHECK(t.has_value());
  T81_TEST_CHECK(std::fabs(t->value_at(0).value() - 5.0f) < 1e-5f);
  T81_TEST_CHECK(std::fabs(t->value_at(1).value() - 7.0f) < 1e-5f);
  T81_TEST_CHECK(std::fabs(t->value_at(2).value() - 9.0f) < 1e-5f);
  return 0;
}

int test_vadd_shape_fault() {
  // Shape mismatch: {3} + {2} → ShapeFault
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  p.tensor_pool.push_back(T729DynamicTensor({2}, {1.0f, 2.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back({Opcode::VAdd, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == Trap::ShapeFault);
  return 0;
}

// -------------------------------------------------------------------------
// VFMA [A-0005-03]
// -------------------------------------------------------------------------

int test_vfma_multiply_accumulate() {
  // RD (accum) = [10, 20], RS1 = [2, 3], RS2 = [3, 4]
  // Result = RS1 * RS2 + RD = [2*3+10, 3*4+20] = [16, 32]
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({2}, {10.0f, 20.0f}));  // accum  @ handle 1
  p.tensor_pool.push_back(T729DynamicTensor({2}, {2.0f,  3.0f}));   // src1   @ handle 2
  p.tensor_pool.push_back(T729DynamicTensor({2}, {3.0f,  4.0f}));   // src2   @ handle 3
  p.insns.push_back(load_tensor_handle(1, 1));  // R1 = accum
  p.insns.push_back(load_tensor_handle(2, 2));  // R2 = src1
  p.insns.push_back(load_tensor_handle(3, 3));  // R3 = src2
  // VFma R1, R2, R3 → R1 = R2*R3 + R1
  p.insns.push_back({Opcode::VFma, 1, 2, 3});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  auto handle = vm->state().contexts[0].registers[1];
  const auto& tensors = vm->state().tensors;
  T81_TEST_CHECK(handle >= 1 && static_cast<std::size_t>(handle) <= tensors.size());
  const auto& t = tensors[static_cast<std::size_t>(handle) - 1];
  T81_TEST_CHECK(t.has_value());
  T81_TEST_CHECK(std::fabs(t->value_at(0).value() - 16.0f) < 1e-4f);
  T81_TEST_CHECK(std::fabs(t->value_at(1).value() - 32.0f) < 1e-4f);
  return 0;
}

int test_vfma_shape_fault() {
  // Accumulator shape {2} vs src1 shape {3} — ShapeFault.
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({2}, {10.0f, 20.0f}));
  p.tensor_pool.push_back(T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  p.tensor_pool.push_back(T729DynamicTensor({3}, {1.0f, 2.0f, 3.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back(load_tensor_handle(3, 3));
  p.insns.push_back({Opcode::VFma, 1, 2, 3});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == Trap::ShapeFault);
  return 0;
}

// -------------------------------------------------------------------------
// VLOAD [A-0005-02]
// -------------------------------------------------------------------------

int test_vload_reshape() {
  // Reshape {6} → {2, 3}: element count matches → ok.
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({6}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}));
  p.shape_pool.push_back({2, 3});
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_shape_handle(2, 1));
  p.insns.push_back({Opcode::VLoad, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  auto handle = vm->state().contexts[0].registers[3];
  const auto& tensors = vm->state().tensors;
  T81_TEST_CHECK(handle >= 1 && static_cast<std::size_t>(handle) <= tensors.size());
  const auto& t = tensors[static_cast<std::size_t>(handle) - 1];
  T81_TEST_CHECK(t.has_value());
  T81_TEST_CHECK(t->shape() == std::vector<int>({2, 3}));
  T81_TEST_CHECK(std::fabs(t->value_at(0).value() - 1.0f) < 1e-5f);
  T81_TEST_CHECK(std::fabs(t->value_at(5).value() - 6.0f) < 1e-5f);
  return 0;
}

int test_vload_shape_fault() {
  // Reshape {5} → {2, 3}: element count mismatch (5 ≠ 6) → ShapeFault.
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({5}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f}));
  p.shape_pool.push_back({2, 3});
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_shape_handle(2, 1));
  p.insns.push_back({Opcode::VLoad, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == Trap::ShapeFault);
  return 0;
}

// -------------------------------------------------------------------------
// VSTORE [A-0005-02]
// -------------------------------------------------------------------------

int test_vstore_validated_copy() {
  // Source shape {3} == expected shape {3} → validated copy stored in RD.
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({3}, {7.0f, 8.0f, 9.0f}));
  p.shape_pool.push_back({3});
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_shape_handle(2, 1));
  p.insns.push_back({Opcode::VStore, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  auto handle = vm->state().contexts[0].registers[3];
  const auto& tensors = vm->state().tensors;
  T81_TEST_CHECK(handle >= 1 && static_cast<std::size_t>(handle) <= tensors.size());
  const auto& t = tensors[static_cast<std::size_t>(handle) - 1];
  T81_TEST_CHECK(t.has_value());
  T81_TEST_CHECK(t->shape() == std::vector<int>({3}));
  T81_TEST_CHECK(std::fabs(t->value_at(0).value() - 7.0f) < 1e-5f);
  T81_TEST_CHECK(std::fabs(t->value_at(2).value() - 9.0f) < 1e-5f);
  return 0;
}

int test_vstore_shape_fault() {
  // Source shape {3} ≠ expected shape {2} → ShapeFault.
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({3}, {7.0f, 8.0f, 9.0f}));
  p.shape_pool.push_back({2});
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_shape_handle(2, 1));
  p.insns.push_back({Opcode::VStore, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(!result.has_value());
  T81_TEST_CHECK(result.error() == Trap::ShapeFault);
  return 0;
}

// -------------------------------------------------------------------------
// Axion audit trail: v0.4 ops emit trace events [A-0005-05]
// -------------------------------------------------------------------------

int test_v04_axion_trace() {
  // VAdd should record an audit event with reason "VAdd kernel execution".
  Program p;
  p.tensor_pool.push_back(T729DynamicTensor({2}, {1.0f, 2.0f}));
  p.tensor_pool.push_back(T729DynamicTensor({2}, {3.0f, 4.0f}));
  p.insns.push_back(load_tensor_handle(1, 1));
  p.insns.push_back(load_tensor_handle(2, 2));
  p.insns.push_back({Opcode::VAdd, 3, 1, 2});
  p.insns.push_back({Opcode::Halt, 0, 0, 0});

  auto vm = make_interpreter_vm();
  vm->load_program(p);
  auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  const auto& log = vm->state().axion_log;
  bool found = std::any_of(log.begin(), log.end(), [](const auto& ev) {
    return ev.verdict.reason.find("VAdd kernel execution") != std::string::npos;
  });
  T81_TEST_CHECK(found);
  return 0;
}

}  // namespace

int main() {
  int failures = 0;
  failures += test_read_isa_version();
  failures += test_vadd_elementwise();
  failures += test_vadd_shape_fault();
  failures += test_vfma_multiply_accumulate();
  failures += test_vfma_shape_fault();
  failures += test_vload_reshape();
  failures += test_vload_shape_fault();
  failures += test_vstore_validated_copy();
  failures += test_vstore_shape_fault();
  failures += test_v04_axion_trace();
  return failures;
}
