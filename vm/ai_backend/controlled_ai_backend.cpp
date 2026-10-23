#include "t81/ai_backend/controlled_ai_backend.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/axion/context.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include <atomic>

namespace t81::ai_backend {

std::string AIOperationEvidence::to_json() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"operation_id\": \"" << operation_id << "\",\n";
    json << "  \"operation_type\": \"" << operation_type << "\",\n";
    json << "  \"determinism_level\": \"" << determinism_level << "\",\n";
    json << "  \"user_consent\": " << (user_consent ? "true" : "false") << ",\n";
    json << "  \"external_ai_used\": " << (external_ai_used ? "true" : "false") << ",\n";
    
    auto start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        start_time.time_since_epoch()).count();
    auto end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time.time_since_epoch()).count();
    json << "  \"duration_ms\": " << (end_ms - start_ms) << ",\n";
    
    json << "  \"metadata\": {\n";
    bool first = true;
    for (const auto& [key, value] : metadata) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << key << "\": \"" << value << "\"";
    }
    json << "\n  },\n";
    
    json << "  \"policy_decisions\": [\n";
    for (size_t i = 0; i < policy_decisions.size(); ++i) {
        json << "    \"" << policy_decisions[i] << "\"";
        if (i + 1 < policy_decisions.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n";
    json << "}\n";
    return json.str();
}

ControlledAIBackend::ControlledAIBackend(std::unique_ptr<t81::axion::PolicyEngine> policy_engine,
                                       std::optional<std::string> external_ai_endpoint,
                                       std::optional<std::string> external_ai_auth_token)
    : policy_engine_(std::move(policy_engine)),
      external_ai_endpoint_(external_ai_endpoint),
      external_ai_auth_token_(external_ai_auth_token) {
    auto now = std::chrono::steady_clock::now();
    auto seed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    randomness_ = std::make_unique<ControlledRandomness>(seed, false);
}

ControlledAIBackend::~ControlledAIBackend() = default;

void ControlledAIBackend::set_determinism_level(const std::string& level) {
    bool enabled = (level == "controlled" || level == "permissive");
    auto seed = randomness_ ? randomness_->get_seed() : 0;
    randomness_ = std::make_unique<ControlledRandomness>(seed, enabled);
}

void ControlledAIBackend::set_require_user_consent(bool req) {
    require_user_consent_ = req;
}

void ControlledAIBackend::enable_external_ai(const std::string& endpoint, const std::string& auth_token) {
    external_ai_endpoint_ = endpoint;
    external_ai_auth_token_ = auth_token;
}

std::string ControlledAIBackend::generate_operation_id() const {
    static std::atomic<uint64_t> counter{0};
    return "ai_op_" + std::to_string(++counter);
}

bool ControlledAIBackend::check_policy_consent(const std::string& operation_type, AIOperationEvidence& evidence) {
    if (!require_user_consent_) {
        evidence.add_policy_decision("deny", "User consent missing");
        return false;
    }

    if (!policy_engine_) {
        evidence.add_policy_decision("allow", "No policy engine configured");
        return true;
    }
    
    t81::axion::SyscallContext ctx;
    ctx.syscall = "ai.inference." + operation_type;
    ctx.caller = "controlled_ai_backend";
    ctx.payload = "consent:granted";
    
    auto verdict = policy_engine_->evaluate(ctx);
    bool allowed = (verdict.kind == t81::axion::VerdictKind::Allow || verdict.kind == t81::axion::VerdictKind::Defer);
    evidence.add_policy_decision(
        allowed ? "allow" : "deny", 
        verdict.reason.empty() ? (allowed ? "allowed" : "denied by policy") : verdict.reason);
    
    return allowed;
}

InferenceResult ControlledAIBackend::inference(const InferenceRequest& request) {
    current_operation_id_ = generate_operation_id();
    
    AIOperationEvidence evidence;
    evidence.operation_id = current_operation_id_;
    evidence.operation_type = "inference";
    evidence.determinism_level = randomness_ && randomness_->is_enabled() ? "controlled" : "strict";
    evidence.start_time = std::chrono::steady_clock::now();
    evidence.metadata["model_hash"] = request.model_hash;
    evidence.metadata["input_tokens"] = std::to_string(request.input_tokens);
    evidence.metadata["temperature"] = std::to_string(request.temperature);
    evidence.external_ai_used = external_ai_endpoint_.has_value();

    evidence.user_consent = check_policy_consent("inference", evidence);

    InferenceResult result;
    if (!evidence.user_consent) {
        result.success = false;
        result.error_message = "Policy denied AI operation: user consent missing or policy violation";
        evidence.metadata["success"] = "false";
        evidence.metadata["error"] = result.error_message;
        evidence.end_time = std::chrono::steady_clock::now();
        evidence_log_.push_back(evidence);
        return result;
    }
    
    try {
        if (external_ai_endpoint_ && external_ai_auth_token_) {
            result = inference_with_external_ai(request, evidence);
        } else {
            result = inference_with_controlled_randomness(request, evidence);
        }
        evidence.metadata["success"] = result.success ? "true" : "false";
        evidence.metadata["output_tokens"] = std::to_string(result.output_tokens);
    } catch (const std::exception& e) {
        evidence.metadata["success"] = "false";
        evidence.metadata["error"] = e.what();
        result.success = false;
        result.error_message = e.what();
    }
    
    evidence.end_time = std::chrono::steady_clock::now();
    evidence_log_.push_back(evidence);
    return result;
}

InferenceResult ControlledAIBackend::inference_with_external_ai(const InferenceRequest& request, 
                                                               AIOperationEvidence& evidence) {
    if (external_ai_endpoint_) {
        evidence.metadata["ai_service"] = *external_ai_endpoint_;
    }
    
    return with_controlled_randomness<InferenceResult>([&](ControlledRandomness& rng) {
        InferenceResult result;
        result.success = true;
        
        if (request.temperature > 0.0) {
            auto creativity_factor = rng.uniform_float(0.8f, 1.2f);
            result.output_tokens = static_cast<size_t>(
                request.input_tokens * creativity_factor);
        } else {
            result.output_tokens = request.input_tokens;
        }
        
        result.output_text = "External AI response for: " + 
                          std::to_string(request.input_tokens) + " tokens";
        
        return result;
    });
}

InferenceResult ControlledAIBackend::inference_with_controlled_randomness(const InferenceRequest& request,
                                                                        AIOperationEvidence& evidence) {
    evidence.metadata["ai_service"] = "local_controlled";
    
    return with_controlled_randomness<InferenceResult>([&](ControlledRandomness& rng) {
        InferenceResult result;
        result.success = true;
        
        if (request.temperature > 0.0 && rng.is_enabled()) {
            auto variance = rng.uniform_float(-0.1f, 0.1f);
            auto base_output = static_cast<float>(request.input_tokens);
            result.output_tokens = static_cast<size_t>(base_output * (1.0f + variance));
            result.output_text = "Controlled AI response with variance: " + 
                              std::to_string(result.output_tokens);
        } else {
            result.output_tokens = request.input_tokens;
            result.output_text = "Deterministic AI response: " + 
                              std::to_string(result.output_tokens);
        }
        
        return result;
    });
}

std::vector<AIOperationEvidence> ControlledAIBackend::get_evidence_log() const {
    return evidence_log_;
}

std::string ControlledAIBackend::get_evidence_json() const {
    std::ostringstream json;
    json << "[\n";
    for (size_t i = 0; i < evidence_log_.size(); ++i) {
        json << evidence_log_[i].to_json();
        if (i + 1 < evidence_log_.size()) json << ",";
        json << "\n";
    }
    json << "]\n";
    return json.str();
}

void ControlledAIBackend::clear_evidence_log() {
    evidence_log_.clear();
}

std::unique_ptr<ControlledAIBackend> create_controlled_ai_backend(
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine,
    const AIBackendConfig& config) {
    
    auto backend = std::make_unique<ControlledAIBackend>(
        std::move(policy_engine),
        config.external_ai_endpoint,
        config.external_ai_auth_token);
    
    backend->set_determinism_level(config.determinism_level);
    backend->set_require_user_consent(config.require_user_consent);
    
    return backend;
}

} // namespace t81::ai_backend
