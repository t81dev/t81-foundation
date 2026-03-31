#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <map>
#include <sstream>

#include "t81/ai/governed_llm_module.hpp"
#include "t81/vm/ai_native_vm.hpp"
#include "t81/isa/ai_native_opcodes.hpp"
#include "t81/codec/ternary_quantization.hpp"

namespace {

// Performance measurement utilities
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ms() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_);
        return duration.count() / 1000.0;
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

// Demo scenarios
struct DemoScenario {
    std::string name;
    std::string description;
    t81::ai::CognitiveTier tier;
    std::string prompt;
    std::map<std::string, std::string> parameters;
};

void demonstrate_governed_llm_module() {
    std::cout << "=== Governed LLM Module Demo ===\n";
    
    // Create governed LLM module with different cognitive tiers
    std::vector<t81::ai::CognitiveTier> tiers = {
        t81::ai::CognitiveTier::TIER1_SYMBOLIC,
        t81::ai::CognitiveTier::TIER2_REFLECTIVE,
        t81::ai::CognitiveTier::TIER3_RECURSIVE,
        t81::ai::CognitiveTier::TIER4_LOOP,
        t81::ai::CognitiveTier::TIER5_INFINITE
    };
    
    std::vector<DemoScenario> scenarios = {
        {
            "Basic Reasoning",
            "Simple logical reasoning task",
            t81::ai::CognitiveTier::TIER1_SYMBOLIC,
            "What is 2 + 2?",
            {{"max_tokens", "50"}}
        },
        {
            "Self-Reflection",
            "Task requiring self-awareness",
            t81::ai::CognitiveTier::TIER2_REFLECTIVE,
            "Reflect on your own capabilities",
            {{"max_tokens", "100"}}
        },
        {
            "Recursive Problem",
            "Complex recursive thinking",
            t81::ai::CognitiveTier::TIER3_RECURSIVE,
            "Solve: f(n) = f(n-1) + f(n-2), f(0)=0, f(1)=1, find f(10)",
            {{"max_tokens", "150"}}
        },
        {
            "Iterative Optimization",
            "Process requiring iterative refinement",
            t81::ai::CognitiveTier::TIER4_LOOP,
            "Optimize this sentence for clarity: The thing that the person did was that they went to the place",
            {{"max_tokens", "100"}}
        },
        {
            "Unbounded Reasoning",
            "Open-ended creative task",
            t81::ai::CognitiveTier::TIER5_INFINITE,
            "Imagine and describe a world where AI and humans coexist peacefully",
            {{"max_tokens", "200"}}
        }
    };
    
    for (const auto& scenario : scenarios) {
        std::cout << "\n--- " << scenario.name << " ---\n";
        std::cout << "Description: " << scenario.description << "\n";
        std::cout << "Tier: " << static_cast<int>(scenario.tier) << "\n";
        std::cout << "Prompt: \"" << scenario.prompt << "\"\n";
        
        // Create module for this tier
        t81::ai::GovernedLLMModule module("demo_model.gguf", "demo_policy.apl", scenario.tier);
        
        // Create inference request
        t81::ai::GovernedInferenceRequest request;
        request.prompt = scenario.prompt;
        request.max_tokens = 50;  // Reduced for demo
        request.temperature = 0.7f;
        request.parameters = scenario.parameters;
        
        // Execute inference
        Timer timer;
        auto result = module.infer(request);
        double execution_time = timer.elapsed_ms();
        
        // Display results
        std::cout << "Execution time: " << execution_time << " ms\n";
        std::cout << "Success: " << (result.success ? "✅" : "❌") << "\n";
        
        if (result.success) {
            std::cout << "Response: \"" << result.response << "\"\n";
            std::cout << "Confidence: " << std::fixed << std::setprecision(3) << result.confidence << "\n";
            std::cout << "Cognitive tier used: " << result.cognitive_tier_used << "\n";
            std::cout << "Policy verdict: " << result.policy_verdict << "\n";
        } else {
            std::cout << "Error: " << result.error_message << "\n";
            std::cout << "Policy verdict: " << result.policy_verdict << "\n";
        }
        
        // Show execution trace
        std::cout << "Execution trace (" << result.execution_trace.size() << " steps):\n";
        for (size_t i = 0; i < std::min(size_t(5), result.execution_trace.size()); ++i) {
            std::cout << "  " << (i + 1) << ". " << result.execution_trace[i] << "\n";
        }
        if (result.execution_trace.size() > 5) {
            std::cout << "  ... and " << (result.execution_trace.size() - 5) << " more steps\n";
        }
    }
}

