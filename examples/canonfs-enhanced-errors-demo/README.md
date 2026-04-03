# CanonFS Enhanced Errors Demo

This directory demonstrates the enhanced CanonFS error handling framework and CLI contract validation.

## Overview

This example shows how CanonFS interchange operations provide structured error responses with:
- Consistent JSON schemas (`t81.canonfs-import.v1`, `t81.canonfs-export.v1`)
- Structured error objects with `kind`, `message`, `code`, `reason` fields
- Policy profile integration with proper error classification
- RFC-00D1 compliant error handling

## Files

- `import-policy-denied.request.json` - Import request that triggers policy denial
- `import-policy-denied.output.json` - Expected error response
- `import-file-not-found.request.json` - Import request for non-existent file
- `import-file-not-found.output.json` - Expected error response
- `import-symlink-not-supported.request.json` - Import request with symlink
- `import-symlink-not-supported.output.json` - Expected error response
- `export-policy-denied.request.json` - Export request that triggers policy denial
- `export-policy-denied.output.json` - Expected error response
- `test-data.txt` - Test file for import operations

## Usage Examples

### 1. Policy Denied Import

```bash
# Request (will fail with policy denial)
t81 canonfs import test-data.txt --policy-profile deny-all --json < import-policy-denied.request.json

# Expected response structure in import-policy-denied.output.json
```

### 2. File Not Found Import

```bash
# Request (will fail with missing source error)
t81 canonfs import nonexistent.txt --json < import-file-not-found.request.json

# Expected response structure in import-file-not-found.output.json
```

### 3. Symlink Not Supported

```bash
# Create symlink and try to import (will fail)
ln -s test-data.txt symlink.txt
t81 canonfs import symlink.txt --json < import-symlink-not-supported.request.json

# Expected response structure in import-symlink-not-supported.output.json
```

### 4. Export Policy Denied

```bash
# First import a file, then try to export with deny-all policy
t81 canonfs import test-data.txt --policy-profile permissive
t81 canonfs export <object-ref> --policy-profile deny-all --json < export-policy-denied.request.json

# Expected response structure in export-policy-denied.output.json
```

## Error Classification

The enhanced error framework provides these error classifications:

### Import Errors
- `canonfs-policy-denied` - Policy profile denied the operation
- `canonfs-import-missing-source` - Source file/directory not found
- `canonfs-import-symlink-not-supported` - Symlinks are not supported
- `canonfs-import-storage-write-failed` - Failed to write to CanonFS storage

### Export Errors  
- `canonfs-policy-denied` - Policy profile denied the operation
- `canonfs-export-missing-object` - Referenced object not found in CanonFS
- `canonfs-export-unsafe-target-path` - Target path is unsafe
- `canonfs-export-target-write-failed` - Failed to write to target location

## JSON Schema Compliance

All error responses follow RFC-00D1 v1 contract:

```json
{
  "schema": "t81.canonfs-import.v1",
  "status": "error",
  "errors": [
    {
      "kind": "policy-failure",
      "message": "policy denied import of test-data.txt: policy-profile deny-all denies canonfs.import before CanonFS or host-side materialization",
      "code": "canonfs-policy-denied",
      "reason": "policy_denied"
    }
  ],
  "policy_result": "policy denied: policy-profile deny-all denies canonfs.import before CanonFS or host-side materialization",
  "policy_profile": "deny-all"
}
```

## Integration with CLI

The enhanced error handling integrates seamlessly with the T81 CLI:

- `t81 canonfs import --json` - Returns structured JSON with error classification
- `t81 canonfs export --json` - Returns structured JSON with error classification  
- Error objects include machine-readable `code` field for programmatic handling
- Human-readable `message` field provides detailed explanation
- `reason` field enables error categorization and routing

## Testing

Run the demo scripts to verify error handling:

```bash
# Test all error scenarios
./test-all-errors.sh
```

This demonstrates the complete CanonFS error handling pipeline from CLI request to structured JSON response.
