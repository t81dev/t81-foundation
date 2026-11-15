#include "t81/ai_backend/controlled_ai_backend.hpp"
#include <bit>
#include <chrono>
#include <random>
#include <span>
#include <sstream>
#include "t81/axion/context.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/support/json_string.hpp"

namespace t81::ai_backend {

namespace {
using t81::support::json_string;

std::string verdict_name(t81::axion::VerdictKind kind) {
  using K = t81::axion::VerdictKind;
  switch (kind) {
    case K::Allow:
      return "Allow";
    case K::Deny:
      return "Deny";
    case K::Defer:
      return "Defer";
    case K::Warn:
      return "Warn";
    case K::Quarantine:
      return "Quarantine";
  }
  return "Unknown";
}

// Length-prefixed request identity, not a consent proof or policy evaluation.
std::string request_digest(const InferenceRequest& request, const std::string& mode,
                           const std::string& target) {
  std::string framed;
  for (const auto& field : {std::string("t81.controlled-ai.request.v1"), request.model_hash,
                            std::to_string(request.input_tokens),
                            std::to_string(std::bit_cast<uint32_t>(request.temperature)),
                            request.input_text, mode, target}) {
    framed += std::to_string(field.size()) + ":" + field;
  }
  return "sha3-512:" + t81::crypto::sha3_512_hex(std::span<const uint8_t>(
                           reinterpret_cast<const uint8_t*>(framed.data()), framed.size()));
}
}  // namespace

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
  json << "{\n  \"operation_id\": " << json_string(operation_id)
       << ",\n  \"operation_type\": " << json_string(operation_type)
       << ",\n  \"determinism_level\": " << json_string(determinism_level)
       << ",\n  \"user_consent\": " << (user_consent ? "true" : "false")
       << ",\n  \"external_ai_used\": " << (external_ai_used ? "true" : "false")
       << ",\n  \"policy_authorized\": " << (policy_authorized ? "true" : "false")
       << ",\n  \"inference_dispatched\": " << (inference_dispatched ? "true" : "false")
       << ",\n  \"failure_reason\": " << json_string(failure_reason)
       << ",\n  \"authorization_verdict\": ";
  if (authorization_verdict) {
    json << "{\"kind\":" << json_string(verdict_name(authorization_verdict->kind))
         << ",\"kind_value\":" << static_cast<int>(authorization_verdict->kind)
         << ",\"reason\":" << json_string(authorization_verdict->reason) << "}";
  } else {
    json << "null";
  }
  json << ",\n  \"metadata\": {";
  bool first = true;
  for (const auto& [key, value] : metadata) {
    if (!first) json << ",";
    first = false;
    json << "\n    " << json_string(key) << ": " << json_string(value);
  }
  json << "\n  },\n  \"policy_decisions\": [";
  for (size_t i = 0; i < policy_decisions.size(); ++i) {
    if (i) json << ",";
    json << "\n    " << json_string(policy_decisions[i]);
  }
  json << "\n  ]\n}\n";
  return json.str();
}