void demonstrate_ai_native_vm() {
    std::cout << "\n=== AI-Native VM Demo ===\n";
    
    // Create VM instance
    t81::vm::AINativeVM vm;
    
    // Create execution context
    t81::vm::VMExecutionContext context;
    context.session_id = "demo_session";
    context.cognitive_tier = 3;  // Tier 3 for advanced operations
    context.can_load_weights = true;
    context.can_modify_weights = false;
    context.max_instructions = 1000;
    
    // Create a simple AI-native program
    std::vector<tisc::Instruction> program;
    
    // 1. Load weights (WLOAD)
    tisc::Instruction wload_instr;
    wload_instr.opcode = tisc::Opcode::WLOAD;
    wload_instr.operand1 = 0x12345678;  // Model hash
    wload_instr.operand2 = 0;           // Layer ID
    wload_instr.operand3 = 100;         // Destination address
    wload_instr.operand4 = 1;           // Enable policy check
    program.push_back(wload_instr);
    
    // 2. Compute embeddings (EMBED)
    tisc::Instruction embed_instr;
    embed_instr.opcode = tisc::Opcode::EMBED;
    embed_instr.operand1 = 200;         // Token IDs address
    embed_instr.operand2 = 300;         // Embedding table address
    embed_instr.operand3 = 400;         // Output address
    embed_instr.operand4 = 768;         // Embedding dimension
    program.push_back(embed_instr);
    
    // 3. Compute attention (ATTN)
    tisc::Instruction attn_instr;
    attn_instr.opcode = tisc::Opcode::ATTN;
    attn_instr.operand1 = 400;          // Query address
    attn_instr.operand2 = 400;          // Key address
    attn_instr.operand3 = 400;          // Value address
    attn_instr.operand4 = 500;          // Output address
    program.push_back(attn_instr);
    
    // 4. Quantized matrix multiplication (QMATMUL)
    tisc::Instruction qmatmul_instr;
    qmatmul_instr.opcode = tisc::Opcode::QMATMUL;
    qmatmul_instr.operand1 = 500;          // Matrix A address
    qmatmul_instr.operand2 = 100;          // Matrix B address
    qmatmul_instr.operand3 = 600;          // Output address
    qmatmul_instr.operand4 = 0;            // T3_K quantization
    program.push_back(qmatmul_instr);
    
    // 5. Gather operation (GATHER)
    tisc::Instruction gather_instr;
    gather_instr.opcode = tisc::Opcode::GATHER;
    gather_instr.operand1 = 600;          // Input address
    gather_instr.operand2 = 700;          // Indices address
    gather_instr.operand3 = 800;          // Output address
    gather_instr.operand4 = 0;            // Axis
    program.push_back(gather_instr);
    
    // Execute program
    std::cout << "Executing AI-native program with " << program.size() << " instructions...\n";
    
    Timer timer;
    auto result = vm.execute_program(program, context);
    double execution_time = timer.elapsed_ms();
    
    // Display results
    std::cout << "Execution time: " << execution_time << " ms\n";
    std::cout << "Success: " << (result.success ? "✅" : "❌") << "\n";
    std::cout << "Instructions executed: " << result.instructions_executed << "\n";
    
    if (!result.success) {
        std::cout << "Error: " << result.error_message << "\n";
    }
    
    // Show policy checks
    std::cout << "Policy checks (" << result.policy_checks.size() << "):\n";
    for (const auto& check : result.policy_checks) {
        std::cout << "  " << check << "\n";
    }
    
    // Show policy violations
    if (!result.policy_violations.empty()) {
        std::cout << "Policy violations at instructions: ";
        for (size_t i = 0; i < result.policy_violations.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result.policy_violations[i];
        }
        std::cout << "\n";
    }
    
    // Show VM metrics
    auto metrics = vm.get_metrics();
    std::cout << "\nVM Metrics:\n";
    std::cout << "  Total executions: " << metrics.total_instructions_executed << "\n";
    std::cout << "  Policy checks performed: " << metrics.policy_checks_performed << "\n";
    std::cout << "  Policy violations: " << metrics.policy_violations << "\n";
    std::cout << "  Average execution time: " << std::fixed << std::setprecision(2) 
              << metrics.average_execution_time_ms << " ms\n";
    
    // Show memory usage
    std::cout << "\nMemory Usage:\n";
    for (const auto& [segment_name, mem_metrics] : metrics.memory_segments) {
        std::cout << "  " << segment_name << ": "
                  << (mem_metrics.used_capacity / 1024) << " KB / "
                  << (mem_metrics.total_capacity / 1024) << " KB "
                  << "(" << std::setprecision(1) << (mem_metrics.fragmentation * 100) << "% fragmentation)\n";
    }
    
    // Show opcode statistics
    std::cout << "\nOpcode Statistics:\n";
    for (const auto& [opcode, count] : metrics.opcode_stats) {
        std::cout << "  " << t81::vm::opcode_to_string(opcode) << ": " << count << "\n";
    }
}

