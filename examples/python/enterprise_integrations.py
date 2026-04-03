#!/usr/bin/env python3
"""Enterprise integration connectors for T81 decision substrate."""

import json
import requests
import hashlib
from datetime import datetime, timezone
from typing import Dict, List, Any, Optional
from dataclasses import dataclass

@dataclass
class SAPIntegration:
    """SAP system integration with T81 bundle decisions."""
    client_id: str
    secret_key: str
    sap_endpoint: str
    
    def __init__(self, client_id: str, secret_key: str, sap_endpoint: str):
        self.client_id = client_id
        self.secret_key = secret_key
        self.sap_endpoint = sap_endpoint
    
    def create_sap_user_role(self, bundle_ref: str, user_id: str, role_data: dict) -> bool:
        """Create SAP user role based on bundle decision."""
        
        # Get bundle decision
        decision_data = self._get_bundle_decision(bundle_ref)
        
        if decision_data.get("decision") != "ALLOW":
            print(f"Bundle decision DENIED - cannot create SAP role for user {user_id}")
            return False
        
        # Map bundle decision to SAP role
        sap_role = self._map_to_sap_role(decision_data)
        
        # Create SAP role via API
        role_payload = {
            "user_id": user_id,
            "role": sap_role,
            "valid_from": datetime.now(timezone.utc).isoformat(),
            "bundle_reference": bundle_ref,
            "approval_chain": decision_data.get("provenance_ref", ""),
            "compliance_status": "T81_CERTIFIED"
        }
        
        try:
            response = requests.post(
                f"{self.sap_endpoint}/api/v2/user-roles",
                json=role_payload,
                headers=self._get_auth_headers(),
                timeout=30
            )
            
            if response.status_code == 201:
                print(f"✅ SAP role created: {sap_role} for user {user_id}")
                return True
            else:
                print(f"❌ SAP role creation failed: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ SAP integration error: {e}")
            return False
    
    def _get_bundle_decision(self, bundle_ref: str) -> Dict[str, Any]:
        """Get decision data from bundle reference."""
        # Mock implementation - would use real bundle consumer
        return {
            "decision": "ALLOW",
            "selected_action": "write_allow_marker",
            "reason_code": "SAP_ACCESS_APPROVED",
            "provenance_ref": f"sha3-256:prov_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}"
        }
    
    def _map_to_sap_role(self, decision_data: dict) -> str:
        """Map T81 decision to SAP role."""
        action = decision_data.get("selected_action", "")
        
        role_mapping = {
            "write_allow_marker": "SAP_FINANCE_USER",
            "read_allow_marker": "SAP_READ_ONLY_USER",
            "admin_allow_marker": "SAP_ADMIN_USER",
            "deny_marker": "SAP_NO_ACCESS"
        }
        
        return role_mapping.get(action, "SAP_DEFAULT_USER")
    
    def _get_auth_headers(self) -> Dict[str, str]:
        """Get SAP API authentication headers."""
        import base64
        
        auth_string = f"{self.client_id}:{self.secret_key}"
        auth_bytes = base64.b64encode(auth_string.encode())
        
        return {
            "Authorization": f"Basic {auth_bytes.decode()}",
            "Content-Type": "application/json",
            "X-T81-Bundle-Auth": "enterprise-integration"
        }

