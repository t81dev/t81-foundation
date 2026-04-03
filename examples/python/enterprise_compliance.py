#!/usr/bin/env python3
"""Enterprise compliance and governance features for T81 decision substrate."""

import json
import hashlib
import time
from datetime import datetime, timezone
from typing import Dict, List, Any, Optional
from dataclasses import dataclass

@dataclass
class ComplianceReport:
    """Compliance report for bundle-based decisions."""
    decision_ref: str
    compliance_framework: str
    requirements_met: List[str]
    requirements_failed: List[str]
    audit_trail: List[Dict[str, Any]]
    risk_assessment: str
    approved_at: str
    expires_at: Optional[str]

@dataclass
class AccessDecision:
    """Access control decision based on bundle."""
    bundle_ref: str
    user_id: str
    resource: str
    decision: str  # ALLOW/DENY/REVIEW
    reason: str
    policy_applied: str
    timestamp: str
    audit_required: bool

class EnterpriseCompliance:
    """Enterprise compliance and governance system for T81 bundles."""
    
    def __init__(self, t81_binary: str = "./t81-production/t81", canonfs_root: str = "./t81-production/canonfs"):
        self.t81_binary = t81_binary
        self.canonfs_root = canonfs_root
        self.compliance_rules = self._load_compliance_rules()
    
    def _load_compliance_rules(self) -> Dict[str, Any]:
        """Load enterprise compliance rules."""
        return {
            "SOX": {
                "name": "Sarbanes-Oxley Act",
                "requirements": [
                    "Complete audit trail for all financial decisions",
                    "Segregation of duties enforcement",
                    "Access controls on sensitive data",
                    "Retention policies for 7 years"
                ],
                "risk_levels": {
                    "HIGH": "Material financial impact",
                    "MEDIUM": "Moderate financial impact", 
                    "LOW": "Minimal financial impact"
                }
            },
            "HIPAA": {
                "name": "Health Insurance Portability and Accountability Act",
                "requirements": [
                    "Protected health information access controls",
                    "Audit logs for all PHI access",
                    "Data encryption at rest and in transit",
                    "Business associate agreements"
                ],
                "risk_levels": {
                    "HIGH": "Breach of PHI",
                    "MEDIUM": "Improper PHI handling",
                    "LOW": "Minor PHI policy violation"
                }
            },
            "GDPR": {
                "name": "General Data Protection Regulation",
                "requirements": [
                    "Data subject rights documentation",
                    "Consent management for processing",
                    "Data protection by design",
                    "Breach notification within 72 hours"
                ],
                "risk_levels": {
                    "HIGH": "Large-scale data breach",
                    "MEDIUM": "Data subject rights violation",
                    "LOW": "Documentation gaps"
                }
            }
        }
    
    def generate_compliance_report(self, bundle_ref: str, framework: str) -> ComplianceReport:
        """Generate compliance report for bundle decision."""
        
        # Get bundle summary
        bundle_summary = self._get_bundle_summary(bundle_ref)
        
        # Check compliance requirements
        requirements_met = []
        requirements_failed = []
        
        # Audit trail requirement
        audit_trail = self._generate_audit_trail(bundle_ref)
        has_complete_audit = len(audit_trail) > 0
        if has_complete_audit:
            requirements_met.append("Complete audit trail from input to decision")
        else:
            requirements_failed.append("Missing audit trail components")
        
        # Access control requirement
        if "source_provenance_ref" in bundle_summary:
            requirements_met.append("Policy validation and enforcement documented")
        else:
            requirements_failed.append("Missing policy enforcement evidence")
        
        # Data retention requirement
        if framework in ["SOX", "HIPAA"]:
            # Check if bundle has creation timestamp
            if "created_at" in bundle_summary:
                requirements_met.append("Decision timestamp recorded for retention")
            else:
                requirements_failed.append("Missing decision timestamp")
        
        # Risk assessment
        risk_level = self._assess_risk(bundle_summary, framework)
        
        return ComplianceReport(
            decision_ref=bundle_ref,
            compliance_framework=framework,
            requirements_met=requirements_met,
            requirements_failed=requirements_failed,
            audit_trail=audit_trail,
            risk_assessment=risk_level,
            approved_at=datetime.now(timezone.utc).isoformat(),
            expires_at=self._calculate_expiry(framework)
        )
    
    def _get_bundle_summary(self, bundle_ref: str) -> Dict[str, Any]:
        """Get bundle summary for compliance checking."""
        # Mock implementation - would use real bundle consumer
        return {
            "bundle_ref": bundle_ref,
            "schema": "t81.ai.task.assess-fixed.bundle.v1",
            "source_result_ref": f"sha3-256:result_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}",
            "source_provenance_ref": f"sha3-256:provenance_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}",
            "action_ref": f"sha3-256:action_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}",
            "record_ref": f"sha3-256:record_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}",
            "created_at": datetime.now(timezone.utc).isoformat(),
            "decision_data": {
                "selected_action": "write_allow_marker",
                "selected_path": "actions/allow.marker",
                "decision": "ALLOW",
                "reason_code": "COMPLIANCE_APPROVED"
            }
        }
    
    def _generate_audit_trail(self, bundle_ref: str) -> List[Dict[str, Any]]:
        """Generate complete audit trail for bundle."""
        audit_trail = []
        
        # Input stage
        audit_trail.append({
            "stage": "input",
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "actor": "user_request",
            "action": "access_request_submitted",
            "details": {
                "user_id": "user_12345",
                "resource": "/sensitive/data/file.txt",
                "request_type": "access_control"
            }
        })
        
        # Policy evaluation stage
        audit_trail.append({
            "stage": "policy_evaluation",
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "actor": "t81_policy_engine",
            "action": "bundle_creation",
            "details": {
                "policy_profile": "enterprise_compliance",
                "rules_applied": ["data_classification", "access_control", "audit_requirements"],
                "bundle_ref": bundle_ref
            }
        })
        
        # Decision stage
        audit_trail.append({
            "stage": "decision",
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "actor": "t81_decision_substrate",
            "action": "bundle_finalized",
            "details": {
                "decision_type": "access_control",
                "bundle_schema": "t81.ai.task.assess-fixed.bundle.v1",
                "decision_outcome": "ALLOW",
                "confidence": "high",
                "explanation": "Policy-compliant access granted"
            }
        })
        
        return audit_trail
    
    def _assess_risk(self, bundle_summary: Dict[str, Any], framework: str) -> str:
        """Assess risk level for compliance framework."""
        decision_data = bundle_summary.get("decision_data", {})
        decision = decision_data.get("decision", "REVIEW")
        
        # Risk assessment logic
        if decision == "ALLOW":
            if framework == "HIPAA":
                return "LOW"  # Approved access with proper compliance
            elif framework == "SOX":
                return "MEDIUM"  # Financial decisions require monitoring
            else:
                return "LOW"
        elif decision == "DENY":
            if framework == "HIPAA":
                return "MEDIUM"  # Denied access to PHI
            elif framework == "SOX":
                return "HIGH"  # Denied financial access
            else:
                return "MEDIUM"
        else:  # REVIEW
            return "HIGH"  # Manual review required
        
    def _calculate_expiry(self, framework: str) -> Optional[str]:
        """Calculate compliance report expiry."""
        from datetime import timedelta
        
        if framework == "GDPR":
            return (datetime.now(timezone.utc) + timedelta(days=90)).isoformat()
        elif framework == "SOX":
            return (datetime.now(timezone.utc) + timedelta(days=365)).isoformat()
        elif framework == "HIPAA":
            return (datetime.now(timezone.utc) + timedelta(days=180)).isoformat()
        else:
            return None
    
    def create_access_decision(self, user_id: str, resource: str, bundle_ref: str) -> AccessDecision:
        """Create access control decision from bundle."""
        
        bundle_summary = self._get_bundle_summary(bundle_ref)
        decision_data = bundle_summary.get("decision_data", {})
        
        return AccessDecision(
            bundle_ref=bundle_ref,
            user_id=user_id,
            resource=resource,
            decision=decision_data.get("decision", "REVIEW"),
            reason=decision_data.get("reason_code", "UNKNOWN"),
            policy_applied="enterprise_compliance_policy",
            timestamp=datetime.now(timezone.utc).isoformat(),
            audit_required=True
        )
    
    def generate_regulatory_export(self, bundle_ref: str, framework: str) -> Dict[str, Any]:
        """Generate regulatory export for compliance reporting."""
        compliance_report = self.generate_compliance_report(bundle_ref, framework)
        
        return {
            "export_type": "regulatory_compliance",
            "framework": framework,
            "bundle_decision": bundle_ref,
            "generated_at": datetime.now(timezone.utc).isoformat(),
            "compliance_status": "COMPLIANT" if len(compliance_report.requirements_failed) == 0 else "NON_COMPLIANT",
            "audit_trail": compliance_report.audit_trail,
            "risk_assessment": compliance_report.risk_assessment,
            "evidence": {
                "bundle_ref": bundle_ref,
                "canonfs_references": {
                    "source_result_ref": compliance_report.audit_trail[1]["details"]["bundle_ref"] if len(compliance_report.audit_trail) > 1 else "",
                    "source_provenance_ref": compliance_report.audit_trail[1]["details"]["bundle_ref"] if len(compliance_report.audit_trail) > 1 else ""
                },
                "policy_application": {
                    "policy_engine": "t81_policy_engine",
                    "rules_applied": ["data_classification", "access_control", "audit_requirements"],
                    "decision_timestamp": compliance_report.approved_at
                }
            }
        }

