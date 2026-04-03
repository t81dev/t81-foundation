// Refactored CanonFS CLI driver using new interchange engine
// This demonstrates how the CLI can use the new CanonFSInterchangeEngine

#include "t81/canonfs/interchange_engine.hpp"
#include "t81/canonfs/json_renderer.hpp"
#include "t81/canonfs/interchange_ops.hpp"
#include <iostream>
#include <memory>

using namespace t81::canonfs;

// Helper to create ImportRequest from CLI arguments
ImportRequest create_import_request(
    const std::filesystem::path& input,
    const std::filesystem::path& canonfs_root,
    InterchangePolicyProfile policy_profile,
    const std::optional<std::string>& policy_file) {
    
    ImportRequest request;
    request.input_path = input;
    request.canonfs_root = canonfs_root;
    request.policy_profile = policy_profile;
    
    // Create policy evaluator if policy file provided
    if (policy_file) {
        // For now, use default evaluator - this would be enhanced
        // to use the new policy evaluation framework
        request.policy_evaluator = [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
            return InterchangePolicyDecision{true, "allow"}; // Placeholder
        };
    } else {
        // Use existing Axion policy evaluator
        request.policy_evaluator = t81::canonfs::make_axion_interchange_policy_evaluator(
            t81::canonfs::load_interchange_axion_policy(*policy_file));
    }
    
    return request;
}

// Helper to create ExportRequest from CLI arguments
ExportRequest create_export_request(
    const std::string& canonical_hash,
    const std::filesystem::path& output_path,
    const std::filesystem::path& canonfs_root,
    InterchangePolicyProfile policy_profile,
    const std::optional<std::string>& policy_file) {
    
    ExportRequest request;
    request.canonical_hash = canonical_hash;
    request.output_path = output_path;
    request.canonfs_root = canonfs_root;
    request.policy_profile = policy_profile;
    
    // Create policy evaluator if policy file provided
    if (policy_file) {
        request.policy_evaluator = [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
            return InterchangePolicyDecision{true, "allow"}; // Placeholder
        };
    } else {
        // Use existing Axion policy evaluator for export
        request.policy_evaluator = t81::canonfs::make_axion_interchange_policy_evaluator(
            t81::canonfs::load_interchange_axion_policy(*policy_file), true);
    }
    
    return request;
}

// Refactored import function using new engine
int canonfs_import_refactored(const std::filesystem::path& input, 
                           const std::filesystem::path& canonfs_root,
                           const std::optional<std::filesystem::path>& policy_path,
                           const std::optional<std::string>& policy_profile, 
                           bool as_json) {
    
    // Determine policy profile
    InterchangePolicyProfile profile = InterchangePolicyProfile::Permissive;
    if (policy_profile) {
        auto parsed = parse_interchange_policy_profile(*policy_profile);
        if (parsed) {
            profile = *parsed;
        }
    }
    
    // Create request
    auto request = create_import_request(input, canonfs_root, profile, policy_path);
    
    // Create and configure engine
    auto engine = create_interchange_engine();
    auto renderer = create_default_json_renderer();
    engine->set_json_renderer(renderer);
    
    // Perform import
    auto outcome = engine->import(request);
    
    // Handle output
    if (as_json) {
        auto json = renderer.render_import_result(outcome);
        if (!renderer.validate_import_schema(json)) {
            error("canonfs import: internal JSON validation failed");
            return 1;
        }
        std::cout << json;
    } else {
        // Human-readable output (maintain compatibility)
        if (!outcome.ok()) {
            error("canonfs import: " +
                  (outcome.errors.empty() ? std::string("import failed") : outcome.errors.front().message));
            return 1;
        }
        
        std::cout << "CanonFS import " << outcome.status << ".\n";
        std::cout << "Source:      " << input.string() << "\n";
        std::cout << "Imported:    " << outcome.imported_objects.size() << " object(s)\n";
        if (!outcome.manifest_ref.empty()) {
            std::cout << "Manifest:    " << outcome.manifest_ref << "\n";
        }
        if (!outcome.provenance_ref.empty()) {
            std::cout << "Provenance:  " << outcome.provenance_ref << "\n";
        }
        std::cout << "Result:      manifest/provenance refs are reusable handles for later export or audit\n";
        if (!outcome.manifest_ref.empty()) {
            std::cout << "Next (progress): t81 canonfs export <manifest-ref> --out <dir> to materialize the imported tree\n";
        }
    }
    
    return outcome.ok() ? 0 : 1;
}

