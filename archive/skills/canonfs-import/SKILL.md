---
name: t81_canonfs_import
description: Import files into T81 CanonFS for immutable, hash-verified storage
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 CanonFS Import Skill

This skill imports files into T81's CanonFS (Canonical File System) for immutable, hash-verified storage with complete provenance.

## Usage

When the user wants to store a file immutably with cryptographic verification, use this skill to import it into CanonFS.

### Commands

**Import a single file:**
```
import <file_path> [--canonfs-root <path>] [--policy <policy_file>] [--json]
```

**Import multiple files:**
```
import-batch <file1> <file2> <file3> [--canonfs-root <path>] [--policy <policy_file>]
```

**List imported objects:**
```
list [--canonfs-root <path>] [--format json|table]
```

### Examples

```bash
# Import a model file with default settings
import model.t81w

# Import with custom CanonFS root and policy
import sensitive_data.bin --canonfs-root /secure/canonfs --policy secure.apl

# Import multiple files
import-batch model.t81w config.json weights.bin

# List all objects in JSON format
list --canonfs-root ~/.t81_canonfs --format json
```

### Output Format

The skill returns:
- **CanonHash81**: Content-addressed hash for the imported object
- **Status**: Import success/failure with detailed error messages
- **Metadata**: File size, import timestamp, policy validation result

### Error Handling

- Returns clear error messages for missing files, policy violations, or storage issues
- Provides suggestions for resolving common import problems
- Maintains deterministic error reporting for automation

### Security Notes

- All imports are subject to Axion policy validation before storage
- Files are stored with content-addressed hashing preventing tampering
- Complete provenance is maintained for audit and compliance

### Integration

This skill wraps the T81 CLI:
```bash
t81 canonfs import <file> --canonfs-root <path> --policy <policy> --json
```
