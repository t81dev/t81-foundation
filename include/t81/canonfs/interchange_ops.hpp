#pragma once

#include "t81/canonfs/interchange.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace t81::canonfs {

enum class InterchangePolicyProfile {
  Permissive,
  ImportOnly,
  ExportOnly,
  DenyAll,
};

struct InterchangePolicyDecision {
  bool allowed = true;
  std::string reason = "allow";
};

using InterchangePolicyEvaluator = std::function<InterchangePolicyDecision(
    std::string_view canonical_ref, std::string_view operation, std::string_view path)>;

struct ImportOptions {
  std::filesystem::path canonfs_root = ".t81_canonfs";
  InterchangePolicyProfile policy_profile = InterchangePolicyProfile::Permissive;
  InterchangePolicyEvaluator policy_evaluator{};
};

struct ExportOptions {
  std::filesystem::path canonfs_root = ".t81_canonfs";
  InterchangePolicyProfile policy_profile = InterchangePolicyProfile::Permissive;
  InterchangePolicyEvaluator policy_evaluator{};
};

struct ImportOutcome {
  std::string status;
  std::string source_kind;
  std::string source_ref;
  std::vector<std::string> imported_objects;
  std::string provenance_ref;
  std::string manifest_ref;
  std::vector<std::string> imported_paths;
  std::vector<std::string> warnings;
  std::vector<interchange::Issue> errors;
  std::string policy_result;
  std::string policy_profile;

  [[nodiscard]] bool ok() const { return status != "error"; }
};

struct ExportOutcome {
  std::string status;
  std::vector<std::string> source_objects;
  std::string target_kind;
  std::string target_ref;
  std::string provenance_ref;
  std::string manifest_ref;
  std::vector<std::string> materialized_paths;
  std::vector<std::string> warnings;
  std::vector<interchange::Issue> errors;
  std::string policy_result;
  std::string policy_profile;

  [[nodiscard]] bool ok() const { return status != "error"; }
};

[[nodiscard]] std::string_view interchange_policy_profile_name(InterchangePolicyProfile profile);
[[nodiscard]] std::optional<InterchangePolicyProfile> parse_interchange_policy_profile(
    std::string_view profile_name);

ImportOutcome import_path(const std::filesystem::path& input, const ImportOptions& options = {});
ExportOutcome export_ref(const std::string& canonical_hash,
                         const std::filesystem::path& output_path,
                         const ExportOptions& options = {});

}  // namespace t81::canonfs