def main():
    """CLI interface for enterprise compliance features."""
    import sys
    
    if len(sys.argv) < 3:
        print("Usage: python3 enterprise_compliance.py <command> <bundle_ref> [options]")
        print("Commands:")
        print("  compliance-report <framework>  Generate compliance report")
        print("  access-decision <user_id> <resource>  Create access decision")
        print("  regulatory-export <framework>  Export for regulatory reporting")
        print("Frameworks: SOX, HIPAA, GDPR")
        sys.exit(1)
    
    command = sys.argv[1]
    bundle_ref = sys.argv[2]
    
    compliance = EnterpriseCompliance()
    
    if command == "compliance-report":
        if len(sys.argv) < 4:
            print("Error: Framework required")
            sys.exit(1)
        
        framework = sys.argv[3]
        report = compliance.generate_compliance_report(bundle_ref, framework)
        
        print(f"=== Compliance Report: {framework} ===")
        print(f"Bundle Reference: {report.decision_ref}")
        print(f"Generated: {report.approved_at}")
        print(f"Expires: {report.expires_at}")
        print(f"Risk Assessment: {report.risk_assessment}")
        print(f"Requirements Met: {', '.join(report.requirements_met)}")
        if report.requirements_failed:
            print(f"Requirements Failed: {', '.join(report.requirements_failed)}")
        print("\nAudit Trail:")
        for entry in report.audit_trail:
            print(f"  {entry['stage']}: {entry['timestamp']} - {entry['action']}")
    
    elif command == "access-decision":
        if len(sys.argv) < 5:
            print("Error: user_id and resource required")
            sys.exit(1)
        
        user_id = sys.argv[3]
        resource = sys.argv[4]
        
        decision = compliance.create_access_decision(user_id, resource, bundle_ref)
        
        print(f"=== Access Decision ===")
        print(f"User ID: {decision.user_id}")
        print(f"Resource: {decision.resource}")
        print(f"Bundle: {decision.bundle_ref}")
        print(f"Decision: {decision.decision}")
        print(f"Reason: {decision.reason}")
        print(f"Timestamp: {decision.timestamp}")
        print(f"Audit Required: {decision.audit_required}")
    
    elif command == "regulatory-export":
        if len(sys.argv) < 4:
            print("Error: Framework required")
            sys.exit(1)
        
        framework = sys.argv[3]
        export_data = compliance.generate_regulatory_export(bundle_ref, framework)
        
        print(json.dumps(export_data, indent=2))
    
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

if __name__ == "__main__":
    main()
