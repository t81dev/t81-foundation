#include "t81/canonfs/canonfs_interchange_ops.hpp"
#include <format>
#include <chrono>

namespace t81::canonfs::interchange {

// Error code to string mapping
std::string error_code_to_string(ErrorCode code) {
  switch (code) {
    case ErrorCode::PolicyDenied: return "policy_denied";
    case ErrorCode::PolicyNotConfigured: return "policy_not_configured";
    case ErrorCode::PolicyEvaluationFailed: return "policy_evaluation_failed";
    
    case ErrorCode::HashMismatch: return "hash_mismatch";
    case ErrorCode::ContentCorrupted: return "content_corrupted";
    case ErrorCode::ChecksumValidationFailed: return "checksum_validation_failed";
    
    case ErrorCode::FileNotFound: return "file_not_found";
    case ErrorCode::AccessDenied: return "access_denied";
    case ErrorCode::DiskFull: return "disk_full";
    case ErrorCode::InvalidPath: return "invalid_path";
    case ErrorCode::SymlinkNotSupported: return "symlink_not_supported";
    
    case ErrorCode::SchemaValidationFailed: return "schema_validation_failed";
    case ErrorCode::InvalidJsonFormat: return "invalid_json_format";
    case ErrorCode::MissingRequiredField: return "missing_required_field";
    
    case ErrorCode::ObjectNotFound: return "object_not_found";
    case ErrorCode::StorageWriteFailed: return "storage_write_failed";
    case ErrorCode::DriverError: return "driver_error";
    case ErrorCode::DecodeError: return "decode_error";
    
    default: return "unknown_error";
  }
}

// Enhanced issue creation with structured error codes
Issue make_enhanced_issue(ErrorCode code, const std::string& context, const std::string& details) {
  Issue issue;
  issue.reason = error_code_to_string(code);
  
  // Format: [code] reason: context - details
  issue.message = std::format("[{}] {}: {} - {}", 
    static_cast<int>(code), issue.reason, context);
  
  if (!details.empty()) {
    issue.message += " - " + details;
  }
  
  return issue;
}

// Specific error creators for common scenarios
Issue make_policy_denied_issue(const std::string& object_path, const std::string& policy_reason) {
  return make_enhanced_issue(ErrorCode::PolicyDenied, object_path, 
    "Policy denied: " + policy_reason);
}

Issue make_hash_mismatch_issue(const std::string& expected_hash, const std::string& actual_hash) {
  return make_enhanced_issue(ErrorCode::HashMismatch, "content verification", 
    std::format("Expected {}, got {}", expected_hash, actual_hash));
}

Issue make_file_not_found_issue(const std::string& path) {
  return make_enhanced_issue(ErrorCode::FileNotFound, path, "File does not exist");
}

Issue make_schema_validation_issue(const std::string& field_name, const std::string& validation_error) {
  return make_enhanced_issue(ErrorCode::SchemaValidationFailed, field_name, validation_error);
}

Issue make_storage_write_issue(const std::string& operation, const std::string& driver_error) {
  return make_enhanced_issue(ErrorCode::StorageWriteFailed, operation, driver_error);
}

} // namespace t81::canonfs::interchange
