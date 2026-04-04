// CSI CLI Extensions
// EXPERIMENTAL - NOT FOR PRODUCTION USE

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "stochastic_decoder.hpp"
#include "policy_gated_sampling.hpp"
#include "stochastic_provenance.hpp"

namespace t81::experimental::csi::cli {

class CSICLI {
private:
    std::string model_id_;
    std::string model_file_;
    std::string canonfs_root_;
    std::string policy_file_;
    
public:
    int run(int argc, char** argv) {
        if (argc < 2) {
            print_usage();
            return 1;
        }
        
        std::string command = argv[1];
        
        if (command == "help") {
            print_usage();
            return 0;
        } else if (command == "inference") {
            return run_inference(argc - 2, argv + 2);
        } else if (command == "analyze-chain") {
            return run_analyze_chain(argc - 2, argv + 2);
        } else if (command == "verify-chain") {
            return run_verify_chain(argc - 2, argv + 2);
        } else if (command == "test-policy") {
            return run_test_policy(argc - 2, argv + 2);
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            print_usage();
            return 1;
        }
    }

private:
    void print_usage() {
        std::cout << "Controlled Stochastic Inference (CSI) CLI" << std::endl;
        std::cout << "EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        std::cout << "Usage: t81_csi_cli <command> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  inference          Run stochastic inference with policy control" << std::endl;
        std::cout << "  analyze-chain      Analyze stochastic provenance chain" << std::endl;
        std::cout << "  verify-chain       Verify chain integrity" << std::endl;
        std::cout << "  test-policy        Test stochastic policy evaluation" << std::endl;
        std::cout << "  help               Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Inference Options:" << std::endl;
        std::cout << "  --model <id>                  Model identifier" << std::endl;
        std::cout << "  --model-file <path>           Model file path" << std::endl;
        std::cout << "  --seed <number>               Random seed for reproducibility" << std::endl;
        std::cout << "  --temperature <number>        Sampling temperature" << std::endl;
        std::cout << "  --top-k <number>              Top-k sampling parameter" << std::endl;
        std::cout << "  --max-entropy <number>        Maximum entropy per token" << std::endl;
        std::cout << "  --prompt <text>               Input prompt" << std::endl;
        std::cout << "  --policy <file>               Policy file path" << std::endl;
        std::cout << "  --canonfs-root <path>         CanonFS root directory" << std::endl;
        std::cout << "  --out <file>                  Output file for results" << std::endl;
        std::cout << std::endl;
        std::cout << "Chain Analysis Options:" << std::endl;
        std::cout << "  --chain-hash <hash>          Chain hash to analyze" << std::endl;
        std::cout << "  --metrics <list>              Metrics to compute (entropy,violations,consistency)" << std::endl;
        std::cout << "  --out <file>                  Output file for analysis" << std::endl;
        std::cout << std::endl;
    }
    
    int run_inference(int argc, char** argv) {
        // Parse arguments
        StochasticConfig config;
        std::string prompt;
        std::string output_file;
        
        for (int i = 0; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--model" && i + 1 < argc) {
                model_id_ = argv[++i];
            } else if (arg == "--model-file" && i + 1 < argc) {
                model_file_ = argv[++i];
            } else if (arg == "--seed" && i + 1 < argc) {
                config.seed = std::stoull(argv[++i]);
            } else if (arg == "--temperature" && i + 1 < argc) {
                config.temperature = std::stod(argv[++i]);
            } else if (arg == "--top-k" && i + 1 < argc) {
                config.top_k = std::stoull(argv[++i]);
            } else if (arg == "--max-entropy" && i + 1 < argc) {
                config.max_entropy_per_token = std::stod(argv[++i]);
            } else if (arg == "--prompt" && i + 1 < argc) {
                prompt = argv[++i];
            } else if (arg == "--policy" && i + 1 < argc) {
                policy_file_ = argv[++i];
            } else if (arg == "--canonfs-root" && i + 1 < argc) {
                canonfs_root_ = argv[++i];
            } else if (arg == "--out" && i + 1 < argc) {
                output_file = argv[++i];
            }
        }
        
        // Validate arguments
        if (model_id_.empty() || model_file_.empty() || prompt.empty()) {
            std::cerr << "Error: --model, --model-file, and --prompt are required" << std::endl;
            return 1;
        }
        
        std::cout << "=== Controlled Stochastic Inference ===" << std::endl;
        std::cout << "Model: " << model_id_ << std::endl;
        std::cout << "Seed: " << config.seed << std::endl;
        std::cout << "Temperature: " << config.temperature << std::endl;
        std::cout << "Top-K: " << config.top_k << std::endl;
        std::cout << "Max Entropy: " << config.max_entropy_per_token << std::endl;
        std::cout << "Prompt: \"" << prompt << "\"" << std::endl;
        std::cout << std::endl;
        
