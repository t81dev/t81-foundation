#include "t81/cli/artifact_family.hpp"

#include "t81/canonfs/canon_types.hpp"
#include "t81/isa/base81_view.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace t81::cli::artifact_family {
namespace {

constexpr std::string_view kAssessRecordSchema =
    "t81.ai.task.assess-fixed.host-action-record.v1";
constexpr std::string_view kAssessBundleSchema = "t81.ai.task.assess-fixed.bundle.v1";
constexpr std::string_view kRouteRecordSchema =
    "t81.ai.task.route-fixed.path-selection-record.v1";
constexpr std::string_view kRouteBundleSchema = "t81.ai.task.route-fixed.bundle.v1";
constexpr std::string_view kClassifyRecordSchema =
    "t81.ai.task.classify-fixed.rule-selection-record.v1";
constexpr std::string_view kClassifyBundleSchema = "t81.ai.task.classify-fixed.bundle.v1";

constexpr std::array<std::string_view, 8> kAssessRequiredFields{{
    "source_result_ref",
    "source_provenance_ref",
    "decision",
    "reason_code",
    "termination_reason",
    "selected_action",
    "selected_path",
    "action_ref",
}};
constexpr std::array<std::string_view, 7> kRouteRequiredFields{{
    "source_result_ref",
    "source_provenance_ref",
    "route",
    "termination_reason",
    "selected_action",
    "selected_path",
    "action_ref",
}};
constexpr std::array<std::string_view, 6> kClassifyRequiredFields{{
    "source_result_ref",
    "source_provenance_ref",
    "label",
    "termination_reason",
    "selected_rule_set",
    "rule_set_ref",
}};
constexpr std::array<std::string_view, 4> kBundleRequiredFields{{
    "source_result_ref",
    "source_provenance_ref",
    "action_ref",
    "record_ref",
}};
constexpr std::array<std::string_view, 0> kNoRequiredFields{};

std::optional<std::string> extract_json_string_field(std::string_view text, std::string_view field) {
  const std::string needle = "\"" + std::string(field) + "\": \"";
  const std::size_t start = text.find(needle);
  if (start == std::string_view::npos) {
    return std::nullopt;
  }
  const std::size_t value_start = start + needle.size();
  std::string value;
  bool escaped = false;
  for (std::size_t i = value_start; i < text.size(); ++i) {
    const char ch = text[i];
    if (escaped) {
      switch (ch) {
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
          value.push_back(ch);
          break;
      }
      escaped = false;
      continue;
    }
    if (ch == '\\') {
      escaped = true;
      continue;
    }
    if (ch == '"') {
      return value;
    }
    value.push_back(ch);
  }
  return std::nullopt;
}

bool parse_canonical_hash(std::string_view prefixed_hash, t81::canonfs::CanonRef& ref,
                          std::string& error_message) {
  constexpr std::string_view prefix = "sha3-256:";
  if (!prefixed_hash.starts_with(prefix)) {
    error_message = "invalid canonical hash (expected sha3-256:...)";
    return false;
  }
  try {
    ref.hash.h = t81::hash::CanonHash81::from_string(std::string(prefixed_hash.substr(prefix.size())));
  } catch (const std::exception& e) {
    error_message = std::string("invalid canonical hash: ") + e.what();
    return false;
  }
  return true;
}

std::string json_escape(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    switch (c) {
      case '\\':
      case '"':
        out.push_back('\\');
        out.push_back(c);
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

bool validate_canonfs_ref_field(std::string_view artifact_text, std::string_view field_name,
                                std::string& validation_error) {
  const auto value = extract_json_string_field(artifact_text, field_name);
  if (!value || value->empty()) {
    validation_error = "missing required field \"" + std::string(field_name) + "\"";
    return false;
  }
  t81::canonfs::CanonRef ignored_ref;
  std::string ref_error;
  if (!parse_canonical_hash(*value, ignored_ref, ref_error)) {
    validation_error = "invalid CanonFS ref in field \"" + std::string(field_name) +
                       "\": " + ref_error;
    return false;
  }
  return true;
}

}  // namespace

std::string_view assess_record_schema() { return kAssessRecordSchema; }
std::string_view assess_bundle_schema() { return kAssessBundleSchema; }
std::string_view route_record_schema() { return kRouteRecordSchema; }
std::string_view route_bundle_schema() { return kRouteBundleSchema; }
std::string_view classify_record_schema() { return kClassifyRecordSchema; }
std::string_view classify_bundle_schema() { return kClassifyBundleSchema; }

bool is_supported_schema(std::string_view schema) {
  return schema == kAssessRecordSchema || schema == kAssessBundleSchema ||
         schema == kRouteRecordSchema || schema == kRouteBundleSchema ||
         schema == kClassifyRecordSchema || schema == kClassifyBundleSchema;
}

bool is_record_schema(std::string_view schema) {
  return schema == kAssessRecordSchema || schema == kRouteRecordSchema ||
         schema == kClassifyRecordSchema;
}

bool is_bundle_schema(std::string_view schema) {
  return schema == kAssessBundleSchema || schema == kRouteBundleSchema ||
         schema == kClassifyBundleSchema;
}

std::span<const std::string_view> required_fields_for_schema(std::string_view schema) {
  if (schema == kAssessRecordSchema) {
    return kAssessRequiredFields;
  }
  if (schema == kRouteRecordSchema) {
    return kRouteRequiredFields;
  }
  if (schema == kClassifyRecordSchema) {
    return kClassifyRequiredFields;
  }
  if (is_bundle_schema(schema)) {
    return kBundleRequiredFields;
  }
  return kNoRequiredFields;
}

bool is_supported_field(std::string_view schema, std::string_view field) {
  if (schema == kAssessRecordSchema) {
    return field == "schema" || field == "decision" || field == "reason_code" ||
           field == "selected_action" || field == "selected_path" || field == "action_ref" ||
           field == "source_result_ref" || field == "source_provenance_ref" ||
           field == "termination_reason";
  }
  if (schema == kAssessBundleSchema || schema == kRouteBundleSchema ||
      schema == kClassifyBundleSchema) {
    return field == "schema" || field == "source_result_ref" ||
           field == "source_provenance_ref" || field == "action_ref" || field == "record_ref";
  }
  if (schema == kRouteRecordSchema) {
    return field == "schema" || field == "route" || field == "selected_action" ||
           field == "selected_path" || field == "action_ref" || field == "source_result_ref" ||
           field == "source_provenance_ref" || field == "termination_reason";
  }
  if (schema == kClassifyRecordSchema) {
    return field == "schema" || field == "label" || field == "selected_rule_set" ||
           field == "rule_set_ref" || field == "source_result_ref" ||
           field == "source_provenance_ref" || field == "termination_reason";
  }
  return false;
}

std::string render_artifact(std::string_view schema, const std::map<std::string, std::string>& fields) {
  std::ostringstream out;
  if (schema == kAssessRecordSchema) {
    out << "{\n"
        << "  \"schema\": \"" << kAssessRecordSchema << "\",\n"
        << "  \"source_result_ref\": \"" << json_escape(fields.at("source_result_ref")) << "\",\n"
        << "  \"source_provenance_ref\": \"" << json_escape(fields.at("source_provenance_ref"))
        << "\",\n"
        << "  \"decision\": \"" << json_escape(fields.at("decision")) << "\",\n"
        << "  \"reason_code\": \"" << json_escape(fields.at("reason_code")) << "\",\n"
        << "  \"termination_reason\": \"" << json_escape(fields.at("termination_reason")) << "\",\n"
        << "  \"selected_action\": \"" << json_escape(fields.at("selected_action")) << "\",\n"
        << "  \"selected_path\": \"" << json_escape(fields.at("selected_path")) << "\",\n"
        << "  \"action_ref\": \"" << json_escape(fields.at("action_ref")) << "\"\n"
        << "}\n";
    return out.str();
  }
  if (schema == kRouteRecordSchema) {
    out << "{\n"
        << "  \"schema\": \"" << kRouteRecordSchema << "\",\n"
        << "  \"source_result_ref\": \"" << json_escape(fields.at("source_result_ref")) << "\",\n"
        << "  \"source_provenance_ref\": \"" << json_escape(fields.at("source_provenance_ref"))
        << "\",\n"
        << "  \"route\": \"" << json_escape(fields.at("route")) << "\",\n"
        << "  \"termination_reason\": \"" << json_escape(fields.at("termination_reason")) << "\",\n"
        << "  \"selected_action\": \"" << json_escape(fields.at("selected_action")) << "\",\n"
        << "  \"selected_path\": \"" << json_escape(fields.at("selected_path")) << "\",\n"
        << "  \"action_ref\": \"" << json_escape(fields.at("action_ref")) << "\"\n"
        << "}\n";
    return out.str();
  }
  if (schema == kClassifyRecordSchema) {
    out << "{\n"
        << "  \"schema\": \"" << kClassifyRecordSchema << "\",\n"
        << "  \"source_result_ref\": \"" << json_escape(fields.at("source_result_ref")) << "\",\n"
        << "  \"source_provenance_ref\": \"" << json_escape(fields.at("source_provenance_ref"))
        << "\",\n"
        << "  \"label\": \"" << json_escape(fields.at("label")) << "\",\n"
        << "  \"termination_reason\": \"" << json_escape(fields.at("termination_reason")) << "\",\n"
        << "  \"selected_rule_set\": \"" << json_escape(fields.at("selected_rule_set")) << "\",\n"
        << "  \"rule_set_ref\": \"" << json_escape(fields.at("rule_set_ref")) << "\"\n"
        << "}\n";
    return out.str();
  }
  if (schema == kAssessBundleSchema) {
    out << "{\n"
        << "  \"schema\": \"" << kAssessBundleSchema << "\",\n"
        << "  \"source_result_ref\": \"" << json_escape(fields.at("source_result_ref")) << "\",\n"
        << "  \"source_provenance_ref\": \"" << json_escape(fields.at("source_provenance_ref"))
        << "\",\n"
        << "  \"action_ref\": \"" << json_escape(fields.at("action_ref")) << "\",\n"
        << "  \"record_ref\": \"" << json_escape(fields.at("record_ref")) << "\"\n"
        << "}\n";
    return out.str();
  }
  if (schema == kRouteBundleSchema) {
    out << "{\n"
        << "  \"schema\": \"" << kRouteBundleSchema << "\",\n"
        << "  \"source_result_ref\": \"" << json_escape(fields.at("source_result_ref")) << "\",\n"
        << "  \"source_provenance_ref\": \"" << json_escape(fields.at("source_provenance_ref"))
        << "\",\n"
        << "  \"action_ref\": \"" << json_escape(fields.at("action_ref")) << "\",\n"
        << "  \"record_ref\": \"" << json_escape(fields.at("record_ref")) << "\"\n"
        << "}\n";
    return out.str();
  }
  if (schema == kClassifyBundleSchema) {
    out << "{\n"
        << "  \"schema\": \"" << kClassifyBundleSchema << "\",\n"
        << "  \"source_result_ref\": \"" << json_escape(fields.at("source_result_ref")) << "\",\n"
        << "  \"source_provenance_ref\": \"" << json_escape(fields.at("source_provenance_ref"))
        << "\",\n"
        << "  \"action_ref\": \"" << json_escape(fields.at("action_ref")) << "\",\n"
        << "  \"record_ref\": \"" << json_escape(fields.at("record_ref")) << "\"\n"
        << "}\n";
    return out.str();
  }
  return {};
}

bool validate_artifact(std::string_view schema, std::string_view artifact_text,
                       std::string& validation_error) {
  const auto schema_field = extract_json_string_field(artifact_text, "schema");
  if (!schema_field || *schema_field != schema) {
    validation_error = "schema mismatch";
    return false;
  }
  for (std::string_view required_field : required_fields_for_schema(schema)) {
    const auto value = extract_json_string_field(artifact_text, required_field);
    if (!value || value->empty()) {
      validation_error = "missing required field \"" + std::string(required_field) + "\"";
      return false;
    }
  }

  if (schema == kAssessRecordSchema || schema == kRouteRecordSchema) {
    return validate_canonfs_ref_field(artifact_text, "source_result_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "source_provenance_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "action_ref", validation_error);
  }
  if (schema == kClassifyRecordSchema) {
    return validate_canonfs_ref_field(artifact_text, "source_result_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "source_provenance_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "rule_set_ref", validation_error);
  }
  if (is_bundle_schema(schema)) {
    return validate_canonfs_ref_field(artifact_text, "source_result_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "source_provenance_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "action_ref", validation_error) &&
           validate_canonfs_ref_field(artifact_text, "record_ref", validation_error);
  }

  validation_error = supported_schema_error_suffix();
  return false;
}

std::string_view next_inspect_field_for_schema(std::string_view schema) {
  if (schema == kAssessRecordSchema) {
    return "selected_action";
  }
  if (schema == kRouteRecordSchema) {
    return "selected_path";
  }
  if (schema == kClassifyRecordSchema) {
    return "selected_rule_set";
  }
  if (is_bundle_schema(schema)) {
    return "record_ref";
  }
  return "schema";
}

std::string supported_schema_error_suffix() {
  return "supports only schema t81.ai.task.assess-fixed.host-action-record.v1, "
         "t81.ai.task.assess-fixed.bundle.v1, "
         "t81.ai.task.route-fixed.path-selection-record.v1, "
         "t81.ai.task.route-fixed.bundle.v1, "
         "t81.ai.task.classify-fixed.rule-selection-record.v1, or "
         "t81.ai.task.classify-fixed.bundle.v1";
}

}  // namespace t81::cli::artifact_family
