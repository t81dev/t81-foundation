#include "t81/canonfs/interchange.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <initializer_list>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string escape_json_text(std::string_view input) {
  std::ostringstream out;
  for (char c : input) {
    switch (c) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << c;
        break;
    }
  }
  return out.str();
}

std::string json_array_of_strings(const std::vector<std::string>& values, int indent = 2) {
  std::ostringstream out;
  const std::string indent_text(static_cast<std::size_t>(indent), ' ');
  out << "[";
  if (!values.empty()) {
    out << "\n";
    for (std::size_t i = 0; i < values.size(); ++i) {
      out << indent_text << "\"" << escape_json_text(values[i]) << "\"";
      if (i + 1 < values.size()) {
        out << ",";
      }
      out << "\n";
    }
  }
  out << std::string(static_cast<std::size_t>(std::max(0, indent - 2)), ' ') << "]";
  return out.str();
}

struct ErrorClassification {
  std::string_view kind;
  std::string_view code;
};

std::optional<ErrorClassification> classify_import_reason(std::string_view reason) {
  if (reason == "policy_denied") {
    return ErrorClassification{"policy-failure", "canonfs-policy-denied"};
  }
  if (reason == "missing_source") {
    return ErrorClassification{"source-failure", "canonfs-import-missing-source"};
  }
  if (reason == "unsupported_source_kind") {
    return ErrorClassification{"source-failure", "canonfs-import-unsupported-source-kind"};
  }
  if (reason == "symlink_not_supported") {
    return ErrorClassification{"normalization-failure",
                               "canonfs-import-symlink-not-supported"};
  }
  if (reason == "invalid_policy_profile") {
    return ErrorClassification{"source-failure", "canonfs-import-invalid-policy-profile"};
  }
  if (reason == "invalid_policy_document") {
    return ErrorClassification{"source-failure", "canonfs-import-invalid-policy-document"};
  }
  if (reason == "manifest_write_failed") {
    return ErrorClassification{"source-failure", "canonfs-import-manifest-write-failed"};
  }
  if (reason == "provenance_write_failed") {
    return ErrorClassification{"source-failure", "canonfs-import-provenance-write-failed"};
  }
  if (reason == "storage_write_failed") {
    return ErrorClassification{"source-failure", "canonfs-import-storage-write-failed"};
  }
  return std::nullopt;
}

std::optional<ErrorClassification> classify_export_reason(std::string_view reason) {
  if (reason == "policy_denied") {
    return ErrorClassification{"policy-failure", "canonfs-policy-denied"};
  }
  if (reason == "unsafe_target_path") {
    return ErrorClassification{"target-failure", "canonfs-export-unsafe-target-path"};
  }
  if (reason == "target_directory_create_failed") {
    return ErrorClassification{"target-failure",
                               "canonfs-export-target-directory-create-failed"};
  }
  if (reason == "target_open_failed") {
    return ErrorClassification{"target-failure", "canonfs-export-target-open-failed"};
  }
  if (reason == "target_write_failed") {
    return ErrorClassification{"target-failure", "canonfs-export-target-write-failed"};
  }
  if (reason == "invalid_source_ref") {
    return ErrorClassification{"materialization-failure", "canonfs-export-invalid-source-ref"};
  }
  if (reason == "missing_object") {
    return ErrorClassification{"materialization-failure", "canonfs-export-missing-object"};
  }
  if (reason == "hash_mismatch") {
    return ErrorClassification{"materialization-failure", "canonfs-export-hash-mismatch"};
  }
  if (reason == "invalid_schema") {
    return ErrorClassification{"materialization-failure", "canonfs-export-invalid-schema"};
  }
  if (reason == "malformed_manifest") {
    return ErrorClassification{"materialization-failure", "canonfs-export-malformed-manifest"};
  }
  if (reason == "invalid_policy_profile") {
    return ErrorClassification{"materialization-failure",
                               "canonfs-export-invalid-policy-profile"};
  }
  if (reason == "invalid_policy_document") {
    return ErrorClassification{"materialization-failure",
                               "canonfs-export-invalid-policy-document"};
  }
  if (reason == "provenance_write_failed") {
    return ErrorClassification{"materialization-failure",
                               "canonfs-export-provenance-write-failed"};
  }
  return std::nullopt;
}

