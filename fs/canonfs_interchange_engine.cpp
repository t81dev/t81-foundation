#include <atomic>
#include <chrono>
#include "t81/canonfs/canonfs_interchange_ops.hpp"
#include "t81/canonfs/interchange_engine.hpp"
#include "t81/canonfs/json_renderer.hpp"

namespace t81::canonfs {

CanonFSInterchangeEngine::CanonFSInterchangeEngine() {
    json_renderer_ = create_default_json_renderer();
    evidence_log_.reserve(1000); // Reserve space to reduce allocations
}

std::unique_ptr<CanonFSInterchangeEngine> create_interchange_engine() {
  return std::make_unique<CanonFSInterchangeEngine>();
}

void CanonFSInterchangeEngine::set_json_renderer(
    std::shared_ptr<InterchangeJSONRenderer> renderer) {
  json_renderer_ = renderer;
}

ImportOutcome CanonFSInterchangeEngine::import(const std::filesystem::path& input_path, const ImportOptions& options) {
  return perform_import(input_path, options);
}

ExportOutcome CanonFSInterchangeEngine::export_bundle(const std::string& canonical_hash, const std::filesystem::path& output_path, const ExportOptions& options) {
  return perform_export(canonical_hash, output_path, options);
}

std::string CanonFSInterchangeEngine::generate_operation_id() const {
  static std::atomic<uint64_t> counter{0};
  return "canonfs_op_" + std::to_string(++counter);
}

void CanonFSInterchangeEngine::collect_evidence(const OperationContext& context) {
  // Add bounds checking to prevent unbounded growth
  constexpr size_t MAX_EVIDENCE_ENTRIES = 10000;
  
  if (evidence_log_.size() >= MAX_EVIDENCE_ENTRIES) {
    // Remove oldest entries to maintain bounds
    evidence_log_.erase(evidence_log_.begin(), evidence_log_.begin() + 1000);
  }
  
  evidence_log_.push_back(context);
}

std::vector<OperationContext> CanonFSInterchangeEngine::get_evidence_log() const {
  return evidence_log_;
}

void CanonFSInterchangeEngine::clear_evidence_log() { 
    evidence_log_.clear(); 
}

std::optional<InterchangePolicyDecision> CanonFSInterchangeEngine::evaluate_policy(
    const PolicyContext& context) {
  // Implement actual policy evaluation based on profile and context
  std::optional<InterchangePolicyDecision> decision;
  if (context.operation == "import") {
    // Check if import is allowed based on policy profile
    if (context.user_has_explicit_consent) {
      decision = InterchangePolicyDecision{true, "user_explicit_consent"};
    } else {
      // Default import policy: allow if path is safe
      if (context.path.find("..") == std::string_view::npos && 
          !context.path.empty()) {
        decision = InterchangePolicyDecision{true, "safe_import_path"};
      } else {
        decision = InterchangePolicyDecision{false, "unsafe_import_path"};
      }
    }
  } else if (context.operation == "export") {
    // Check if export is allowed based on policy profile
    if (context.user_has_explicit_consent) {
      decision = InterchangePolicyDecision{true, "user_explicit_consent"};
    } else {
      // Default export policy: allow if canonical_ref is valid format
      if (context.canonical_ref.find("sha3-256:") == 0 && 
          context.canonical_ref.length() > 9) {
        decision = InterchangePolicyDecision{true, "valid_canonical_ref"};
      } else {
        decision = InterchangePolicyDecision{false, "invalid_canonical_ref"};
      }
    }
  } else {
    decision = InterchangePolicyDecision{false, "unknown_operation"};
  }

  return decision;
}

ImportOutcome CanonFSInterchangeEngine::import(const ImportRequest& request) {
  auto operation_id = generate_operation_id();

  OperationContext context;
  context.operation_id = operation_id;
  context.operation_type = "import";
  context.start_time = std::chrono::steady_clock::now();
  context.metadata["input_path"] = request.input_path.string();
  context.metadata["canonfs_root"] = request.canonfs_root.string();
  context.metadata["policy_profile"] = interchange_policy_profile_name(request.policy_profile);

  collect_evidence(context);
  
  // Get reference to the stored context for updates
  size_t context_index = evidence_log_.size() - 1;

  // Use existing import_path implementation
  ImportOutcome outcome = perform_import(request);

  // Add evidence to outcome metadata
  auto end_time = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - context.start_time).count();

