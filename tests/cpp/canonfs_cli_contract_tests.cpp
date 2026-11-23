#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "t81/canonfs/interchange_ops.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace t81::canonfs;
using namespace t81::canonfs::interchange;

class CanonFSCLIContractTests : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary directory for tests
    test_dir_ = std::filesystem::temp_directory_path() / "canonfs_cli_contract_test";
    std::filesystem::create_directories(test_dir_);
    canonfs_root_ = test_dir_ / ".t81_canonfs";
    std::filesystem::create_directories(canonfs_root_);
  }

  void TearDown() override {
    // Cleanup test directory
    std::filesystem::remove_all(test_dir_);
  }

  // Helper to create a test file
  void create_test_file(const std::filesystem::path& path,
                        const std::string& content = "test content") {
    std::ofstream file(path);
    file << content;
    file.close();
  }

  // Helper to check if JSON contains required field
  bool json_contains_field(const std::string& json_str, const std::string& field) {
    return json_str.find("\"" + field + "\"") != std::string::npos;
  }

  // Helper to check if JSON has valid structure (basic validation)
  void validate_basic_json_structure(const std::string& json_str,
                                     const std::vector<std::string>& required_fields) {
    // Check for opening and closing braces
    EXPECT_TRUE(json_str.find("{") != std::string::npos);
    EXPECT_TRUE(json_str.find("}") != std::string::npos);

    // Check for required fields
    for (const auto& field : required_fields) {
      EXPECT_TRUE(json_contains_field(json_str, field)) << "Missing required field: " << field;
    }
  }

  // Helper to validate import result JSON structure
  void validate_import_result_schema(const std::string& json_str) {
    std::vector<std::string> required_fields = {
        "schema",         "status",           "source_kind",
        "source_ref",     "imported_objects", "provenance_schema",
        "provenance_ref", "manifest_schema",  "manifest_ref",
        "imported_paths", "warnings",         "errors",
        "policy_result",  "policy_profile",   "normalization_summary"};

    validate_basic_json_structure(json_str, required_fields);

    // Validate specific schema values
    EXPECT_TRUE(json_contains_field(json_str, "t81.canonfs-import.v1"));
  }

  // Helper to validate export result JSON structure
  void validate_export_result_schema(const std::string& json_str) {
    std::vector<std::string> required_fields = {"schema",
                                                "status",
                                                "source_objects",
                                                "target_kind",
                                                "target_ref",
                                                "provenance_schema",
                                                "provenance_ref",
                                                "manifest_schema",
                                                "manifest_ref",
                                                "materialized_paths",
                                                "warnings",
                                                "errors",
                                                "policy_result",
                                                "policy_profile",
                                                "materialization_summary"};

    validate_basic_json_structure(json_str, required_fields);

    // Validate specific schema values
    EXPECT_TRUE(json_contains_field(json_str, "t81.canonfs-export.v1"));
  }

  // Helper to validate error object structure
  void validate_error_object_structure(const std::string& json_str) {
    // Basic validation that error object has required fields
    EXPECT_TRUE(json_contains_field(json_str, "kind"));
    EXPECT_TRUE(json_contains_field(json_str, "message"));
    EXPECT_TRUE(json_contains_field(json_str, "code"));
    EXPECT_TRUE(json_contains_field(json_str, "reason"));
  }

  std::filesystem::path test_dir_;
  std::filesystem::path canonfs_root_;
};

// Test import success JSON structure
TEST_F(CanonFSCLIContractTests, ImportSuccessJSONStructure) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);

  ImportOptions options;
  options.canonfs_root = canonfs_root_.string();
  options.policy_profile = InterchangePolicyProfile::Permissive;
  options.policy_evaluator = [](std::string_view, std::string_view,
                                std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome = import_path(test_file, options);

  // Should succeed
  EXPECT_TRUE(outcome.ok());

  // Render the JSON
  std::string json_output = render_import_result(
      outcome.status, outcome.source_kind, outcome.source_ref, outcome.imported_objects,
      outcome.provenance_ref, outcome.manifest_ref, outcome.imported_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  // Basic JSON structure validation
  EXPECT_TRUE(json_output.find("{") != std::string::npos);
  EXPECT_TRUE(json_output.find("}") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"schema\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"t81.canonfs-import.v1\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"status\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"ok\"") != std::string::npos);
}

