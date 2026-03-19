#pragma once

#include "t81/tensor/tensor.hpp"
#include "t81/types/T81BigInt.hpp"
#include <vector>

namespace t81::jit {

// Hardware acceleration interface for AI workloads
// Provides optimized pathways for different CPU architectures

struct MemoryLayout {
    size_t total_size = 0;
    std::vector<size_t> allocation_order;
    std::string allocation_pattern;
    bool is_ternary_optimized = false;
};

class HardwareAccelerator {
public:
    enum class Architecture {
        Generic,
        ARM64_NEON,
        x86_64_AVX2,
        Apple_Silicon
    };
    
    struct AccelerationCapabilities {
        bool has_simd = false;
        bool has_tensor_cores = false;
        bool has_ternary_optimization = false;
        size_t simd_width = 1;
        size_t cache_line_size = 64;
    };
    
    explicit HardwareAccelerator();
    
    // Detect and initialize hardware capabilities
    bool initialize_acceleration();
    
    // Get optimal memory layout for current hardware
    MemoryLayout get_optimal_memory_layout(
        const std::vector<t81::TensorShape>& shapes) const;
    
    // Execute accelerated tensor operations
    t81::T729DynamicTensor accelerated_matmul(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B);
    
    t81::T729DynamicTensor accelerated_attention(
        const t81::T729DynamicTensor& Q,
        const t81::T729DynamicTensor& K,
        const t81::T729DynamicTensor& V);
    
    t81::T729DynamicTensor accelerated_embedding_lookup(
        const t81::T729DynamicTensor& input_ids,
        const t81::T729DynamicTensor& embedding_table);
    
    // Get hardware information
    Architecture get_architecture() const;
    const AccelerationCapabilities& get_capabilities() const;

private:
    Architecture arch_;
    AccelerationCapabilities caps_;
    
    // Hardware detection methods
    Architecture detect_architecture();
    AccelerationCapabilities detect_capabilities();
    
    // Architecture-specific implementations
    t81::T729DynamicTensor apple_silicon_matmul(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B);
    
    t81::T729DynamicTensor arm64_neon_matmul(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B);
    
    t81::T729DynamicTensor avx2_matmul(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B);
    
    t81::T729DynamicTensor generic_ternary_matmul(
        const t81::T729DynamicTensor& A,
        const t81::T729DynamicTensor& B);
};

} // namespace t81::jit