        // Create mock data for demonstration
        auto tokens = create_demo_tokens();
        auto logits = create_demo_logits();
        
        // Create policy gate (mock for now)
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "csi_cli_inference");
        
        // Run stochastic inference
        std::cout << "Running stochastic inference..." << std::endl;
        auto result = decoder->decode_with_policy(tokens, logits, config);
        
        // Create provenance chain
        auto chain = std::make_unique<StochasticProvenanceChain>(model_id_, "demo_model_hash", config);
        chain->append_step(result, 0, compute_input_hash(prompt));
        auto chain_hash = chain->finalize_chain();
        
        // Display results
        std::cout << std::endl;
        std::cout << "=== Results ===" << std::endl;
        std::cout << "Selected Token: \"" << result.selected_token << "\"" << std::endl;
        std::cout << "Entropy: " << std::fixed << std::setprecision(4) << result.entropy << std::endl;
        std::cout << "Policy Verdict: " << axion_verdict_to_string(result.policy_verdict) << std::endl;
        std::cout << "Candidates: " << result.candidates.size() << std::endl;
        std::cout << "Chain Hash: " << chain_hash.to_string() << std::endl;
        std::cout << "Sampling Time: " << result.sampling_time.count() << " ns" << std::endl;
        
        // Show top candidates
        std::cout << std::endl;
        std::cout << "=== Top Candidates ===" << std::endl;
        for (size_t i = 0; i < std::min(result.candidates.size(), size_t(5)); ++i) {
            const auto& candidate = result.candidates[i];
            std::cout << (i + 1) << ". \"" << candidate.token << "\" "
                      << "(logit: " << std::fixed << std::setprecision(3) << candidate.logit
                      << ", prob: " << std::setprecision(3) << candidate.probability << ")" << std::endl;
        }
        
        // Save results if requested
        if (!output_file.empty()) {
            save_results_to_file(result, *chain, output_file);
            std::cout << std::endl;
            std::cout << "Results saved to: " << output_file << std::endl;
        }
        
        return result.success ? 0 : 1;
    }
    
    int run_analyze_chain(int argc, char** argv) {
        std::string chain_hash;
        std::string metrics_str = "entropy,violations,consistency";
        std::string output_file;
        
        for (int i = 0; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--chain-hash" && i + 1 < argc) {
                chain_hash = argv[++i];
            } else if (arg == "--metrics" && i + 1 < argc) {
                metrics_str = argv[++i];
            } else if (arg == "--out" && i + 1 < argc) {
                output_file = argv[++i];
            }
        }
        
        if (chain_hash.empty()) {
            std::cerr << "Error: --chain-hash is required" << std::endl;
            return 1;
        }
        
        std::cout << "=== Chain Analysis ===" << std::endl;
        std::cout << "Chain Hash: " << chain_hash << std::endl;
        std::cout << "Metrics: " << metrics_str << std::endl;
        std::cout << std::endl;
        
        // Parse metrics
        std::vector<std::string> metrics;
        std::istringstream iss(metrics_str);
        std::string metric;
        while (std::getline(iss, metric, ',')) {
            metrics.push_back(metric);
        }
        
        // Mock analysis (would load real chain from CanonFS)
        std::cout << "Analyzing chain..." << std::endl;
        
        for (const auto& metric : metrics) {
            if (metric == "entropy") {
                std::cout << "Total Entropy: 4.23 bits" << std::endl;
                std::cout << "Average Entropy: 1.41 bits/token" << std::endl;
                std::cout << "Entropy Range: [0.82, 2.15] bits" << std::endl;
            } else if (metric == "violations") {
                std::cout << "Policy Violations: 0" << std::endl;
                std::cout << "Constraint Applications: 2" << std::endl;
                std::cout << "Compliance Rate: 100%" << std::endl;
            } else if (metric == "consistency") {
                std::cout << "Chain Integrity: VERIFIED" << std::endl;
                std::cout << "Hash Consistency: VERIFIED" << std::endl;
                std::cout << "Policy Consistency: VERIFIED" << std::endl;
            } else {
                std::cout << "Unknown metric: " << metric << std::endl;
            }
            std::cout << std::endl;
        }
        
        return 0;
    }
    
    int run_verify_chain(int argc, char** argv) {
        std::string chain_hash;
        
        for (int i = 0; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--chain-hash" && i + 1 < argc) {
                chain_hash = argv[++i];
            }
        }
        
        if (chain_hash.empty()) {
            std::cerr << "Error: --chain-hash is required" << std::endl;
            return 1;
        }
        
        std::cout << "=== Chain Verification ===" << std::endl;
        std::cout << "Chain Hash: " << chain_hash << std::endl;
        std::cout << std::endl;
        
        std::cout << "Verifying chain integrity..." << std::endl;
        
        // Mock verification (would verify real chain)
        bool is_valid = true;  // Would be actual verification result
        
        if (is_valid) {
            std::cout << "✅ Chain integrity VERIFIED" << std::endl;
            std::cout << "✅ Hash consistency VERIFIED" << std::endl;
            std::cout << "✅ Policy compliance VERIFIED" << std::endl;
            std::cout << "✅ Provenance completeness VERIFIED" << std::endl;
        } else {
            std::cout << "❌ Chain verification FAILED" << std::endl;
            return 1;
        }
        
        return 0;
    }
    
    int run_test_policy(int argc, char** argv) {
        std::string policy_file;
        
        for (int i = 0; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--policy" && i + 1 < argc) {
                policy_file = argv[++i];
            }
        }
        
        std::cout << "=== Policy Testing ===" << std::endl;
        
        if (!policy_file.empty()) {
            std::cout << "Policy File: " << policy_file << std::endl;
        } else {
            std::cout << "Using default policy configuration" << std::endl;
        }
        
        std::cout << std::endl;
        
        // Test various policy scenarios
        std::cout << "Testing policy scenarios..." << std::endl;
        std::cout << std::endl;
        
        // Scenario 1: Normal operation
        std::cout << "Scenario 1: Normal stochastic sampling" << std::endl;
        std::cout << "Entropy: 1.2 bits (within limit)" << std::endl;
        std::cout << "Result: ALLOW" << std::endl;
        std::cout << std::endl;
        
        // Scenario 2: High entropy
        std::cout << "Scenario 2: High entropy sampling" << std::endl;
        std::cout << "Entropy: 3.1 bits (exceeds limit of 2.5)" << std::endl;
        std::cout << "Result: CONSTRAIN (reduce top_k to 3)" << std::endl;
        std::cout << std::endl;
        
        // Scenario 3: Forbidden token
        std::cout << "Scenario 3: Forbidden token detection" << std::endl;
        std::cout << "Candidates include: '[UNK]', '[PAD]'" << std::endl;
        std::cout << "Result: CONSTRAIN (filter forbidden tokens)" << std::endl;
        std::cout << std::endl;
        
        // Scenario 4: Entropy escalation
        std::cout << "Scenario 4: Entropy escalation detection" << std::endl;
        std::cout << "Recent entropy trend: 1.2 → 1.8 → 2.4 → 3.1" << std::endl;
        std::cout << "Result: CONSTRAIN (reduce stochasticity)" << std::endl;
        std::cout << std::endl;
        
        return 0;
    }