// Test error object classification consistency
TEST_F(CanonFSCLIContractTests, ErrorObjectClassificationConsistency) {
  // Test that error objects have consistent classification
  std::vector<std::string> test_reasons = {"policy_denied", "missing_source",
                                           "symlink_not_supported", "storage_write_failed"};

  for (const auto& reason : test_reasons) {
    ImportOptions options;
    options.canonfs_root = canonfs_root_.string();
    options.policy_profile = InterchangePolicyProfile::Permissive;
    options.policy_evaluator = [reason](std::string_view, std::string_view,
                                        std::string_view) -> InterchangePolicyDecision {
      return InterchangePolicyDecision{false, "test denial"};
    };

    // Create a scenario that would trigger this error
    auto outcome = import_path(test_dir_ / "trigger_error", options);

    if (!outcome.ok() && !outcome.errors.empty()) {
      // Render JSON
      std::string json_output = render_import_result(
          outcome.status, outcome.source_kind, outcome.source_ref, outcome.imported_objects,
          outcome.provenance_ref, outcome.manifest_ref, outcome.imported_paths, outcome.warnings,
          outcome.errors, outcome.policy_result, outcome.policy_profile);

      // Basic validation that JSON contains error structure
      EXPECT_TRUE(json_output.find("\"errors\"") != std::string::npos);
      EXPECT_TRUE(json_output.find("\"kind\"") != std::string::npos);
      EXPECT_TRUE(json_output.find("\"code\"") != std::string::npos);
      EXPECT_TRUE(json_output.find("\"reason\"") != std::string::npos);
    }
  }
}

// Test import failure JSON structure with policy denial
TEST_F(CanonFSCLIContractTests, ImportPolicyDenialJSONStructure) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);

  ImportOptions options;
  options.canonfs_root = canonfs_root_.string();
  options.policy_profile = InterchangePolicyProfile::DenyAll;
  options.policy_evaluator = [](std::string_view, std::string_view,
                                std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome = import_path(test_file, options);

  // Should fail
  EXPECT_FALSE(outcome.ok());
  EXPECT_FALSE(outcome.errors.empty());

  // Render the JSON
  std::string json_output = render_import_result(
      outcome.status, outcome.source_kind, outcome.source_ref, outcome.imported_objects,
      outcome.provenance_ref, outcome.manifest_ref, outcome.imported_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  // Basic JSON structure validation
  EXPECT_TRUE(json_output.find("{") != std::string::npos);
  EXPECT_TRUE(json_output.find("}") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"schema\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"t81.canonfs-import.v1\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"status\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"error\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"errors\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"policy_result\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"deny-all\"") != std::string::npos);
}

// Test import failure JSON structure with file not found
TEST_F(CanonFSCLIContractTests, ImportFileNotFoundJSONStructure) {
  auto nonexistent_file = test_dir_ / "nonexistent.txt";

  ImportOptions options;
  options.canonfs_root = canonfs_root_.string();
  options.policy_profile = InterchangePolicyProfile::Permissive;
  options.policy_evaluator = [](std::string_view, std::string_view,
                                std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome = import_path(nonexistent_file, options);

  // Should fail
  EXPECT_FALSE(outcome.ok());
  EXPECT_FALSE(outcome.errors.empty());

  // Render the JSON
  std::string json_output = render_import_result(
      outcome.status, outcome.source_kind, outcome.source_ref, outcome.imported_objects,
      outcome.provenance_ref, outcome.manifest_ref, outcome.imported_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  // Basic JSON structure validation
  EXPECT_TRUE(json_output.find("{") != std::string::npos);
  EXPECT_TRUE(json_output.find("}") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"schema\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"t81.canonfs-import.v1\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"status\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"error\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"errors\"") != std::string::npos);
}

// Test export success JSON structure
TEST_F(CanonFSCLIContractTests, ExportSuccessJSONStructure) {
  // First, import a file to have something to export
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);

  ImportOptions import_options;
  import_options.canonfs_root = canonfs_root_.string();
  import_options.policy_profile = InterchangePolicyProfile::Permissive;
  import_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto import_outcome = import_path(test_file, import_options);
  ASSERT_TRUE(import_outcome.ok());
  ASSERT_FALSE(import_outcome.imported_objects.empty());

  // Now export the imported object
  auto export_dir = test_dir_ / "export";
  std::filesystem::create_directories(export_dir);

  ExportOptions export_options;
  export_options.canonfs_root = canonfs_root_.string();
  export_options.policy_profile = InterchangePolicyProfile::Permissive;
  export_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome =
      export_ref(import_outcome.imported_objects[0], export_dir / "exported.bin", export_options);

  // Should succeed
  EXPECT_TRUE(outcome.ok());

  // Render the JSON
  std::string json_output = render_export_result(
      outcome.status, outcome.source_objects, outcome.target_kind, outcome.target_ref,
      outcome.provenance_ref, outcome.manifest_ref, outcome.materialized_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  // Basic JSON structure validation
  EXPECT_TRUE(json_output.find("{") != std::string::npos);
  EXPECT_TRUE(json_output.find("}") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"schema\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"t81.canonfs-export.v1\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"status\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"ok\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"source_objects\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"target_kind\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"target_ref\"") != std::string::npos);
}

