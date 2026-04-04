---
name: t81_bundle_validator
description: Validate and verify T81 decision bundles for deterministic execution
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Bundle Validator Skill

This skill validates T81 decision bundles, ensuring they meet deterministic execution requirements and policy compliance.

## Usage

When the user needs to verify the integrity and validity of T81 decision bundles, use this skill to perform comprehensive validation.

### Commands

**Validate a single bundle:**
```
validate <bundle_file> [--strict] [--policy <policy_file>] [--output json|summary]
```

**Verify bundle provenance:**
```
verify-provenance <bundle_file> [--canonfs-root <path>]
```

**Check bundle determinism:**
```
check-determinism <bundle_file> [--replay-count <count>]
```

**Batch validation:**
```
validate-batch <bundle_dir> [--recursive] [--strict]
```

### Examples

```bash
# Basic bundle validation
validate decision_bundle.v1.json

# Strict validation with custom policy
validate critical_bundle.v1.json --strict --policy security.apl

# Verify complete provenance chain
verify-provenance bundle.v1.json --canonfs-root /secure/canonfs

# Test determinism with multiple replays
check-determinism bundle.v1.json --replay-count 5

# Validate all bundles in directory
validate-batch ./bundles/ --recursive --strict
```

### Validation Checks

The skill performs these validations:

**Structural Validation:**
- Bundle schema compliance
- Required fields presence
- Data type correctness
- Version compatibility

**Integrity Validation:**
- Cryptographic hash verification
- Content-addressed storage consistency
- Signature verification (if present)

**Policy Validation:**
- Axion policy compliance
- Execution permission checks
- Resource usage validation

**Determinism Validation:**
- Replay consistency testing
- Input/output determinism
- Side-effect verification

### Output Format

**Success Output:**
```json
{
  "status": "valid",
  "bundle_id": "CanonHash81...",
  "validation_timestamp": "2026-04-04T15:30:00Z",
  "checks": {
    "structural": "passed",
    "integrity": "passed", 
    "policy": "passed",
    "determinism": "passed"
  },
  "provenance": {
    "created": "2026-04-04T14:15:00Z",
    "creator": "t81_vm_v1.9.0",
    "policy_hash": "CanonHash81..."
  }
}
```

**Error Output:**
```json
{
  "status": "invalid",
  "error_type": "policy_violation",
  "error_message": "Bundle violates security policy: unauthorized resource access",
  "bundle_id": "CanonHash81...",
  "validation_timestamp": "2026-04-04T15:30:00Z",
  "failed_checks": ["policy"]
}
```

### Error Handling

- Provides specific error types and messages for each validation failure
- Suggests remediation steps for common issues
- Maintains detailed audit logs for compliance

### Security Notes

- All validation respects Axion policy constraints
- Bundle content is never modified during validation
- Validation results are cryptographically verifiable

### Integration

This skill integrates with T81's bundle consumption contract and validation APIs.