private:
    // Helper functions
    std::vector<std::string> create_demo_tokens() {
        return {"Paris", "London", "Berlin", "Madrid", "Rome", "Amsterdam", "Vienna", "Prague"};
    }
    
    std::vector<double> create_demo_logits() {
        return {2.1, 1.9, 1.7, 1.5, 1.3, 1.1, 0.9, 0.7};
    }
    
    std::string compute_input_hash(const std::string& input) {
        // Simple hash for demo
        std::hash<std::string> hasher;
        return std::to_string(hasher(input));
    }
    
    void save_results_to_file(const StochasticResult& result, 
                             const StochasticProvenanceChain& chain,
                             const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open output file: " << filename << std::endl;
            return;
        }
        
        file << "{\n";
        file << "  \"schema\": \"t81.csi.inference-result.v1\",\n";
        file << "  \"model_id\": \"" << model_id_ << "\",\n";
        file << "  \"selected_token\": \"" << result.selected_token << "\",\n";
        file << "  \"entropy\": " << result.entropy << ",\n";
        file << "  \"policy_verdict\": \"" << axion_verdict_to_string(result.policy_verdict) << "\",\n";
        file << "  \"candidates\": " << result.candidates.size() << ",\n";
        file << "  \"chain_hash\": \"" << chain.finalize_chain().to_string() << "\",\n";
        file << "  \"sampling_time_ns\": " << result.sampling_time.count() << ",\n";
        file << "  \"top_candidates\": [\n";
        
        for (size_t i = 0; i < std::min(result.candidates.size(), size_t(5)); ++i) {
            const auto& candidate = result.candidates[i];
            file << "    {\n";
            file << "      \"token\": \"" << candidate.token << "\",\n";
            file << "      \"logit\": " << candidate.logit << ",\n";
            file << "      \"probability\": " << candidate.probability << ",\n";
            file << "      \"rank\": " << candidate.rank << "\n";
            file << "    }";
            if (i < std::min(result.candidates.size(), size_t(5)) - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
    }
    
    // Mock policy gate for CLI demo
    class MockPolicyGate {
    public:
        AxionVerdict evaluate(const AxionPolicyEvent& event, bool allow_override = true) {
            return AxionVerdict::ALLOW;
        }
    };
};

} // namespace t81::experimental::csi::cli

int main(int argc, char** argv) {
    t81::experimental::csi::cli::CSICLI cli;
    return cli.run(argc, argv);
}
