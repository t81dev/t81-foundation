#include <cmath>
#include <memory>
#include <vector>
#include "test_runtime_check.hpp"

#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/native.hpp"
#include "t81/vm/vm.hpp"
#include "t81/weights.hpp"

using namespace t81;

int main() {
  [[maybe_unused]] tisc::Program program;
  program.insns.push_back({tisc::Opcode::TVecAdd, 3, 1, 2});
  program.insns.push_back({tisc::Opcode::TMatMul, 4, 5, 6});
  program.insns.push_back({tisc::Opcode::LoadImm, 9, 3, 0});
  program.insns.push_back({tisc::Opcode::I2F, 8, 9, 0});
  program.insns.push_back({tisc::Opcode::F2I, 10, 8, 0});
  program.insns.push_back({tisc::Opcode::I2Frac, 11, 9, 0});
  program.insns.push_back({tisc::Opcode::Frac2I, 12, 11, 0});
  program.insns.push_back({tisc::Opcode::TExp, 13, 1, 0});
  program.insns.push_back({tisc::Opcode::TVecMul, 14, 1, 2});
  program.insns.push_back({tisc::Opcode::TSiLU, 15, 1, 0});
  program.insns.push_back({tisc::Opcode::TSoftmax, 16, 1, 0});
  program.insns.push_back({tisc::Opcode::TTranspose, 17, 5, 0});
  program.insns.push_back({tisc::Opcode::TRMSNorm, 18, 1, 2});
  program.insns.push_back({tisc::Opcode::TRoPE, 19, 5, 9});
  program.insns.push_back({tisc::Opcode::TSqrt, 20, 1, 0});
  program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  [[maybe_unused]] auto vm = vm::make_interpreter_vm();
  vm->load_program(program);

  auto& mutable_state = const_cast<vm::State&>(vm->state());
  // Seed tensor pool with two vectors and two matrices.
  t81::T729DynamicTensor vecA({3}, {1.0f, 2.0f, 3.0f});
  t81::T729DynamicTensor vecB({3}, {4.0f, 5.0f, 6.0f});
  t81::T729DynamicTensor matA({2, 2}, {1.0f, 2.0f, 3.0f, 4.0f});
  t81::T729DynamicTensor matB({2, 2}, {5.0f, 6.0f, 7.0f, 8.0f});
  mutable_state.tensors.push_back(vecA);  // handle 1
  mutable_state.tensors.push_back(vecB);  // handle 2
  mutable_state.tensors.push_back(matA);  // handle 3
  mutable_state.tensors.push_back(matB);  // handle 4
  mutable_state.contexts[0].registers[1] = 1;
  mutable_state.contexts[0].registers[2] = 2;
  mutable_state.contexts[0].registers[5] = 3;
  mutable_state.contexts[0].registers[6] = 4;

  [[maybe_unused]] auto result = vm->run_to_halt();
  T81_TEST_CHECK(result.has_value());

  // Vector addition
  [[maybe_unused]] auto vecHandle = vm->state().contexts[0].registers[3];
  T81_TEST_CHECK(vecHandle == 5);  // 4th tensor inserted next index
  const auto& vecRes = mutable_state.tensors[static_cast<std::size_t>(vecHandle - 1)];
  T81_TEST_CHECK(vecRes.has_value());
  T81_TEST_CHECK(vecRes.value().shape()[0] == 3);
  T81_TEST_CHECK(vecRes.value().data()[0] == 5.0f && vecRes.value().data()[2] == 9.0f);

  // Matrix multiplication
  [[maybe_unused]] auto matHandle = vm->state().contexts[0].registers[4];
  const auto& matRes = mutable_state.tensors[static_cast<std::size_t>(matHandle - 1)];
  T81_TEST_CHECK(matRes.has_value());
  T81_TEST_CHECK(matRes.value().shape()[0] == 2 && matRes.value().shape()[1] == 2);
  T81_TEST_CHECK(static_cast<int>(matRes.value().data()[0]) == 19);  // 1*5 + 2*7

  // Conversion ops
  T81_TEST_CHECK(vm->state().contexts[0].registers[10] == 3);
  T81_TEST_CHECK(vm->state().contexts[0].registers[12] == 3);

  // Tensor unary exp
  [[maybe_unused]] auto expHandle = vm->state().contexts[0].registers[13];
  const auto& expRes = mutable_state.tensors[static_cast<std::size_t>(expHandle - 1)];
  T81_TEST_CHECK(expRes.has_value());
  T81_TEST_CHECK(expRes.value().shape()[0] == 3);
  T81_TEST_CHECK(expRes.value().has_canonical_fixed_data());
  T81_TEST_CHECK(std::fabs(expRes.value().data()[0] - std::exp(1.0f)) < 1e-4f);
  T81_TEST_CHECK(std::fabs(expRes.value().data()[1] - std::exp(2.0f)) < 1e-4f);
  T81_TEST_CHECK(std::fabs(expRes.value().data()[2] - std::exp(3.0f)) < 1e-4f);

  // Vector multiplication
  [[maybe_unused]] auto mulHandle = vm->state().contexts[0].registers[14];
  const auto& mulRes = mutable_state.tensors[static_cast<std::size_t>(mulHandle - 1)];
  T81_TEST_CHECK(mulRes.has_value());
  T81_TEST_CHECK(mulRes.value().shape()[0] == 3);
  T81_TEST_CHECK(std::fabs(mulRes.value().data()[0] - 4.0f) < 1e-6f);
  T81_TEST_CHECK(std::fabs(mulRes.value().data()[1] - 10.0f) < 1e-6f);
  T81_TEST_CHECK(std::fabs(mulRes.value().data()[2] - 18.0f) < 1e-6f);

  // SiLU and softmax must preserve shape and produce finite deterministic outputs.
  [[maybe_unused]] auto siluHandle = vm->state().contexts[0].registers[15];
  const auto& siluRes = mutable_state.tensors[static_cast<std::size_t>(siluHandle - 1)];
  T81_TEST_CHECK(siluRes.has_value());
  T81_TEST_CHECK(siluRes.value().shape()[0] == 3);
  T81_TEST_CHECK(siluRes.value().has_canonical_fixed_data());
  T81_TEST_CHECK(std::fabs(siluRes.value().data()[0] - (1.0f / (1.0f + std::exp(-1.0f)))) < 1e-4f);

  [[maybe_unused]] auto softmaxHandle = vm->state().contexts[0].registers[16];
  const auto& softmaxRes = mutable_state.tensors[static_cast<std::size_t>(softmaxHandle - 1)];
  T81_TEST_CHECK(softmaxRes.has_value());
  T81_TEST_CHECK(softmaxRes.value().shape()[0] == 3);
  T81_TEST_CHECK(softmaxRes.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  const float softmax_sum =
      softmaxRes.value().data()[0] + softmaxRes.value().data()[1] + softmaxRes.value().data()[2];
  T81_TEST_CHECK(std::fabs(softmax_sum - 1.0f) < 1e-5f);

  // Transpose of 2x2 matrix handle 3.
  [[maybe_unused]] auto transposeHandle = vm->state().contexts[0].registers[17];
  const auto& transposeRes = mutable_state.tensors[static_cast<std::size_t>(transposeHandle - 1)];
  T81_TEST_CHECK(transposeRes.has_value());
  T81_TEST_CHECK(transposeRes.value().shape()[0] == 2 && transposeRes.value().shape()[1] == 2);
  T81_TEST_CHECK(std::fabs(transposeRes.value().data()[0] - 1.0f) < 1e-6f);
  T81_TEST_CHECK(std::fabs(transposeRes.value().data()[1] - 3.0f) < 1e-6f);

  // RMSNorm and RoPE should match kernel implementations.
  [[maybe_unused]] auto rmsHandle = vm->state().contexts[0].registers[18];
  const auto& rmsRes = mutable_state.tensors[static_cast<std::size_t>(rmsHandle - 1)];
  T81_TEST_CHECK(rmsRes.has_value());
  T81_TEST_CHECK(rmsRes.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  auto rmsExpected = t81::ops::rmsnorm(vecA, vecB);
  T81_TEST_CHECK(rmsRes.value().shape() == rmsExpected.shape());
  T81_TEST_CHECK(rmsRes.value().data().size() == rmsExpected.data().size());
  for (std::size_t i = 0; i < rmsExpected.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(rmsRes.value().data()[i] - rmsExpected.data()[i]) < 1e-3f);
  }

  [[maybe_unused]] auto ropeHandle = vm->state().contexts[0].registers[19];
  const auto& ropeRes = mutable_state.tensors[static_cast<std::size_t>(ropeHandle - 1)];
  T81_TEST_CHECK(ropeRes.has_value());
  T81_TEST_CHECK(ropeRes.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  auto ropeExpected = t81::ops::rope(matA, 3);
  T81_TEST_CHECK(ropeRes.value().shape() == ropeExpected.shape());
  T81_TEST_CHECK(ropeRes.value().data().size() == ropeExpected.data().size());
  for (std::size_t i = 0; i < ropeExpected.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(ropeRes.value().data()[i] - ropeExpected.data()[i]) < 1e-3f);
  }

  // Tensor unary sqrt
  [[maybe_unused]] auto sqrtHandle = vm->state().contexts[0].registers[20];
  const auto& sqrtRes = mutable_state.tensors[static_cast<std::size_t>(sqrtHandle - 1)];
  T81_TEST_CHECK(sqrtRes.has_value());
  T81_TEST_CHECK(sqrtRes.value().shape()[0] == 3);
  T81_TEST_CHECK(sqrtRes.value().has_canonical_fixed_data());
  T81_TEST_CHECK(std::fabs(sqrtRes.value().data()[0] - std::sqrt(1.0f)) < 1e-4f);
  T81_TEST_CHECK(std::fabs(sqrtRes.value().data()[1] - std::sqrt(2.0f)) < 1e-4f);
  T81_TEST_CHECK(std::fabs(sqrtRes.value().data()[2] - std::sqrt(3.0f)) < 1e-4f);

  // Native balanced-trit weights should take the direct TExp fast path and
  // still expose the same tensor-level result surface.
  [[maybe_unused]] tisc::Program native_exp_program;
  native_exp_program.symbol_pool = {"weightsA"};
  auto native_model = std::make_shared<t81::weights::ModelFile>();
  t81::weights::NativeTensor native_exp_tensor;
  native_exp_tensor.format = t81::weights::NativeFormat::BalancedTernary;
  native_exp_tensor.shape = {2, 2};
  native_exp_tensor.trits = 4;
  native_exp_tensor.data = {40};  // [-1, -1, -1, -1]
  native_model->native["weightsA"] = native_exp_tensor;
  native_exp_program.weights_model = native_model;
  native_exp_program.insns.push_back({tisc::Opcode::WeightsLoad, 1, 1, 0});
  native_exp_program.insns.push_back({tisc::Opcode::TExp, 2, 1, 0});
  native_exp_program.insns.push_back({tisc::Opcode::TSiLU, 3, 1, 0});
  native_exp_program.insns.push_back({tisc::Opcode::TSoftmax, 4, 1, 0});
  native_exp_program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  [[maybe_unused]] auto native_exp_vm = vm::make_interpreter_vm();
  native_exp_vm->load_program(native_exp_program);
  [[maybe_unused]] auto native_exp_result = native_exp_vm->run_to_halt();
  T81_TEST_CHECK(native_exp_result.has_value());
  const auto native_exp_handle = native_exp_vm->state().contexts[0].registers[2];
  const auto& native_exp_res =
      const_cast<vm::State&>(native_exp_vm->state())
          .tensors[static_cast<std::size_t>(native_exp_handle - 1)];
  T81_TEST_CHECK(native_exp_res.has_value());
  T81_TEST_CHECK(native_exp_res.value().shape() == std::vector<int>({2, 2}));
  T81_TEST_CHECK(native_exp_res.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  T81_TEST_CHECK(std::fabs(native_exp_res.value().data()[0] - std::exp(-1.0f)) < 1e-4f);
  T81_TEST_CHECK(std::fabs(native_exp_res.value().data()[3] - std::exp(-1.0f)) < 1e-4f);

  const auto native_silu_handle = native_exp_vm->state().contexts[0].registers[3];
  const auto& native_silu_res =
      const_cast<vm::State&>(native_exp_vm->state())
          .tensors[static_cast<std::size_t>(native_silu_handle - 1)];
  T81_TEST_CHECK(native_silu_res.has_value());
  T81_TEST_CHECK(native_silu_res.value().shape() == std::vector<int>({2, 2}));
  T81_TEST_CHECK(native_silu_res.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  const float silu_neg_one = -1.0f / (1.0f + std::exp(1.0f));
  T81_TEST_CHECK(std::fabs(native_silu_res.value().data()[0] - silu_neg_one) < 1e-4f);
  T81_TEST_CHECK(std::fabs(native_silu_res.value().data()[3] - silu_neg_one) < 1e-4f);

  const auto native_softmax_handle = native_exp_vm->state().contexts[0].registers[4];
  const auto& native_softmax_res =
      const_cast<vm::State&>(native_exp_vm->state())
          .tensors[static_cast<std::size_t>(native_softmax_handle - 1)];
  T81_TEST_CHECK(native_softmax_res.has_value());
  T81_TEST_CHECK(native_softmax_res.value().shape() == std::vector<int>({2, 2}));
  T81_TEST_CHECK(native_softmax_res.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  const float softmax_row_sum0 =
      native_softmax_res.value().data()[0] + native_softmax_res.value().data()[1];
  const float softmax_row_sum1 =
      native_softmax_res.value().data()[2] + native_softmax_res.value().data()[3];
  T81_TEST_CHECK(std::fabs(softmax_row_sum0 - 1.0f) < 1e-5f);
  T81_TEST_CHECK(std::fabs(softmax_row_sum1 - 1.0f) < 1e-5f);

  [[maybe_unused]] tisc::Program native_rmsnorm_program;
  native_rmsnorm_program.symbol_pool = {"weightsA"};
  native_rmsnorm_program.tensor_pool.push_back(t81::T729DynamicTensor::from_host_float_data(
      {2}, std::vector<float>{0.5f, 1.25f}, t81::TensorNumericClass::HostFloat));
  native_rmsnorm_program.weights_model = native_model;
  native_rmsnorm_program.insns.push_back({tisc::Opcode::WeightsLoad, 1, 1, 0});
  tisc::Insn native_rms_weights{tisc::Opcode::LoadImm, 2, 1, 0};
  native_rms_weights.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  native_rmsnorm_program.insns.push_back(native_rms_weights);
  native_rmsnorm_program.insns.push_back({tisc::Opcode::TRMSNorm, 3, 1, 2});
  native_rmsnorm_program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  [[maybe_unused]] auto native_rms_vm = vm::make_interpreter_vm();
  native_rms_vm->load_program(native_rmsnorm_program);
  [[maybe_unused]] auto native_rms_result = native_rms_vm->run_to_halt();
  T81_TEST_CHECK(native_rms_result.has_value());
  const auto native_rms_handle = native_rms_vm->state().contexts[0].registers[3];
  const auto& native_rms_res =
      const_cast<vm::State&>(native_rms_vm->state())
          .tensors[static_cast<std::size_t>(native_rms_handle - 1)];
  T81_TEST_CHECK(native_rms_res.has_value());
  T81_TEST_CHECK(native_rms_res.value().shape() == std::vector<int>({2, 2}));
  T81_TEST_CHECK(native_rms_res.value().numeric_class() == t81::TensorNumericClass::HostFloat);
  const auto decoded_native = t81::tensor_native::decode(
      native_exp_tensor, t81::tensor_native::DecodeMode::StrictCanonical);
  T81_TEST_CHECK(decoded_native.has_value());
  const auto native_rms_expected =
      t81::ops::rmsnorm(*decoded_native, native_rmsnorm_program.tensor_pool[0]);
  T81_TEST_CHECK(native_rms_res.value().data().size() == native_rms_expected.data().size());
  for (std::size_t i = 0; i < native_rms_expected.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(native_rms_res.value().data()[i] - native_rms_expected.data()[i]) <
                   1e-4f);
  }

  [[maybe_unused]] tisc::Program native_rope_program;
  native_rope_program.symbol_pool = {"weightsA"};
  native_rope_program.weights_model = native_model;
  native_rope_program.insns.push_back({tisc::Opcode::WeightsLoad, 1, 1, 0});
  native_rope_program.insns.push_back({tisc::Opcode::LoadImm, 2, 3, 0});
  native_rope_program.insns.push_back({tisc::Opcode::TRoPE, 3, 1, 2});
  native_rope_program.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  [[maybe_unused]] auto native_rope_vm = vm::make_interpreter_vm();
  native_rope_vm->load_program(native_rope_program);
  [[maybe_unused]] auto native_rope_result = native_rope_vm->run_to_halt();
  T81_TEST_CHECK(native_rope_result.has_value());
  const auto native_rope_handle = native_rope_vm->state().contexts[0].registers[3];
  const auto& native_rope_res =
      const_cast<vm::State&>(native_rope_vm->state())
          .tensors[static_cast<std::size_t>(native_rope_handle - 1)];
  T81_TEST_CHECK(native_rope_res.has_value());
  T81_TEST_CHECK(native_rope_res.value().shape() == std::vector<int>({2, 2}));
  T81_TEST_CHECK(native_rope_res.value().numeric_class() == t81::TensorNumericClass::ExactInt);
  const auto native_rope_expected = t81::ops::rope(*decoded_native, 3);
  T81_TEST_CHECK(native_rope_res.value().data().size() == native_rope_expected.data().size());
  for (std::size_t i = 0; i < native_rope_expected.data().size(); ++i) {
    T81_TEST_CHECK(std::fabs(native_rope_res.value().data()[i] - native_rope_expected.data()[i]) <
                   1e-4f);
  }

  // Shape checks via literal handles.
  [[maybe_unused]] tisc::Program chk;
  chk.tensor_pool.push_back(t81::T729DynamicTensor({2, 2}, {1.0f, 0.0f, 0.0f, 1.0f}));
  chk.shape_pool.push_back({2, 2});
  chk.shape_pool.push_back({2, 3});
  tisc::Insn lt{tisc::Opcode::LoadImm, 1, 1, 0};
  lt.literal_kind = t81::tisc::LiteralKind::TensorHandle;
  chk.insns.push_back(lt);
  tisc::Insn ls{tisc::Opcode::LoadImm, 2, 1, 0};
  ls.literal_kind = t81::tisc::LiteralKind::ShapeHandle;
  chk.insns.push_back(ls);
  chk.insns.push_back({tisc::Opcode::ChkShape, 3, 1, 2});
  tisc::Insn ls_bad{tisc::Opcode::LoadImm, 4, 2, 0};
  ls_bad.literal_kind = t81::tisc::LiteralKind::ShapeHandle;
  chk.insns.push_back(ls_bad);
  chk.insns.push_back({tisc::Opcode::ChkShape, 5, 1, 4});
  chk.insns.push_back({tisc::Opcode::Halt, 0, 0, 0});

  [[maybe_unused]] auto vm_chk = vm::make_interpreter_vm();
  vm_chk->load_program(chk);
  [[maybe_unused]] auto res_chk = vm_chk->run_to_halt();
  T81_TEST_CHECK(res_chk.has_value());
  T81_TEST_CHECK(vm_chk->state().contexts[0].registers[3] == 1);
  T81_TEST_CHECK(vm_chk->state().contexts[0].registers[5] == 0);

  return 0;
}
