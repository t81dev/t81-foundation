#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <atomic>
#include <chrono>
#include <random>

namespace t81::axion { class PolicyEngine; }

namespace t81::ai_backend {

// Controlled randomness generator for reproducible non-determinism
class ControlledRandomness {
private:
    std::mt19937 rng_;
    bool enabled_;
    uint64_t seed_;
    
public:
    explicit ControlledRandomness(uint64_t seed = 0, bool enabled = false) 
        : rng_(seed), enabled_(enabled), seed_(seed) {}
    
    bool is_enabled() const { return enabled_; }
    uint64_t get_seed() const { return seed_; }
    
    template<typename T>
    T uniform(T min_val, T max_val) {
        if (!enabled_) return min_val;
        std::uniform_int_distribution<T> dist(min_val, max_val);
        return dist(rng_);
    }
    
    float uniform_float(float min_val, float max_val) {
        if (!enabled_) return min_val;
        std::uniform_real_distribution<float> dist(min_val, max_val);
        return dist(rng_);
    }
};

// AI inference request structure
struct InferenceRequest {
    std::string model_hash;
    size_t input_tokens = 0;
    float temperature = 0.0f;
    std::string input_text;
    
    InferenceRequest() = default;
    InferenceRequest(const std::string& model, size_t tokens, float temp, const std::string& input)
        : model_hash(model), input_tokens(tokens), temperature(temp), input_text(input) {}
};

// AI inference result structure
struct InferenceResult {
    bool success = false;
    size_t output_tokens = 0;
    std::string output_text;
    std::string error_message;
    
    InferenceResult() = default;
    InferenceResult(bool s, size_t tokens, const std::string& text)
        : success(s), output_tokens(tokens), output_text(text) {}
};

// AI backend configuration
struct AIBackendConfig {
    std::string determinism_level = "strict";  // strict | controlled | permissive
    std::optional<std::string> external_ai_endpoint = std::nullopt;
    std::optional<std::string> external_ai_auth_token = std::nullopt;
    bool evidence_collection = true;
    bool require_user_consent = true;
};

// Evidence collection structure for AI operations
struct AIOperationEvidence {
    std::string operation_id;
    std::string operation_type;
    std::string determinism_level;
    bool user_consent = false;
    bool external_ai_used = false;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> policy_decisions;
    
    void add_policy_decision(const std::string& decision, const std::string& reason) {
        policy_decisions.push_back(decision + ": " + reason);
    }
    
    std::string to_json() const;
};

// Main controlled AI backend class
class ControlledAIBackend {
public:
    ControlledAIBackend(std::unique_ptr<t81::axion::PolicyEngine> policy_engine,
                       std::optional<std::string> external_ai_endpoint = std::nullopt,
                       std::optional<std::string> external_ai_auth_token = std::nullopt);
    ~ControlledAIBackend();
    
    // Configuration
    void set_determinism_level(const std::string& level);
    void set_require_user_consent(bool req);
    void enable_external_ai(const std::string& endpoint, const std::string& auth_token);
    
    // AI operations
    InferenceResult inference(const InferenceRequest& request);
    
    // Evidence collection
    std::vector<AIOperationEvidence> get_evidence_log() const;
    std::string get_evidence_json() const;
    void clear_evidence_log();
    
private:
    InferenceResult inference_with_external_ai(const InferenceRequest& request, 
                                         AIOperationEvidence& evidence);
    InferenceResult inference_with_controlled_randomness(const InferenceRequest& request,
                                                   AIOperationEvidence& evidence);
    
    bool check_policy_consent(const std::string& operation_type, AIOperationEvidence& evidence);
    std::string generate_operation_id() const;
    
    template<typename T>
    T with_controlled_randomness(std::function<T(ControlledRandomness&)> operation) {
        if (randomness_ && randomness_->is_enabled()) {
            return operation(*randomness_);
        } else {
            ControlledRandomness deterministic_fallback(0, false);
            return operation(deterministic_fallback);
        }
    }
    
    std::unique_ptr<ControlledRandomness> randomness_;
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine_;
    std::vector<AIOperationEvidence> evidence_log_;
    std::string current_operation_id_;
    bool require_user_consent_ = true;
    
    std::optional<std::string> external_ai_endpoint_;
    std::optional<std::string> external_ai_auth_token_;
};

// Factory function
std::unique_ptr<ControlledAIBackend> create_controlled_ai_backend(
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine,
    const AIBackendConfig& config);

} // namespace t81::ai_backend