// Test export failure JSON structure with DenyAll policy profile (RFC-00D1)
TEST_F(CanonFSCLIContractTests, ExportPolicyFailureJSONStructure) {
  auto test_file = test_dir_ / "test_export_deny.txt";
  create_test_file(test_file);

  ImportOptions import_options;
  import_options.canonfs_root = canonfs_root_.string();
  import_options.policy_profile = InterchangePolicyProfile::Permissive;
  import_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };
  auto import_outcome = import_path(test_file, import_options);
  ASSERT_TRUE(import_outcome.ok());
  ASSERT_FALSE(import_outcome.imported_objects.empty());

  auto export_dir = test_dir_ / "export_deny";
  std::filesystem::create_directories(export_dir);

  ExportOptions export_options;
  export_options.canonfs_root = canonfs_root_.string();
  export_options.policy_profile = InterchangePolicyProfile::DenyAll;
  export_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome =
      export_ref(import_outcome.imported_objects[0], export_dir / "denied.bin", export_options);
  EXPECT_FALSE(outcome.ok());
  EXPECT_FALSE(outcome.errors.empty());
  EXPECT_FALSE(std::filesystem::exists(export_dir / "denied.bin"));

  std::string json_output = render_export_result(
      outcome.status, outcome.source_objects, outcome.target_kind, outcome.target_ref,
      outcome.provenance_ref, outcome.manifest_ref, outcome.materialized_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  EXPECT_TRUE(json_output.find("\"t81.canonfs-export.v1\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"error\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"errors\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"deny-all\"") != std::string::npos);
}

