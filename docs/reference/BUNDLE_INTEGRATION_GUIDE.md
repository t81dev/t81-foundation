# Bundle Integration Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Bundle Integration Guide](#bundle-integration-guide)
  - [1. Security Governance Integration](#1-security-governance-integration)
    - [Scenario: Access Control System](#scenario-access-control-system)
- [Usage](#usage)
  - [2. Compliance Auditing Integration](#2-compliance-auditing-integration)
    - [Scenario: Archive Decision Objects](#scenario-archive-decision-objects)
- [Usage](#usage)
- [Later verification](#later-verification)
  - [3. Cross-System Handoff Integration](#3-cross-system-handoff-integration)
    - [Scenario: Transfer Decisions Between Environments](#scenario-transfer-decisions-between-environments)
- [Usage](#usage)
- [Transfer production decision to staging](#transfer-production-decision-to-staging)
  - [4. Release Gate Integration](#4-release-gate-integration)
    - [Scenario: Use Bundle as Deployment Approval](#scenario-use-bundle-as-deployment-approval)
- [Usage](#usage)
  - [Integration Best Practices](#integration-best-practices)
    - [1. Always Verify Bundle Schema](#1-always-verify-bundle-schema)
    - [2. Handle Bundle Consumption Errors Gracefully](#2-handle-bundle-consumption-errors-gracefully)
    - [3. Cache Bundle Data When Appropriate](#3-cache-bundle-data-when-appropriate)
    - [4. Log All Bundle Operations](#4-log-all-bundle-operations)
  - [Testing Your Integration](#testing-your-integration)
- [Run composition tests to get real bundle refs](#run-composition-tests-to-get-real-bundle-refs)
- [Use generated bundle refs in your integration code](#use-generated-bundle-refs-in-your-integration-code)

<!-- T81-TOC:END -->


This guide shows step-by-step how to integrate T81 decision bundles into common patterns from the integration matrix.

## 1. Security Governance Integration

### Scenario: Access Control System
Consume assess-fixed bundles to make allow/deny decisions.

```python
from bundle_consumer import BundleConsumer

class AccessController:
    def __init__(self):
        self.consumer = BundleConsumer()
    
    def evaluate_request(self, bundle_ref: str, request_context: dict) -> bool:
        """Consume bundle to make security decision."""
        try:
            summary = self.consumer.summarize_bundle(bundle_ref)
            
            # Extract decision
            selected_action = summary.get("selected_action", "")
            
            # Map to security decision
            if selected_action == "write_allow_marker":
                decision = True
                reason = "Policy-approved access granted"
            elif selected_action == "write_deny_marker":
                decision = False  
                reason = "Policy-approved access denied"
            elif selected_action == "write_review_marker":
                decision = False
                reason = "Manual review required"
            else:
                decision = False
                reason = f"Unknown action: {selected_action}"
            
            # Log decision with full provenance
            self.log_decision({
                "bundle_ref": bundle_ref,
                "decision": decision,
                "reason": reason,
                "provenance": {
                    "source_result_ref": summary["source_result_ref"],
                    "source_provenance_ref": summary["source_provenance_ref"],
                    "record_ref": summary["record_ref"],
                    "action_ref": summary["action_ref"]
                },
                "context": request_context
            })
            
            return decision
            
        except Exception as e:
            # Fail secure on bundle consumption errors
            self.log_error(f"Bundle consumption failed: {e}")
            return False
    
    def log_decision(self, decision_data: dict):
        """Log decision with audit trail."""
        # Implementation depends on your logging system
        print(f"[AUDIT] Decision: {decision_data}")

# Usage
controller = AccessController()
allowed = controller.evaluate_request(
    "sha3-256:access_decision_bundle",
    {"user": "alice", "resource": "/sensitive/data", "action": "read"}
)
```

## 2. Compliance Auditing Integration

### Scenario: Archive Decision Objects
Store bundles as evidence for compliance review.

```python
import json
from datetime import datetime
from bundle_consumer import BundleConsumer

class ComplianceAuditor:
    def __init__(self, archive_path: str):
        self.consumer = BundleConsumer()
        self.archive_path = Path(archive_path)
        self.archive_path.mkdir(exist_ok=True)
    
    def archive_decision(self, bundle_ref: str, compliance_context: dict) -> str:
        """Archive bundle with compliance metadata."""
        # Get full bundle data
        summary = self.consumer.summarize_bundle(bundle_ref)
        
        # Create compliance record
        compliance_record = {
            "archived_at": datetime.utcnow().isoformat(),
            "bundle_ref": bundle_ref,
            "bundle_schema": summary["bundle_schema"],
            "decision_family": summary["family"],
            "compliance_context": compliance_context,
            "decision_data": {
                "selected_action": summary.get("selected_action"),
                "selected_path": summary.get("selected_path"),
                "selected_rule_set": summary.get("selected_rule_set")
            },
            "provenance_chain": {
                "source_result_ref": summary["source_result_ref"],
                "source_provenance_ref": summary["source_provenance_ref"],
                "record_ref": summary["record_ref"],
                "action_ref": summary["action_ref"]
            },
            "verification_status": "verified"
        }
        
        # Archive with content-addressed filename
        archive_file = self.archive_path / f"{bundle_ref.replace(':', '_')}.json"
        with open(archive_file, 'w') as f:
            json.dump(compliance_record, f, indent=2)
        
        return str(archive_file)
    
    def verify_archive_integrity(self, bundle_ref: str) -> bool:
        """Verify archived bundle hasn't been tampered."""
        archive_file = self.archive_path / f"{bundle_ref.replace(':', '_')}.json"
        
        if not archive_file.exists():
            return False
        
        with open(archive_file, 'r') as f:
            record = json.load(f)
        
        # Verify bundle still resolves correctly
        try:
            summary = self.consumer.summarize_bundle(bundle_ref)
            return record["bundle_ref"] == bundle_ref
        except Exception:
            return False

# Usage
auditor = ComplianceAuditor("./compliance_archive")
archive_path = auditor.archive_decision(
    "sha3-256:compliance_decision_bundle",
    {"regulation": "SOX", "department": "finance", "risk_level": "high"}
)

# Later verification
is_valid = auditor.verify_archive_integrity("sha3-256:compliance_decision_bundle")
print(f"Archive integrity: {is_valid}")
```

## 3. Cross-System Handoff Integration

### Scenario: Transfer Decisions Between Environments
Move bundles from production to staging for analysis.

```python
import requests
from bundle_consumer import BundleConsumer

class BundleTransporter:
    def __init__(self, source_consumer: BundleConsumer, target_api_base: str):
        self.source_consumer = source_consumer
        self.target_api_base = target_api_base
    
    def transfer_bundle(self, bundle_ref: str, target_env: str) -> str:
        """Transfer bundle to target environment."""
        try:
            # Get bundle from source
            summary = self.source_consumer.summarize_bundle(bundle_ref)
            
            # Prepare transfer payload
            payload = {
                "source_bundle_ref": bundle_ref,
                "target_environment": target_env,
                "bundle_data": {
                    "schema": summary["bundle_schema"],
                    "family": summary["family"],
                    "decision_fields": {
                        k: v for k, v in summary.items() 
                        if k in ["selected_action", "selected_path", "selected_rule_set", "rule_set_ref"]
                    }
                },
                "provenance": {
                    "source_result_ref": summary["source_result_ref"],
                    "source_provenance_ref": summary["source_provenance_ref"],
                    "record_ref": summary["record_ref"],
                    "action_ref": summary["action_ref"]
                }
            }
            
            # Transfer to target
            response = requests.post(
                f"{self.target_api_base}/bundles/import",
                json=payload,
                headers={"Content-Type": "application/json"}
            )
            
            if response.status_code == 200:
                result = response.json()
                new_bundle_ref = result.get("bundle_ref")
                
                # Verify transfer integrity
                if self.verify_transfer_integrity(bundle_ref, new_bundle_ref):
                    print(f"Bundle transferred successfully: {bundle_ref} -> {new_bundle_ref}")
                    return new_bundle_ref
                else:
                    raise Exception("Transfer integrity verification failed")
            else:
                raise Exception(f"Transfer failed: {response.text}")
                
        except Exception as e:
            print(f"Bundle transfer error: {e}")
            raise
    
    def verify_transfer_integrity(self, source_ref: str, target_ref: str) -> bool:
        """Verify bundle integrity after transfer."""
        # This would use target system's bundle consumer
        # For now, simulate verification
        return True

# Usage
source_consumer = BundleConsumer()
transporter = BundleTransporter(
    source_consumer, 
    "https://staging-system.example.com/api"
)

# Transfer production decision to staging
staging_ref = transporter.transfer_bundle(
    "sha3-256:production_decision_bundle",
    "staging"
)
```

## 4. Release Gate Integration

### Scenario: Use Bundle as Deployment Approval
Require valid bundle before allowing deployment.

```python
import hashlib
from bundle_consumer import BundleConsumer

class ReleaseGate:
    def __init__(self):
        self.consumer = BundleConsumer()
        self.required_bundle_families = ["assess-fixed", "classify-fixed"]
    
    def validate_deployment(self, bundle_ref: str, deployment_context: dict) -> dict:
        """Validate deployment against bundle decision."""
        try:
            summary = self.consumer.summarize_bundle(bundle_ref)
            
            # Check bundle family
            if summary["family"] not in self.required_bundle_families:
                return {
                    "allowed": False,
                    "reason": f"Bundle family {summary['family']} not approved for deployment",
                    "bundle_ref": bundle_ref
                }
            
            # Extract decision
            if summary["family"] == "assess-fixed":
                selected_action = summary.get("selected_action", "")
                if selected_action != "write_allow_marker":
                    return {
                        "allowed": False,
                        "reason": f"Deployment not allowed: action {selected_action}",
                        "bundle_ref": bundle_ref
                    }
            
            elif summary["family"] == "classify-fixed":
                selected_rule_set = summary.get("selected_rule_set", "")
                if not selected_rule_set.startswith("approved-"):
                    return {
                        "allowed": False,
                        "reason": f"Rule set {selected_rule_set} not approved for production",
                        "bundle_ref": bundle_ref
                    }
            
            # If we get here, deployment is approved
            approval_token = hashlib.sha256(
                f"{bundle_ref}:{deployment_context['deployment_id']}".encode()
            ).hexdigest()
            
            return {
                "allowed": True,
                "approval_token": approval_token,
                "bundle_ref": bundle_ref,
                "deployment_context": deployment_context
            }
            
        except Exception as e:
            return {
                "allowed": False,
                "reason": f"Bundle validation failed: {e}",
                "bundle_ref": bundle_ref
            }

# Usage
gate = ReleaseGate()
deployment_result = gate.validate_deployment(
    "sha3-256:deployment_approval_bundle",
    {
        "deployment_id": "deploy-2024-001",
        "environment": "production",
        "service": "api-gateway"
    }
)

if deployment_result["allowed"]:
    print(f"Deployment approved: {deployment_result['approval_token']}")
else:
    print(f"Deployment rejected: {deployment_result['reason']}")
```

## Integration Best Practices

### 1. Always Verify Bundle Schema
```python
def validate_bundle_schema(bundle_ref: str, expected_schemas: list) -> bool:
    consumer = BundleConsumer()
    try:
        summary = consumer.summarize_bundle(bundle_ref)
        return summary["bundle_schema"] in expected_schemas
    except Exception:
        return False
```

### 2. Handle Bundle Consumption Errors Gracefully
```python
def safe_bundle_consumption(bundle_ref: str, fallback_action: callable):
    consumer = BundleConsumer()
    try:
        summary = consumer.summarize_bundle(bundle_ref)
        return summary
    except Exception as e:
        print(f"Bundle consumption failed: {e}")
        return fallback_action()
```

### 3. Cache Bundle Data When Appropriate
```python
from functools import lru_cache

class CachedBundleConsumer(BundleConsumer):
    @lru_cache(maxsize=128)
    def summarize_bundle(self, bundle_ref: str):
        return super().summarize_bundle(bundle_ref)
```

### 4. Log All Bundle Operations
```python
import logging

class LoggedBundleConsumer(BundleConsumer):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.logger = logging.getLogger(__name__)
    
    def summarize_bundle(self, bundle_ref: str):
        self.logger.info(f"Consuming bundle: {bundle_ref}")
        try:
            result = super().summarize_bundle(bundle_ref)
            self.logger.info(f"Bundle consumed successfully: {bundle_ref}")
            return result
        except Exception as e:
            self.logger.error(f"Bundle consumption failed: {bundle_ref}, error: {e}")
            raise
```

## Testing Your Integration

Use the test bundles from composition tests:

```bash
# Run composition tests to get real bundle refs
./build/t81_ai_task_assess_fixed_composition_test ./build/t81

# Use generated bundle refs in your integration code
python3 your_integration.py --bundle-ref <generated_bundle_ref>
```

These patterns provide concrete starting points for integrating T81 decision bundles into real systems while maintaining security, compliance, and reliability guarantees.
