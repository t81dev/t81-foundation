---
name: t81_canonfs_export
description: Export files from T81 CanonFS with hash verification and integrity validation
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 CanonFS Export Skill

This skill exports files from T81's CanonFS (Canonical File System) with cryptographic hash verification and complete integrity validation.

## Usage

When the user needs to retrieve files from CanonFS with guaranteed integrity and verification, use this skill to export immutable objects.

### Commands

**Export a single object:**
```
export <canon_hash> [--output <output_file>] [--canonfs-root <path>] [--verify] [--json]
```

**Export multiple objects:**
```
export-batch <hash1> <hash2> <hash3> [--output-dir <dir>] [--canonfs-root <path>]
```

**Verify export integrity:**
```
verify-export <canon_hash> [--exported-file <file>] [--canonfs-root <path>]
```

**List available objects:**
```
list [--canonfs-root <path>] [--format json|table] [--filter <pattern>]
```

**Search objects:**
```
search <query> [--canonfs-root <path>] [--field name|hash|type] [--max-results <count>]
```

**Export with metadata:**
```
export-with-meta <canon_hash> [--output <file>] [--include-provenance] [--canonfs-root <path>]
```

### Examples

```bash
# Export object by CanonHash81
export CanonHash81_ABC123... --output restored_model.t81w

# Export with verification
export CanonHash81_DEF456... --output data.bin --verify --canonfs-root /secure/canonfs

# Export multiple objects to directory
export-batch hash1 hash2 hash3 --output-dir ./exports/

# Verify exported file integrity
verify-export CanonHash81_ABC123... --exported-file restored_model.t81w

# List all objects in JSON format
list --canonfs-root ~/.t81_canonfs --format json

# Search for objects by name pattern
search "model.*\.t81w" --field name --max-results 10

# Export with complete provenance
export-with-meta CanonHash81_GHI789... --include-provenance --output bundle_with_meta.zip
```

### Export Verification

**Hash Verification:**
- Cryptographic hash validation against CanonHash81
- Content integrity verification
- Tamper-evidence confirmation

**Provenance Validation:**
- Import timestamp verification
- Source authentication
- Policy compliance checking

**Integrity Checks:**
- File size consistency
- Content hash matching
- Metadata completeness

### Output Format

**Successful Export:**
```json
{
  "status": "success",
  "canon_hash": "CanonHash81_ABC123...",
  "output_file": "restored_model.t81w",
  "timestamp": "2026-04-04T15:30:00Z",
  "verification": {
    "hash_verified": true,
    "integrity_check": "passed",
    "provenance_valid": true
  },
  "metadata": {
    "original_name": "model.t81w",
    "import_time": "2026-04-04T14:15:00Z",
    "file_size": 1048576,
    "content_type": "t81_weights",
    "policy_hash": "CanonHash789..."
  },
  "provenance": {
    "importer": "t81_v1.9.0",
    "source_location": "/models/original.t81w",
    "policy_applied": "secure_import.apl",
    "import_bundle": "CanonHash81_IMPORT..."
  }
}
```

**Verification Failure:**
```json
{
  "status": "verification_failed",
  "canon_hash": "CanonHash81_DEF456...",
  "error": {
    "type": "hash_mismatch",
    "expected_hash": "CanonHash81_DEF456...",
    "computed_hash": "CanonHash81_MISMATCH...",
    "integrity_status": "corrupted"
  },
  "suggestions": [
    "Verify CanonFS storage integrity",
    "Check for storage corruption",
    "Re-import original file if available"
  ]
}
```

### Search and Filtering

**Search Fields:**
- `name` - Original filename pattern matching
- `hash` - CanonHash81 pattern matching
- `type` - Content type filtering
- `size` - File size ranges
- `date` - Import date ranges

**Filter Examples:**
```bash
# Search by name pattern
search ".*\.t81w$" --field name

# Search by hash prefix
search "CanonHash81_ABC.*" --field hash

# Filter by content type
search "t81_weights" --field type

# Search by date range
search "2026-04-.*" --field date
```

### Batch Operations

**Batch Export Features:**
- Parallel export processing
- Progress tracking
- Error collection and reporting
- Integrity verification for all exports

**Batch Export Result:**
```json
{
  "batch_id": "CanonHash81_BATCH...",
  "status": "completed",
  "total_objects": 10,
  "successful_exports": 9,
  "failed_exports": 1,
  "output_directory": "./exports/",
  "timestamp": "2026-04-04T15:30:00Z",
  "results": [
    {
      "hash": "CanonHash81_ABC123...",
      "status": "success",
      "file": "model1.t81w"
    },
    {
      "hash": "CanonHash81_DEF456...",
      "status": "failed",
      "error": "object_not_found"
    }
  ]
}
```

### Security Features

**Access Control:**
- Policy-based export permissions
- User authorization verification
- Resource access logging

**Integrity Protection:**
- Cryptographic hash verification
- Tamper-evidence detection
- Provenance chain validation

**Audit Trail:**
- Complete export logging
- Access attempt recording
- Policy decision documentation

### Error Handling

- **Object Not Found**: Clear hash validation and location checking
- **Permission Denied**: Policy violation explanations
- **Storage Corruption**: Integrity failure reporting
- **Network Issues**: Retry mechanisms and timeout handling

### Performance Considerations

**Large File Handling:**
- Streaming export for large objects
- Memory-efficient processing
- Progress reporting for long operations

**Concurrent Access:**
- Thread-safe export operations
- Lock management for shared resources
- Queue management for batch operations

### Integration

This skill wraps T81's CanonFS export functionality:
```bash
t81 canonfs export <hash> --out <file> --canonfs-root <path> --verify --json
```
