#include "t81/direct_api/t81_direct_api.hpp"
#include <chrono>
#include <iostream>
#include <vector>

using namespace t81::direct_api;

int main() {
    std::cout << "🚀 T81 Direct API Implementation Test" << std::endl;
    
    // Create API instance
    auto api = create_direct_api();
    
    // Initialize with model and policy
    std::cout << "\n📋 Initializing API..." << std::endl;
    if (!api->initialize(
        "/Users/t81dev/Code/t81-foundation/models/tiny-random-llama.t81w",
        "/tmp/test_policy.apl")) {
        std::cerr << "❌ Failed to initialize API" << std::endl;
        return 1;
    }
    
    // Test single inference
    std::cout << "\n🧪 Single Inference Test:" << std::endl;
    auto result = api->assess_fixed("test input");
    std::cout << "⏱️  Execution time: " << result.execution_time_ms << " ms" << std::endl;
    std::cout << "📊 Success: " << (result.success ? "YES" : "NO") << std::endl;
    std::cout << "🔤 Decision: " << result.decision << std::endl;
    std::cout << "📝 Reason: " << result.reason_code << std::endl;
    
    // Test batch processing
    std::cout << "\n🧪 Batch Processing Test:" << std::endl;
    std::vector<std::string> inputs;
    for (int i = 1; i <= 10; ++i) {
        inputs.push_back("test input " + std::to_string(i));
    }
    
    auto batch_results = api->batch_assess_fixed(inputs);
    
    // Performance comparison
    std::cout << "\n📊 Performance Analysis:" << std::endl;
    std::cout << "• CLI baseline: ~3,200 ms per inference" << std::endl;
    std::cout << "• Direct API: " << result.execution_time_ms << " ms per inference" << std::endl;
    
    double speedup = 3200.0 / result.execution_time_ms;
    std::cout << "🚀 Speedup: " << speedup << "x faster" << std::endl;
    
    // Statistics
    auto stats = api->get_stats();
    std::cout << "\n📈 Statistics:" << std::endl;
    std::cout << "• Total inferences: " << stats.total_inferences << std::endl;
    std::cout << "• Average time: " << stats.avg_time_ms << " ms" << std::endl;
    std::cout << "• Cache hits: " << stats.cache_hits << std::endl;
    std::cout << "• Cache misses: " << stats.cache_misses << std::endl;
    
    // Success criteria
    if (speedup >= 100.0) {
        std::cout << "\n✅ SUCCESS: 100x speedup achieved!" << std::endl;
        std::cout << "🎯 Phase 2 implementation complete!" << std::endl;
    } else {
        std::cout << "\n❌ Need more optimization (current: " << speedup << "x)" << std::endl;
    }
    
    // Test JSON output
    std::cout << "\n📄 JSON Output Example:" << std::endl;
    std::cout << result.to_json() << std::endl;
    
    return 0;
}
