#pragma once

#include <vector>
#include <string>
#include "t81/isa/ir.hpp"
#include "t81/tensor/tensor.hpp"

namespace t81::jit {

// AI JIT optimization context and configuration
struct OptimizationContext {
    bool has_tensor_operations = false;
    bool has_following_norm_op = false;
    size_t loop_depth = 0;
    std::vector<t81::TensorShape> tensor_shapes;
};

struct TensorOperation {
    enum class Type {
        MatMul,
        Attention,
        Embedding,
        RMSNorm,
        Generic
    };
    
    Type type;
    std::map<std::string, std::string> config;
};

struct KernelConfig {
    bool use_ternary_optimization = true;
    bool enable_deterministic_guards = true;
    size_t optimization_level = 2;  // 0=none, 1=basic, 2=aggressive
};

struct CompiledKernel {
    std::string name;
    std::string code;
    std::string deterministic_hash;
    std::vector<std::string> optimizations;
    std::string guard_code;
    bool has_deterministic_guards = false;
};

struct MemoryLayout {
    size_t total_size = 0;
    std::vector<size_t> allocation_order;
    std::string allocation_pattern;
    bool is_ternary_optimized = false;
};

// Main AI JIT Optimizer class for Phase 3 Integration
class AIJitOptimizer {
public:
    struct OptimizationConfig {
        bool enable_tensor_fusion = true;
        bool enable_attention_optimization = true;
        bool enable_memory_pool_optimization = true;
        bool enable_deterministic_caching = true;
        size_t min_tensor_size_for_optimization = 1024;
    };

    explicit AIJitOptimizer(OptimizationConfig config = {});
    
    // Optimize TISC instruction sequence for AI workloads
    std::vector<t81::tisc::Insn> optimize_sequence(
        const std::vector<t81::tisc::Insn>& instructions,
        const OptimizationContext& context);
    
    // Generate specialized tensor operation kernels
    CompiledKernel compile_tensor_kernel(
        const TensorOperation& op,
        const t81::TensorShape& input_shape);
    
    // Optimize memory layout for tensor operations
    MemoryLayout optimize_memory_layout(
        const std::vector<t81::TensorShape>& shapes);

private:
    OptimizationConfig config_;
    
    bool should_optimize_instruction(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    std::vector<t81::tisc::Insn> apply_optimization(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    std::vector<t81::tisc::Insn> optimize_matmul_sequence(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    std::vector<t81::tisc::Insn> optimize_attention_sequence(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    // Kernel compilation methods
    CompiledKernel compile_matmul_kernel(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    CompiledKernel compile_attention_kernel(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    CompiledKernel compile_embedding_kernel(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    CompiledKernel compile_rmsnorm_kernel(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    CompiledKernel compile_generic_kernel(
        const TensorOperation& op,
        const t81::TensorShape& shape);
    
    std::vector<t81::tisc::Insn> apply_global_optimizations(
        const std::vector<t81::tisc::Insn>& instructions,
        const OptimizationContext& context);
    
    void add_deterministic_guards(CompiledKernel& kernel);
    
    // Ternary-specific optimization methods
    std::string generate_ternary_matmul_code(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    std::string generate_ternary_attention_code(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    std::string generate_ternary_embedding_code(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    std::string generate_ternary_rmsnorm_code(
        const t81::TensorShape& shape,
        const KernelConfig& config);
    
    std::string generate_ternary_generic_code(
        const TensorOperation& op,
        const t81::TensorShape& shape);
    
    // Utility methods
    std::string calculate_kernel_hash(const std::string& code);
    
    bool can_fuse_matmul(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    std::vector<t81::tisc::Insn> create_fused_matmul_kernel(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    std::vector<t81::tisc::Insn> apply_ternary_matmul_optimizations(
        const t81::tisc::Insn& insn);
    
    std::vector<t81::tisc::Insn> apply_ternary_attention_optimizations(
        const t81::tisc::Insn& insn,
        const OptimizationContext& context);
    
    // Helper methods for creating optimized instructions
    t81::tisc::Insn create_ternary_balance_check(const t81::tisc::Insn& base);
    t81::tisc::Insn create_optimized_matmul_loop(const t81::tisc::Insn& base);
    t81::tisc::Insn create_deterministic_result_check(const t81::tisc::Insn& base);
};

} // namespace t81::jit