ErrorClassification classify_reason(std::string_view reason, bool is_export) {
  const auto classification =
      is_export ? classify_export_reason(reason) : classify_import_reason(reason);
  if (classification.has_value()) {
    return *classification;
  }
  return is_export
             ? ErrorClassification{"materialization-failure", "canonfs-export-failure"}
             : ErrorClassification{"source-failure", "canonfs-import-failure"};
}

std::string json_array_of_error_objects(const std::vector<t81::canonfs::interchange::Issue>& errors,
                                        bool is_export, int indent = 2) {
  std::ostringstream out;
  const std::string indent_text(static_cast<std::size_t>(indent), ' ');
  out << "[";
  if (!errors.empty()) {
    out << "\n";
    for (std::size_t i = 0; i < errors.size(); ++i) {
      const auto classification = classify_reason(errors[i].reason, is_export);
      out << indent_text << "{\"kind\": \"" << classification.kind << "\", \"message\": \""
          << escape_json_text(errors[i].message) << "\", \"code\": \"" << classification.code
          << "\", \"reason\": \"" << escape_json_text(errors[i].reason) << "\"}";
      if (i + 1 < errors.size()) {
        out << ",";
      }
      out << "\n";
    }
  }
  out << std::string(static_cast<std::size_t>(std::max(0, indent - 2)), ' ') << "]";
  return out.str();
}

std::optional<std::string> extract_json_string_field(std::string_view text, std::string_view key) {
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = text.find(needle);
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  pos = text.find(':', pos + needle.size());
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  if (pos >= text.size() || text[pos] != '"') {
    return std::nullopt;
  }
  ++pos;
  std::string value;
  while (pos < text.size()) {
    const char c = text[pos++];
    if (c == '\\') {
      if (pos >= text.size()) {
        return std::nullopt;
      }
      const char escaped = text[pos++];
      switch (escaped) {
        case '\\':
        case '"':
        case '/':
          value.push_back(escaped);
          break;
        case 'n':
          value.push_back('\n');
          break;
        case 'r':
          value.push_back('\r');
          break;
        case 't':
          value.push_back('\t');
          break;
        default:
          value.push_back(escaped);
          break;
      }
      continue;
    }
    if (c == '"') {
      return value;
    }
    value.push_back(c);
  }
  return std::nullopt;
}

