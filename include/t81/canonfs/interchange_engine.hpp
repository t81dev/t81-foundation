#pragma once

#include "t81/canonfs/interchange_ops.hpp"
#include "t81/canonfs/interchange.hpp"
#include <functional>
#include <memory>
#include <optional>

namespace t81::canonfs {

// Forward declarations
class InterchangeJSONRenderer;

// Policy evaluation context
struct PolicyContext {
    std::string_view canonical_ref;
    std::string_view operation;
    std::string_view path;
    bool user_has_explicit_consent = false;
    std::map<std::string, std::string> metadata;
};

// Operation context for evidence collection
struct OperationContext {
    std::string operation_id;
    std::string operation_type;
    std::string determinism_level;
    std::chrono::steady_clock::time_point start_time;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> policy_decisions;
};

// Main CanonFS interchange engine
class CanonFSInterchangeEngine {
public:
    CanonFSInterchangeEngine() = default;
    ~CanonFSInterchangeEngine() = default;
    
    // Core operations
    ImportOutcome import(const ImportRequest& request);
    ExportOutcome export(const ExportRequest& request);
    
    // Policy evaluation
    std::optional<InterchangePolicyDecision> evaluate_policy(
        const PolicyContext& context);
    
    // Evidence collection
    void collect_evidence(const OperationContext& context);
    std::vector<OperationContext> get_evidence_log() const;
    void clear_evidence_log();
    
    // Configuration
    void set_json_renderer(std::shared_ptr<InterchangeJSONRenderer> renderer);
    
private:
    ImportOutcome perform_import(const ImportRequest& request);
    ExportOutcome perform_export(const ExportRequest& request);
    
    bool check_policy_compliance(const PolicyContext& context,
                              const ImportOptions& options);
    bool check_policy_compliance(const PolicyContext& context,
                              const ExportOptions& options);
    
    std::vector<OperationContext> evidence_log_;
    std::shared_ptr<InterchangeJSONRenderer> json_renderer_;
    
    std::string generate_operation_id() const;
    void add_evidence(const OperationContext& context);
};

// Factory function
std::unique_ptr<CanonFSInterchangeEngine> create_interchange_engine();

} // namespace t81::canonfs
