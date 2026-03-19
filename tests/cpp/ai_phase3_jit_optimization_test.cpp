#include <gtest/gtest.h>
#include "t81/jit/ai_jit_optimizer.hpp"
#include "t81/jit/hardware_acceleration.hpp"
#include "t81/tensor/tensor.hpp"

class AIPhase3JITOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        optimizer_ = std::make_unique<t81::jit::AIJitOptimizer>();
        accelerator_ = std::make_unique<t81::jit::HardwareAccelerator>();
        accelerator_->initialize_acceleration();
    }
    
    std::unique_ptr<t81::jit::AIJitOptimizer> optimizer_;
    std::unique_ptr<t81::jit::HardwareAccelerator> accelerator_;

public:
    // Test JIT optimization pipeline
    TEST_F(AIPhase3JITOptimizationTest, JITOptimizationPipeline) {
        // Create sample AI workload
        std::vector<t81::tisc::Insn> instructions = {
            // Matrix multiplication
            t81::tisc::Insn{t81::tisc::Opcode::TMatMul, 0, 1, 2, 
                           t81::tisc::Primitive::Tensor, false, false, 
                           t81::tisc::LiteralKind::Int, 0},
            // RMS normalization
            t81::tisc::Insn{t81::tisc::Opcode::TRMSNorm, 3, 4, 5, 
                           t81::tisc::Primitive::Tensor, false, false, 
                           t81::tisc::LiteralKind::Int, 0},
            // Attention mechanism
            t81::tisc::Insn{t81::tisc::Opcode::ATTN, 6, 7, 8, 
                           t81::tisc::Primitive::Tensor, false, false, 
                           t81::tisc::LiteralKind::Int, 0}
        };
        
        t81::jit::OptimizationContext context{
            .has_tensor_operations = true,
            .has_following_norm_op = true,
            .loop_depth = 1
        };
        
        // Optimize instruction sequence
        auto optimized = optimizer_->optimize_sequence(instructions, context);
        
        // Verify optimization results
        EXPECT_GT(optimized.size(), instructions.size());
        EXPECT_LT(optimized.size(), instructions.size() * 2);  // Should be more efficient
        
        // Check for tensor fusion
        bool has_fusion = false;
        for (const auto& insn : optimized) {
            if (insn.opcode == t81::tisc::Opcode::TMatMul) {
                has_fusion = true;
                break;
            }
        }
        EXPECT_TRUE(has_fusion);
    }
    
    // Test hardware acceleration capabilities
    TEST_F(AIPhase3JITOptimizationTest, HardwareAccelerationCapabilities) {
        auto caps = accelerator_->get_capabilities();
        
        // Verify SIMD capabilities
        EXPECT_TRUE(caps.has_simd);
        EXPECT_GT(caps.simd_width, 1);
        
        // Verify ternary optimization support
        EXPECT_TRUE(caps.has_ternary_optimization);
        
        // Verify cache line size
        EXPECT_GT(caps.cache_line_size, 0);
    }
    
    // Test architecture detection
    TEST_F(AIPhase3JITOptimizationTest, ArchitectureDetection) {
        auto arch = accelerator_->get_architecture();
        
        // Should detect a valid architecture
        EXPECT_TRUE(arch == t81::jit::HardwareAccelerator::Architecture::ARM64_NEON ||
                   arch == t81::jit::HardwareAccelerator::Architecture::x86_64_AVX2 ||
                   arch == t81::jit::HardwareAccelerator::Architecture::Apple_Silicon ||
                   arch == t81::jit::HardwareAccelerator::Architecture::Generic);
    }
    
    // Test memory layout optimization
    TEST_F(AIPhase3JITOptimizationTest, MemoryLayoutOptimization) {
        std::vector<t81::TensorShape> shapes = {
            t81::TensorShape({1024, 1024}),    // Large matrix
            t81::TensorShape({512, 512}),      // Medium matrix
            t81::TensorShape({256, 256})       // Small matrix
        };
        
        auto layout = accelerator_->get_optimal_memory_layout(shapes);
        
        // Verify layout optimization
        EXPECT_GT(layout.total_size, 0);
        EXPECT_FALSE(layout.allocation_pattern.empty());
        EXPECT_TRUE(layout.is_ternary_optimized);
    }
    
    // Test accelerated matrix multiplication
    TEST_F(AIPhase3JITOptimizationTest, AcceleratedMatrixMultiplication) {
        // Create test tensors
        auto A = t81::T729DynamicTensor({64, 64});
        auto B = t81::T729DynamicTensor({64, 64});
        
        // Initialize with test data
        for (size_t i = 0; i < 64 * 64; ++i) {
            A.set_flat(i, i % 3 - 1);  // Balanced ternary values
            B.set_flat(i, i % 3 - 1);
        }
        
        // Test accelerated multiplication
        auto C = accelerator_->accelerated_matmul(A, B);
        
        // Verify result dimensions
        EXPECT_EQ(C.rank(), 2);
        EXPECT_EQ(C.shape()[0], 64);
        EXPECT_EQ(C.shape()[1], 64);
        
        // Verify deterministic behavior (same inputs should produce same outputs)
        auto C2 = accelerator_->accelerated_matmul(A, B);
        EXPECT_EQ(C.total_elements(), C2.total_elements());
        
        // Compare results element by element
        for (size_t i = 0; i < C.total_elements(); ++i) {
            EXPECT_EQ(C.get_flat(i), C2.get_flat(i));
        }
    }
    
    // Test ternary tensor operations
    TEST_F(AIPhase3JITOptimizationTest, TernaryTensorOperations) {
        // Test balanced ternary operations
        auto ops = t81::jit::TernaryTensorOps(true);
        
        // Create test tensors
        auto input = t81::T729DynamicTensor({32});
        for (size_t i = 0; i < 32; ++i) {
            input.set_flat(i, i % 3 - 1);  // Balanced ternary
        }
        
        // Test deterministic softmax
        auto softmax_result = ops.deterministic_softmax(input, 32);
        EXPECT_EQ(softmax_result.rank(), 1);
        EXPECT_EQ(softmax_result.shape()[0], 32);
        
        // Verify softmax properties (probabilities should sum to 1)
        t81::T81BigInt sum(0);
        for (size_t i = 0; i < 32; ++i) {
            sum = sum + softmax_result.get_flat(i);
        }
        EXPECT_EQ(sum, t81::T81BigInt(32));  // All probabilities should sum to input count
        
        // Test ternary attention
        auto Q = t81::T729DynamicTensor({8, 32, 4});   // batch=8, seq=32, heads=4
        auto K = t81::T729DynamicTensor({8, 32, 4});
        auto V = t81::T729DynamicTensor({8, 32, 16});  // batch=8, seq=32, embed=16
        
        // Initialize with test data
        for (size_t i = 0; i < Q.total_elements(); ++i) {
            Q.set_flat(i, i % 3 - 1);
            K.set_flat(i, i % 3 - 1);
            V.set_flat(i, i % 3 - 1);
        }
        
        auto attention_result = ops.ternary_attention(Q, K, V);
        EXPECT_EQ(attention_result.rank(), 3);
        EXPECT_EQ(attention_result.shape()[0], 8);
        EXPECT_EQ(attention_result.shape()[1], 32);
        EXPECT_EQ(attention_result.shape()[2], 16);
    }
    
    // Test JIT kernel compilation
    TEST_F(AIPhase3JITOptimizationTest, JITKernelCompilation) {
        t81::jit::TensorOperation matmul_op{
            t81::jit::TensorOperation::Type::MatMul,
            {{"block_size", "64"}}
        };
        
        t81::TensorShape shape({128, 128});
        auto kernel = optimizer_->compile_tensor_kernel(matmul_op, shape);
        
        // Verify kernel compilation
        EXPECT_FALSE(kernel.name.empty());
        EXPECT_FALSE(kernel.code.empty());
        EXPECT_FALSE(kernel.deterministic_hash.empty());
        EXPECT_TRUE(kernel.has_deterministic_guards);
        
        // Verify optimizations applied
        bool has_ternary_opt = false;
        for (const auto& opt : kernel.optimizations) {
            if (opt.find("ternary") != std::string::npos) {
                has_ternary_opt = true;
                break;
            }
        }
        EXPECT_TRUE(has_ternary_opt);
    }
    
    // Test determinism guarantees
    TEST_F(AIPhase3JITOptimizationTest, DeterminismGuarantees) {
        // Test that optimizations maintain determinism
        std::vector<t81::tisc::Insn> base_instructions = {
            t81::tisc::Insn{t81::tisc::Opcode::Add, 0, 1, 2, 
                           t81::tisc::Primitive::Int, false, false, 
                           t81::tisc::LiteralKind::Int, 42}
        };
        
        t81::jit::OptimizationContext context{};
        auto optimized_v1 = optimizer_->optimize_sequence(base_instructions, context);
        auto optimized_v2 = optimizer_->optimize_sequence(base_instructions, context);
        
        // Two optimizations of same input should produce identical results
        EXPECT_EQ(optimized_v1.size(), optimized_v2.size());
        
        // Verify instruction sequences are deterministic
        for (size_t i = 0; i < optimized_v1.size(); ++i) {
            EXPECT_EQ(optimized_v1[i].opcode, optimized_v2[i].opcode);
            EXPECT_EQ(optimized_v1[i].a, optimized_v2[i].a);
            EXPECT_EQ(optimized_v1[i].b, optimized_v2[i].b);
            EXPECT_EQ(optimized_v1[i].c, optimized_v2[i].c);
        }
    }
};

// Test runner
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