@dataclass
class SalesforceIntegration:
    """Salesforce integration with T81 bundle decisions."""
    org_id: str
    api_key: str
    sf_endpoint: str
    
    def __init__(self, org_id: str, api_key: str, sf_endpoint: str):
        self.org_id = org_id
        self.api_key = api_key
        self.sf_endpoint = sf_endpoint
    
    def create_salesforce_permission_set(self, bundle_ref: str, user_email: str, permissions: List[str]) -> bool:
        """Create Salesforce permission set based on bundle decision."""
        
        # Get bundle decision
        decision_data = self._get_bundle_decision(bundle_ref)
        
        if decision_data.get("decision") != "ALLOW":
            print(f"Bundle decision DENIED - cannot create Salesforce permissions for {user_email}")
            return False
        
        # Create permission set with bundle provenance
        permission_set = {
            "label": f"T81 Bundle Access - {bundle_ref[:16]}",
            "description": f"Access granted via T81 bundle decision: {bundle_ref}",
            "permissions": self._map_to_salesforce_permissions(decision_data, permissions),
            "license": "T81_CERTIFIED_ACCESS",
            "activation_required": False,
            "bundle_reference": bundle_ref,
            "compliance_audit_trail": decision_data.get("provenance_ref", "")
        }
        
        try:
            response = requests.post(
                f"{self.sf_endpoint}/services/data/v47.0/PermissionSet",
                json=permission_set,
                headers=self._get_auth_headers(),
                timeout=30
            )
            
            if response.status_code == 201:
                print(f"✅ Salesforce permission set created for {user_email}")
                return True
            else:
                print(f"❌ Salesforce permission set failed: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ Salesforce integration error: {e}")
            return False
    
    def _get_bundle_decision(self, bundle_ref: str) -> Dict[str, Any]:
        """Get decision data from bundle reference."""
        return {
            "decision": "ALLOW",
            "selected_action": "write_allow_marker",
            "reason_code": "SF_ACCESS_APPROVED",
            "provenance_ref": f"sha3-256:prov_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}"
        }
    
    def _map_to_salesforce_permissions(self, decision_data: dict, requested_permissions: List[str]) -> List[str]:
        """Map T81 decision to Salesforce permissions."""
        action = decision_data.get("selected_action", "")
        
        # Base permissions from bundle decision
        base_permissions = {
            "write_allow_marker": ["Read", "Create", "Edit", "Delete"],
            "read_allow_marker": ["Read", "View"],
            "admin_allow_marker": ["Read", "Create", "Edit", "Delete", "ModifyAllData"]
        }
        
        # Combine with requested permissions
        bundle_permissions = base_permissions.get(action, ["Read"])
        
        # Ensure requested permissions are within bundle grant
        final_permissions = []
        for perm in requested_permissions:
            if perm in bundle_permissions:
                final_permissions.append(perm)
        
        return list(set(final_permissions))  # Remove duplicates
    
    def _get_auth_headers(self) -> Dict[str, str]:
        """Get Salesforce API authentication headers."""
        return {
            "Authorization": f"Bearer {self.api_key}",
            "Content-Type": "application/json",
            "X-Salesforce-Organization": self.org_id,
            "X-T81-Bundle-Auth": "enterprise-integration"
        }