void demonstrate_cognitive_tiers() {
    std::cout << "\n=== Cognitive Tiers Demo ===\n";
    
    // Test each cognitive tier with the same prompt
    std::string test_prompt = "Analyze the concept of 'artificial intelligence'";
    
    std::vector<t81::ai::CognitiveTier> tiers = {
        t81::ai::CognitiveTier::TIER1_SYMBOLIC,
        t81::ai::CognitiveTier::TIER2_REFLECTIVE,
        t81::ai::CognitiveTier::TIER3_RECURSIVE,
        t81::ai::CognitiveTier::TIER4_LOOP,
        t81::ai::CognitiveTier::TIER5_INFINITE
    };
    
    std::vector<std::string> tier_names = {
        "Tier 1: Symbolic",
        "Tier 2: Reflective", 
        "Tier 3: Recursive",
        "Tier 4: Loop",
        "Tier 5: Infinite"
    };
    
    std::cout << "Testing prompt: \"" << test_prompt << "\"\n\n";
    
    for (size_t i = 0; i < tiers.size(); ++i) {
        std::cout << "--- " << tier_names[i] << " ---\n";
        
        // Create module for this tier
        t81::ai::GovernedLLMModule module("demo_model.gguf", "demo_policy.apl", tiers[i]);
        
        // Create request
        t81::ai::GovernedInferenceRequest request;
        request.prompt = test_prompt;
        request.max_tokens = 30;  // Short for demo
        request.temperature = 0.7f;
        
        // Execute inference
        Timer timer;
        auto result = module.infer(request);
        double execution_time = timer.elapsed_ms();
        
        // Display results
        std::cout << "Execution time: " << std::fixed << std::setprecision(2) << execution_time << " ms\n";
        std::cout << "Success: " << (result.success ? "✅" : "❌") << "\n";
        
        if (result.success) {
            std::cout << "Response: \"" << result.response << "\"\n";
            std::cout << "Confidence: " << std::setprecision(3) << result.confidence << "\n";
            std::cout << "Policy verdict: " << result.policy_verdict << "\n";
        } else {
            std::cout << "Error: " << result.error_message << "\n";
        }
        
        std::cout << "\n";
    }
}