ControlledAIBackend::ControlledAIBackend(std::unique_ptr<t81::axion::PolicyEngine> policy_engine,
                                         std::optional<std::string> external_ai_endpoint,
                                         std::optional<std::string> external_ai_auth_token,
                                         bool require_user_consent)
    : policy_engine_(std::move(policy_engine)),
      external_ai_endpoint_(external_ai_endpoint),
      external_ai_auth_token_(external_ai_auth_token),
      require_user_consent_(require_user_consent) {
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

bool ControlledAIBackend::authorize(const InferenceRequest& request,
                                    AIOperationEvidence& evidence) {
  t81::axion::SyscallContext ctx;
  ctx.caller = "controlled_ai_backend";
  ctx.syscall = "ai.inference.inference";  // Preserve the existing syscall identifier.
  const bool external = external_ai_endpoint_ && external_ai_auth_token_;
  evidence.metadata["dispatch_kind"] = external ? "external_simulated" : "local_controlled";
  ctx.payload = request_digest(
      request, evidence.determinism_level,
      external ? "external_simulated:" + *external_ai_endpoint_ : "local_controlled");
  ctx.final_authorization = true;
  ctx.policy = policy_engine_ ? policy_engine_->active_policy() : nullptr;
  evidence.metadata["authorization_caller"] = ctx.caller;
  evidence.metadata["authorization_syscall"] = ctx.syscall;
  evidence.metadata["authorization_payload"] = ctx.payload;
  evidence.metadata["final_authorization"] = "true";
  evidence.metadata["active_policy"] = ctx.policy ? "true" : "false";
  evidence.metadata["consent_status"] = "unverified";
  evidence.metadata["consent_required"] = require_user_consent_ ? "true" : "false";

  evidence.metadata["authorization_status"] = "not_evaluated";
  if (!policy_engine_) {
    evidence.policy_decisions.push_back("NotEvaluated: Missing policy engine");
    evidence.failure_reason = "ERR_AI_POLICY_ENGINE_MISSING: Missing policy engine";
    return false;
  }
  evidence.metadata["authorization_status"] = "evaluating";
  const auto verdict = policy_engine_->evaluate(ctx);
  evidence.metadata["authorization_status"] = "evaluated";
  evidence.authorization_verdict = verdict;
  evidence.policy_decisions.push_back(verdict_name(verdict.kind) + ": " + verdict.reason);
  evidence.policy_authorized = ctx.policy && verdict.kind == t81::axion::VerdictKind::Allow;
  if (!evidence.policy_authorized) {
    evidence.failure_reason =
        "ERR_AI_AUTHORIZATION_DENIED: " + verdict_name(verdict.kind) + ": " + verdict.reason;
    if (!ctx.policy) evidence.failure_reason += " (active policy required)";
    return false;
  }
  if (require_user_consent_) {
    evidence.failure_reason = "ERR_AI_CONSENT_UNSUPPORTED: No verified user-consent contract";
    return false;
  }
  return true;
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

  evidence.metadata["model_hash"] = request.model_hash;
  evidence.metadata["input_tokens"] = std::to_string(request.input_tokens);
  evidence.metadata["temperature"] = std::to_string(request.temperature);

  InferenceResult result;

  try {
    if (!authorize(request, evidence)) {
      result.error_message = evidence.failure_reason;
    } else if (external_ai_endpoint_ && external_ai_auth_token_) {
      result = inference_with_external_ai(request, evidence);
    } else {
      result = inference_with_controlled_randomness(request, evidence);
    }

    evidence.metadata["success"] = result.success ? "true" : "false";
    evidence.metadata["output_tokens"] = std::to_string(result.output_tokens);

  } catch (const std::exception& e) {
    evidence.metadata["success"] = "false";
    evidence.failure_reason = std::string("ERR_AI_OPERATION_FAILED: ") + e.what();
    result.success = false;
    result.error_message = evidence.failure_reason;
  } catch (...) {
    evidence.metadata["success"] = "false";
    evidence.failure_reason = "ERR_AI_OPERATION_FAILED: Unknown exception";
    result.error_message = evidence.failure_reason;
  }

  if (!result.success) evidence.metadata["error"] = result.error_message;
  if (evidence.policy_decisions.empty()) {
    evidence.policy_decisions.push_back((evidence.metadata["authorization_status"] == "evaluating"
                                             ? "EvaluationError: "
                                             : "NotEvaluated: ") +
                                        evidence.failure_reason);
  }
  evidence_log_.push_back(std::move(evidence));

  return result;
}

InferenceResult ControlledAIBackend::inference_with_external_ai(const InferenceRequest& request,
                                                                AIOperationEvidence& evidence) {
  evidence.inference_dispatched = true;
  evidence.external_ai_used = true;
  evidence.metadata["ai_service"] = *external_ai_endpoint_;

  return with_controlled_randomness<InferenceResult>([&](ControlledRandomness& rng) {
    InferenceResult result;
    result.success = true;

    if (request.temperature > 0.0) {
      auto creativity_factor = rng.uniform_float(static_cast<float>(0.8), static_cast<float>(1.2));
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
  evidence.inference_dispatched = true;
  evidence.metadata["ai_service"] = "local_controlled";

  return with_controlled_randomness<InferenceResult>([&](ControlledRandomness& rng) {
    InferenceResult result;
    result.success = true;

    if (request.temperature > 0.0 && rng.is_enabled()) {
      auto variance = rng.uniform_float(static_cast<float>(-0.1), static_cast<float>(0.1));
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
      std::move(policy_engine), config.external_ai_endpoint, config.external_ai_auth_token,
      config.require_user_consent);

  backend->set_determinism_level(config.determinism_level);

  return backend;
}

}  // namespace t81::ai_backend
