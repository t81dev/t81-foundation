#include "t81/canonfs/json_renderer.hpp"
#include "t81/canonfs/interchange.hpp"

namespace t81::canonfs {

std::string DefaultInterchangeJSONRenderer::render_import_result(const ImportOutcome& outcome) {
    // Use existing interchange module for now
    return interchange::render_import_result(
        outcome.status, outcome.source_kind, outcome.source_ref,
        outcome.imported_objects, outcome.provenance_ref, outcome.manifest_ref,
        outcome.imported_paths, outcome.warnings, outcome.errors,
        outcome.policy_result, outcome.policy_profile);
}

std::string DefaultInterchangeJSONRenderer::render_export_result(const ExportOutcome& outcome) {
    // Use existing interchange module for now
    return interchange::render_export_result(
        outcome.status, outcome.source_objects, outcome.target_kind, outcome.target_ref,
        outcome.provenance_ref, outcome.manifest_ref, outcome.materialized_paths,
        outcome.warnings, outcome.errors, outcome.policy_result, outcome.policy_profile);
}

bool DefaultInterchangeJSONRenderer::validate_import_schema(const std::string& json) const {
    // Use existing interchange module validation
    std::string error_message;
    return interchange::validate_import_result_document(json, error_message);
}

bool DefaultInterchangeJSONRenderer::validate_export_schema(const std::string& json) const {
    // Use existing interchange module validation
    std::string error_message;
    return interchange::validate_export_result_document(json, error_message);
}

std::string DefaultInterchangeJSONRenderer::get_import_schema_version() const {
    return "t81.canonfs-import.v1";
}

std::string DefaultInterchangeJSONRenderer::get_export_schema_version() const {
    return "t81.canonfs-export.v1";
}

std::unique_ptr<InterchangeJSONRenderer> create_default_json_renderer() {
    return std::make_unique<DefaultInterchangeJSONRenderer>();
}

} // namespace t81::canonfs
