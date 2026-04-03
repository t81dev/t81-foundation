#include "t81/canonfs/interchange_engine.hpp"
#include "t81/canonfs/json_renderer.hpp"
#include "t81/canonfs/canonfs_interchange_ops.hpp"
#include <chrono>
#include <atomic>

namespace t81::canonfs {

CanonFSInterchangeEngine::CanonFSInterchangeEngine() 
    : json_renderer_(create_default_json_renderer()) {
}

std::unique_ptr<CanonFSInterchangeEngine> create_interchange_engine() {
    return std::make_unique<CanonFSInterchangeEngine>();
}

void CanonFSInterchangeEngine::set_json_renderer(
    std::shared_ptr<InterchangeJSONRenderer> renderer) {
    json_renderer_ = renderer;
}

std::string CanonFSInterchangeEngine::generate_operation_id() const {
    static std::atomic<uint64_t> counter{0};
    return "canonfs_op_" + std::to_string(++counter);
}

void CanonFSInterchangeEngine::collect_evidence(const OperationContext& context) {
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
    
    // For now, use the existing policy evaluator from interchange_ops
    // This will be enhanced in future iterations
    if (context.operation == "import") {
        ImportOptions options;
        // Convert PolicyContext to ImportOptions for compatibility
        // This is a simplified conversion - full implementation would be more sophisticated
        
        return std::nullopt; // Placeholder - use default behavior
    } else if (context.operation == "export") {
        ExportOptions options;
        // Similar conversion for export
        
        return std::nullopt; // Placeholder - use default behavior
    }
    
    return InterchangePolicyDecision{true, "allow"};
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
    
    // Use existing import_path implementation
    ImportOutcome outcome = perform_import(request);
    
    // Add evidence to outcome metadata
    // For now, we'll store this in the evidence log
    // Future versions could include this in the outcome itself
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - context.start_time).count();
    
    context.metadata["duration_ms"] = std::to_string(duration);
    context.metadata["success"] = outcome.ok() ? "true" : "false";
    
    // Update the last evidence entry with completion info
    if (!evidence_log_.empty()) {
        evidence_log_.back().metadata = context.metadata;
        evidence_log_.back().end_time = end_time;
    }
    
    return outcome;
}

ExportOutcome CanonFSInterchangeEngine::export(const ExportRequest& request) {
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
    
    // Use existing export_ref implementation
    ExportOutcome outcome = perform_export(request);
    
    // Add evidence to outcome metadata
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - context.start_time).count();
    
    context.metadata["duration_ms"] = std::to_string(duration);
    context.metadata["success"] = outcome.ok() ? "true" : "false";
    
    // Update the last evidence entry with completion info
    if (!evidence_log_.empty()) {
        evidence_log_.back().metadata = context.metadata;
        evidence_log_.back().end_time = end_time;
    }
    
    return outcome;
}

ImportOutcome CanonFSInterchangeEngine::perform_import(const ImportRequest& request) {
    // Delegate to existing implementation for now
    // This maintains backward compatibility while allowing future enhancements
    return import_path(request.input_path, request);
}

ExportOutcome CanonFSInterchangeEngine::perform_export(const ExportRequest& request) {
    // Delegate to existing implementation for now
    // This maintains backward compatibility while allowing future enhancements
    return export_ref(request.canonical_hash, request.output_path, request);
}

bool CanonFSInterchangeEngine::check_policy_compliance(const PolicyContext& context,
                                                const ImportOptions& options) {
    // For now, use existing policy evaluation logic
    // This will be enhanced in future iterations to use the new evaluate_policy method
    return true; // Placeholder implementation
}

bool CanonFSInterchangeEngine::check_policy_compliance(const PolicyContext& context,
                                                const ExportOptions& options) {
    // For now, use existing policy evaluation logic
    // This will be enhanced in future iterations to use the new evaluate_policy method
    return true; // Placeholder implementation
}

} // namespace t81::canonfs
