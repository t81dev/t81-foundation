#pragma once

#include <map>
#include <span>
#include <string>
#include <string_view>

namespace t81::cli::artifact_family {

std::string_view assess_record_schema();
std::string_view assess_bundle_schema();
std::string_view route_record_schema();
std::string_view route_bundle_schema();
std::string_view classify_record_schema();
std::string_view classify_bundle_schema();

bool is_supported_schema(std::string_view schema);
bool is_record_schema(std::string_view schema);
bool is_bundle_schema(std::string_view schema);

std::span<const std::string_view> required_fields_for_schema(std::string_view schema);
bool is_supported_field(std::string_view schema, std::string_view field);

std::string render_artifact(std::string_view schema,
                            const std::map<std::string, std::string>& fields);
bool validate_artifact(std::string_view schema, std::string_view artifact_text,
                       std::string& validation_error);
std::string_view next_inspect_field_for_schema(std::string_view schema);
std::string supported_schema_error_suffix();

}  // namespace t81::cli::artifact_family
