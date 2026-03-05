#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <cmath>
#include <cstring>

#include "t81/codec/ternary_quantization.hpp"

namespace {

std::string load_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void demonstrate_minimal_integration() {
    std::cout << "=== Minimal Integration Demo ===\n";
    
    // Simple policy allowing basic inference
    const std::string policy_text = R"(
# Basic inference policy
policy:
  name: "basic_inference"
  tier: 1
  rules:
    - action: "allow"
      opcode: "llama.cpp.infer"
      conditions:
        - "model_hash:verified"
    - action: "deny"
      opcode: "system.modify"
)";
    
    // Note: In a real scenario, you'd provide actual model path
    std::cout << "Policy loaded:\n" << policy_text << "\n";
    
    // Demonstrate ternary quantization
    std::vector<float> test_weights = {0.8f, -0.3f, 0.1f, 0.9f, -0.7f, 0.0f};
    
    std::cout << "\nOriginal weights: ";
    for (float w : test_weights) std::cout << w << " ";
    std::cout << "\n";
    
    // Quantize using T3_K
    auto t3k_quantized = t81::codec::T3_K_Quantizer::quantize(
        test_weights.data(), test_weights.size());
    auto t3k_dequantized = t81::codec::T3_K_Quantizer::dequantize(
        t3k_quantized.data(), test_weights.size());
    
    std::cout << "T3_K dequantized: ";
    for (float w : t3k_dequantized) std::cout << w << " ";
    std::cout << "\n";
    
    // Evaluate quantization quality
    auto metrics = t81::codec::evaluate_quantization(
        test_weights.data(), t3k_dequantized.data(), test_weights.size());
    
    std::cout << "Quantization metrics:\n";
    std::cout << "  MSE: " << metrics.mse << "\n";
    std::cout << "  RMSE: " << metrics.rmse << "\n";
    std::cout << "  SNR (dB): " << metrics.snr_db << "\n";
    std::cout << "  Compression ratio: " << metrics.compression_ratio << ":1\n";
    
    // Demonstrate adaptive quantization
    t81::codec::AdaptiveQuantizer adaptive(0.9f);
    auto adaptive_quantized = adaptive.quantize(test_weights.data(), test_weights.size());
    auto adaptive_dequantized = adaptive.dequantize(adaptive_quantized.data(), test_weights.size());
    
    std::cout << "\nAdaptive scheme selected: ";
    switch (adaptive.get_selected_scheme()) {
        case t81::codec::QuantizationScheme::T3_K:
            std::cout << "T3_K\n"; break;
        case t81::codec::QuantizationScheme::BASE81:
            std::cout << "BASE81\n"; break;
        default:
            std::cout << "UNKNOWN\n"; break;
    }
    
    std::cout << "Adaptive dequantized: ";
    for (float w : adaptive_dequantized) std::cout << w << " ";
    std::cout << "\n";
}

void demonstrate_policy_enforcement() {
    std::cout << "\n=== Policy Enforcement Demo ===\n";
    
    // Simple policy demonstration (without actual policy parsing)
    std::cout << "Policy framework demonstration:\n";
    std::cout << "  - Basic inference: ALLOWED\n";
    std::cout << "  - Weight modification: DENIED\n";
    std::cout << "  - Attention computation: LOGGED\n";
    std::cout << "  - System calls: DENIED\n";
    
    std::cout << "Policy enforcement would be handled by Axion kernel in full integration.\n";
}

void benchmark_quantization() {
    std::cout << "\n=== Quantization Benchmark ===\n";
    
    // Generate test data
    const size_t size = 100000;
    std::vector<float> large_weights(size);
    
    // Create realistic weight distribution (normal-like)
    std::srand(42);
    for (size_t i = 0; i < size; ++i) {
        // Simple approximation of normal distribution
        float sum = 0.0f;
        for (int j = 0; j < 6; ++j) {
            sum += (std::rand() / float(RAND_MAX)) * 2.0f - 1.0f;
        }
        large_weights[i] = sum / 6.0f;
    }
    
    // Benchmark T3_K
    auto start = std::chrono::high_resolution_clock::now();
    auto t3k_q = t81::codec::T3_K_Quantizer::quantize(large_weights.data(), size);
    auto t3k_dq = t81::codec::T3_K_Quantizer::dequantize(t3k_q.data(), size);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto t3k_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Benchmark Base-81
    start = std::chrono::high_resolution_clock::now();
    auto b81_q = t81::codec::Base81_Quantizer::quantize(large_weights.data(), size);
    auto b81_dq = t81::codec::Base81_Quantizer::dequantize(b81_q.data(), size);
    end = std::chrono::high_resolution_clock::now();
    
    auto b81_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Benchmark Adaptive
    start = std::chrono::high_resolution_clock::now();
    t81::codec::AdaptiveQuantizer adaptive(0.95f);
    auto adap_q = adaptive.quantize(large_weights.data(), size);
    auto adap_dq = adaptive.dequantize(adap_q.data(), size);
    end = std::chrono::high_resolution_clock::now();
    
    auto adap_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Performance results for " << size << " weights:\n";
    std::cout << "  T3_K:   " << t3k_time.count() << " μs\n";
    std::cout << "  Base-81: " << b81_time.count() << " μs\n";
    std::cout << "  Adaptive: " << adap_time.count() << " μs\n";
    
    // Quality comparison
    auto t3k_metrics = t81::codec::evaluate_quantization(
        large_weights.data(), t3k_dq.data(), size);
    auto b81_metrics = t81::codec::evaluate_quantization(
        large_weights.data(), b81_dq.data(), size);
    auto adap_metrics = t81::codec::evaluate_quantization(
        large_weights.data(), adap_dq.data(), size);
    
    std::cout << "\nQuality comparison:\n";
    std::cout << "  T3_K   SNR: " << t3k_metrics.snr_db << " dB, Compression: " << t3k_metrics.compression_ratio << ":1\n";
    std::cout << "  Base-81 SNR: " << b81_metrics.snr_db << " dB, Compression: " << b81_metrics.compression_ratio << ":1\n";
    std::cout << "  Adaptive SNR: " << adap_metrics.snr_db << " dB, Compression: " << adap_metrics.compression_ratio << ":1\n";
}

}  // anonymous namespace

int main() {
    std::cout << "T81 + llama.cpp Minimal Integration Demo\n";
    std::cout << "======================================\n";
    
    try {
        demonstrate_minimal_integration();
        demonstrate_policy_enforcement();
        benchmark_quantization();
        
        std::cout << "\n=== Demo completed successfully ===\n";
        std::cout << "Next steps:\n";
        std::cout << "1. Build with: cmake -DT81_ENABLE_LLAMA_CPP=ON\n";
        std::cout << "2. Run: ./build/minimal_integration_demo\n";
        std::cout << "3. Test with actual GGUF model files\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