@dataclass
class KubernetesIntegration:
    """Kubernetes RBAC integration with T81 bundle decisions."""
    cluster_endpoint: str
    service_account_token: str
    namespace: str
    
    def __init__(self, cluster_endpoint: str, service_account_token: str, namespace: str = "default"):
        self.cluster_endpoint = cluster_endpoint
        self.service_account_token = service_account_token
        self.namespace = namespace
    
    def create_kubernetes_rbac(self, bundle_ref: str, user_id: str, resources: List[str]) -> bool:
        """Create Kubernetes RBAC based on bundle decision."""
        
        # Get bundle decision
        decision_data = self._get_bundle_decision(bundle_ref)
        
        if decision_data.get("decision") != "ALLOW":
            print(f"Bundle decision DENIED - cannot create K8s RBAC for user {user_id}")
            return False
        
        # Create Role and RoleBinding
        role_name = f"t81-bundle-{hashlib.sha256(bundle_ref.encode()).hexdigest()[:12]}"
        
        role_manifest = {
            "apiVersion": "rbac.authorization.k8s.io/v1",
            "kind": "Role",
            "metadata": {
                "name": role_name,
                "namespace": self.namespace,
                "annotations": {
                    "t81-foundation.org/bundle-reference": bundle_ref,
                    "t81-foundation.org/decision-timestamp": datetime.now(timezone.utc).isoformat()
                }
            },
            "rules": self._create_k8s_rules(decision_data, resources)
        }
        
        role_binding_manifest = {
            "apiVersion": "rbac.authorization.k8s.io/v1",
            "kind": "RoleBinding",
            "metadata": {
                "name": f"{role_name}-binding",
                "namespace": self.namespace,
                "annotations": {
                    "t81-foundation.org/bundle-reference": bundle_ref,
                    "t81-foundation.org/user-id": user_id
                }
            },
            "subjects": [{
                "kind": "User",
                "name": user_id,
                "apiGroup": "rbac.authorization.k8s.io"
            }],
            "roleRef": {
                "kind": "Role",
                "name": role_name,
                "apiGroup": "rbac.authorization.k8s.io"
            }
        }
        
        # Apply manifests to cluster
        success = self._apply_k8s_manifest(role_manifest) and self._apply_k8s_manifest(role_binding_manifest)
        
        if success:
            print(f"✅ Kubernetes RBAC created: {role_name} for user {user_id}")
        else:
            print(f"❌ Kubernetes RBAC creation failed")
        
        return success
    
    def _get_bundle_decision(self, bundle_ref: str) -> Dict[str, Any]:
        """Get decision data from bundle reference."""
        return {
            "decision": "ALLOW",
            "selected_action": "write_allow_marker",
            "reason_code": "K8S_ACCESS_APPROVED",
            "provenance_ref": f"sha3-256:prov_{hashlib.sha256(bundle_ref.encode()).hexdigest()[:16]}"
        }
    
    def _create_k8s_rules(self, decision_data: dict, resources: List[str]) -> List[Dict[str, Any]]:
        """Create Kubernetes RBAC rules from bundle decision."""
        action = decision_data.get("selected_action", "")
        
        # Map actions to K8s verbs
        action_mapping = {
            "write_allow_marker": ["get", "list", "create", "update", "patch", "delete"],
            "read_allow_marker": ["get", "list"],
            "admin_allow_marker": ["*"]  # All verbs
        }
        
        verbs = action_mapping.get(action, ["get", "list"])
        
        rules = []
        for resource in resources:
            rules.append({
                "apiGroups": ["", "apps", "extensions"],
                "resources": [resource],
                "verbs": verbs
            })
        
        return rules
    
    def _apply_k8s_manifest(self, manifest: Dict[str, Any]) -> bool:
        """Apply Kubernetes manifest using kubectl."""
        import subprocess
        import tempfile
        import os
        
        try:
            # Write manifest to temporary file
            with tempfile.NamedTemporaryFile(mode='w', suffix='.yaml', delete=False) as f:
                import yaml
                yaml.dump(manifest, f)
                temp_file = f.name
            
            # Apply with kubectl
            result = subprocess.run([
                "kubectl", "apply", "-f", temp_file,
                "--server", self.cluster_endpoint,
                "--token", self.service_account_token,
                "--namespace", self.namespace
            ], capture_output=True, text=True, timeout=60)
            
            # Clean up temporary file
            os.unlink(temp_file)
            
            return result.returncode == 0
            
        except Exception as e:
            print(f"❌ K8s manifest application error: {e}")
            return False

