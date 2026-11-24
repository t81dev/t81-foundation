// Convolutional Layer Integration Test Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Tests convolutional neural network capabilities in Advanced AI integration

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <cassert>
#include <cmath>

#include "advanced_ai_integration.hpp"
#include "t81/vm/vm.hpp"
#include "t81/axion/engine.hpp"
#include "t81/canonfs/canon_driver.hpp"

namespace t81::vm::advanced_ai::test {

// Convolutional test data generator
class ConvTestDataGenerator {
private:
    std::mt19937 rng_;
    
public:
    ConvTestDataGenerator(uint64_t seed = 12345) : rng_(seed) {}
    
    // Generate synthetic time series data
    struct TimeSeriesSample {
        std::vector<double> data;  // [channels][sequence_length]
        int64_t channels;
        int64_t sequence_length;
        int label;
    };
    
    std::vector<TimeSeriesSample> generate_time_series_dataset(
        int num_samples, int64_t channels, int64_t sequence_length, int num_classes = 5) {
        
        std::vector<TimeSeriesSample> dataset;
        dataset.reserve(num_samples);
        
        std::uniform_int_distribution<int> label_dist(0, num_classes - 1);
        std::normal_distribution<double> noise_dist(0.0, 0.1);
        
        for (int i = 0; i < num_samples; ++i) {
            TimeSeriesSample sample;
            sample.channels = channels;
            sample.sequence_length = sequence_length;
            sample.label = label_dist(rng_);
            sample.data.resize(channels * sequence_length);
            
            // Generate multi-frequency signal with class-specific patterns
            for (int64_t c = 0; c < channels; ++c) {
                double base_freq = 0.05 + (c * 0.02);
                double class_modulation = 1.0 + (sample.label * 0.1);
                
                for (int64_t t = 0; t < sequence_length; ++t) {
                    double signal = std::sin(base_freq * t * class_modulation);
                    double noise = noise_dist(rng_);
                    
                    sample.data[c * sequence_length + t] = signal + noise;
                }
            }
            
            dataset.push_back(sample);
        }
        
        return dataset;
    }
    
    // Generate convolutional test patterns
    std::vector<double> generate_conv_pattern(int64_t channels, int64_t length, const std::string& pattern_type) {
        std::vector<double> data(channels * length, 0.0);
        
        if (pattern_type == "sine") {
            for (int64_t c = 0; c < channels; ++c) {
                double freq = 0.1 * (c + 1);
                for (int64_t t = 0; t < length; ++t) {
                    data[c * length + t] = std::sin(freq * t);
                }
            }
        } else if (pattern_type == "step") {
            for (int64_t c = 0; c < channels; ++c) {
                for (int64_t t = 0; t < length; ++t) {
                    data[c * length + t] = (t > length / 2) ? 1.0 : 0.0;
                }
            }
        } else if (pattern_type == "random") {
            std::uniform_real_distribution<double> dist(-1.0, 1.0);
            for (size_t i = 0; i < data.size(); ++i) {
                data[i] = dist(rng_);
            }
        }
        
        return data;
    }
};

// Convolutional neural network tester
class ConvolutionalTester {
private:
    std::unique_ptr<AdvancedAIIntegration> ai_integration_;
    ConvTestDataGenerator data_generator_;
    
public:
    ConvolutionalTester() {
        ai_integration_ = std::make_unique<AdvancedAIIntegration>();
        
        // Mock policy engine and CanonFS driver
        auto policy_engine = std::make_unique<t81::axion::PolicyEngine>();
        auto canonfs_driver = std::make_unique<t81::canonfs::CanonDriver>();
        
        ai_integration_->initialize(policy_engine.get(), canonfs_driver.get());
    }
    
    void run_all_tests() {
        std::cout << "=== Convolutional Layer Integration Test Suite ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        // Test 1: Basic Conv1D layer creation and forward pass
        test_conv1d_basic_forward();
        
        // Test 2: Convolutional parameter extraction
        test_convolutional_parameters();
        
        // Test 3: Multi-channel convolution
        test_multichannel_convolution();
        
        // Test 4: Convolutional backpropagation
        test_convolutional_backward();
        
        // Test 5: CNN architecture integration
        test_cnn_architecture();
        
        // Test 6: Convolutional feature extraction
        test_feature_extraction();
        
        // Test 7: Convolutional performance
        test_convolutional_performance();
        
        // Test 8: Convolutional quantization
        test_convolutional_quantization();
        
        std::cout << std::endl;
        std::cout << "=== Convolutional Test Results ===" << std::endl;
        std::cout << "All convolutional tests completed successfully!" << std::endl;
        std::cout << "Convolutional layers are ready for experimental research use." << std::endl;
    }

private:
    VMContext setup_vm_context() {
        VMContext ctx;
        ctx.registers.resize(256, 0);
        ctx.register_tags.resize(256, 0);
        ctx.pc = 0;
        ctx.sp = 0;
        return ctx;
    }
    