  // Update the stored context directly
  if (context_index < evidence_log_.size()) {
    evidence_log_[context_index].metadata["duration_ms"] = std::to_string(duration);
    evidence_log_[context_index].metadata["success"] = outcome.ok() ? "true" : "false";
    evidence_log_[context_index].end_time = end_time;
  }

  return outcome;
}

ExportOutcome CanonFSInterchangeEngine::export_bundle(const ExportRequest& request) {
  auto operation_id = generate_operation_id();

  OperationContext context;
  context.operation_id = operation_id;
  context.operation_type = "export";
  context.start_time = std::chrono::steady_clock::now();
  context.metadata["canonical_hash"] = request.canonical_hash;
  context.metadata["output_path"] = request.output_path.string();
  context.metadata["canonfs_root"] = request.canonfs_root.string();
  context.metadata["policy_profile"] = interchange_policy_profile_name(request.policy_profile);

  collect_evidence(context);
  
  // Get reference to the stored context for updates
  size_t context_index = evidence_log_.size() - 1;

  // Use existing export_ref implementation
  ExportOutcome outcome = perform_export(request);

  // Add evidence to outcome metadata
  auto end_time = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - context.start_time).count();

  // Update the stored context directly
  if (context_index < evidence_log_.size()) {
    evidence_log_[context_index].metadata["duration_ms"] = std::to_string(duration);
    evidence_log_[context_index].metadata["success"] = outcome.ok() ? "true" : "false";
    evidence_log_[context_index].end_time = end_time;
  }

  return outcome;
}

ImportOutcome CanonFSInterchangeEngine::perform_import(const ImportRequest& request) {
  // Convert ImportRequest to ImportOptions for compatibility
  ImportOptions options;
  options.canonfs_root = request.canonfs_root;
  options.policy_profile = request.policy_profile;
  options.policy_evaluator = request.policy_evaluator;
  
  // Delegate to existing implementation for now
  // This maintains backward compatibility while allowing future enhancements
  return import_path(request.input_path, options);
}

ImportOutcome CanonFSInterchangeEngine::perform_import(const std::filesystem::path& input_path,
                                                       const ImportOptions& options) {
  // Delegate to existing implementation for now
  // This maintains backward compatibility while allowing future enhancements
  return import_path(input_path, options);
}

ExportOutcome CanonFSInterchangeEngine::perform_export(const ExportRequest& request) {
  // Convert ExportRequest to ExportOptions for compatibility
  ExportOptions options;
  options.canonfs_root = request.canonfs_root;
  options.policy_profile = request.policy_profile;
  options.policy_evaluator = request.policy_evaluator;
  
  // Delegate to existing implementation for now
  // This maintains backward compatibility while allowing future enhancements
  return export_ref(request.canonical_hash, request.output_path, options);
}

ExportOutcome CanonFSInterchangeEngine::perform_export(const std::string& canonical_hash,
                                                       const std::filesystem::path& output_path,
                                                       const ExportOptions& options) {
  // Delegate to existing implementation for now
  // This maintains backward compatibility while allowing future enhancements
  return export_ref(canonical_hash, output_path, options);
}

bool CanonFSInterchangeEngine::check_policy_compliance(const PolicyContext& context,
                                                       const ImportOptions& options) {
    // Use actual policy evaluation
    auto decision = evaluate_policy(context);
    if (decision.has_value()) {
        return decision->allowed;
    }
    
    // Fallback to profile-based checking
    return options.policy_profile != InterchangePolicyProfile::DenyAll;
}

bool CanonFSInterchangeEngine::check_policy_compliance(const PolicyContext& context,
                                                       const ExportOptions& options) {
    // Use actual policy evaluation
    auto decision = evaluate_policy(context);
    if (decision.has_value()) {
        return decision->allowed;
    }
    
    // Fallback to profile-based checking
    return options.policy_profile != InterchangePolicyProfile::DenyAll;
}

}  // namespace t81::canonfs
