#include "test_runtime_check.hpp"

#include "vm/internal/tensor_helpers.hpp"
#include "t81/tensor.hpp"

int main() {
  using t81::T729DynamicTensor;
  using t81::TensorNumericClass;
  using t81::vm::internal::tensor_elementwise_compatible;
  using t81::vm::internal::tensor_binary_elementwise;
  using t81::vm::internal::tensor_embed_checked;
  using t81::vm::internal::tensor_matmul_compatible;
  using t81::vm::internal::tensor_matmul_2d;
  using t81::vm::internal::tensor_rmsnorm_compatible;
  using t81::vm::internal::tensor_rmsnorm;
  using t81::vm::internal::tensor_rope_compatible;
  using t81::vm::internal::tensor_rope;
  using t81::vm::internal::tensor_contract_dot;
  using t81::vm::internal::tensor_softmax_compatible;
  using t81::vm::internal::tensor_transpose_2d;
  using t81::vm::internal::tensor_transpose_2d_compatible;
  using t81::vm::internal::tensor_unary_softmax;

  const T729DynamicTensor vec3({3}, {1.0f, 2.0f, 3.0f});
  const T729DynamicTensor vec2({2}, {4.0f, 5.0f});
  const T729DynamicTensor mat23({2, 3}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f});
  const T729DynamicTensor mat32({3, 2}, {1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f});
  const T729DynamicTensor mat22({2, 2}, {1.0f, 2.0f, 3.0f, 4.0f});
  const T729DynamicTensor scalar({}, {});
  const T729DynamicTensor trits({3}, {-1.0f, 0.0f, 1.0f});
  const T729DynamicTensor floats({3}, {0.5f, 1.5f, -2.5f});

  T81_TEST_CHECK(trits.numeric_class() == TensorNumericClass::ExactTrit);
  T81_TEST_CHECK(trits.strict_core_eligible());
  T81_TEST_CHECK(vec3.numeric_class() == TensorNumericClass::ExactInt);
  T81_TEST_CHECK(vec3.strict_core_eligible());
  T81_TEST_CHECK(floats.numeric_class() == TensorNumericClass::HostFloat);
  T81_TEST_CHECK(!floats.strict_core_eligible());
  T81_TEST_CHECK(floats.has_canonical_fixed_data());
  T81_TEST_CHECK(floats.canonical_fixed_authoritative());
  T81_TEST_CHECK(floats.value_at(1).has_value());
  T81_TEST_CHECK(std::fabs(*floats.value_at(1) - 1.5f) < 1e-6f);

  auto authority = floats;
  T81_TEST_CHECK(authority.set_value_at(0, 0.25f));
  T81_TEST_CHECK(authority.canonical_fixed_authoritative());
  T81_TEST_CHECK(authority.value_at(0).has_value());
  T81_TEST_CHECK(std::fabs(*authority.value_at(0) - 0.25f) < 1e-6f);
  authority.data()[0] = 0.75f;
  T81_TEST_CHECK(!authority.canonical_fixed_authoritative());
  T81_TEST_CHECK(authority.has_canonical_fixed_data());
  T81_TEST_CHECK(authority.canonical_fixed_authoritative());
  T81_TEST_CHECK(std::fabs(authority.data()[0] - 0.75f) < 1e-6f);

  T81_TEST_CHECK(tensor_elementwise_compatible(vec3, vec3));
  T81_TEST_CHECK(!tensor_elementwise_compatible(vec3, vec2));

  T81_TEST_CHECK(tensor_matmul_compatible(mat23, mat32));
  T81_TEST_CHECK(!tensor_matmul_compatible(mat23, mat22));

  T81_TEST_CHECK(tensor_rmsnorm_compatible(mat23, vec3));
  T81_TEST_CHECK(!tensor_rmsnorm_compatible(mat23, vec2));
  T81_TEST_CHECK(!tensor_rmsnorm_compatible(vec3, mat22));

  T81_TEST_CHECK(tensor_rope_compatible(mat22));
  T81_TEST_CHECK(!tensor_rope_compatible(vec3));

  T81_TEST_CHECK(tensor_softmax_compatible(vec3));
  T81_TEST_CHECK(!tensor_softmax_compatible(scalar));

  T81_TEST_CHECK(tensor_transpose_2d_compatible(mat22));
  T81_TEST_CHECK(!tensor_transpose_2d_compatible(vec3));

  const auto add_exact = tensor_binary_elementwise(trits, trits, false);
  T81_TEST_CHECK(add_exact.numeric_class() == TensorNumericClass::ExactInt);

  const auto mul_exact = tensor_binary_elementwise(trits, trits, true);
  T81_TEST_CHECK(mul_exact.numeric_class() == TensorNumericClass::ExactTrit);

  const auto add_float = tensor_binary_elementwise(floats, trits, false);
  T81_TEST_CHECK(add_float.numeric_class() == TensorNumericClass::HostFloat);
  T81_TEST_CHECK(add_float.has_canonical_fixed_data());

  const auto transposed = tensor_transpose_2d(mat22);
  T81_TEST_CHECK(transposed.numeric_class() == TensorNumericClass::ExactInt);

  const auto matmul = tensor_matmul_2d(mat23, mat32);
  T81_TEST_CHECK(matmul.numeric_class() == TensorNumericClass::ExactInt);

  const auto dot_exact = tensor_contract_dot(trits, trits);
  T81_TEST_CHECK(dot_exact.has_value());
  T81_TEST_CHECK(dot_exact->numeric_class() == TensorNumericClass::ExactInt);

  const auto dot_float = tensor_contract_dot(vec3, floats);
  T81_TEST_CHECK(dot_float.has_value());
  T81_TEST_CHECK(dot_float->numeric_class() == TensorNumericClass::HostFloat);

  auto embedded = tensor_embed_checked(mat23, 1);
  T81_TEST_CHECK(embedded.has_value());
  T81_TEST_CHECK(embedded->numeric_class() == TensorNumericClass::ExactInt);
  T81_TEST_CHECK(embedded->canonical_fixed_authoritative());
  T81_TEST_CHECK(embedded->shape() == std::vector<int>({3}));
  T81_TEST_CHECK(std::fabs(embedded->data()[0] - 4.0f) < 1e-6f);
  T81_TEST_CHECK(std::fabs(embedded->data()[1] - 5.0f) < 1e-6f);
  T81_TEST_CHECK(std::fabs(embedded->data()[2] - 6.0f) < 1e-6f);

  const auto softmax = tensor_unary_softmax(trits);
  T81_TEST_CHECK(softmax.numeric_class() == TensorNumericClass::ExactInt);
  T81_TEST_CHECK(softmax.strict_core_eligible());
  T81_TEST_CHECK(softmax.has_canonical_fixed_data());

  const auto rmsnorm = tensor_rmsnorm(mat23, vec3);
  T81_TEST_CHECK(rmsnorm.numeric_class() == TensorNumericClass::ExactInt);
  T81_TEST_CHECK(rmsnorm.strict_core_eligible());
  T81_TEST_CHECK(rmsnorm.shape() == mat23.shape());
  T81_TEST_CHECK(rmsnorm.has_canonical_fixed_data());

  const auto rope = tensor_rope(mat22, 3);
  T81_TEST_CHECK(rope.numeric_class() == TensorNumericClass::ExactInt);
  T81_TEST_CHECK(rope.strict_core_eligible());
  T81_TEST_CHECK(rope.shape() == mat22.shape());
  T81_TEST_CHECK(rope.has_canonical_fixed_data());

  return 0;
}