// Test export failure JSON structure with invalid/malformed object ref (RFC-00D1)
TEST_F(CanonFSCLIContractTests, ExportInvalidRefJSONStructure) {
  auto export_dir = test_dir_ / "export_invalid_ref";
  std::filesystem::create_directories(export_dir);

  ExportOptions export_options;
  export_options.canonfs_root = canonfs_root_.string();
  export_options.policy_profile = InterchangePolicyProfile::Permissive;
  export_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome =
      export_ref("sha3-256:not-a-valid-canonfs-hash", export_dir / "invalid.bin", export_options);
  EXPECT_FALSE(outcome.ok());
  EXPECT_FALSE(outcome.errors.empty());
  EXPECT_FALSE(std::filesystem::exists(export_dir / "invalid.bin"));

  std::string json_output = render_export_result(
      outcome.status, outcome.source_objects, outcome.target_kind, outcome.target_ref,
      outcome.provenance_ref, outcome.manifest_ref, outcome.materialized_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  EXPECT_TRUE(json_output.find("\"t81.canonfs-export.v1\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"error\"") != std::string::npos);
  EXPECT_TRUE(json_output.find("\"errors\"") != std::string::npos);
}

// Test import denial under ExportOnly policy profile (RFC-00D1)
TEST_F(CanonFSCLIContractTests, ImportDeniedByExportOnlyProfileJSONStructure) {
  auto test_file = test_dir_ / "export_only_import.txt";
  create_test_file(test_file);

  ImportOptions options;
  options.canonfs_root = canonfs_root_.string();
  options.policy_profile = InterchangePolicyProfile::ExportOnly;
  options.policy_evaluator = [](std::string_view, std::string_view,
                                std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome = import_path(test_file, options);
  EXPECT_FALSE(outcome.ok());
  EXPECT_FALSE(outcome.errors.empty());
  EXPECT_TRUE(outcome.imported_objects.empty());

  std::string json_output = render_import_result(
      outcome.status, outcome.source_kind, outcome.source_ref, outcome.imported_objects,
      outcome.provenance_ref, outcome.manifest_ref, outcome.imported_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  validate_import_result_schema(json_output);
  validate_error_object_structure(json_output);
  EXPECT_TRUE(json_output.find("\"export-only\"") != std::string::npos);
}

// Test export denial under ImportOnly policy profile (RFC-00D1)
TEST_F(CanonFSCLIContractTests, ExportDeniedByImportOnlyProfileJSONStructure) {
  auto test_file = test_dir_ / "import_only_export.txt";
  create_test_file(test_file);

  ImportOptions import_options;
  import_options.canonfs_root = canonfs_root_.string();
  import_options.policy_profile = InterchangePolicyProfile::ImportOnly;
  import_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };
  auto import_outcome = import_path(test_file, import_options);
  ASSERT_TRUE(import_outcome.ok());
  ASSERT_FALSE(import_outcome.imported_objects.empty());

  auto export_dir = test_dir_ / "export_import_only";
  std::filesystem::create_directories(export_dir);

  ExportOptions export_options;
  export_options.canonfs_root = canonfs_root_.string();
  export_options.policy_profile = InterchangePolicyProfile::ImportOnly;
  export_options.policy_evaluator = [](std::string_view, std::string_view,
                                       std::string_view) -> InterchangePolicyDecision {
    return InterchangePolicyDecision{true, "allow"};
  };

  auto outcome =
      export_ref(import_outcome.imported_objects[0], export_dir / "blocked.bin", export_options);
  EXPECT_FALSE(outcome.ok());
  EXPECT_FALSE(outcome.errors.empty());
  EXPECT_FALSE(std::filesystem::exists(export_dir / "blocked.bin"));

  std::string json_output = render_export_result(
      outcome.status, outcome.source_objects, outcome.target_kind, outcome.target_ref,
      outcome.provenance_ref, outcome.manifest_ref, outcome.materialized_paths, outcome.warnings,
      outcome.errors, outcome.policy_result, outcome.policy_profile);

  validate_export_result_schema(json_output);
  validate_error_object_structure(json_output);
  EXPECT_TRUE(json_output.find("\"import-only\"") != std::string::npos);
}

// Test policy profile parser and preflight error JSON contracts (RFC-00D1)
TEST_F(CanonFSCLIContractTests, ParseInterchangePolicyProfileAndPreflightErrorContracts) {
  EXPECT_EQ(parse_interchange_policy_profile("permissive"), InterchangePolicyProfile::Permissive);
  EXPECT_EQ(parse_interchange_policy_profile("import-only"), InterchangePolicyProfile::ImportOnly);
  EXPECT_EQ(parse_interchange_policy_profile("export-only"), InterchangePolicyProfile::ExportOnly);
  EXPECT_EQ(parse_interchange_policy_profile("deny-all"), InterchangePolicyProfile::DenyAll);
  EXPECT_EQ(parse_interchange_policy_profile("unknown-profile"), std::nullopt);

  auto import_err = make_import_preflight_error("host-file", "bad.txt", "invalid_policy_profile",
                                                "Unknown policy profile", "unknown-profile");
  EXPECT_FALSE(import_err.ok());
  ASSERT_FALSE(import_err.errors.empty());
  std::string import_json = render_import_result(
      import_err.status, import_err.source_kind, import_err.source_ref, import_err.imported_objects,
      import_err.provenance_ref, import_err.manifest_ref, import_err.imported_paths,
      import_err.warnings, import_err.errors, import_err.policy_result, import_err.policy_profile);
  validate_import_result_schema(import_json);
  validate_error_object_structure(import_json);

  auto export_err =
      make_export_preflight_error("sha3-256:bad", "host-file", "out.bin", "invalid_policy_profile",
                                  "Unknown policy profile", "unknown-profile");
  EXPECT_FALSE(export_err.ok());
  ASSERT_FALSE(export_err.errors.empty());
  std::string export_json = render_export_result(
      export_err.status, export_err.source_objects, export_err.target_kind, export_err.target_ref,
      export_err.provenance_ref, export_err.manifest_ref, export_err.materialized_paths,
      export_err.warnings, export_err.errors, export_err.policy_result, export_err.policy_profile);
  validate_export_result_schema(export_json);
  validate_error_object_structure(export_json);
}
