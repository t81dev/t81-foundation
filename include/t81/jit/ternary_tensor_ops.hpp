#pragma once

#include "t81/tensor/tensor.hpp"
#include "t81/types/T81BigInt.hpp"
#include <vector>
#include <stdexcept>

namespace t81::jit {

// Ternary-optimized tensor operations for AI workloads
// Implements balanced ternary arithmetic optimizations

class TernaryTensorOps {
public:
    explicit TernaryTensorOps(bool enable_optimizations = true);
    
    // Core AI operations with ternary optimizations
    t81::T729DynamicTensor balanced_ternary_matmul(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B);
    
    t81::T729DynamicTensor deterministic_softmax(
        const t81::T729DynamicTensor& input,
        size_t dimension);
    
    t81::T729DynamicTensor ternary_attention(
        const t81::T729DynamicTensor& Q,
        const t81::T729DynamicTensor& K,
        const t81::T729DynamicTensor& V);
    
    t81::T729DynamicTensor ternary_embedding_lookup(
        const t81::T729DynamicTensor& input_ids,
        const t81::T729DynamicTensor& embedding_table);
    
    t81::T729DynamicTensor ternary_rmsnorm(
        const t81::T729DynamicTensor& input,
        const std::vector<size_t>& normalized_dims);

private:
    bool enable_optimizations_;
    
    // Block processing methods
    void process_ternary_matmul_block(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B,
        t81::T729DynamicTensor& C,
        size_t i_start, size_t k_start, size_t j_start,
        size_t block_i, size_t block_j);
    
    // Helper operations
    t81::T729DynamicTensor balanced_ternary_transpose(
        const t81::T729DynamicTensor& input);
    
    t81::T729DynamicTensor balanced_ternary_attention_multiply(
        const t81::T729DynamicTensor& attention_weights,
        const t81::T729DynamicTensor& V);
    
    // Deterministic mathematical operations
    t81::T81BigInt balanced_ternary_multiply(
        const t81::T81BigInt& a, const t81::T81BigInt& b);
    
    t81::T81BigInt balanced_ternary_divide(
        const t81::T81BigInt& numerator, const t81::T81BigInt& denominator);
    
    t81::T81BigInt deterministic_ternary_exp(const t81::T81BigInt& x);
    t81::T81BigInt deterministic_ternary_sqrt(const t81::T81BigInt& x);
    
    // Verification methods
    void verify_ternary_balance(const t81::T729DynamicTensor& tensor);
    bool is_balanced_ternary(const t81::T81BigInt& value);
    
    void assert_attention_determinism(const t81::T729DynamicTensor& output);
    void assert_rmsnorm_determinism(const t81::T729DynamicTensor& output);
    void assert_embedding_balance(const t81::T729DynamicTensor& embeddings);
    
    t81::T729DynamicTensor deterministic_softmax_scaling(
        const t81::T729DynamicTensor& scores);
};

} // namespace t81::jit
