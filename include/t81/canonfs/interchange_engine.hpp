#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include "t81/canonfs/interchange.hpp"
#include "t81/canonfs/interchange_ops.hpp"

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
  std::chrono::steady_clock::time_point end_time;
  std::map<std::string, std::string> metadata;
  std::vector<std::string> policy_decisions;
};

// Main CanonFS interchange engine
class CanonFSInterchangeEngine {
public:
  CanonFSInterchangeEngine();
  ~CanonFSInterchangeEngine() = default;

  // Core operations
  ImportOutcome import(const std::filesystem::path& input_path, const ImportOptions& options = {});
  ExportOutcome export_bundle(const std::string& canonical_hash, const std::filesystem::path& output_path,
                       const ExportOptions& options = {});
  ImportOutcome import(const ImportRequest& request);
  ExportOutcome export_bundle(const ExportRequest& request);

  // Policy evaluation
  std::optional<InterchangePolicyDecision> evaluate_policy(const PolicyContext& context);

  // Evidence collection
  void collect_evidence(const OperationContext& context);
  std::vector<OperationContext> get_evidence_log() const;
  void clear_evidence_log();

  // Configuration
  void set_json_renderer(std::shared_ptr<InterchangeJSONRenderer> renderer);
  
  // Performance monitoring
  void enable_performance_monitoring(bool enable = true);
  std::string get_performance_summary() const;

private:
  ImportOutcome perform_import(const ImportRequest& request);
  ImportOutcome perform_import(const std::filesystem::path& input_path,
                               const ImportOptions& options);
  ExportOutcome perform_export(const ExportRequest& request);
  ExportOutcome perform_export(const std::string& canonical_hash,
                               const std::filesystem::path& output_path,
                               const ExportOptions& options);
  
  bool check_policy_compliance(const PolicyContext& context, const ImportOptions& options);
  bool check_policy_compliance(const PolicyContext& context, const ExportOptions& options);

  std::vector<OperationContext> evidence_log_;
  std::shared_ptr<InterchangeJSONRenderer> json_renderer_;
  std::string generate_operation_id() const;
};

// Factory function
std::unique_ptr<CanonFSInterchangeEngine> create_interchange_engine();

}  // namespace t81::canonfs