// Refactored export function using new engine
int canonfs_export_refactored(const std::string& canonical_hash,
                           const std::filesystem::path& output_path,
                           const std::filesystem::path& canonfs_root,
                           const std::optional<std::filesystem::path>& policy_path,
                           const std::optional<std::string>& policy_profile, 
                           bool as_json) {
    
    // Determine policy profile
    InterchangePolicyProfile profile = InterchangePolicyProfile::Permissive;
    if (policy_profile) {
        auto parsed = parse_interchange_policy_profile(*policy_profile);
        if (parsed) {
            profile = *parsed;
        }
    }
    
    // Create request
    auto request = create_export_request(canonical_hash, output_path, canonfs_root, profile, policy_path);
    
    // Create and configure engine
    auto engine = create_interchange_engine();
    auto renderer = create_default_json_renderer();
    engine->set_json_renderer(renderer);
    
    // Perform export
    auto outcome = engine->export(request);
    
    // Handle output
    if (as_json) {
        auto json = renderer.render_export_result(outcome);
        if (!renderer.validate_export_schema(json)) {
            error("canonfs export: internal JSON validation failed");
            return 1;
        }
        std::cout << json;
    } else {
        // Human-readable output (maintain compatibility)
        if (!outcome.ok()) {
            error("canonfs export: " +
                  (outcome.errors.empty() ? std::string("export failed") : outcome.errors.front().message));
            return 1;
        }
        
        std::cout << "CanonFS export " << outcome.status << ".\n";
        std::cout << "Target:      " << output_path.string() << "\n";
        std::cout << "Materialized:" << " " << outcome.materialized_paths.size() << " path(s)\n";
        if (!outcome.manifest_ref.empty()) {
            std::cout << "Manifest:    " << outcome.manifest_ref << "\n";
        }
        if (!outcome.provenance_ref.empty()) {
            std::cout << "Provenance:  " << outcome.provenance_ref << "\n";
        }
        std::cout << "Result:      CanonFS materialized host-visible files at target path\n";
        std::cout << "Next (inspect):  re-import it to compare imported paths or object identity\n";
    }
    
    return outcome.ok() ? 0 : 1;
}

// Demonstration of evidence collection capabilities
void demonstrate_evidence_collection() {
    auto engine = create_interchange_engine();
    
    std::cout << "=== CanonFS Interchange Engine Evidence Collection Demo ===\n";
    
    // Show evidence log is initially empty
    std::cout << "Initial evidence count: " << engine->get_evidence_log().size() << "\n";
    
    // Perform a mock operation to generate evidence
    ImportRequest mock_request;
    mock_request.input_path = "/tmp/mock_file.txt";
    mock_request.canonfs_root = "/tmp/canonfs";
    mock_request.policy_profile = InterchangePolicyProfile::Permissive;
    mock_request.policy_evaluator = [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
        return InterchangePolicyDecision{true, "allow"};
    };
    
    // This will fail and generate evidence
    auto outcome = engine->import(mock_request);
    
    // Show collected evidence
    auto evidence = engine->get_evidence_log();
    std::cout << "Evidence count after operation: " << evidence.size() << "\n";
    
    if (!evidence.empty()) {
        const auto& e = evidence[0];
        std::cout << "Operation ID: " << e.operation_id << "\n";
        std::cout << "Operation Type: " << e.operation_type << "\n";
        std::cout << "Determinism Level: " << e.determinism_level << "\n";
        std::cout << "Metadata entries: " << e.metadata.size() << "\n";
        for (const auto& [key, value] : e.metadata) {
            std::cout << "  " << key << ": " << value << "\n";
        }
        std::cout << "Policy Decisions: " << e.policy_decisions.size() << "\n";
        for (const auto& decision : e.policy_decisions) {
            std::cout << "  " << decision << "\n";
        }
    }
    
    std::cout << "\n=== Evidence Collection Benefits ===\n";
    std::cout << "✅ Complete audit trail for all operations\n";
    std::cout << "✅ Operation tracking with unique IDs\n";
    std::cout << "✅ Policy decision logging\n";
    std::cout << "✅ Performance metrics collection\n";
    std::cout << "✅ Enhanced debugging capabilities\n";
}