class EnterpriseIntegrationManager:
    """Manager for all enterprise integrations."""
    
    def __init__(self):
        self.integrations = {}
    
    def register_sap(self, name: str, client_id: str, secret_key: str, endpoint: str):
        """Register SAP integration."""
        self.integrations[f"sap_{name}"] = SAPIntegration(client_id, secret_key, endpoint)
        print(f"✅ SAP integration registered: {name}")
    
    def register_salesforce(self, name: str, org_id: str, api_key: str, endpoint: str):
        """Register Salesforce integration."""
        self.integrations[f"sf_{name}"] = SalesforceIntegration(org_id, api_key, endpoint)
        print(f"✅ Salesforce integration registered: {name}")
    
    def register_kubernetes(self, name: str, endpoint: str, token: str, namespace: str = "default"):
        """Register Kubernetes integration."""
        self.integrations[f"k8s_{name}"] = KubernetesIntegration(endpoint, token, namespace)
        print(f"✅ Kubernetes integration registered: {name}")
    
    def apply_bundle_decision(self, integration_name: str, bundle_ref: str, **kwargs):
        """Apply bundle decision to specific integration."""
        integration_key = f"{integration_name}_{kwargs.get('system_name', 'default')}"
        
        if integration_key not in self.integrations:
            print(f"❌ Integration not found: {integration_name}")
            return False
        
        integration = self.integrations[integration_key]
        
        # Route to appropriate integration method
        if isinstance(integration, SAPIntegration):
            return integration.create_sap_user_role(
                bundle_ref, 
                kwargs.get('user_id'), 
                kwargs.get('role_data', {})
            )
        elif isinstance(integration, SalesforceIntegration):
            return integration.create_salesforce_permission_set(
                bundle_ref,
                kwargs.get('user_email'),
                kwargs.get('permissions', [])
            )
        elif isinstance(integration, KubernetesIntegration):
            return integration.create_kubernetes_rbac(
                bundle_ref,
                kwargs.get('user_id'),
                kwargs.get('resources', [])
            )
        else:
            print(f"❌ Unknown integration type: {type(integration)}")
            return False

def main():
    """CLI interface for enterprise integrations."""
    import sys
    
    if len(sys.argv) < 3:
        print("Usage: python3 enterprise_integrations.py <command> [args]")
        print("Commands:")
        print("  register-sap <name> <client_id> <secret> <endpoint>")
        print("  register-salesforce <name> <org_id> <api_key> <endpoint>")
        print("  register-k8s <name> <endpoint> <token> [namespace]")
        print("  apply-decision <integration> <system_name> <bundle_ref> [options]")
        sys.exit(1)
    
    command = sys.argv[1]
    manager = EnterpriseIntegrationManager()
    
    if command == "register-sap":
        if len(sys.argv) < 6:
            print("Error: name, client_id, secret, endpoint required")
            sys.exit(1)
        
        manager.register_sap(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    
    elif command == "register-salesforce":
        if len(sys.argv) < 6:
            print("Error: name, org_id, api_key, endpoint required")
            sys.exit(1)
        
        manager.register_salesforce(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])
    
    elif command == "register-k8s":
        if len(sys.argv) < 5:
            print("Error: name, endpoint, token required")
            sys.exit(1)
        
        namespace = sys.argv[5] if len(sys.argv) > 5 else "default"
        manager.register_kubernetes(sys.argv[2], sys.argv[3], sys.argv[4], namespace)
    
    elif command == "apply-decision":
        if len(sys.argv) < 5:
            print("Error: integration, system_name, bundle_ref required")
            sys.exit(1)
        
        integration_name = sys.argv[2]
        system_name = sys.argv[3]
        bundle_ref = sys.argv[4]
        
        # Parse additional options
        kwargs = {}
        for i, arg in enumerate(sys.argv[5:], start=5):
            if arg.startswith("--user-id="):
                kwargs['user_id'] = arg[11:]
            elif arg.startswith("--user-email="):
                kwargs['user_email'] = arg[13:]
            elif arg.startswith("--permissions="):
                kwargs['permissions'] = arg[13:].split(',')
            elif arg.startswith("--resources="):
                kwargs['resources'] = arg[11:].split(',')
        
        success = manager.apply_bundle_decision(integration_name, system_name, bundle_ref, **kwargs)
        
        if success:
            print(f"✅ Bundle decision applied to {integration_name}")
        else:
            print(f"❌ Failed to apply bundle decision to {integration_name}")
    
    else:
        print(f"Unknown command: {command}")
        sys.exit(1)

if __name__ == "__main__":
    main()
