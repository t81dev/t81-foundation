#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "t81/canonfs/canonfs_interchange_ops.hpp"

using namespace t81::canonfs::interchange;

class CanonFSEnhancedErrorsSimpleTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary directory for tests
    test_dir_ = std::filesystem::temp_directory_path();
    std::filesystem::create_directories(test_dir_);
  }
  
  void TearDown() override {
    // Cleanup test directory
    std::filesystem::remove_all(test_dir_);
  }
  
  std::filesystem::path test_dir_;
};

TEST_F(CanonFSEnhancedErrorsSimpleTest, ErrorCodeToStringMapping) {
  // Test all error codes map to correct strings
  EXPECT_EQ(error_code_to_string(ErrorCode::PolicyDenied), "policy_denied");
  EXPECT_EQ(error_code_to_string(ErrorCode::HashMismatch), "hash_mismatch");
  EXPECT_EQ(error_code_to_string(ErrorCode::FileNotFound), "file_not_found");
  EXPECT_EQ(error_code_to_string(ErrorCode::SchemaValidationFailed), "schema_validation_failed");
  EXPECT_EQ(error_code_to_string(ErrorCode::StorageWriteFailed), "storage_write_failed");
  
  // Test unknown error code
  EXPECT_EQ(error_code_to_string(static_cast<ErrorCode>(9999)), "unknown_error");
}

TEST_F(CanonFSEnhancedErrorsSimpleTest, EnhancedIssueCreation) {
  // Test enhanced issue creation
  auto issue = make_enhanced_issue(ErrorCode::PolicyDenied, "/test/file", "Test policy denied");
  
  EXPECT_EQ(issue.reason, "policy_denied");
  EXPECT_TRUE(issue.message.find("[1001]") != std::string::npos);
  EXPECT_TRUE(issue.message.find("policy_denied") != std::string::npos);
  EXPECT_TRUE(issue.message.find("/test/file") != std::string::npos);
  EXPECT_TRUE(issue.message.find("Test policy denied") != std::string::npos);
}

TEST_F(CanonFSEnhancedErrorsSimpleTest, SpecificErrorCreators) {
  // Test specific error creators
  auto policy_issue = make_policy_denied_issue("/test/file.txt", "Access denied by security policy");
  EXPECT_EQ(policy_issue.reason, "policy_denied");
  EXPECT_TRUE(policy_issue.message.find("Access denied by security policy") != std::string::npos);
  
  auto hash_issue = make_hash_mismatch_issue("sha3-256:abc123", "sha3-256:def456");
  EXPECT_EQ(hash_issue.reason, "hash_mismatch");
  EXPECT_TRUE(hash_issue.message.find("Expected abc123, got def456") != std::string::npos);
  
  auto file_issue = make_file_not_found_issue("/nonexistent/path.txt");
  EXPECT_EQ(file_issue.reason, "file_not_found");
  EXPECT_TRUE(file_issue.message.find("/nonexistent/path.txt") != std::string::npos);
  
  auto schema_issue = make_schema_validation_issue("bundle_schema", "Missing required field: version");
  EXPECT_EQ(schema_issue.reason, "schema_validation_failed");
  EXPECT_TRUE(schema_issue.message.find("bundle_schema") != std::string::npos);
  EXPECT_TRUE(schema_issue.message.find("Missing required field: version") != std::string::npos);
  
  auto storage_issue = make_storage_write_issue("write_object", "Disk full");
  EXPECT_EQ(storage_issue.reason, "storage_write_failed");
  EXPECT_TRUE(storage_issue.message.find("write_object") != std::string::npos);
  EXPECT_TRUE(storage_issue.message.find("Disk full") != std::string::npos);
}

TEST_F(CanonFSEnhancedErrorsSimpleTest, ErrorContextCreation) {
  // Test error context creation
  auto context = create_error_context("canonfs.import", "/test/object", "permissive");
  
  EXPECT_EQ(context.operation, "canonfs.import");
  EXPECT_EQ(context.object_path, "/test/object");
  EXPECT_EQ(context.policy_profile, "permissive");
  EXPECT_TRUE((std::chrono::system_clock::now() - context.timestamp).count() < 1000); // Within 1 second
}