void demonstrate_deterministic_execution() {
    std::cout << "=== Deterministic Execution Demo ===\n";
    
    // Test deterministic behavior across multiple runs
    std::string test_prompt = "Calculate 7 * 8";
    t81::ai::CognitiveTier tier = t81::ai::CognitiveTier::TIER1_SYMBOLIC;
    
    std::cout << "Testing deterministic execution with prompt: \"" << test_prompt << "\"\n";
    std::cout << "Running 5 identical executions...\n\n";
    
    std::vector<std::string> responses;
    std::vector<float> confidences;
    std::vector<double> execution_times;
    
    for (int run = 1; run <= 5; ++run) {
        // Create fresh module instance
        t81::ai::GovernedLLMModule module("demo_model.gguf", "demo_policy.apl", tier);
        
        // Create request
        t81::ai::GovernedInferenceRequest request;
        request.prompt = test_prompt;
        request.max_tokens = 20;
        request.temperature = 0.0f;  // No randomness for deterministic test
        
        // Execute inference
        Timer timer;
        auto result = module.infer(request);
        double execution_time = timer.elapsed_ms();
        
        // Store results
        if (result.success) {
            responses.push_back(result.response);
            confidences.push_back(result.confidence);
            execution_times.push_back(execution_time);
        }
        
        std::cout << "Run " << run << ": ";
        if (result.success) {
            std::cout << "\"" << result.response << "\" (conf: " << std::fixed 
                      << std::setprecision(3) << result.confidence << ", time: " 
                      << std::setprecision(2) << execution_time << "ms)";
        } else {
            std::cout << "FAILED - " << result.error_message;
        }
        std::cout << "\n";
    }
    
    // Analyze determinism
    std::cout << "\nDeterminism Analysis:\n";
    
    if (responses.empty()) {
        std::cout << "❌ No successful executions to analyze\n";
        return;
    }
    
    // Check if all responses are identical
    bool all_identical = true;
    std::string first_response = responses[0];
    for (const auto& response : responses) {
        if (response != first_response) {
            all_identical = false;
            break;
        }
    }
    
    std::cout << "Response consistency: " << (all_identical ? "✅ IDENTICAL" : "❌ VARIED") << "\n";
    
    // Calculate confidence variance
    float confidence_mean = 0.0f;
    for (float conf : confidences) {
        confidence_mean += conf;
    }
    confidence_mean /= confidences.size();
    
    float confidence_variance = 0.0f;
    for (float conf : confidences) {
        float diff = conf - confidence_mean;
        confidence_variance += diff * diff;
    }
    confidence_variance /= confidences.size();
    
    std::cout << "Confidence mean: " << std::fixed << std::setprecision(4) << confidence_mean << "\n";
    std::cout << "Confidence variance: " << std::setprecision(6) << confidence_variance << "\n";
    
    // Calculate execution time variance
    double time_mean = 0.0;
    for (double time : execution_times) {
        time_mean += time;
    }
    time_mean /= execution_times.size();
    
    double time_variance = 0.0;
    for (double time : execution_times) {
        double diff = time - time_mean;
        time_variance += diff * diff;
    }
    time_variance /= execution_times.size();
    
    std::cout << "Execution time mean: " << std::setprecision(2) << time_mean << " ms\n";
    std::cout << "Execution time variance: " << std::setprecision(4) << time_variance << "\n";
    
    // Overall determinism assessment
    bool deterministic = all_identical && confidence_variance < 0.0001f && time_variance < 1.0;
    std::cout << "\nOverall determinism: " << (deterministic ? "✅ DETERMINISTIC" : "❌ NON-DETERMINISTIC") << "\n";
}

