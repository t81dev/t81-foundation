#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace t81::canonfs::interchange {

struct Issue {
  std::string reason;
  std::string message;
};

struct Entry {
  std::string path;
  std::string object_ref;
  std::size_t size_bytes = 0;
};

struct Manifest {
  std::vector<Entry> entries;
};

inline constexpr std::string_view kManifestSchema = "t81.canonfs-interchange-manifest.v1";

bool is_manifest_document(std::string_view text);
bool parse_manifest(std::string_view text, Manifest& manifest, std::string& error_message);

std::string render_manifest(std::string_view source_kind, std::string_view source_ref,
                            const std::vector<Entry>& entries);
std::string render_import_provenance(std::string_view source_kind, std::string_view source_ref,
                                     const std::vector<std::string>& imported_objects,
                                     std::string_view manifest_ref,
                                     std::string_view policy_profile);
std::string render_export_provenance(const std::vector<std::string>& source_objects,
                                     std::string_view target_kind, std::string_view target_ref,
                                     std::string_view manifest_ref,
                                     std::string_view policy_profile);
std::string render_import_result(std::string_view status, std::string_view source_kind,
                                 std::string_view source_ref,
                                 const std::vector<std::string>& imported_objects,
                                 std::string_view provenance_ref, std::string_view manifest_ref,
                                 const std::vector<std::string>& imported_paths,
                                 const std::vector<std::string>& warnings,
                                 const std::vector<Issue>& errors,
                                 std::string_view policy_result, std::string_view policy_profile);
std::string render_export_result(std::string_view status,
                                 const std::vector<std::string>& source_objects,
                                 std::string_view target_kind, std::string_view target_ref,
                                 std::string_view provenance_ref, std::string_view manifest_ref,
                                 const std::vector<std::string>& materialized_paths,
                                 const std::vector<std::string>& warnings,
                                 const std::vector<Issue>& errors,
                                 std::string_view policy_result, std::string_view policy_profile);

bool validate_manifest_document(std::string_view text, std::string& error_message);
bool validate_import_provenance_document(std::string_view text, std::string& error_message);
bool validate_export_provenance_document(std::string_view text, std::string& error_message);
bool validate_import_result_document(std::string_view text, std::string& error_message);
bool validate_export_result_document(std::string_view text, std::string& error_message);

}  // namespace t81::canonfs::interchange
