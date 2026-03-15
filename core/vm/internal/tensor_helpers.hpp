#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/tensor/mutation.hpp"
#include "t81/support/expected.hpp"
#include "t81/vm/state.hpp"
#include "t81/weights.hpp"

namespace t81::vm::internal {

enum class TensorAllocPolicyResult {
  Allow = 0,
  MaxTensorsExceeded,
  MaxTensorElementsExceeded,
};

enum class TensorDecodeMode {
  StrictCanonical = 0,
  Lenient,
};

std::size_t tensor_shape_complexity(const t81::T729DynamicTensor& tensor);

TensorAllocPolicyResult evaluate_tensor_alloc_policy(const State& state,
                                                     std::size_t tensor_elements);

std::size_t store_tensor_slot(State& state, t81::T729DynamicTensor tensor);

void account_tensor_allocation(State& state, std::size_t tensor_elements);

std::optional<t81::T729DynamicTensor> decode_native_tensor(const t81::weights::NativeTensor& native,
                                                           TensorDecodeMode mode);

std::optional<t81::weights::NativeTensor> parse_canon_tensor_object(
    const std::vector<std::byte>& bytes);

std::optional<t81::T729DynamicTensor> decode_canon_tensor_object(
    const std::vector<std::byte>& bytes);

std::optional<t81::canonfs::CanonRef> parse_canon_tensor_ref(std::string_view hash_text);

enum class TensorLoadHashStatus {
  Ok = 0,
  InvalidHash,
  CanonFsMiss,
  DecodeFault,
};

struct TensorLoadHashResult {
  TensorLoadHashStatus status{TensorLoadHashStatus::DecodeFault};
  std::optional<t81::T729DynamicTensor> tensor;
};

TensorLoadHashResult load_canon_tensor_by_hash(t81::canonfs::Driver& driver,
                                               std::string_view hash_text);

t81::T729DynamicTensor tensor_unary_exp(const t81::T729DynamicTensor& tensor);
t81::T729DynamicTensor tensor_unary_sqrt(const t81::T729DynamicTensor& tensor);
t81::T729DynamicTensor tensor_unary_silu(const t81::T729DynamicTensor& tensor);
t81::T729DynamicTensor tensor_unary_softmax(const t81::T729DynamicTensor& tensor);
bool tensor_elementwise_compatible(const t81::T729DynamicTensor& lhs,
                                   const t81::T729DynamicTensor& rhs);
bool tensor_softmax_compatible(const t81::T729DynamicTensor& tensor);
t81::T729DynamicTensor tensor_matmul_2d(const t81::T729DynamicTensor& lhs,
                                        const t81::T729DynamicTensor& rhs);
bool tensor_matmul_compatible(const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs);
t81::T729DynamicTensor tensor_binary_elementwise(const t81::T729DynamicTensor& lhs,
                                                 const t81::T729DynamicTensor& rhs, bool multiply);
bool tensor_transpose_2d_compatible(const t81::T729DynamicTensor& tensor);
t81::T729DynamicTensor tensor_transpose_2d(const t81::T729DynamicTensor& tensor);
std::optional<t81::T729DynamicTensor> tensor_contract_dot(const t81::T729DynamicTensor& lhs,
                                                          const t81::T729DynamicTensor& rhs);
t81::T729DynamicTensor tensor_rmsnorm(const t81::T729DynamicTensor& tensor,
                                      const t81::T729DynamicTensor& weights);
bool tensor_rmsnorm_compatible(const t81::T729DynamicTensor& tensor,
                               const t81::T729DynamicTensor& weights);
t81::T729DynamicTensor tensor_rope(const t81::T729DynamicTensor& tensor, int pos);
bool tensor_rope_compatible(const t81::T729DynamicTensor& tensor);
bool tensor_attention_compatible(const t81::T729DynamicTensor& q, const t81::T729DynamicTensor& k,
                                 const t81::T729DynamicTensor& v);
bool tensor_embed_compatible(const t81::T729DynamicTensor& table, std::int64_t index);

std::optional<t81::T729DynamicTensor> tensor_new_1d(std::int64_t size);
t81::T729DynamicTensor tensor_identity_copy(const t81::T729DynamicTensor& tensor);
std::optional<float> tensor_get_at(const t81::T729DynamicTensor& tensor, std::int64_t index);
bool tensor_set_at(t81::T729DynamicTensor& tensor, std::int64_t index, float value,
                   t81::tensor_mutation::ScalarWriteKind source_kind);

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vec_binary_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs, bool multiply);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_transpose_checked(
    const t81::T729DynamicTensor& tensor);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_matmul_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_contract_dot_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_attention_checked(
    const t81::T729DynamicTensor& q, const t81::T729DynamicTensor& k, const t81::T729DynamicTensor& v);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_embed_checked(
    const t81::T729DynamicTensor& table, std::int64_t index);
// RFC-0005 v0.4 vector helpers.
// VLoad: reshape src to new_shape (fault if element counts differ).
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vload_checked(
    const t81::T729DynamicTensor& src, const std::vector<int>& new_shape);
// VStore: validate src shape == expected_shape, then return a canonical copy.
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vstore_checked(
    const t81::T729DynamicTensor& src, const std::vector<int>& expected_shape);
// VFma: fused multiply-accumulate — result = src1 * src2 + accumulator.
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vfma_checked(
    const t81::T729DynamicTensor& accumulator, const t81::T729DynamicTensor& src1,
    const t81::T729DynamicTensor& src2);

// RFC-0026 phase-1 extension (AI-M5: axis-aware gather/scatter).
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_wload_checked(
    const t81::T729DynamicTensor& src);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_gather_checked(
    const t81::T729DynamicTensor& src, std::int64_t index, int axis = 0);
std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_scatter_checked(
    const t81::T729DynamicTensor& dst, std::int64_t index, const t81::T729DynamicTensor& src,
    int axis = 0);
std::expected<float, t81::vm::Trap> tensor_get_checked(const t81::T729DynamicTensor& tensor,
                                                       std::int64_t index);
std::expected<void, t81::vm::Trap> tensor_set_checked(t81::T729DynamicTensor& tensor,
                                                      std::int64_t index, float value,
                                                      t81::tensor_mutation::ScalarWriteKind source_kind);

}  // namespace t81::vm::internal