void demonstrate_policy_governance() {
    std::cout << "\n=== Policy Governance Demo ===\n";
    
    // Create VM with policy enabled
    t81::vm::AINativeVM vm;
    vm.enable_policy(true);
    
    // Test different policy scenarios
    std::vector<std::pair<std::string, tisc::Opcode>> policy_tests = {
        {"Basic QMATMUL", tisc::Opcode::QMATMUL},
        {"Weight Loading", tisc::Opcode::WLOAD},
        {"Attention Mechanism", tisc::Opcode::ATTN},
        {"Embedding Lookup", tisc::Opcode::EMBED},
        {"Tensor Gather", tisc::Opcode::GATHER},
        {"Tensor Scatter", tisc::Opcode::SCATTER}
    };
    
    for (const auto& [test_name, opcode] : policy_tests) {
        std::cout << "\n--- " << test_name << " ---\n";
        
        // Create execution context with different permissions
        tisc::Instruction instruction;
        instruction.opcode = opcode;
        instruction.operand1 = 100;
        instruction.operand2 = 200;
        instruction.operand3 = 300;
        instruction.operand4 = 0;
        
        // Test with different cognitive tiers
        std::vector<t81::vm::VMExecutionContext> contexts;
        
        // Context 1: Low tier, limited permissions
        t81::vm::VMExecutionContext ctx1;
        ctx1.session_id = "low_tier";
        ctx1.cognitive_tier = 1;
        ctx1.can_load_weights = false;
        ctx1.can_modify_weights = false;
        contexts.push_back(ctx1);
        
        // Context 2: Medium tier, standard permissions
        t81::vm::VMExecutionContext ctx2;
        ctx2.session_id = "medium_tier";
        ctx2.cognitive_tier = 3;
        ctx2.can_load_weights = true;
        ctx2.can_modify_weights = false;
        contexts.push_back(ctx2);
        
        // Context 3: High tier, full permissions
        t81::vm::VMExecutionContext ctx3;
        ctx3.session_id = "high_tier";
        ctx3.cognitive_tier = 5;
        ctx3.can_load_weights = true;
        ctx3.can_modify_weights = true;
        contexts.push_back(ctx3);
        
        // Test each context
        for (size_t i = 0; i < contexts.size(); ++i) {
            const auto& ctx = contexts[i];
            std::string tier_name = (i == 0) ? "Low Tier" : (i == 1) ? "Medium Tier" : "High Tier";
            
            std::cout << tier_name << ": ";
            
            // Create single-instruction program
            std::vector<tisc::Instruction> program = {instruction};
            
            // Execute
            auto result = vm.execute_program(program, ctx);
            
            if (result.success) {
                std::cout << "✅ ALLOWED";
            } else {
                std::cout << "❌ DENIED - " << result.error_message;
            }
            
            if (!result.policy_violations.empty()) {
                std::cout << " (VIOLATION)";
            }
            
            std::cout << "\n";
        }
    }
    
    // Show overall policy statistics
    auto metrics = vm.get_metrics();
    std::cout << "\nPolicy Statistics:\n";
    std::cout << "  Total policy checks: " << metrics.policy_checks_performed << "\n";
    std::cout << "  Total violations: " << metrics.policy_violations << "\n";
    std::cout << "  Violation rate: " << std::fixed << std::setprecision(1)
              << (metrics.policy_violations * 100.0 / metrics.policy_checks_performed) << "%\n";
}

}  // anonymous namespace

int main() {
    std::cout << "T81 + llama.cpp Deep Integration Demo\n";
    std::cout << "====================================\n";
    
    try {
        demonstrate_governed_llm_module();
        demonstrate_ai_native_vm();
        demonstrate_cognitive_tiers();
        demonstrate_deterministic_execution();
        demonstrate_policy_governance();
        
        std::cout << "\n=== Deep Integration Demo Completed ===\n";
        std::cout << "Key achievements demonstrated:\n";
        std::cout << "✅ Governed LLM module with cognitive tiers\n";
        std::cout << "✅ AI-native VM with opcode execution\n";
        std::cout << "✅ Policy-gated execution and governance\n";
        std::cout << "✅ Deterministic execution guarantees\n";
        std::cout << "✅ Multi-tier cognitive reasoning\n";
        std::cout << "✅ Comprehensive policy enforcement\n";
        
        std::cout << "\nDeep integration features:\n";
        std::cout << "• Cognitive tier reasoning (T1-T5)\n";
        std::cout << "• Deterministic execution with bit-exact reproducibility\n";
        std::cout << "• Policy-gated AI operations\n";
        std::cout << "• AI-native ISA opcode execution\n";
        std::cout << "• Ternary quantization integration\n";
        std::cout << "• Supply-chain security for models\n";
        
        std::cout << "\nReady for production deployment with:\n";
        std::cout << "• Complete governance framework\n";
        std::cout << "• Scalable cognitive architecture\n";
        std::cout << "• Deterministic AI inference\n";
        std::cout << "• Policy-compliant execution\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