std::optional<std::size_t> extract_json_size_field(std::string_view text, std::string_view key) {
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = text.find(needle);
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  pos = text.find(':', pos + needle.size());
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  std::size_t end = pos;
  while (end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) {
    ++end;
  }
  if (end == pos) {
    return std::nullopt;
  }
  std::size_t value = 0;
  const auto parsed = std::from_chars(text.data() + pos, text.data() + end, value);
  if (parsed.ec != std::errc()) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::string_view> extract_json_array_field(std::string_view text,
                                                         std::string_view key) {
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = text.find(needle);
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  pos = text.find(':', pos + needle.size());
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  if (pos >= text.size() || text[pos] != '[') {
    return std::nullopt;
  }
  std::size_t end = pos;
  int depth = 0;
  bool in_string = false;
  while (end < text.size()) {
    const char c = text[end];
    if (c == '"' && (end == 0 || text[end - 1] != '\\')) {
      in_string = !in_string;
    } else if (!in_string) {
      if (c == '[') {
        ++depth;
      } else if (c == ']') {
        --depth;
        if (depth == 0) {
          return text.substr(pos, end - pos + 1);
        }
      }
    }
    ++end;
  }
  return std::nullopt;
}

bool is_json_null_field(std::string_view text, std::string_view key) {
  const std::string needle = "\"" + std::string(key) + "\"";
  std::size_t pos = text.find(needle);
  if (pos == std::string_view::npos) {
    return false;
  }
  pos = text.find(':', pos + needle.size());
  if (pos == std::string_view::npos) {
    return false;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  return text.substr(pos, 4) == "null";
}

bool is_valid_canonfs_ref(std::string_view value) {
  return value.rfind("sha3-256:", 0) == 0 && value.size() > std::string_view("sha3-256:").size();
}

bool is_one_of(std::string_view value, std::initializer_list<std::string_view> allowed) {
  return std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

std::optional<std::vector<std::string>> extract_json_string_array(std::string_view text,
                                                                  std::string_view key) {
  const auto array = extract_json_array_field(text, key);
  if (!array) {
    return std::nullopt;
  }
  std::vector<std::string> values;
  std::size_t pos = 0;
  while (true) {
    while (pos < array->size() &&
           (std::isspace(static_cast<unsigned char>((*array)[pos])) || (*array)[pos] == '[' ||
            (*array)[pos] == ']' || (*array)[pos] == ',')) {
      ++pos;
    }
    if (pos >= array->size()) {
      break;
    }
    if ((*array)[pos] != '"') {
      return std::nullopt;
    }
    std::size_t local_pos = pos + 1;
    std::string value;
    while (local_pos < array->size()) {
      const char c = (*array)[local_pos++];
      if (c == '\\') {
        if (local_pos >= array->size()) {
          return std::nullopt;
        }
        const char escaped = (*array)[local_pos++];
        switch (escaped) {
          case '\\':
          case '"':
          case '/':
            value.push_back(escaped);
            break;
          case 'n':
            value.push_back('\n');
            break;
          case 'r':
            value.push_back('\r');
            break;
          case 't':
            value.push_back('\t');
            break;
          default:
            value.push_back(escaped);
            break;
        }
        continue;
      }
      if (c == '"') {
        values.push_back(std::move(value));
        pos = local_pos;
        break;
      }
      value.push_back(c);
    }
    if (local_pos > array->size()) {
      return std::nullopt;
    }
  }
  return values;
}

bool validate_string_array(std::string_view text, std::string_view key,
                           bool require_non_empty_items, bool require_canonfs_refs,
                           std::string& error_message) {
  const auto values = extract_json_string_array(text, key);
  if (!values) {
    error_message = std::string("missing array field: ") + std::string(key);
    return false;
  }
  for (const auto& value : *values) {
    if (require_non_empty_items && value.empty()) {
      error_message = std::string("array field contains empty item: ") + std::string(key);
      return false;
    }
    if (require_canonfs_refs && !is_valid_canonfs_ref(value)) {
      error_message = std::string("array field contains invalid CanonFS ref: ") + std::string(key);
      return false;
    }
  }
  return true;
}

bool validate_error_objects_array(std::string_view text, std::string_view key,
                                  bool is_export,
                                  std::initializer_list<std::string_view> allowed_kinds,
                                  std::string& error_message) {
  const auto array = extract_json_array_field(text, key);
  if (!array) {
    error_message = std::string("missing error array field: ") + std::string(key);
    return false;
  }
  std::size_t cursor = 0;
  while (true) {
    const std::size_t object_start = array->find('{', cursor);
    if (object_start == std::string_view::npos) {
      break;
    }
    const std::size_t object_end = array->find('}', object_start);
    if (object_end == std::string_view::npos) {
      error_message = std::string("malformed error object array: ") + std::string(key);
      return false;
    }
    const std::string_view object_text = array->substr(object_start, object_end - object_start + 1);
    const auto kind = extract_json_string_field(object_text, "kind");
    const auto message = extract_json_string_field(object_text, "message");
    const auto code = extract_json_string_field(object_text, "code");
    const auto reason = extract_json_string_field(object_text, "reason");
    if (!kind || !message || !code || !reason || message->empty() || code->empty() ||
        reason->empty()) {
      error_message = std::string("error object missing required fields: ") + std::string(key);
      return false;
    }
    if (!is_one_of(*kind, allowed_kinds)) {
      error_message = std::string("error object has invalid kind in field: ") + std::string(key);
      return false;
    }
    const auto classification = classify_reason(*reason, is_export);
    if (*kind != classification.kind || *code != classification.code) {
      error_message =
          std::string("error object code/kind mismatch for reason in field: ") + std::string(key);
      return false;
    }
    cursor = object_end + 1;
  }
  return true;
}

}  // namespace

namespace t81::canonfs::interchange {

bool is_manifest_document(std::string_view text) {
  const auto schema = extract_json_string_field(text, "schema");
  return schema && *schema == kManifestSchema;
}

bool parse_manifest(std::string_view text, Manifest& manifest, std::string& error_message) {
  if (!is_manifest_document(text)) {
    error_message = "object is not a t81.canonfs-interchange-manifest.v1 document";
    return false;
  }
  std::size_t entries_pos = text.find("\"entries\"");
  if (entries_pos == std::string_view::npos) {
    error_message = "manifest is missing entries";
    return false;
  }
  entries_pos = text.find('[', entries_pos);
  if (entries_pos == std::string_view::npos) {
    error_message = "manifest entries are not an array";
    return false;
  }
  const std::size_t entries_end = text.find(']', entries_pos);
  if (entries_end == std::string_view::npos) {
    error_message = "manifest entries array is not terminated";
    return false;
  }
  manifest.entries.clear();
  std::size_t cursor = entries_pos;
  while (true) {
    const std::size_t object_start = text.find('{', cursor);
    if (object_start == std::string_view::npos || object_start > entries_end) {
      break;
    }
    const std::size_t object_end = text.find('}', object_start);
    if (object_end == std::string_view::npos || object_end > entries_end) {
      error_message = "manifest entry is malformed";
      return false;
    }
    const std::string_view entry_text = text.substr(object_start, object_end - object_start + 1);
    Entry entry;
    const auto path = extract_json_string_field(entry_text, "path");
    const auto object_ref = extract_json_string_field(entry_text, "object_ref");
    const auto size_bytes = extract_json_size_field(entry_text, "size_bytes");
    if (!path || !object_ref || !size_bytes) {
      error_message = "manifest entry is missing required fields";
      return false;
    }
    entry.path = *path;
    entry.object_ref = *object_ref;
    entry.size_bytes = *size_bytes;
    manifest.entries.push_back(std::move(entry));
    cursor = object_end + 1;
  }
  if (manifest.entries.empty()) {
    error_message = "manifest must contain at least one entry";
    return false;
  }
  return true;
}

std::string render_manifest(std::string_view source_kind, std::string_view source_ref,
                            const std::vector<Entry>& entries) {
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"" << kManifestSchema << "\",\n"
      << "  \"source_kind\": \"" << escape_json_text(source_kind) << "\",\n"
      << "  \"source_ref\": \"" << escape_json_text(source_ref) << "\",\n"
      << "  \"entries\": [\n";
  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];
    out << "    {\"path\": \"" << escape_json_text(entry.path) << "\", "
        << "\"object_ref\": \"" << escape_json_text(entry.object_ref) << "\", "
        << "\"size_bytes\": " << entry.size_bytes << "}";
    if (i + 1 < entries.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n}\n";
  return out.str();
}

std::string render_import_provenance(std::string_view source_kind, std::string_view source_ref,
                                     const std::vector<std::string>& imported_objects,
                                     std::string_view manifest_ref,
                                     std::string_view policy_profile) {
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"t81.canonfs-import-provenance.v1\",\n"
      << "  \"source_kind\": \"" << escape_json_text(source_kind) << "\",\n"
      << "  \"source_ref\": \"" << escape_json_text(source_ref) << "\",\n"
      << "  \"imported_objects\": " << json_array_of_strings(imported_objects, 4) << ",\n"
      << "  \"manifest_ref\": ";
  if (manifest_ref.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << escape_json_text(manifest_ref) << "\",\n";
  }
  out << "  \"policy_profile\": \"" << escape_json_text(policy_profile) << "\"\n";
  out << "}\n";
  return out.str();
}

std::string render_export_provenance(const std::vector<std::string>& source_objects,
                                     std::string_view target_kind, std::string_view target_ref,
                                     std::string_view manifest_ref,
                                     std::string_view policy_profile) {
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"t81.canonfs-export-provenance.v1\",\n"
      << "  \"source_objects\": " << json_array_of_strings(source_objects, 4) << ",\n"
      << "  \"target_kind\": \"" << escape_json_text(target_kind) << "\",\n"
      << "  \"target_ref\": \"" << escape_json_text(target_ref) << "\",\n"
      << "  \"manifest_ref\": ";
  if (manifest_ref.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << escape_json_text(manifest_ref) << "\",\n";
  }
  out << "  \"policy_profile\": \"" << escape_json_text(policy_profile) << "\"\n";
  out << "}\n";
  return out.str();
}

std::string render_import_result(std::string_view status, std::string_view source_kind,
                                 std::string_view source_ref,
                                 const std::vector<std::string>& imported_objects,
                                 std::string_view provenance_ref, std::string_view manifest_ref,
                                 const std::vector<std::string>& imported_paths,
                                 const std::vector<std::string>& warnings,
                                 const std::vector<Issue>& errors,
                                 std::string_view policy_result, std::string_view policy_profile) {
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"t81.canonfs-import.v1\",\n"
      << "  \"status\": \"" << escape_json_text(status) << "\",\n"
      << "  \"source_kind\": \"" << escape_json_text(source_kind) << "\",\n"
      << "  \"source_ref\": \"" << escape_json_text(source_ref) << "\",\n"
      << "  \"imported_objects\": " << json_array_of_strings(imported_objects, 4) << ",\n"
      << "  \"provenance_schema\": \"t81.canonfs-import-provenance.v1\",\n"
      << "  \"provenance_ref\": \"" << escape_json_text(provenance_ref) << "\",\n"
      << "  \"manifest_schema\": ";
  if (manifest_ref.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << kManifestSchema << "\",\n";
  }
  out << "  \"manifest_ref\": ";
  if (manifest_ref.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << escape_json_text(manifest_ref) << "\",\n";
  }
  out << "  \"imported_paths\": " << json_array_of_strings(imported_paths, 4) << ",\n"
      << "  \"warnings\": " << json_array_of_strings(warnings, 4) << ",\n"
      << "  \"errors\": " << json_array_of_error_objects(errors, false, 4) << ",\n"
      << "  \"policy_result\": \"" << escape_json_text(policy_result) << "\",\n"
      << "  \"policy_profile\": ";
  if (policy_profile.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << escape_json_text(policy_profile) << "\",\n";
  }
  out << "  \"normalization_summary\": {\n"
      << "    \"timestamps\": \"provenance-only\",\n"
      << "    \"ownership\": \"provenance-only\",\n"
      << "    \"mode_hint\": \"preserved\"\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string render_export_result(std::string_view status,
                                 const std::vector<std::string>& source_objects,
                                 std::string_view target_kind, std::string_view target_ref,
                                 std::string_view provenance_ref, std::string_view manifest_ref,
                                 const std::vector<std::string>& materialized_paths,
                                 const std::vector<std::string>& warnings,
                                 const std::vector<Issue>& errors,
                                 std::string_view policy_result, std::string_view policy_profile) {
  std::ostringstream out;
  out << "{\n"
      << "  \"schema\": \"t81.canonfs-export.v1\",\n"
      << "  \"status\": \"" << escape_json_text(status) << "\",\n"
      << "  \"source_objects\": " << json_array_of_strings(source_objects, 4) << ",\n"
      << "  \"target_kind\": \"" << escape_json_text(target_kind) << "\",\n"
      << "  \"target_ref\": \"" << escape_json_text(target_ref) << "\",\n"
      << "  \"provenance_schema\": \"t81.canonfs-export-provenance.v1\",\n"
      << "  \"provenance_ref\": \"" << escape_json_text(provenance_ref) << "\",\n"
      << "  \"manifest_schema\": ";
  if (manifest_ref.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << kManifestSchema << "\",\n";
  }
  out << "  \"manifest_ref\": ";
  if (manifest_ref.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << escape_json_text(manifest_ref) << "\",\n";
  }
  out << "  \"materialized_paths\": " << json_array_of_strings(materialized_paths, 4) << ",\n"
      << "  \"warnings\": " << json_array_of_strings(warnings, 4) << ",\n"
      << "  \"errors\": " << json_array_of_error_objects(errors, true, 4) << ",\n"
      << "  \"policy_result\": \"" << escape_json_text(policy_result) << "\",\n"
      << "  \"policy_profile\": ";
  if (policy_profile.empty()) {
    out << "null,\n";
  } else {
    out << "\"" << escape_json_text(policy_profile) << "\",\n";
  }
  out << "  \"materialization_summary\": {\n"
      << "    \"timestamps\": \"not-restored\",\n"
      << "    \"ownership\": \"synthesized\",\n"
      << "    \"mode_hint\": \"preserved\"\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

bool validate_manifest_document(std::string_view text, std::string& error_message) {
  Manifest manifest;
  if (!parse_manifest(text, manifest, error_message)) {
    return false;
  }
  const auto source_kind = extract_json_string_field(text, "source_kind");
  const auto source_ref = extract_json_string_field(text, "source_ref");
  if (!source_kind || !is_one_of(*source_kind, {"host-file", "host-directory"})) {
    error_message = "manifest has invalid source_kind";
    return false;
  }
  if (!source_ref || source_ref->empty()) {
    error_message = "manifest has invalid source_ref";
    return false;
  }
  for (const auto& entry : manifest.entries) {
    if (entry.path.empty() || !is_valid_canonfs_ref(entry.object_ref)) {
      error_message = "manifest entry has invalid path or object_ref";
      return false;
    }
  }
  return true;
}

bool validate_import_provenance_document(std::string_view text, std::string& error_message) {
  const auto schema = extract_json_string_field(text, "schema");
  const auto source_kind = extract_json_string_field(text, "source_kind");
  const auto source_ref = extract_json_string_field(text, "source_ref");
  const auto policy_profile = extract_json_string_field(text, "policy_profile");
  if (!schema || *schema != "t81.canonfs-import-provenance.v1") {
    error_message = "invalid import provenance schema";
    return false;
  }
  if (!source_kind || !is_one_of(*source_kind, {"host-file", "host-directory"})) {
    error_message = "invalid import provenance source_kind";
    return false;
  }
  if (!source_ref || source_ref->empty()) {
    error_message = "invalid import provenance source_ref";
    return false;
  }
  if (!validate_string_array(text, "imported_objects", false, true, error_message)) {
    return false;
  }
  if (!is_json_null_field(text, "manifest_ref")) {
    const auto manifest_ref = extract_json_string_field(text, "manifest_ref");
    if (!manifest_ref || !is_valid_canonfs_ref(*manifest_ref)) {
      error_message = "invalid import provenance manifest_ref";
      return false;
    }
  }
  if (!policy_profile ||
      !is_one_of(*policy_profile, {"permissive", "import-only", "export-only", "deny-all"})) {
    error_message = "invalid import provenance policy_profile";
    return false;
  }
  return true;
}

bool validate_export_provenance_document(std::string_view text, std::string& error_message) {
  const auto schema = extract_json_string_field(text, "schema");
  const auto target_kind = extract_json_string_field(text, "target_kind");
  const auto target_ref = extract_json_string_field(text, "target_ref");
  const auto policy_profile = extract_json_string_field(text, "policy_profile");
  if (!schema || *schema != "t81.canonfs-export-provenance.v1") {
    error_message = "invalid export provenance schema";
    return false;
  }
  if (!target_kind || !is_one_of(*target_kind, {"host-file", "host-directory"})) {
    error_message = "invalid export provenance target_kind";
    return false;
  }
  if (!target_ref || target_ref->empty()) {
    error_message = "invalid export provenance target_ref";
    return false;
  }
  if (!validate_string_array(text, "source_objects", false, true, error_message)) {
    return false;
  }
  if (!is_json_null_field(text, "manifest_ref")) {
    const auto manifest_ref = extract_json_string_field(text, "manifest_ref");
    if (!manifest_ref || !is_valid_canonfs_ref(*manifest_ref)) {
      error_message = "invalid export provenance manifest_ref";
      return false;
    }
  }
  if (!policy_profile ||
      !is_one_of(*policy_profile, {"permissive", "import-only", "export-only", "deny-all"})) {
    error_message = "invalid export provenance policy_profile";
    return false;
  }
  return true;
}

bool validate_import_result_document(std::string_view text, std::string& error_message) {
  const auto schema = extract_json_string_field(text, "schema");
  const auto status = extract_json_string_field(text, "status");
  const auto source_kind = extract_json_string_field(text, "source_kind");
  const auto source_ref = extract_json_string_field(text, "source_ref");
  const auto provenance_schema = extract_json_string_field(text, "provenance_schema");
  const auto provenance_ref = extract_json_string_field(text, "provenance_ref");
  const auto manifest_schema = extract_json_string_field(text, "manifest_schema");
  const auto policy_result = extract_json_string_field(text, "policy_result");
  const auto policy_profile = extract_json_string_field(text, "policy_profile");
  const auto timestamps = extract_json_string_field(text, "timestamps");
  const auto ownership = extract_json_string_field(text, "ownership");
  const auto mode_hint = extract_json_string_field(text, "mode_hint");
  if (!schema || *schema != "t81.canonfs-import.v1") {
    error_message = "invalid import result schema";
    return false;
  }
  if (!status || !is_one_of(*status, {"ok", "partial", "error"})) {
    error_message = "invalid import result status";
    return false;
  }
  if (!source_kind || !is_one_of(*source_kind, {"host-file", "host-directory"})) {
    error_message = "invalid import result source_kind";
    return false;
  }
  if (!source_ref || source_ref->empty()) {
    error_message = "invalid import result source_ref";
    return false;
  }
  if (!provenance_schema || *provenance_schema != "t81.canonfs-import-provenance.v1") {
    error_message = "invalid import result provenance_schema";
    return false;
  }
  if (!provenance_ref) {
    error_message = "missing import result provenance_ref";
    return false;
  }
  if ((*status == "ok" || *status == "partial") &&
      (!is_valid_canonfs_ref(*provenance_ref))) {
    error_message = "invalid import result provenance_ref";
    return false;
  }
  if (!provenance_ref->empty() && !is_valid_canonfs_ref(*provenance_ref)) {
    error_message = "invalid import result provenance_ref";
    return false;
  }
  if (!policy_result || !is_one_of(*policy_result, {"allowed", "denied", "partial"})) {
    error_message = "invalid import result policy_result";
    return false;
  }
  if (!is_json_null_field(text, "policy_profile") &&
      (!policy_profile ||
      !is_one_of(*policy_profile, {"permissive", "import-only", "export-only", "deny-all"}))) {
    error_message = "invalid import result policy_profile";
    return false;
  }
  if (is_json_null_field(text, "manifest_ref") != is_json_null_field(text, "manifest_schema")) {
    error_message = "import result manifest schema/ref mismatch";
    return false;
  }
  if (!is_json_null_field(text, "manifest_schema") &&
      (!manifest_schema || *manifest_schema != kManifestSchema)) {
    error_message = "invalid import result manifest_schema";
    return false;
  }
  if (!timestamps || !ownership || !mode_hint) {
    error_message = "missing import normalization summary fields";
    return false;
  }
  if (!validate_string_array(text, "imported_objects", false, true, error_message) ||
      !validate_string_array(text, "warnings", false, false, error_message) ||
      !validate_string_array(text, "imported_paths", false, false, error_message) ||
      !validate_error_objects_array(text, "errors", false,
                                    {"source-failure", "normalization-failure", "policy-failure"},
                                    error_message)) {
    return false;
  }
  if (!is_json_null_field(text, "manifest_ref")) {
    const auto manifest_ref = extract_json_string_field(text, "manifest_ref");
    if (!manifest_ref || !is_valid_canonfs_ref(*manifest_ref)) {
      error_message = "invalid import result manifest_ref";
      return false;
    }
  }
  return true;
}

bool validate_export_result_document(std::string_view text, std::string& error_message) {
  const auto schema = extract_json_string_field(text, "schema");
  const auto status = extract_json_string_field(text, "status");
  const auto target_kind = extract_json_string_field(text, "target_kind");
  const auto target_ref = extract_json_string_field(text, "target_ref");
  const auto provenance_schema = extract_json_string_field(text, "provenance_schema");
  const auto provenance_ref = extract_json_string_field(text, "provenance_ref");
  const auto manifest_schema = extract_json_string_field(text, "manifest_schema");
  const auto policy_result = extract_json_string_field(text, "policy_result");
  const auto policy_profile = extract_json_string_field(text, "policy_profile");
  const auto timestamps = extract_json_string_field(text, "timestamps");
  const auto ownership = extract_json_string_field(text, "ownership");
  const auto mode_hint = extract_json_string_field(text, "mode_hint");
  if (!schema || *schema != "t81.canonfs-export.v1") {
    error_message = "invalid export result schema";
    return false;
  }
  if (!status || !is_one_of(*status, {"ok", "partial", "error"})) {
    error_message = "invalid export result status";
    return false;
  }
  if (!target_kind || !is_one_of(*target_kind, {"host-file", "host-directory"})) {
    error_message = "invalid export result target_kind";
    return false;
  }
  if (!target_ref || target_ref->empty()) {
    error_message = "invalid export result target_ref";
    return false;
  }
  if (!provenance_schema || *provenance_schema != "t81.canonfs-export-provenance.v1") {
    error_message = "invalid export result provenance_schema";
    return false;
  }
  if (!provenance_ref) {
    error_message = "missing export result provenance_ref";
    return false;
  }
  if ((*status == "ok" || *status == "partial") &&
      (!is_valid_canonfs_ref(*provenance_ref))) {
    error_message = "invalid export result provenance_ref";
    return false;
  }
  if (!provenance_ref->empty() && !is_valid_canonfs_ref(*provenance_ref)) {
    error_message = "invalid export result provenance_ref";
    return false;
  }
  if (!policy_result || !is_one_of(*policy_result, {"allowed", "denied", "partial"})) {
    error_message = "invalid export result policy_result";
    return false;
  }
  if (!is_json_null_field(text, "policy_profile") &&
      (!policy_profile ||
       !is_one_of(*policy_profile, {"permissive", "import-only", "export-only", "deny-all"}))) {
    error_message = "invalid export result policy_profile";
    return false;
  }
  if (is_json_null_field(text, "manifest_ref") != is_json_null_field(text, "manifest_schema")) {
    error_message = "export result manifest schema/ref mismatch";
    return false;
  }
  if (!is_json_null_field(text, "manifest_schema") &&
      (!manifest_schema || *manifest_schema != kManifestSchema)) {
    error_message = "invalid export result manifest_schema";
    return false;
  }
  if (!timestamps || !ownership || !mode_hint) {
    error_message = "missing export materialization summary fields";
    return false;
  }
  if (!validate_string_array(text, "source_objects", false, true, error_message) ||
      !validate_string_array(text, "warnings", false, false, error_message) ||
      !validate_string_array(text, "materialized_paths", false, false, error_message) ||
      !validate_error_objects_array(text, "errors", true,
                                    {"materialization-failure", "target-failure", "policy-failure"},
                                    error_message)) {
    return false;
  }
  if (!is_json_null_field(text, "manifest_ref")) {
    const auto manifest_ref = extract_json_string_field(text, "manifest_ref");
    if (!manifest_ref || !is_valid_canonfs_ref(*manifest_ref)) {
      error_message = "invalid export result manifest_ref";
      return false;
    }
  }
  return true;
}

}  // namespace t81::canonfs::interchange
