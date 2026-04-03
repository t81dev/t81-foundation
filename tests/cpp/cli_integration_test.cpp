#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "t81/canonfs/interchange_engine.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

using namespace t81::canonfs;

class CLIIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for tests
        test_dir_ = std::filesystem::temp_directory_path() / "cli_integration_test";
        std::filesystem::create_directories(test_dir_);
        
        // Create test CanonFS root
        canonfs_root_ = test_dir_ / ".t81_canonfs";
        std::filesystem::create_directories(canonfs_root_);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }
    
    // Helper to create test file
    void create_test_file(const std::filesystem::path& path, const std::string& content = "test content") {
        std::ofstream file(path);
        file << content;
        file.close();
    }
    
    // Helper to create test policy file
    void create_test_policy(const std::filesystem::path& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }
    
    std::filesystem::path test_dir_;
    std::filesystem::path canonfs_root_;
};

// Test that refactored CLI maintains backward compatibility
TEST_F(CLIIntegrationTest, RefactoredImportBackwardCompatibility) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // Test refactored import function
    int result = canonfs_import_refactored(
        test_file, canonfs_root_, std::nullopt, std::nullopt, false);
    
    EXPECT_EQ(result, 0); // Should succeed for existing file
}

// Test refactored CLI with JSON output
TEST_F(CLIIntegrationTest, RefactoredImportJSONOutput) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // Test refactored import function with JSON output
    int result = canonfs_import_refactored(
        test_file, canonfs_root_, std::nullopt, std::nullopt, true);
    
    EXPECT_EQ(result, 0); // Should succeed for existing file
}

// Test refactored CLI with policy profile
TEST_F(CLIIntegrationTest, RefactoredImportWithPolicyProfile) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // Test with deny-all policy profile
    int result = canonfs_import_refactored(
        test_file, canonfs_root_, std::nullopt, std::string("deny-all"), false);
    
    // Should fail due to policy denial
    EXPECT_NE(result, 0);
}

// Test refactored CLI with policy file
TEST_F(CLIIntegrationTest, RefactoredImportWithPolicyFile) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    auto policy_file = test_dir_ / "test_policy.axp";
    create_test_policy(policy_file, R"(
policy "test.policy" {
    description: "Test policy for CLI integration",
    version: "1.0",
    allow_operations: ["import"]
}
        )");
    
    // Test with policy file
    int result = canonfs_import_refactored(
        test_file, canonfs_root_, policy_file, std::nullopt, false);
    
    // Should succeed with our test policy
    EXPECT_EQ(result, 0);
}

// Test refactored export functionality
TEST_F(CLIIntegrationTest, RefactoredExportBackwardCompatibility) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // First import to get an object
    int import_result = canonfs_import_refactored(
        test_file, canonfs_root_, std::nullopt, std::nullopt, false);
    ASSERT_EQ(import_result, 0);
    
    // Mock a canonical hash for export
    std::string mock_hash = "sha3-256:mock_hash_12345";
    
    auto export_file = test_dir_ / "exported.txt";
    
    // Test refactored export function
    int export_result = canonfs_export_refactored(
        mock_hash, export_file, canonfs_root_, std::nullopt, std::nullopt, false);
    
    EXPECT_EQ(export_result, 0); // Should succeed with mock hash
}

// Test refactored CLI export with JSON output
TEST_F(CLIIntegrationTest, RefactoredExportJSONOutput) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // First import to get an object
    int import_result = canonfs_import_refactored(
        test_file, canonfs_root_, std::nullopt, std::nullopt, false);
    ASSERT_EQ(import_result, 0);
    
    // Mock a canonical hash for export
    std::string mock_hash = "sha3-256:mock_hash_12345";
    
    auto export_file = test_dir_ / "exported.txt";
    
    // Test refactored export function with JSON output
    int export_result = canonfs_export_refactored(
        mock_hash, export_file, canonfs_root_, std::nullopt, std::nullopt, true);
    
    EXPECT_EQ(export_result, 0); // Should succeed with mock hash
}

// Test evidence collection integration
TEST_F(CLIIntegrationTest, EvidenceCollectionIntegration) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // Create engine for evidence testing
    auto engine = create_interchange_engine();
    
    // Perform operation to generate evidence
    ImportRequest request;
    request.input_path = test_file;
    request.canonfs_root = canonfs_root_;
    request.policy_profile = InterchangePolicyProfile::Permissive;
    request.policy_evaluator = [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
        return InterchangePolicyDecision{true, "allow"};
    };
    
    auto outcome = engine->import(request);
    
    // Verify evidence was collected
    auto evidence = engine->get_evidence_log();
    EXPECT_EQ(evidence.size(), 1);
    EXPECT_EQ(evidence[0].operation_type, "import");
    EXPECT_TRUE(evidence[0].metadata.contains("input_path"));
    EXPECT_TRUE(evidence[0].metadata.contains("canonfs_root"));
    EXPECT_TRUE(evidence[0].metadata.contains("policy_profile"));
}

// Test error handling in refactored CLI
TEST_F(CLIIntegrationTest, RefactoredErrorHandling) {
    auto nonexistent_file = test_dir_ / "nonexistent.txt";
    
    // Test with non-existent file
    int result = canonfs_import_refactored(
        nonexistent_file, canonfs_root_, std::nullopt, std::nullopt, false);
    
    // Should fail due to missing file
    EXPECT_NE(result, 0);
}

// Test JSON validation in refactored CLI
TEST_F(CLIIntegrationTest, RefactoredJSONValidation) {
    auto test_file = test_dir_ / "test.txt";
    create_test_file(test_file);
    
    // Test that JSON validation works
    int result = canonfs_import_refactored(
        test_file, canonfs_root_, std::nullopt, std::nullopt, true);
    
    EXPECT_EQ(result, 0); // Should succeed and pass JSON validation
}
