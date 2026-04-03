#pragma once

#include "t81/canonfs/interchange.hpp"

namespace t81::canonfs::interchange {

// Enhanced error codes for better debugging and handling
enum class ErrorCode {
  // Policy and permission errors
  PolicyDenied = 1001,
  PolicyNotConfigured = 1002,
  PolicyEvaluationFailed = 1003,
  
  // Content integrity errors  
  HashMismatch = 2001,
  ContentCorrupted = 2002,
  ChecksumValidationFailed = 2003,
  
  // Filesystem and I/O errors
  FileNotFound = 3001,
  AccessDenied = 3002,
  DiskFull = 3003,
  InvalidPath = 3004,
  SymlinkNotSupported = 3005,
  
  // Schema and validation errors
  SchemaValidationFailed = 4001,
  InvalidJsonFormat = 4002,
  MissingRequiredField = 4003,
  
  // CanonFS specific errors
  ObjectNotFound = 5001,
  StorageWriteFailed = 5002,
  DriverError = 5003,
  DecodeError = 5004
};

// Error code to string mapping
std::string error_code_to_string(ErrorCode code);

// Enhanced issue creation with structured error codes
Issue make_enhanced_issue(ErrorCode code, const std::string& context, const std::string& details = "");

// Specific error creators for common scenarios
Issue make_policy_denied_issue(const std::string& object_path, const std::string& policy_reason);
Issue make_hash_mismatch_issue(const std::string& expected_hash, const std::string& actual_hash);
Issue make_file_not_found_issue(const std::string& path);
Issue make_schema_validation_issue(const std::string& field_name, const std::string& validation_error);
Issue make_storage_write_issue(const std::string& operation, const std::string& driver_error);

// Enhanced error context collection
struct ErrorContext {
  std::string operation;
  std::string object_path;
  std::string policy_profile;
  std::vector<std::string> validation_failures;
  std::chrono::system_clock::time_point timestamp;
};

ErrorContext create_error_context(std::string_view operation, 
                              const std::string& object_path = "",
                              std::string_view policy_profile = "");

} // namespace t81::canonfs::interchange
