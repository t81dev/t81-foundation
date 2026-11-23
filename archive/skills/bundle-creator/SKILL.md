---
name: t81_bundle_creator
description: Create T81 decision bundles with complete provenance and cryptographic verification
metadata:
  openclaw:
    os: ["darwin", "linux"]
    requires:
      bins: ["t81"]
---

# T81 Bundle Creator Skill

This skill creates T81 decision bundles that package AI decisions with complete provenance, cryptographic verification, and immutable audit trails.

## Usage

When the user needs to create decision bundles from T81 execution results, use this skill to package decisions with complete provenance.

### Commands

**Create decision bundle:**
```
create-bundle <execution_result> [--output <bundle_file>] [--policy <policy_file>] [--include-provenance]
```

**Bundle from execution:**
```
bundle-from-execution <program_file> [--input <input_data>] [--policy <policy_file>] [--bundle-output <bundle_file>]
```

**Bundle information:**
```
bundle-info <bundle_file> [--detailed] [--verify-integrity] [--output <info_file>]
```

**Sign bundle:**
```
sign-bundle <bundle_file> [--key <key_file>] [--certificate <cert_file>]
```

**Verify bundle:**
```
verify-bundle <bundle_file> [--check-signature] [--check-provenance] [--check-integrity]
```

**Merge bundles:**
```
merge-bundles <bundle1> <bundle2> [--output <merged_bundle>] [--conflict-resolution keep|merge|fail]
```

**Extract from bundle:**
```
extract-bundle <bundle_file> [--component result|provenance|metadata] [--output <output_file>]
```

### Examples

```bash
# Create bundle from execution result
create-bundle execution_result.json --output decision.bundle --include-provenance

# Bundle directly from program execution
bundle-from-execution inference.t81 --input data.json --policy security.apl --bundle-output decisions.bundle

# Get detailed bundle information
bundle-info decision.bundle --detailed --verify-integrity --output bundle_info.json

# Sign bundle with cryptographic key
sign-bundle decision.bundle --key private_key.pem --certificate cert.pem

# Verify bundle integrity and signature
verify-bundle decision.bundle --check-signature --check-provenance --check-integrity

# Merge multiple bundles
merge-bundles bundle1.bundle bundle2.bundle --output merged.bundle --conflict-resolution merge

# Extract specific component from bundle
extract-bundle decision.bundle --component provenance --output provenance.json
```

### Bundle Structure

**Core Components:**
- **Decision Result**: Primary AI decision or computation result
- **Provenance Chain**: Complete execution trace and audit trail
- **Policy Evidence**: All policy decisions and justifications
- **Metadata**: Timestamps, versions, environment information
- **Cryptographic Hashes**: Content verification and integrity

**Optional Components:**
- **Input Data**: Original inputs (if permitted)
- **Execution Logs**: Detailed step-by-step execution
- **Performance Metrics**: Timing and resource usage
- **Signatures**: Cryptographic signatures for verification

### Bundle Format

**Bundle Directory Structure:**
```
decision_bundle.v1/
├── bundle.json          # Main bundle manifest
├── result.json          # Decision result
├── provenance.json      # Complete provenance
├── policy_evidence.json # Policy decisions
├── metadata.json        # Bundle metadata
├── hashes.json          # Component hashes
├── signature.sig        # Cryptographic signature (optional)
└── components/          # Additional components
    ├── input_data/
    ├── execution_logs/
    └── performance/
```

**Bundle Manifest (bundle.json):**
```json
{
  "bundle_version": "v1",
  "bundle_id": "CanonHash81_BUNDLE...",
  "created_at": "2026-04-04T15:30:00Z",
  "creator": "t81_v1.9.0",
  "components": {
    "result": "result.json",
    "provenance": "provenance.json",
    "policy_evidence": "policy_evidence.json",
    "metadata": "metadata.json",
    "hashes": "hashes.json"
  },
  "integrity_hash": "CanonHash81_INTEGRITY...",
  "policy_compliance": "verified",
  "determinism_guaranteed": true
}
```

### Output Format

**Bundle Creation Result:**
```json
{
  "status": "success",
  "bundle_file": "decision.bundle",
  "bundle_id": "CanonHash81_BUNDLE...",
  "timestamp": "2026-04-04T15:30:00Z",
  "components": {
    "result": "included",
    "provenance": "included",
    "policy_evidence": "included",
    "metadata": "included"
  },
  "verification": {
    "integrity_hash": "CanonHash81_INTEGRITY...",
    "policy_compliance": "verified",
    "determinism_guaranteed": true,
    "cryptographic_secure": true
  },
  "size": {
    "total_bytes": 1048576,
    "compressed": false,
    "components_count": 5
  }
}
```

**Bundle Verification Result:**
```json
{
  "bundle_file": "decision.bundle",
  "verification_status": "passed",
  "timestamp": "2026-04-04T15:30:00Z",
  "checks": {
    "integrity": "passed",
    "signature": "passed",
    "provenance": "passed",
    "policy_compliance": "passed",
    "determinism": "verified"
  },
  "details": {
    "bundle_id": "CanonHash81_BUNDLE...",
    "creator": "t81_v1.9.0",
    "created_at": "2026-04-04T15:30:00Z",
    "components_verified": 5,
    "hashes_valid": true
  }
}
```

### Cryptographic Security

**Content Hashing:**
- SHA-256 for individual components
- Merkle tree for bundle integrity
- CanonHash81 for T81-specific verification

**Digital Signatures:**
- RSA/ECDSA signature support
- Certificate-based verification
- Key management integration

**Integrity Protection:**
- Tamper-evidence detection
- Content verification
- Secure storage recommendations

### Policy Integration

**Policy Evidence:**
- All policy decisions recorded
- Rule-based explanations
- Compliance documentation

**Access Control:**
- Bundle creation permissions
- Component access restrictions
- Sensitive data handling

**Audit Requirements:**
- Regulatory compliance support
- Complete audit trails
- Immutable record keeping

### Bundle Operations

**Creation Options:**
- Include/exclude specific components
- Compression settings
- Encryption options
- Signature requirements

**Verification Features:**
- Multi-level integrity checking
- Signature validation
- Provenance verification
- Policy compliance checking

**Merge Capabilities:**
- Conflict resolution strategies
- Component combination rules
- Metadata consolidation
- Integrity preservation

### Error Handling

- **Creation Errors**: Component validation and guidance
- **Verification Errors**: Specific failure explanations
- **Signature Errors**: Cryptographic problem resolution
- **Merge Errors**: Conflict resolution assistance

### Use Cases

**Regulatory Compliance:**
- Financial decision documentation
- Healthcare audit trails
- Government record keeping

**Supply Chain Integrity:**
- AI decision provenance
- Model version tracking
- Result verification

**Research Reproducibility:**
- Experiment documentation
- Result verification
- Method reproducibility

### Integration

This skill wraps T81's bundle creation and management:
```bash
t81 bundle create <result> --output <bundle> --policy <policy> --include-provenance
```
