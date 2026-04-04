#include "t81/direct_api/t81_direct_api.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/tracing/canonhash.hpp"
#include <chrono>
#include <sstream>
#include <fstream>

namespace t81::direct_api {

// AssessResult JSON serialization
std::string AssessResult::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"success\": " << (success ? "true" : "false") << ",\n";
    oss << "  \"decision\": \"" << decision << "\",\n";
    oss << "  \"reason_code\": \"" << reason_code << "\",\n";
    oss << "  \"result_ref\": \"" << result_ref << "\",\n";
    oss << "  \"provenance_ref\": \"" << provenance_ref << "\",\n";
    oss << "  \"execution_time_ms\": " << execution_time_ms << ",\n";
    if (!error_message.empty()) {
        oss << "  \"error_message\": \"" << error_message << "\",\n";
    }
    oss << "  \"schema\": \"t81.ai.task.assess-fixed.result.v1\"\n";
    oss << "}";
    return oss.str();
}

// T81DirectAPI implementation
T81DirectAPI::T81DirectAPI() = default;
T81DirectAPI::~T81DirectAPI() = default;

bool T81DirectAPI::initialize(const std::string& model_path, 
                            const std::string& policy_path,
                            const DirectAPIConfig& config) {
    config_ = config;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Load and cache model
    if (!load_model_cached(model_path)) {
        std::cerr << "Failed to load model: " << model_path << std::endl;
        return false;
    }
    
    // Load and cache policy
    if (!load_policy_cached(policy_path)) {
        std::cerr << "Failed to load policy: " << policy_path << std::endl;
        return false;
    }
    
    // Create AI backend with cached policy engine
    ai_backend_ = std::make_unique<t81::ai_backend::ControlledAIBackend>(
        std::move(policy_engine_)
    );
    
    // Configure determinism
    ai_backend_->set_determinism_level(config_.determinism_level);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "✅ Direct API initialized in " << duration.count() << " µs" << std::endl;
    return true;
}

AssessResult T81DirectAPI::assess_fixed(const std::string& input) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Update stats
    stats_.total_inferences++;
    
    // Validate input
    if (!validate_input(input)) {
        AssessResult result;
        result.success = false;
        result.error_message = "Invalid input: " + input;
        result.execution_time_ms = 0.0;
        return result;
    }
    
    // Core implementation
    AssessResult result = assess_fixed_impl(input);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    result.execution_time_ms = duration.count() / 1000.0;
    
    // Update statistics
    stats_.total_time_ms += result.execution_time_ms;
    stats_.avg_time_ms = stats_.total_time_ms / stats_.total_inferences;
    
    return result;
}

