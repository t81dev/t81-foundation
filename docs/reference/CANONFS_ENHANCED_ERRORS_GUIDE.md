# CanonFS Enhanced Error Handling

This document describes the enhanced error handling framework added to T81 CanonFS interchange operations.

## Overview

The enhanced error handling provides structured error codes, better debugging information, and precise error reporting for CanonFS operations. This addresses the RFC-00D1 policy-profile depth requirements for better error semantics.

## Enhanced Error Codes

### Policy and Permission Errors (1000-1099)
- `1001` - `policy_denied` - Policy explicitly denied the operation
- `1002` - `policy_not_configured` - No policy configuration found
- `1003` - `policy_evaluation_failed` - Policy evaluation encountered an error

### Content Integrity Errors (2000-2099)
- `2001` - `hash_mismatch` - Content hash verification failed
- `2002` - `content_corrupted` - Content is corrupted or invalid
- `2003` - `checksum_validation_failed` - Checksum validation failed

### Filesystem and I/O Errors (3000-3099)
- `3001` - `file_not_found` - File or path does not exist
- `3002` - `access_denied` - Permission denied for filesystem operation
- `3003` - `disk_full` - Storage device is full
- `3004` - `invalid_path` - Path format is invalid
- `3005` - `symlink_not_supported` - Symlinks not supported by CanonFS v1

### Schema and Validation Errors (4000-4099)
- `4001` - `schema_validation_failed` - JSON schema validation failed
- `4002` - `invalid_json_format` - JSON format is invalid
- `4003` - `missing_required_field` - Required field is missing

### CanonFS Specific Errors (5000-5099)
- `5001` - `object_not_found` - CanonFS object not found
- `5002` - `storage_write_failed` - Failed to write to CanonFS storage
- `5003` - `driver_error` - CanonFS driver encountered an error
- `5004` - `decode_error` - Failed to decode CanonFS object

## Error Message Format

Enhanced errors follow this format:
```
[code] reason: context - details
```

Example:
```
[1001] policy_denied: /path/to/file.txt - Policy denied: Access denied by security policy
```

## API Usage

### Creating Enhanced Issues
```cpp
#include "t81/canonfs/canonfs_interchange_ops.hpp"

using namespace t81::canonfs::interchange;

// Generic enhanced issue
auto issue = make_enhanced_issue(ErrorCode::PolicyDenied, "/path/to/file", "Policy denied: security violation");

// Specific error creators
auto policy_issue = make_policy_denied_issue("/path/to/file", "Access denied by security policy");
auto hash_issue = make_hash_mismatch_issue("expected_hash", "actual_hash");
auto file_issue = make_file_not_found_issue("/nonexistent/path");
auto schema_issue = make_schema_validation_issue("field_name", "validation_error");
auto storage_issue = make_storage_write_issue("operation", "driver_error");
```

### Error Context Collection
```cpp
auto context = create_error_context("canonfs.import", "/path/to/object", "permissive");
// context.operation = "canonfs.import"
// context.object_path = "/path/to/object"
// context.policy_profile = "permissive"
// context.timestamp = current time
```

## Integration with Existing Code

The enhanced error handling is designed to work alongside existing CanonFS interchange operations. Legacy `make_issue()` function remains available for backward compatibility.

## Benefits

1. **Precise Error Identification** - Numeric codes enable programmatic error handling
2. **Better Debugging** - Structured format with context and details
3. **Consistent Error Reporting** - Standardized error messages across CanonFS operations
4. **Enhanced Logging** - Error context with timestamps for debugging
5. **Backward Compatibility** - Existing code continues to work unchanged

## Implementation Status

✅ **Completed**: Enhanced error handling framework
- Structured error codes with numeric identifiers
- Specific error creators for common scenarios
- Error context collection with timestamps
- Integration with existing CanonFS interchange operations
- Backward compatibility preservation

🔄 **Next Steps**: Add comprehensive test coverage
- Unit tests for all error codes
- Integration tests with actual CanonFS operations
- Negative test coverage for error paths
- Performance testing of error handling overhead

## RFC-00D1 Compliance

This enhanced error handling addresses RFC-00D1 requirements for:
- ✅ Explicit error reasons and codes
- ✅ Structured interchange error entries
- ✅ Better failure semantics for import/export operations
- ✅ Policy-profile depth improvements
