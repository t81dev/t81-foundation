# Bundle Transport Examples

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Bundle Transport Examples](#bundle-transport-examples)
  - [HTTP Transport](#http-transport)
    - [Simple Bundle Transfer](#simple-bundle-transfer)
- [Usage](#usage)
  - [File System Transport](#file-system-transport)
    - [Bundle Export/Import](#bundle-exportimport)
- [Export bundle from source system](#export-bundle-from-source-system)
- [Transfer bundle.json to target system (scp, rsync, etc.)](#transfer-bundlejson-to-target-system-scp-rsync-etc)
- [Import into target system](#import-into-target-system)
  - [Verification After Transport](#verification-after-transport)
    - [Always Verify Bundle Integrity](#always-verify-bundle-integrity)
- [Usage](#usage)
  - [Cross-System Considerations](#cross-system-considerations)
    - [What Transfers](#what-transfers)
    - [What Doesn't Transfer](#what-doesn't-transfer)
    - [Security Notes](#security-notes)

<!-- T81-TOC:END -->


This document shows how to move T81 decision bundles between systems while maintaining verifiability.

## HTTP Transport

### Simple Bundle Transfer
```python
import requests
import json

def transfer_bundle_http(bundle_ref: str, target_url: str):
    # Get bundle from source system
    consumer = BundleConsumer()
    bundle = consumer.get_bundle(bundle_ref)
    
    # Transfer to target system
    response = requests.post(f"{target_url}/bundles", json=bundle)
    
    if response.status_code == 200:
        return response.json()["bundle_ref"]  # New ref in target system
    else:
        raise Exception(f"Transfer failed: {response.text}")

# Usage
new_ref = transfer_bundle_http(
    "sha3-256:source_bundle_ref",
    "https://target-system.example.com/api"
)
```

## File System Transport

### Bundle Export/Import
```bash
# Export bundle from source system
t81 canonfs get <bundle_ref> --canonfs-root /source/.t81_canonfs --out bundle.json --json

# Transfer bundle.json to target system (scp, rsync, etc.)
scp bundle.json target-system:/tmp/

# Import into target system
t81 canonfs put-file /tmp/bundle.json --canonfs-root /target/.t81_canonfs
```

## Verification After Transport

### Always Verify Bundle Integrity
```python
def verify_transferred_bundle(bundle_ref: str, expected_schema: str):
    consumer = BundleConsumer()
    
    # Verify bundle schema
    bundle = consumer.get_bundle(bundle_ref)
    if bundle["schema"] != expected_schema:
        raise Exception(f"Schema mismatch: expected {expected_schema}, got {bundle['schema']}")
    
    # Verify all references resolve
    for field in ["record_ref", "action_ref", "source_result_ref", "source_provenance_ref"]:
        ref = bundle[field]
        try:
            consumer.read_field(bundle_ref, bundle["schema"], field)
        except Exception as e:
            raise Exception(f"Reference {field} invalid: {e}")
    
    print(f"Bundle {bundle_ref} verified successfully")

# Usage
verify_transferred_bundle(new_ref, "t81.ai.task.assess-fixed.bundle.v1")
```

## Cross-System Considerations

### What Transfers
- Bundle content (JSON with 4 reference fields)
- All referenced artifacts (if using CanonFS sync)
- Decision integrity (cryptographic hashes)

### What Doesn't Transfer
- Original CanonFS storage location
- Local execution environment
- Temporary files from creation

### Security Notes
- Bundle references are content-addressed - tampering is detectable
- Always verify bundle schema after transport
- Use secure channels for bundle transfer
- Consider bundle access permissions in target system
