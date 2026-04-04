#pragma once

#include "t81/ai_backend/controlled_ai_backend.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/weights.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include <memory>
#include <string>
#include <vector>

namespace t81::direct_api {

// Fast assess-fixed result structure
struct AssessResult {
    bool success = false;
    std::string decision;
    std::string reason_code;
    std::string result_ref;
    std::string provenance_ref;
    std::string error_message;
    double execution_time_ms = 0.0;
    
    // Convert to JSON for compatibility
    std::string to_json() const;
};

// Configuration for direct API
struct DirectAPIConfig {
    std::string determinism_level = "strict_deterministic";
    bool enable_evidence_collection = true;
    bool cache_policies = true;
    bool cache_models = true;
    size_t max_cache_size_mb = 1000;
};

// Main direct API class - bypasses CLI overhead
class T81DirectAPI {
public:
    T81DirectAPI();
    ~T81DirectAPI();
    
    // Initialize with model and policy (one-time setup)
    bool initialize(const std::string& model_path, 
                   const std::string& policy_path,
                   const DirectAPIConfig& config = {});
    
    // Fast assess-fixed inference
    AssessResult assess_fixed(const std::string& input);
    
    // Batch processing for better throughput
    std::vector<AssessResult> batch_assess_fixed(const std::vector<std::string>& inputs);
    
    // Configuration
    void set_determinism_level(const std::string& level);
    
    // Cache management
    void clear_model_cache();
    void clear_policy_cache();
    
    // Statistics
    struct Stats {
        size_t total_inferences = 0;
        double total_time_ms = 0.0;
        double avg_time_ms = 0.0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
    };
    
    Stats get_stats() const;
    void reset_stats();

private:
    // Core implementation
    AssessResult assess_fixed_impl(const std::string& input);
    
    // Model and policy loading (cached)
    bool load_model_cached(const std::string& path);
    bool load_policy_cached(const std::string& path);
    
    // Internal state
    DirectAPIConfig config_;
    std::unique_ptr<t81::ai_backend::ControlledAIBackend> ai_backend_;
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine_;
    
    // Cached resources
    std::string cached_model_path_;
    std::string cached_policy_path_;
    t81::weights::NativeModel cached_model_;
    t81::axion::Policy cached_policy_;
    
    // Statistics
    mutable Stats stats_;
    
    // Internal helpers
    std::string generate_result_ref(const std::string& input) const;
    std::string generate_provenance_ref(const std::string& input) const;
    bool validate_input(const std::string& input) const;
};

// Factory function for easy creation
std::unique_ptr<T81DirectAPI> create_direct_api(const DirectAPIConfig& config = {});

} // namespace t81::direct_api