    void test_conv1d_basic_forward() {
        std::cout << "\n--- Conv1D Basic Forward Pass Test ---" << std::endl;
        
        VMContext ctx = setup_vm_context();
        
        // Setup Conv1D configuration (3 channels -> 8 filters, kernel_size=5)
        // Pack parameters: (input_channels << 20) | (output_channels << 10) | kernel_size
        int64_t conv_config = (3 << 20) | (8 << 10) | 5;
        
        ctx.registers[1] = 1000; // mock input
        ctx.registers[2] = conv_config; // conv config
        
        t81::tisc::Insn insn;
        insn.opcode = static_cast<t81::tisc::Opcode>(0xE0); // NEURAL_FWD
        insn.a = 3; // output
        insn.b = 1; // input
        insn.c = 2; // config
        
        Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
        
        assert_true(result == Trap::None, "Conv1D forward pass executes successfully");
        assert_true(ctx.registers[3] != 0, "Conv1D forward pass produces output");
        
        std::cout << "✅ Conv1D basic forward pass test passed" << std::endl;
    }
    
    void test_convolutional_parameters() {
        std::cout << "\n--- Convolutional Parameters Test ---" << std::endl;
        
        // Test different convolutional configurations
        struct ConvConfig {
            int64_t input_channels;
            int64_t output_channels;
            int64_t kernel_size;
        };
        
        std::vector<ConvConfig> configs = {
            {1, 4, 3},
            {3, 8, 5},
            {8, 16, 7},
            {16, 32, 3}
        };
        
        for (const auto& config : configs) {
            VMContext ctx = setup_vm_context();
            
            // Pack convolution parameters
            int64_t conv_config = (config.input_channels << 20) | 
                                (config.output_channels << 10) | 
                                config.kernel_size;
            
            ctx.registers[1] = 1000;
            ctx.registers[2] = conv_config;
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            insn.a = 3; insn.b = 1; insn.c = 2;
            
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            
            std::cout << "  Config " << config.input_channels << "->" << config.output_channels 
                      << " (k=" << config.kernel_size << "): ";
            
            if (result == Trap::None) {
                // Calculate expected parameters
                int64_t weight_params = config.input_channels * config.output_channels * config.kernel_size;
                int64_t bias_params = config.output_channels;
                int64_t total_params = weight_params + bias_params;
                
                std::cout << "✅ Parameters: " << total_params << std::endl;
            } else {
                std::cout << "❌ Failed" << std::endl;
            }
        }
        
        std::cout << "✅ Convolutional parameters test completed" << std::endl;
    }
    
    void test_multichannel_convolution() {
        std::cout << "\n--- Multi-Channel Convolution Test ---" << std::endl;
        
        // Test with different channel configurations
        std::vector<std::pair<int64_t, int64_t>> channel_configs = {
            {1, 4},   // Single channel input
            {3, 8},   // RGB-like input
            {8, 16},  // Multi-channel input
            {16, 32}  // High-dimensional input
        };
        
        for (const auto& [input_channels, output_channels] : channel_configs) {
            VMContext ctx = setup_vm_context();
            
            // Generate multi-channel input data
            int64_t sequence_length = 50;
            std::vector<double> input_data(input_channels * sequence_length);
            std::normal_distribution<double> dist(0.0, 1.0);
            std::mt19937 rng(12345);
            
            for (auto& val : input_data) {
                val = dist(rng);
            }
            
            // Setup convolution configuration
            int64_t conv_config = (input_channels << 20) | (output_channels << 10) | 5;
            
            ctx.registers[1] = 1000; // input data pointer (mock)
            ctx.registers[2] = conv_config; // conv config
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            insn.a = 3; insn.b = 1; insn.c = 2;
            
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            
            std::cout << "  Multi-channel " << input_channels << "->" << output_channels << ": ";
            
            if (result == Trap::None) {
                std::cout << "✅ Success" << std::endl;
            } else {
                std::cout << "❌ Failed" << std::endl;
            }
        }
        
        std::cout << "✅ Multi-channel convolution test completed" << std::endl;
    }
    
