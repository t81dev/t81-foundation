// Controlled Stochastic Inference Decoder
// EXPERIMENTAL - NOT FOR PRODUCTION USE
// Part of the Controlled Stochastic Inference (CSI) subsystem

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

#include "t81/canonfs.hpp"
#include "t81/axion.hpp"
#include "t81/tensor.hpp"

namespace t81::experimental::csi {

// Stochastic sampling configuration
struct StochasticConfig {
    uint64_t seed;
    double temperature = 1.0;
    size_t top_k = 5;
    double top_p = 0.9;
    bool deterministic_envelope = true;
    
    // Policy constraints
    double max_entropy_per_token = 2.5;
    size_t max_candidate_set = 10;
    
    // Provenance requirements
    bool capture_logits = true;
    bool capture_candidates = true;
    bool capture_selection_trace = true;
};

// Single candidate in sampling set
struct Candidate {
    std::string token;
    double logit;
    double probability;
    size_t rank;
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << "Candidate{\"" << token << "\", logit=" << std::fixed << std::setprecision(3) 
            << logit << ", prob=" << std::setprecision(3) << probability 
            << ", rank=" << rank << "}";
        return oss.str();
    }
};

// Result of stochastic sampling operation
struct StochasticResult {
    std::string selected_token;
    std::vector<Candidate> candidates;
    double entropy;
    uint64_t seed_used;
    CanonFSHash provenance_hash;
    AxionVerdict policy_verdict;
    
    // Timing and metadata
    std::chrono::nanoseconds sampling_time;
    std::string selection_method;
    
    std::string summary() const {
        std::ostringstream oss;
        oss << "StochasticResult{selected=\"" << selected_token 
            << "\", entropy=" << std::fixed << std::setprecision(3) << entropy
            << ", candidates=" << candidates.size()
            << ", method=" << selection_method
            << ", policy=" << axion_verdict_to_string(policy_verdict) << "}";
        return oss.str();
    }
};

// Policy context for stochastic operations
class StochasticPolicyContext {
private:
    AxionPolicyGate& policy_gate_;
    std::string operation_context_;
    
public:
    StochasticPolicyContext(AxionPolicyGate& gate, const std::string& context)
        : policy_gate_(gate), operation_context_(context) {}
    
    AxionVerdict evaluate_sampling(const StochasticConfig& config, double current_entropy) {
        AxionPolicyEvent event;
        event.type = "stochastic.inference.sampling";
        event.context = operation_context_;
        
        // Add entropy to policy evaluation
        event.add_attribute("entropy", std::to_string(current_entropy));
        event.add_attribute("max_entropy", std::to_string(config.max_entropy_per_token));
        event.add_attribute("top_k", std::to_string(config.top_k));
        event.add_attribute("temperature", std::to_string(config.temperature));
        
        // Check entropy bounds
        if (current_entropy > config.max_entropy_per_token) {
            event.add_attribute("violation", "entropy_exceeded");
            return policy_gate_.evaluate(event, /*allow_override=*/false);
        }
        
        // Check candidate set size
        if (config.top_k > config.max_candidate_set) {
            event.add_attribute("violation", "candidate_set_too_large");
            return policy_gate_.evaluate(event, /*allow_override=*/false);
        }
        
        return policy_gate_.evaluate(event, /*allow_override=*/true);
    }
};

// Main stochastic decoder implementation
class ControlledStochasticDecoder {
private:
    StochasticPolicyContext policy_context_;
    std::mt19937_64 rng_;
    
    // Helper: apply temperature to logits
    std::vector<double> apply_temperature(const std::vector<double>& logits, double temperature) {
        std::vector<double> tempered_logits;
        tempered_logits.reserve(logits.size());
        
        for (double logit : logits) {
            tempered_logits.push_back(logit / temperature);
        }
        
        return tempered_logits;
    }
    
    // Helper: compute softmax probabilities
    std::vector<double> softmax(const std::vector<double>& logits) {
        std::vector<double> probs;
        probs.reserve(logits.size());
        
        // Find max logit for numerical stability
        double max_logit = *std::max_element(logits.begin(), logits.end());
        
        // Compute exp and sum
        double sum_exp = 0.0;
        std::vector<double> exp_vals;
        exp_vals.reserve(logits.size());
        
        for (double logit : logits) {
            double exp_val = std::exp(logit - max_logit);
            exp_vals.push_back(exp_val);
            sum_exp += exp_val;
        }
        
        // Normalize
        for (double exp_val : exp_vals) {
            probs.push_back(exp_val / sum_exp);
        }
        
        return probs;
    }
    
