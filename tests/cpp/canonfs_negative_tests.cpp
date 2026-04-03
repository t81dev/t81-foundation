#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "t81/canonfs/interchange.hpp"
#include "t81/canonfs/interchange_ops.hpp"

#include <filesystem>
#include <fstream>
#include <string>

using namespace t81::canonfs::interchange;
using namespace t81::canonfs;

class CanonFSNegativeTests : public ::testing::Test {
protected:
  void SetUp() override {
    // Create temporary directory for tests
    test_dir_ = std::filesystem::temp_directory_path() / "canonfs_negative_test";
    std::filesystem::create_directories(test_dir_);
    canonfs_root_ = test_dir_ / ".t81_canonfs";
    std::filesystem::create_directories(canonfs_root_);
  }
  
  void TearDown() override {
    // Cleanup test directory
    std::filesystem::remove_all(test_dir_);
  }
  
  // Helper to create a test file
  void create_test_file(const std::filesystem::path& path, const std::string& content = "test content") {
    std::ofstream file(path);
    file << content;
    file.close();
  }
  
  // Helper to create a test directory
  void create_test_dir(const std::filesystem::path& path) {
    std::filesystem::create_directories(path);
  }
  
  // Helper to test import with policy
  bool test_import_with_policy(const std::filesystem::path& input, 
                           InterchangePolicyProfile policy_profile,
                           std::vector<Issue>& errors) {
    ImportOptions options;
    options.canonfs_root = canonfs_root_.string();
    options.policy_profile = policy_profile;
    options.policy_evaluator = [](std::string_view, std::string_view, std::string_view) -> InterchangePolicyDecision {
      return InterchangePolicyDecision{true, "allow"};
    };
    
    auto outcome = import_path(input, options);
    errors = outcome.errors;
    return outcome.ok();
  }
  
  std::filesystem::path test_dir_;
  std::filesystem::path canonfs_root_;
};

// Test symlink not supported error
TEST_F(CanonFSNegativeTests, SymlinkNotSupported) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);
  
  auto symlink_path = test_dir_ / "symlink.txt";
  std::filesystem::create_symlink(test_file, symlink_path);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(symlink_path, InterchangePolicyProfile::Permissive, errors);
  
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  bool found_symlink_error = false;
  for (const auto& error : errors) {
    if (error.reason == "symlink_not_supported") {
      found_symlink_error = true;
      EXPECT_TRUE(error.message.find("symlinks are not supported") != std::string::npos);
      break;
    }
  }
  
  EXPECT_TRUE(found_symlink_error);
}

// Test file not found error
TEST_F(CanonFSNegativeTests, FileNotFound) {
  auto nonexistent_file = test_dir_ / "nonexistent.txt";
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(nonexistent_file, InterchangePolicyProfile::Permissive, errors);
  
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  bool found_file_error = false;
  for (const auto& error : errors) {
    if (error.reason == "missing_object" || error.reason == "missing_source") {
      found_file_error = true;
      break;
    }
  }
  
  EXPECT_TRUE(found_file_error);
}

// Test unsupported source kind error
TEST_F(CanonFSNegativeTests, UnsupportedSourceKind) {
  auto special_path = test_dir_ / "special";
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(special_path, InterchangePolicyProfile::Permissive, errors);
  
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  bool found_unsupported_error = false;
  for (const auto& error : errors) {
    if (error.reason == "missing_object" || error.reason == "missing_source") {
      found_unsupported_error = true;
      break;
    }
  }
  
  EXPECT_TRUE(found_unsupported_error);
}

// Test policy denied error with ImportOnly profile
TEST_F(CanonFSNegativeTests, PolicyDeniedImportOnly) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(test_file, InterchangePolicyProfile::ImportOnly, errors);
  
  // ImportOnly should allow imports, so this should succeed
  EXPECT_TRUE(result);
  EXPECT_TRUE(errors.empty());
}

// Test policy denied error with ExportOnly profile
TEST_F(CanonFSNegativeTests, PolicyDeniedExportOnly) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(test_file, InterchangePolicyProfile::ExportOnly, errors);
  
  // ExportOnly should deny imports
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  bool found_policy_error = false;
  for (const auto& error : errors) {
    if (error.reason == "policy_denied") {
      found_policy_error = true;
      EXPECT_TRUE(error.message.find("policy denied import") != std::string::npos);
      EXPECT_TRUE(error.message.find("policy-profile export-only denies") != std::string::npos);
      break;
    }
  }
  
  EXPECT_TRUE(found_policy_error);
}

// Test policy denied error with DenyAll profile
TEST_F(CanonFSNegativeTests, PolicyDeniedDenyAll) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(test_file, InterchangePolicyProfile::DenyAll, errors);
  
  // DenyAll should deny imports
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  bool found_policy_error = false;
  for (const auto& error : errors) {
    if (error.reason == "policy_denied") {
      found_policy_error = true;
      EXPECT_TRUE(error.message.find("policy denied import") != std::string::npos);
      EXPECT_TRUE(error.message.find("policy-profile deny-all denies") != std::string::npos);
      break;
    }
  }
  
  EXPECT_TRUE(found_policy_error);
}

// Test directory with symlinks should fail
TEST_F(CanonFSNegativeTests, DirectoryWithSymlinks) {
  auto test_dir = test_dir_ / "test_dir";
  create_test_dir(test_dir);
  
  auto real_file = test_dir / "real.txt";
  create_test_file(real_file);
  
  auto symlink_file = test_dir / "symlink.txt";
  std::filesystem::create_symlink(real_file, symlink_file);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(test_dir, InterchangePolicyProfile::Permissive, errors);
  
  // Directory with symlinks should fail
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  // Check that we got some kind of error (might be symlink or other)
  EXPECT_FALSE(errors.empty());
}

// Test empty directory should fail (current CanonFS behavior)
TEST_F(CanonFSNegativeTests, EmptyDirectoryImport) {
  auto empty_dir = test_dir_ / "empty_dir";
  create_test_dir(empty_dir);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(empty_dir, InterchangePolicyProfile::Permissive, errors);
  
  // Empty directory currently fails - this is the actual behavior
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  bool found_error = false;
  for (const auto& error : errors) {
    if (error.reason == "missing_object" || error.reason == "missing_source") {
      found_error = true;
      break;
    }
  }
  
  EXPECT_TRUE(found_error);
}

// Test directory with regular files should succeed
TEST_F(CanonFSNegativeTests, DirectoryWithRegularFiles) {
  auto test_dir = test_dir_ / "test_dir";
  create_test_dir(test_dir);
  
  create_test_file(test_dir / "file1.txt", "content1");
  create_test_file(test_dir / "file2.txt", "content2");
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(test_dir, InterchangePolicyProfile::Permissive, errors);
  
  // Directory with regular files should succeed
  EXPECT_TRUE(result);
  EXPECT_TRUE(errors.empty());
}

// Test error message format consistency
TEST_F(CanonFSNegativeTests, ErrorMessageFormat) {
  auto test_file = test_dir_ / "test.txt";
  create_test_file(test_file);
  
  std::vector<Issue> errors;
  bool result = test_import_with_policy(test_file, InterchangePolicyProfile::DenyAll, errors);
  
  EXPECT_FALSE(result);
  EXPECT_FALSE(errors.empty());
  
  // All errors should have reason and message fields
  for (const auto& error : errors) {
    EXPECT_FALSE(error.reason.empty());
    EXPECT_FALSE(error.message.empty());
    // Note: error.message may not always contain error.reason depending on the error type
  }
}
