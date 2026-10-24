#include "t81/ai_backend/controlled_ai_backend.hpp"
#include <chrono>
#include <random>
#include <sstream>
#include "t81/axion/context.hpp"
#include "t81/axion/policy_engine.hpp"

namespace t81::ai_backend {

// Controlled randomness generator for reproducible non-determinism
class ControlledRandomness {
private:
  std::mt19937 rng_;
  bool enabled_;
  uint64_t seed_;

public:
  explicit ControlledRandomness(uint64_t seed, bool enabled = false)
      : rng_(seed), enabled_(enabled), seed_(seed) {}

  bool is_enabled() const { return enabled_; }

  uint64_t get_seed() const { return seed_; }

  template <typename T>
  T uniform(T min_val, T max_val) {
    if (!enabled_) return min_val;  // Deterministic fallback
    std::uniform_int_distribution<T> dist(min_val, max_val);
    return dist(rng_);
  }

  float uniform_float(float min_val, float max_val) {
    if (!enabled_) return min_val;  // Deterministic fallback
    std::uniform_real_distribution<float> dist(min_val, max_val);
    return dist(rng_);
  }
};

std::string AIOperationEvidence::to_json() const {
  std::ostringstream json;
  json << "{\n";
  json << "  \"operation_id\": \"" << operation_id << "\",\n";
  json << "  \"operation_type\": \"" << operation_type << "\",\n";
  json << "  \"determinism_level\": \"" << determinism_level << "\",\n";
  json << "  \"user_consent\": " << (user_consent ? "true" : "false") << ",\n";
  json << "  \"external_ai_used\": " << (external_ai_used ? "true" : "false") << ",\n";

  json << "  \"metadata\": {\n";
  for (const auto& [key, value] : metadata) {
    json << "    \"" << key << "\": \"" << value << "\"";
    if (&value != &metadata.rbegin()->second) json << ",";
    json << "\n";
  }
  json << "  },\n";

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
  auto seed = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  randomness_ = std::make_unique<ControlledRandomness>(seed, false);
}

ControlledAIBackend::~ControlledAIBackend() = default;

void ControlledAIBackend::set_determinism_level(const std::string& level) {
  bool enabled = (level == "controlled" || level == "permissive");
  auto seed = randomness_ ? randomness_->get_seed() : 0;
  randomness_ = std::make_unique<ControlledRandomness>(seed, enabled);
}

void ControlledAIBackend::enable_external_ai(const std::string& endpoint,
                                             const std::string& auth_token) {
  external_ai_endpoint_ = endpoint;
  external_ai_auth_token_ = auth_token;
}

std::string ControlledAIBackend::generate_operation_id() const {
  static std::atomic<uint64_t> counter{0};
  return "ai_op_" + std::to_string(++counter);
}

bool ControlledAIBackend::check_policy_consent(const std::string& operation_type) {
  if (!policy_engine_) return false;

  t81::axion::SyscallContext ctx;
  ctx.syscall = "ai.inference." + operation_type;

  auto verdict = policy_engine_->evaluate(ctx);
  bool allowed = (verdict.kind == t81::axion::VerdictKind::Allow);

  if (evidence_log_.size() > 0) {
    evidence_log_.back().policy_decisions.push_back(
        std::string(allowed ? "allow" : "deny") + ": " + verdict.reason);
  }

  return allowed;
}

template <typename T>
T ControlledAIBackend::with_controlled_randomness(
    std::function<T(ControlledRandomness&)> operation) {
  if (randomness_ && randomness_->is_enabled()) {
    return operation(*randomness_);
  } else {
    ControlledRandomness deterministic_fallback(0, false);
    return operation(deterministic_fallback);
  }
}

InferenceResult ControlledAIBackend::inference(const InferenceRequest& request) {
  current_operation_id_ = generate_operation_id();

  AIOperationEvidence evidence;
  evidence.operation_id = current_operation_id_;
  evidence.operation_type = "inference";
  evidence.determinism_level = randomness_ && randomness_->is_enabled() ? "controlled" : "strict";
  evidence.user_consent = check_policy_consent("inference");
  evidence.external_ai_used = external_ai_endpoint_.has_value();

  evidence.metadata["model_hash"] = request.model_hash;
  evidence.metadata["input_tokens"] = std::to_string(request.input_tokens);
  evidence.metadata["temperature"] = std::to_string(request.temperature);

  InferenceResult result;

  try {
    if (external_ai_endpoint_ && external_ai_auth_token_) {
      result = inference_with_external_ai(request, evidence);
    } else {
      result = inference_with_controlled_randomness(request, evidence);
    }

    evidence.metadata["success"] = "true";
    evidence.metadata["output_tokens"] = std::to_string(result.output_tokens);

  } catch (const std::exception& e) {
    evidence.metadata["success"] = "false";
    evidence.metadata["error"] = e.what();
    result.success = false;
    result.error_message = e.what();
  }

  evidence_log_.push_back(evidence);

  return result;
}

InferenceResult ControlledAIBackend::inference_with_external_ai(const InferenceRequest& request,
                                                                AIOperationEvidence& evidence) {
  evidence.metadata["ai_service"] = *external_ai_endpoint_;

  return with_controlled_randomness<InferenceResult>([&](ControlledRandomness& rng) {
    InferenceResult result;
    result.success = true;

    if (request.temperature > 0.0) {
      auto creativity_factor = rng.uniform_float(0.8, 1.2);
      result.output_tokens = static_cast<size_t>(request.input_tokens * creativity_factor);
    } else {
      result.output_tokens = request.input_tokens;
    }

    result.output_text =
        "External AI response for: " + std::to_string(request.input_tokens) + " tokens";

    return result;
  });
}

InferenceResult ControlledAIBackend::inference_with_controlled_randomness(
    const InferenceRequest& request, AIOperationEvidence& evidence) {
  evidence.metadata["ai_service"] = "local_controlled";

  return with_controlled_randomness<InferenceResult>([&](ControlledRandomness& rng) {
    InferenceResult result;
    result.success = true;

    if (request.temperature > 0.0 && rng.is_enabled()) {
      auto variance = rng.uniform_float(-0.1, 0.1);
      auto base_output = static_cast<float>(request.input_tokens);
      result.output_tokens = static_cast<size_t>(base_output * (1.0 + variance));
      result.output_text =
          "Controlled AI response with variance: " + std::to_string(result.output_tokens);
    } else {
      result.output_tokens = request.input_tokens;
      result.output_text = "Deterministic AI response: " + std::to_string(result.output_tokens);
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

void ControlledAIBackend::clear_evidence_log() { evidence_log_.clear(); }

// Factory function
std::unique_ptr<ControlledAIBackend> create_controlled_ai_backend(
    std::unique_ptr<t81::axion::PolicyEngine> policy_engine, const AIBackendConfig& config) {
  auto backend = std::make_unique<ControlledAIBackend>(
      std::move(policy_engine), config.external_ai_endpoint, config.external_ai_auth_token);

  backend->set_determinism_level(config.determinism_level);

  return backend;
}

}  // namespace t81::ai_backend