    // Helper: compute entropy of probability distribution
    double compute_entropy(const std::vector<double>& probs) {
        double entropy = 0.0;
        for (double p : probs) {
            if (p > 1e-10) {  // Avoid log(0)
                entropy -= p * std::log2(p);
            }
        }
        return entropy;
    }
    
    // Helper: top-k sampling
    std::vector<Candidate> top_k_sampling(
        const std::vector<std::string>& tokens,
        const std::vector<double>& logits,
        const std::vector<double>& probs,
        size_t k
    ) {
        // Create candidate list with rankings
        std::vector<Candidate> candidates;
        for (size_t i = 0; i < tokens.size(); ++i) {
            candidates.push_back({tokens[i], logits[i], probs[i], 0});
        }
        
        // Sort by logit (descending)
        std::sort(candidates.begin(), candidates.end(), 
                 [](const Candidate& a, const Candidate& b) {
                     return a.logit > b.logit;
                 });
        
        // Take top-k and update rankings
        candidates.resize(k);
        for (size_t i = 0; i < candidates.size(); ++i) {
            candidates[i].rank = i + 1;
        }
        
        return candidates;
    }
    
    // Helper: sample from candidate set
    std::string sample_from_candidates(const std::vector<Candidate>& candidates) {
        if (candidates.empty()) {
            return "";
        }
        
        // Create probability distribution for sampling
        std::vector<double> candidate_probs;
        for (const auto& candidate : candidates) {
            candidate_probs.push_back(candidate.probability);
        }
        
        std::discrete_distribution<size_t> dist(candidate_probs.begin(), candidate_probs.end());
        size_t selected_idx = dist(rng_);
        
        return candidates[selected_idx].token;
    }

public:
    ControlledStochasticDecoder(AxionPolicyGate& policy_gate, const std::string& context)
        : policy_context_(policy_gate, context) {}
    
    StochasticResult decode_with_policy(
        const std::vector<std::string>& tokens,
        const std::vector<double>& logits,
        const StochasticConfig& config
    ) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Set seed for deterministic envelope
        rng_.seed(config.seed);
        
        // Validate inputs
        if (tokens.size() != logits.size()) {
            throw std::invalid_argument("Tokens and logits must have same size");
        }
        
        StochasticResult result;
        result.seed_used = config.seed;
        result.selection_method = "top_k_sampling";
        
        try {
            // Step 1: Apply temperature
            std::vector<double> tempered_logits = apply_temperature(logits, config.temperature);
            
            // Step 2: Compute softmax probabilities
            std::vector<double> probs = softmax(tempered_logits);
            
            // Step 3: Compute entropy
            double entropy = compute_entropy(probs);
            result.entropy = entropy;
            
            // Step 4: Policy evaluation
            result.policy_verdict = policy_context_.evaluate_sampling(config, entropy);
            if (result.policy_verdict != AxionVerdict::ALLOW) {
                result.selected_token = "";
                result.provenance_hash = CanonFSHash::zero();
                return result;
            }
            
            // Step 5: Top-k candidate selection
            result.candidates = top_k_sampling(tokens, logits, probs, config.top_k);
            
            // Step 6: Sample from candidates
            result.selected_token = sample_from_candidates(result.candidates);
            
            // Step 7: Generate provenance hash
            result.provenance_hash = generate_provenance_hash(result, config);
            
        } catch (const std::exception& e) {
            result.policy_verdict = AxionVerdict::DENY;
            result.selected_token = "";
            result.provenance_hash = CanonFSHash::zero();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.sampling_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time);
        
        return result;
    }
    
private:
    CanonFSHash generate_provenance_hash(const StochasticResult& result, const StochasticConfig& config) {
        // Create provenance data for hashing
        std::string provenance_data = std::to_string(config.seed) + "|" +
                                     std::to_string(result.entropy) + "|" +
                                     result.selected_token + "|" +
                                     std::to_string(result.candidates.size());
        
        for (const auto& candidate : result.candidates) {
            provenance_data += "|" + candidate.token + ":" + std::to_string(candidate.probability);
        }
        
        return CanonFSHash::compute(provenance_data);
    }
};

// Utility functions
std::string axion_verdict_to_string(AxionVerdict verdict) {
    switch (verdict) {
        case AxionVerdict::ALLOW: return "allow";
        case AxionVerdict::DENY: return "deny";
        case AxionVerdict::CONstrain: return "constrain";
        default: return "unknown";
    }
}

// Factory function
std::unique_ptr<ControlledStochasticDecoder> create_stochastic_decoder(
    AxionPolicyGate& policy_gate,
    const std::string& context = "csi.inference"
) {
    return std::make_unique<ControlledStochasticDecoder>(policy_gate, context);
}

} // namespace t81::experimental::csi