std::vector<AssessResult> T81DirectAPI::batch_assess_fixed(const std::vector<std::string>& inputs) {
    std::vector<AssessResult> results;
    results.reserve(inputs.size());
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process batch efficiently
    for (const auto& input : inputs) {
        results.push_back(assess_fixed(input));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "📊 Batch: " << inputs.size() << " inputs in " 
              << total_duration.count() << " µs" << std::endl;
    std::cout << "📈 Average: " << (total_duration.count() / inputs.size()) 
              << " µs per input" << std::endl;
    
    return results;
}

AssessResult T81DirectAPI::assess_fixed_impl(const std::string& input) {
    AssessResult result;
    
    try {
        // Create inference request
        t81::ai_backend::InferenceRequest request(
            cached_model_path_,  // Use model hash from cached model
            input.length(),      // Simple tokenization
            0.0f,               // Temperature (not used for assess-fixed)
            input               // Input text
        );
        
        // Run inference through AI backend
        auto inference_result = ai_backend_->inference(request);
        
        if (inference_result.success) {
            result.success = true;
            
            // Parse decision from output (simplified)
            if (inference_result.output_text.find("ALLOW") != std::string::npos) {
                result.decision = "ALLOW";
            } else if (inference_result.output_text.find("DENY") != std::string::npos) {
                result.decision = "DENY";
            } else if (inference_result.output_text.find("REVIEW") != std::string::npos) {
                result.decision = "REVIEW";
            } else {
                result.decision = "UNKNOWN";
                result.reason_code = "UNMAPPED_OUTPUT";
            }
            
            // Generate canonical references
            result.result_ref = generate_result_ref(input);
            result.provenance_ref = generate_provenance_ref(input);
            
        } else {
            result.success = false;
            result.error_message = inference_result.error_message;
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Exception: ") + e.what();
    }
    
    return result;
}

bool T81DirectAPI::load_model_cached(const std::string& path) {
    if (config_.cache_models && path == cached_model_path_) {
        stats_.cache_hits++;
        return true; // Already cached
    }
    
    stats_.cache_misses++;
    
    try {
        // Load model using T81 weights API
        auto model_file = t81::weights::load_t81w(path);
        if (!model_file.has_value()) {
            return false;
        }
        
        cached_model_ = model_file.value();
        cached_model_path_ = path;
        
        std::cout << "✅ Model loaded and cached: " << path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load model: " << e.what() << std::endl;
        return false;
    }
}

bool T81DirectAPI::load_policy_cached(const std::string& path) {
    if (config_.cache_policies && path == cached_policy_path_) {
        stats_.cache_hits++;
        return true; // Already cached
    }
    
    stats_.cache_misses++;
    
    try {
        // Read policy file
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        std::string policy_text((std::istreambuf_iterator<char>(file)), 
                                std::istreambuf_iterator<char>());
        
        // Parse policy
        auto policy_result = t81::axion::parse_policy(policy_text);
        if (!policy_result) {
            return false;
        }
        
        cached_policy_ = policy_result.value();
        cached_policy_path_ = path;
        
        // Create policy engine
        policy_engine_ = std::make_unique<t81::axion::PolicyEngine>(std::make_optional(cached_policy_));
        
        std::cout << "✅ Policy loaded and cached: " << path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load policy: " << e.what() << std::endl;
        return false;
    }
}

std::string T81DirectAPI::generate_result_ref(const std::string& input) const {
    // Generate simple hash for result reference (placeholder)
    std::string data = "assess_fixed_result:" + input + ":" + cached_model_path_;
    std::hash<std::string> hasher;
    size_t hash_value = hasher(data);
    std::ostringstream oss;
    oss << "sha3-256:" << std::hex << hash_value;
    return oss.str();
}

std::string T81DirectAPI::generate_provenance_ref(const std::string& input) const {
    // Generate simple hash for provenance reference (placeholder)
    std::string data = "assess_fixed_provenance:" + input + ":" + cached_policy_path_;
    std::hash<std::string> hasher;
    size_t hash_value = hasher(data);
    std::ostringstream oss;
    oss << "sha3-256:" << std::hex << hash_value;
    return oss.str();
}

bool T81DirectAPI::validate_input(const std::string& input) const {
    // Basic input validation
    if (input.empty()) return false;
    if (input.length() > 10000) return false; // Reasonable limit
    return true;
}

void T81DirectAPI::set_determinism_level(const std::string& level) {
    config_.determinism_level = level;
    if (ai_backend_) {
        ai_backend_->set_determinism_level(level);
    }
}

void T81DirectAPI::clear_model_cache() {
    cached_model_path_.clear();
    cached_model_ = t81::weights::NativeModel{};
}

void T81DirectAPI::clear_policy_cache() {
    cached_policy_path_.clear();
    cached_policy_ = t81::axion::Policy{};
}

T81DirectAPI::Stats T81DirectAPI::get_stats() const {
    return stats_;
}

void T81DirectAPI::reset_stats() {
    stats_ = Stats{};
}

// Factory function
std::unique_ptr<T81DirectAPI> create_direct_api(const DirectAPIConfig& config) {
    auto api = std::make_unique<T81DirectAPI>();
    // Configuration will be set during initialize()
    return api;
}

} // namespace t81::direct_api