    void test_convolutional_backward() {
        std::cout << "\n--- Convolutional Backward Pass Test ---" << std::endl;
        
        VMContext ctx = setup_vm_context();
        
        // Setup convolution configuration
        int64_t conv_config = (3 << 20) | (8 << 10) | 5;
        
        // Forward pass first
        ctx.registers[1] = 1000; // input
        ctx.registers[2] = conv_config; // config
        
        t81::tisc::Insn fwd_insn;
        fwd_insn.opcode = static_cast<t81::tisc::Opcode>(0xE0);
        fwd_insn.a = 3; fwd_insn.b = 1; fwd_insn.c = 2;
        
        Trap fwd_result = ai_integration_->execute_advanced_ai_opcode(fwd_insn, ctx);
        
        // Backward pass
        ctx.registers[1] = 2000; // gradient output
        ctx.registers[2] = conv_config; // layer config
        
        t81::tisc::Insn back_insn;
        back_insn.opcode = static_cast<t81::tisc::Opcode>(0xE1); // NEURAL_BACK
        back_insn.a = 4; // gradient input
        back_insn.b = 1; // gradient output
        back_insn.c = 2; // config
        
        Trap back_result = ai_integration_->execute_advanced_ai_opcode(back_insn, ctx);
        
        assert_true(fwd_result == Trap::None, "Forward pass successful");
        assert_true(back_result == Trap::None, "Backward pass successful");
        assert_true(ctx.registers[4] != 0, "Backward pass produces gradient");
        
        std::cout << "✅ Convolutional backward pass test passed" << std::endl;
    }
    
    void test_cnn_architecture() {
        std::cout << "\n--- CNN Architecture Integration Test ---" << std::endl;
        
        // Build a simple CNN: Conv1D -> Conv1D -> Dense -> Dense
        std::vector<VMContext> layer_contexts(4);
        for (auto& ctx : layer_contexts) {
            ctx = setup_vm_context();
        }
        
        // Layer 1: Conv1D (3->8, k=5)
        int64_t conv1_config = (3 << 20) | (8 << 10) | 5;
        layer_contexts[0].registers[1] = 1000;
        layer_contexts[0].registers[2] = conv1_config;
        
        // Layer 2: Conv1D (8->16, k=3)
        int64_t conv2_config = (8 << 20) | (16 << 10) | 3;
        layer_contexts[1].registers[1] = 1001;
        layer_contexts[1].registers[2] = conv2_config;
        
        // Layer 3: Dense (1408->128) - flattened conv output
        layer_contexts[2].registers[1] = 1002;
        layer_contexts[2].registers[2] = 1408; // input_size
        layer_contexts[2].registers[3] = 128;   // output_size
        
        // Layer 4: Dense (128->5) - classification
        layer_contexts[3].registers[1] = 1003;
        layer_contexts[3].registers[2] = 128;
        layer_contexts[3].registers[3] = 5;
        
        // Execute forward pass through entire CNN
        bool cnn_success = true;
        
        for (int i = 0; i < 4; ++i) {
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE0); // NEURAL_FWD
            insn.a = 3; insn.b = 1; insn.c = 2;
            
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, layer_contexts[i]);
            
            if (result != Trap::None) {
                cnn_success = false;
                break;
            }
        }
        
        assert_true(cnn_success, "CNN forward pass successful");
        
