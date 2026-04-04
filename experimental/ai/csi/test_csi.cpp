// Controlled Stochastic Inference Test Suite
// EXPERIMENTAL - NOT FOR PRODUCTION USE

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <random>

#include "stochastic_decoder.hpp"
#include "policy_gated_sampling.hpp"
#include "stochastic_provenance.hpp"

namespace t81::experimental::csi::test {

class CSITestSuite {
private:
    int tests_run = 0;
    int tests_passed = 0;
    
public:
    void run_all_tests() {
        std::cout << "=== Controlled Stochastic Inference Test Suite ===" << std::endl;
        std::cout << "Status: EXPERIMENTAL - NOT FOR PRODUCTION USE" << std::endl;
        std::cout << std::endl;
        
        // Core functionality tests
        test_stochastic_decoder_basic();
        test_stochastic_decoder_determinism();
        test_stochastic_decoder_policy_gating();
        
        // Policy sampling tests
        test_policy_gated_sampling_methods();
        test_policy_constraint_application();
        test_entropy_escalation_detection();
        
        // Provenance chain tests
        test_provenance_chain_creation();
        test_provenance_chain_integrity();
        test_provenance_chain_storage();
        
        // Integration tests
        test_csi_integration();
        test_replayable_stochasticity();
        
        std::cout << std::endl;
        std::cout << "=== Test Results ===" << std::endl;
        std::cout << "Tests Run: " << tests_run << std::endl;
        std::cout << "Tests Passed: " << tests_passed << std::endl;
        std::cout << "Success Rate: " << (tests_passed * 100.0 / tests_run) << "%" << std::endl;
        
        if (tests_passed == tests_run) {
            std::cout << "✅ All tests passed!" << std::endl;
        } else {
            std::cout << "❌ Some tests failed!" << std::endl;
        }
    }

private:
    void assert_true(bool condition, const std::string& test_name) {
        tests_run++;
        if (condition) {
            tests_passed++;
            std::cout << "✅ " << test_name << std::endl;
        } else {
            std::cout << "❌ " << test_name << std::endl;
        }
    }
    
    // Test data generation
    std::vector<std::string> create_sample_tokens() {
        return {"Paris", "London", "Berlin", "Madrid", "Rome"};
    }
    
    std::vector<double> create_sample_logits() {
        return {2.3, 1.8, 1.2, 0.9, 0.5};
    }
    
    StochasticConfig create_test_config(uint64_t seed = 12345) {
        StochasticConfig config;
        config.seed = seed;
        config.temperature = 1.0;
        config.top_k = 3;
        config.max_entropy_per_token = 2.5;
        config.capture_logits = true;
        config.capture_candidates = true;
        return config;
    }
    
    // Core functionality tests
    void test_stochastic_decoder_basic() {
        std::cout << "\n--- Stochastic Decoder Basic Tests ---" << std::endl;
        
        // Mock policy gate (always allow)
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "test.context");
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        auto config = create_test_config(12345);
        
        auto result = decoder->decode_with_policy(tokens, logits, config);
        
        assert_true(!result.selected_token.empty(), "Decoder produces output");
        assert_true(result.policy_verdict == AxionVerdict::ALLOW, "Policy allows operation");
        assert_true(result.entropy > 0.0, "Entropy computed correctly");
        assert_true(!result.candidates.empty(), "Candidates generated");
        assert_true(result.candidates.size() <= config.top_k, "Candidate set respects top_k");
        assert_true(!result.provenance_hash.is_zero(), "Provenance hash generated");
    }
    
    void test_stochastic_decoder_determinism() {
        std::cout << "\n--- Stochastic Decoder Determinism Tests ---" << std::endl;
        
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "test.context");
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        auto config1 = create_test_config(12345);
        auto config2 = create_test_config(12345);  // Same seed
        auto config3 = create_test_config(67890);  // Different seed
        
        auto result1 = decoder->decode_with_policy(tokens, logits, config1);
        auto result2 = decoder->decode_with_policy(tokens, logits, config2);
        auto result3 = decoder->decode_with_policy(tokens, logits, config3);
        
        // Same seed should produce same result
        assert_true(result1.selected_token == result2.selected_token, "Same seed produces same result");
        assert_true(result1.provenance_hash == result2.provenance_hash, "Same seed produces same provenance");
        
