#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "t81/canonfs/interchange.hpp"
#include "t81/canonfs/interchange_ops.hpp"

using namespace t81::canonfs::interchange;

class CanonFSEnhancedErrorsTest : public ::testing::Test {
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

TEST_F(CanonFSEnhancedErrorsTest, LegacyIssueCreation) {
  // Test that legacy issue creation still works
  auto issue = t81::canonfs::interchange::make_issue("test_reason", "test message");
  
  EXPECT_EQ(issue.reason, "test_reason");
  EXPECT_EQ(issue.message, "test message");
}

TEST_F(CanonFSEnhancedErrorsTest, ErrorContextCreation) {
  // Test error context creation with existing API
  auto context = t81::canonfs::create_error_context("canonfs.import", "/test/object", "permissive");
  
  EXPECT_EQ(context.operation, "canonfs.import");
  EXPECT_EQ(context.object_path, "/test/object");
  EXPECT_EQ(context.policy_profile, "permissive");
  EXPECT_TRUE((std::chrono::system_clock::now() - context.timestamp).count() < 1000); // Within 1 second
}

// Test file not found handling with current implementation
TEST_F(CanonFSEnhancedErrorsTest, CurrentFileNotFoundHandling) {
  // Create a test file
  auto test_file = test_dir_ / "test.txt";
  std::ofstream file(test_file);
  file << "test content";
  file.close();
  
  // Test that file not found error is handled properly
  std::vector<std::string> imported_objects;
  std::vector<std::string> imported_paths;
  std::vector<std::string> warnings;
  std::vector<Issue> errors;
  bool saw_policy_denial = false;
  
  // Remove the file and try to import it
  std::filesystem::remove(test_file);
  
  bool result = collect_import_entries(
    test_file, imported_objects, imported_paths, warnings, errors,
    test_dir_.string(), InterchangePolicyProfile::Permissive, 
    [](std::string_view, std::string_view, std::string_view) -> bool {
      return false; // Allow all for this test
    }, saw_policy_denial);
  
  // Verify we got a file not found error
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  // Check for file not found error
  bool found_file_error = false;
  for (const auto& error : errors) {
    if (error.reason == "missing_object" || error.reason == "missing_source") {
      found_file_error = true;
      break;
    }
  }
  
  EXPECT_TRUE(found_file_error);
}