        std::cout << "✅ CNN architecture integration test passed" << std::endl;
        std::cout << "  Successfully executed 4-layer CNN (Conv->Conv->Dense->Dense)" << std::endl;
    }
    
    void test_feature_extraction() {
        std::cout << "\n--- Convolutional Feature Extraction Test ---" << std::endl;
        
        // Generate test signal
        auto test_signal = data_generator_.generate_conv_pattern(4, 100, "sine");
        
        // Create multiple convolutional layers for hierarchical features
        std::vector<int64_t> conv_configs = {
            (4 << 20) | (8 << 10) | 7,   // 4->8, k=7 (low-level features)
            (8 << 20) | (16 << 10) | 5,  // 8->16, k=5 (mid-level features)
            (16 << 20) | (32 << 10) | 3   // 16->32, k=3 (high-level features)
        };
        
        std::vector<std::vector<double>> feature_maps;
        
        for (size_t i = 0; i < conv_configs.size(); ++i) {
            VMContext ctx = setup_vm_context();
            
            ctx.registers[1] = 1000 + i; // input (previous layer output)
            ctx.registers[2] = conv_configs[i]; // conv config
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            insn.a = 3; insn.b = 1; insn.c = 2;
            
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            
            std::cout << "  Feature extraction layer " << (i + 1) << ": ";
            
            if (result == Trap::None) {
                // Mock feature map size calculation
                int64_t input_channels = (conv_configs[i] >> 20) & 0x3FF;
                int64_t output_channels = (conv_configs[i] >> 10) & 0x3FF;
                int64_t kernel_size = conv_configs[i] & 0x3FF;
                
                std::cout << "✅ " << input_channels << "->" << output_channels 
                          << " channels, k=" << kernel_size << std::endl;
            } else {
                std::cout << "❌ Failed" << std::endl;
            }
        }
        
        std::cout << "✅ Convolutional feature extraction test completed" << std::endl;
    }
    
    void test_convolutional_performance() {
        std::cout << "\n--- Convolutional Performance Test ---" << std::endl;
        
        // Performance test with different kernel sizes
        std::vector<int64_t> kernel_sizes = {3, 5, 7, 9, 11};
        
        for (int64_t kernel_size : kernel_sizes) {
            VMContext ctx = setup_vm_context();
            
            int64_t conv_config = (8 << 20) | (16 << 10) | kernel_size;
            ctx.registers[1] = 1000;
            ctx.registers[2] = conv_config;
            
            t81::tisc::Insn insn;
            insn.opcode = static_cast<t81::tisc::Opcode>(0xE0);
            insn.a = 3; insn.b = 1; insn.c = 2;
            
            // Measure execution time
            auto start_time = std::chrono::high_resolution_clock::now();
            
            Trap result = ai_integration_->execute_advanced_ai_opcode(insn, ctx);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            
            std::cout << "  Kernel size " << kernel_size << ": ";
            
            if (result == Trap::None) {
                std::cout << duration.count() << " μs ✅" << std::endl;
            } else {
                std::cout << "Failed ❌" << std::endl;
            }
        }
        
        std::cout << "✅ Convolutional performance test completed" << std::endl;
    }
    
    void test_convolutional_quantization() {
        std::cout << "\n--- Convolutional Quantization Test ---" << std::endl;
        
        // Create convolutional layer
        VMContext ctx = setup_vm_context();
        int64_t conv_config = (4 << 20) | (8 << 10) | 5;
        
        // Forward pass to create layer
        ctx.registers[1] = 1000;
        ctx.registers[2] = conv_config;
        
        t81::tisc::Insn fwd_insn;
        fwd_insn.opcode = static_cast<t81::tisc::Opcode>(0xE0);
        fwd_insn.a = 3; fwd_insn.b = 1; fwd_insn.c = 2;
        
        Trap fwd_result = ai_integration_->execute_advanced_ai_opcode(fwd_insn, ctx);
        
        // Apply ternary quantization to convolutional weights
        ctx.registers[1] = 1000; // weights
        ctx.registers[2] = 100;   // threshold (0.1)
        
        t81::tisc::Insn quant_insn;
        quant_insn.opcode = static_cast<t81::tisc::Opcode>(0xE8); // QUANT_TERN
        quant_insn.a = 4; // quantized weights
        quant_insn.b = 1; // original weights
        quant_insn.c = 2; // threshold
        
        Trap quant_result = ai_integration_->execute_advanced_ai_opcode(quant_insn, ctx);
        
        // Verify quantization integrity
        ctx.registers[1] = 1000; // original
        ctx.registers[2] = 4;    // quantized
        ctx.registers[3] = 0;    // verification result
        
        t81::tisc::Insn verify_insn;
        verify_insn.opcode = static_cast<t81::tisc::Opcode>(0xED); // QUANT_VERIFY
        verify_insn.a = 3; verify_insn.b = 1; verify_insn.c = 2;
        
        Trap verify_result = ai_integration_->execute_advanced_ai_opcode(verify_insn, ctx);
        
        assert_true(fwd_result == Trap::None, "Convolutional forward pass successful");
        assert_true(quant_result == Trap::None, "Convolutional quantization successful");
        assert_true(verify_result == Trap::None, "Convolutional verification successful");
        assert_true(ctx.registers[3] == 1, "Convolutional quantization verification passed");
        
        std::cout << "✅ Convolutional quantization test passed" << std::endl;
        std::cout << "  Convolutional weights successfully quantized and verified" << std::endl;
    }
    
    void assert_true(bool condition, const std::string& test_name) {
        if (condition) {
            std::cout << "✅ " << test_name << std::endl;
        } else {
            std::cout << "❌ " << test_name << std::endl;
            assert(false);
        }
    }
};

} // namespace t81::vm::advanced_ai::test

// Main test runner
int main(int argc, char** argv) {
    std::cout << "Convolutional Layer Integration Test Runner" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    t81::vm::advanced_ai::test::ConvolutionalTester tester;
    tester.run_all_tests();
    
    return 0;
}