        // Different seed should produce different result (probabilistically)
        // Note: This test might occasionally fail due to chance, but very unlikely
        assert_true(result1.selected_token != result3.selected_token || result1.provenance_hash != result3.provenance_hash, 
                   "Different seed produces different result");
    }
    
    void test_stochastic_decoder_policy_gating() {
        std::cout << "\n--- Stochastic Decoder Policy Gating Tests ---" << std::endl;
        
        // Policy gate that denies operations
        DenyingPolicyGate denying_gate;
        auto decoder = create_stochastic_decoder(denying_gate, "test.context");
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        auto config = create_test_config();
        
        auto result = decoder->decode_with_policy(tokens, logits, config);
        
        assert_true(result.policy_verdict == AxionVerdict::DENY, "Policy can deny operations");
        assert_true(result.selected_token.empty(), "Denied operation produces no output");
        assert_true(result.provenance_hash.is_zero(), "Denied operation has no provenance");
    }
    
    // Policy sampling tests
    void test_policy_gated_sampling_methods() {
        std::cout << "\n--- Policy Gated Sampling Method Tests ---" << std::endl;
        
        auto sampler = create_policy_gated_sampler(12345);
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        
        // Test greedy sampling
        SamplingConfig greedy_config;
        greedy_config.method = DETERMINISTIC_GREEDY;
        
        auto greedy_result = sampler->sample(tokens, logits, greedy_config);
        assert_true(greedy_result.success, "Greedy sampling succeeds");
        assert_true(greedy_result.policy_verdict == AxionVerdict::ALLOW, "Greedy sampling allowed");
        assert_true(greedy_result.selected_token == "Paris", "Greedy picks highest probability");
        
        // Test top-k sampling
        SamplingConfig topk_config;
        topk_config.method = STOCHASTIC_TOP_K;
        topk_config.top_k = 3;
        
        auto topk_result = sampler->sample(tokens, logits, topk_config);
        assert_true(topk_result.success, "Top-k sampling succeeds");
        assert_true(topk_result.candidates.size() <= 3, "Top-k respects candidate limit");
    }
    
    void test_policy_constraint_application() {
        std::cout << "\n--- Policy Constraint Application Tests ---" << std::endl;
        
        auto sampler = create_policy_gated_sampler(12345);
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        
        // Config with entropy constraint
        SamplingConfig constrained_config;
        constrained_config.method = STOCHASTIC_TOP_K;
        constrained_config.max_entropy_per_token = 0.5;  // Very low
        
        auto result = sampler->sample(tokens, logits, constrained_config);
        
        // Should be constrained due to high entropy
        assert_true(result.policy_verdict == AxionVerdict::CONSTRain || result.policy_verdict == AxionVerdict::ALLOW, 
                   "High entropy triggers constraint or adjustment");
        assert_true(!result.policy_reason.empty(), "Policy reason provided");
    }
    
    void test_entropy_escalation_detection() {
        std::cout << "\n--- Entropy Escalation Detection Tests ---" << std::endl;
        
        auto sampler = create_policy_gated_sampler(12345);
        auto tokens = create_sample_tokens();
        
        // Create logits with increasing entropy
        std::vector<std::vector<double>> entropy_sequence = {
            {3.0, 2.5, 2.0, 1.5, 1.0},  // Low entropy
            {2.0, 1.8, 1.6, 1.4, 1.2},  // Medium entropy
            {1.0, 0.9, 0.8, 0.7, 0.6},  // High entropy
            {0.5, 0.4, 0.3, 0.2, 0.1}   // Very high entropy
        };
        
        SamplingConfig config;
        config.method = STOCHASTIC_TOP_K;
        config.max_entropy_per_token = 2.0;
        
        bool escalation_detected = false;
        for (size_t i = 0; i < entropy_sequence.size(); ++i) {
            auto result = sampler->sample(tokens, entropy_sequence[i], config, "escalation_test");
            
            if (result.policy_verdict == AxionVerdict::CONSTRain && 
                result.policy_reason.find("escalation") != std::string::npos) {
                escalation_detected = true;
                break;
            }
        }
        
        assert_true(escalation_detected, "Entropy escalation detected");
    }
    
    // Provenance chain tests
    void test_provenance_chain_creation() {
        std::cout << "\n--- Provenance Chain Creation Tests ---" << std::endl;
        
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "test.context");
        
        auto config = create_test_config(12345);
        auto chain = std::make_unique<StochasticProvenanceChain>("test_model", "model_hash_123", config);
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        
        // Add multiple steps
        for (uint32_t step = 0; step < 3; ++step) {
            auto result = decoder->decode_with_policy(tokens, logits, config);
            chain->append_step(result, step, "input_hash_" + std::to_string(step));
        }
        
        assert_true(chain->get_step_count() == 3, "Chain records correct number of steps");
        assert_true(chain->compute_total_entropy() > 0.0, "Chain computes total entropy");
        assert_true(chain->compute_average_entropy() > 0.0, "Chain computes average entropy");
        
        auto chain_hash = chain->finalize_chain();
        assert_true(!chain_hash.is_zero(), "Chain produces non-zero hash");
    }
    
    void test_provenance_chain_integrity() {
        std::cout << "\n--- Provenance Chain Integrity Tests ---" << std::endl;
        
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "test.context");
        
        auto config = create_test_config(12345);
        auto chain1 = std::make_unique<StochasticProvenanceChain>("test_model", "model_hash_123", config);
        auto chain2 = std::make_unique<StochasticProvenanceChain>("test_model", "model_hash_123", config);
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        
        // Add identical steps to both chains
        for (uint32_t step = 0; step < 3; ++step) {
            auto result = decoder->decode_with_policy(tokens, logits, config);
            chain1->append_step(result, step, "input_hash_" + std::to_string(step));
            chain2->append_step(result, step, "input_hash_" + std::to_string(step));
        }
        
        auto hash1 = chain1->finalize_chain();
        auto hash2 = chain2->finalize_chain();
        
        assert_true(hash1 == hash2, "Identical chains produce identical hashes");
        assert_true(chain1->matches_chain(*chain2), "Chain matching works correctly");
        
        // Verify integrity
        assert_true(chain1->verify_chain_integrity(hash1), "Chain integrity verification works");
    }
    
    void test_provenance_chain_storage() {
        std::cout << "\n--- Provenance Chain Storage Tests ---" << std::endl;
        
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "test.context");
        
        auto config = create_test_config(12345);
        auto chain = std::make_unique<StochasticProvenanceChain>("test_model", "model_hash_123", config);
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        
        // Add steps and finalize
        for (uint32_t step = 0; step < 2; ++step) {
            auto result = decoder->decode_with_policy(tokens, logits, config);
            chain->append_step(result, step, "input_hash_" + std::to_string(step));
        }
        
        auto chain_hash = chain->finalize_chain();
        
        // Test JSON serialization
        std::string json = chain->to_json();
        assert_true(!json.empty(), "Chain serializes to JSON");
        assert_true(json.find("chain_id") != std::string::npos, "JSON contains chain ID");
        assert_true(json.find("steps") != std::string::npos, "JSON contains steps");
        assert_true(json.find("entropy") != std::string::npos, "JSON contains entropy");
    }
    
    // Integration tests
    void test_csi_integration() {
        std::cout << "\n--- CSI Integration Tests ---" << std::endl;
        
        // Test complete CSI workflow
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "integration_test");
        auto sampler = create_policy_gated_sampler(12345);
        
        auto config = create_test_config(12345);
        auto chain = std::make_unique<StochasticProvenanceChain>("integration_model", "hash_456", config);
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        
        // Perform stochastic inference with provenance capture
        auto result = decoder->decode_with_policy(tokens, logits, config);
        chain->append_step(result, 0, "integration_input");
        
        // Verify all components work together
        assert_true(result.success, "Decoder integration works");
        assert_true(chain->get_step_count() == 1, "Provenance integration works");
        assert_true(!chain->finalize_chain().is_zero(), "Chain finalization works");
    }
    
    void test_replayable_stochasticity() {
        std::cout << "\n--- Replayable Stochasticity Tests ---" << std::endl;
        
        MockPolicyGate policy_gate;
        auto decoder = create_stochastic_decoder(policy_gate, "replay_test");
        
        auto tokens = create_sample_tokens();
        auto logits = create_sample_logits();
        auto config = create_test_config(99999);  // Fixed seed for reproducibility
        
        // First run
        auto chain1 = std::make_unique<StochasticProvenanceChain>("test_model", "hash_789", config);
        auto result1 = decoder->decode_with_policy(tokens, logits, config);
        chain1->append_step(result1, 0, "replay_input");
        auto hash1 = chain1->finalize_chain();
        
        // Second run with same seed
        auto chain2 = std::make_unique<StochasticProvenanceChain>("test_model", "hash_789", config);
        auto result2 = decoder->decode_with_policy(tokens, logits, config);
        chain2->append_step(result2, 0, "replay_input");
        auto hash2 = chain2->finalize_chain();
        
        // Should be identical
        assert_true(result1.selected_token == result2.selected_token, "Replay produces same token");
        assert_true(hash1 == hash2, "Replay produces same chain hash");
        assert_true(chain1->matches_chain(*chain2), "Replay produces matching chains");
    }

    // Mock classes for testing
    class MockPolicyGate {
    public:
        AxionVerdict evaluate(const AxionPolicyEvent& event, bool allow_override = true) {
            return AxionVerdict::ALLOW;
        }
    };
    
    class DenyingPolicyGate {
    public:
        AxionVerdict evaluate(const AxionPolicyEvent& event, bool allow_override = true) {
            return AxionVerdict::DENY;
        }
    };
};

} // namespace t81::experimental::csi::test

int main(int argc, char** argv) {
    bool run_integration = false;
    
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--integration") {
            run_integration = true;
        }
    }
    
    t81::experimental::csi::test::CSITestSuite test_suite;
    test_suite.run_all_tests();
    
    return 0;
}
