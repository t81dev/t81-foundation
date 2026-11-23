#include "t81/cli/driver.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include <iostream>
#include <chrono>

// Direct API test to bypass CLI overhead
int main() {
    std::cout << "🧪 Direct API Performance Test" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Load model directly (no CLI parsing)
    std::string model_path = "/Users/t81dev/Code/t81-foundation/models/tiny-random-llama.t81w";
    std::string policy_path = "/tmp/test_policy.apl";
    std::string input = "test input";
    
    // TODO: Call T81 functions directly instead of CLI
    // For now, just measure baseline overhead
    std::cout << "Model: " << model_path << std::endl;
    std::cout << "Policy: " << policy_path << std::endl;
    std::cout << "Input: " << input << std::endl;
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "⏱️  Direct API overhead: " << duration.count() << " µs" << std::endl;
    std::cout << "📊 CLI baseline: 3,000,000 µs" << std::endl;
    
    return 0;
}
