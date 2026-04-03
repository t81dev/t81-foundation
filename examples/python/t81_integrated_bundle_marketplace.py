#!/usr/bin/env python3
"""T81-integrated bundle marketplace for canonical decision objects."""

import json
import hashlib
import subprocess
import tempfile
import os
from datetime import datetime, timezone
from typing import Dict, List, Any, Optional
from dataclasses import dataclass
from pathlib import Path

@dataclass
class T81Bundle:
    """Canonical T81 bundle following AI OS-Object Bundle Consumption Contract."""
    bundle_ref: str
    schema: str  # Must be one of: t81.ai.task.assess-fixed.bundle.v1, etc.
    source_result_ref: str
    source_provenance_ref: str
    action_ref: str
    record_ref: str
    
    def to_dict(self):
        return {
            "bundle_ref": self.bundle_ref,
            "schema": self.schema,
            "source_result_ref": self.source_result_ref,
            "source_provenance_ref": self.source_provenance_ref,
            "action_ref": self.action_ref,
            "record_ref": self.record_ref
        }

@dataclass
class ModelIntegration:
    """Model integration for external AI models."""
    model_id: str
    model_name: str
    provider: str
    model_type: str
    api_endpoint: str
    compliance_level: str
    pricing_model: str
    integration_status: str

class T81IntegratedMarketplace:
    """Bundle marketplace integrated with actual T81 CanonFS and runtime."""
    
    def __init__(self, canonfs_root: str = None):
        self.canonfs_root = canonfs_root or os.environ.get('T81_CANONFS_ROOT', tempfile.mkdtemp())
        self.build_dir = Path(__file__).parent.parent.parent / "build"
        self.t81_cli = self.build_dir / "t81"
        
        # Verify T81 is built
        if not self.t81_cli.exists():
            raise RuntimeError("T81 CLI not found. Build with: cmake --build build --target t81")
    
    def _run_t81_command(self, cmd: List[str], input_data: str = None) -> str:
        """Run T81 CLI command and return output."""
        full_cmd = [str(self.t81_cli)] + cmd
        try:
            result = subprocess.run(
                full_cmd,
                input=input_data,
                text=True,
                capture_output=True,
                check=True
            )
            return result.stdout
        except subprocess.CalledProcessError as e:
            print(f"T81 command failed: {' '.join(full_cmd)}")
            print(f"Error: {e.stderr}")
            raise
    
    def create_assess_fixed_bundle(self, model_integration: ModelIntegration, 
                              input_data: Dict[str, Any], 
                              decision: str = "ALLOW",
                              reason_code: str = "APPROVED") -> T81Bundle:
        """Create a canonical assess-fixed bundle using actual T81 runtime."""
        
        print(f"🔄 Creating assess-fixed bundle with {model_integration.model_name}")
        
        # Create temporary model file (mock for external model integration)
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            # Create mock model config that references external model
            model_config = {
                "model_id": model_integration.model_id,
                "model_name": model_integration.model_name,
                "provider": model_integration.provider,
                "api_endpoint": model_integration.api_endpoint,
                "compliance_level": model_integration.compliance_level
            }
            json.dump(model_config, f, indent=2)
            model_config_path = f.name
        
        try:
            # 1. Create assess-fixed demo model
            demo_model_dir = tempfile.mkdtemp()
            demo_model_path = os.path.join(demo_model_dir, "assess-fixed-demo.t81w")
            
            subprocess.run([
                str(self.build_dir / "t81_make_assess_fixed_demo"),
                demo_model_dir
            ], check=True, stdout=subprocess.DEVNULL)
            
            # 2. Get model hash for policy
            model_hash = self._run_t81_command([
                "determinism", "hash", demo_model_path
            ]).split()[0]
            
            # 3. Create policy allowing this model
            with tempfile.NamedTemporaryFile(mode='w', suffix='.apl', delete=False) as f:
                policy_content = f"""(policy
  (tier 1)
  (allowed-ternary-model-hashes ["sha3-512:{model_hash}"])
  (require-axion-event (reason "task:assess_fixed.v1")))"""
                f.write(policy_content)
                policy_path = f.name
            
            # 4. Run assess-fixed task
            task_input = json.dumps(input_data) if isinstance(input_data, dict) else str(input_data)
            task_output = self._run_t81_command([
                "ai", "task", "assess-fixed",
                "--model", "assess-fixed-demo",
                "--model-file", demo_model_path,
                "--policy", policy_path,
                "--canonfs-root", self.canonfs_root,
                "--mode", "strict_deterministic",
                "--input", task_input
            ])
            
            # 5. Extract refs from task output
            task_data = json.loads(task_output)
            result_ref = task_data.get("result_ref")
            provenance_ref = task_data.get("provenance_ref")
            
            if not result_ref or not provenance_ref:
                raise RuntimeError("Task did not produce required refs")
            
            # 6. Create action artifact based on decision
            action_name = {
                "ALLOW": "write_allow_marker",
                "DENY": "write_deny_marker", 
                "REVIEW": "write_review_marker"
            }.get(decision, "write_allow_marker")
            
            action_content = {
                "decision": decision,
                "reason_code": reason_code,
                "source_result_ref": result_ref,
                "external_model": model_integration.model_id,
                "external_provider": model_integration.provider
            }
            
            with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
                json.dump(action_content, f, indent=2)
                action_path = f.name
            
            action_ref = self._run_t81_command([
                "canonfs", "put-file", action_path,
                "--canonfs-root", self.canonfs_root
            ]).strip()
            
            # 7. Create host-action record
            record_output = self._run_t81_command([
                "artifact", "write-store-record",
                "--schema", "t81.ai.task.assess-fixed.host-action-record.v1",
                "--field", f"source_result_ref={result_ref}",
                "--field", f"source_provenance_ref={provenance_ref}",
                "--field", f"decision={decision}",
                "--field", f"reason_code={reason_code}",
                "--field", f"termination_reason=task_completed",
                "--field", f"selected_action={action_name}",
                "--field", f"selected_path=actions/{decision.lower()}.marker",
                "--field", f"action_ref={action_ref}",
                "--canonfs-root", self.canonfs_root
            ])
            
            record_ref = json.loads(record_output).get("record_ref")
            if not record_ref:
                raise RuntimeError("Failed to create record")
            
            # 8. Create final bundle
            bundle_output = self._run_t81_command([
                "artifact", "store-bundle",
                "--schema", "t81.ai.task.assess-fixed.bundle.v1",
                "--field", f"source_result_ref={result_ref}",
                "--field", f"source_provenance_ref={provenance_ref}",
                "--field", f"action_ref={action_ref}",
                "--field", f"record_ref={record_ref}",
                "--canonfs-root", self.canonfs_root
            ])
            
            bundle_data = json.loads(bundle_output)
            bundle_ref = bundle_data.get("bundle_ref")
            
            if not bundle_ref:
                raise RuntimeError("Failed to create bundle")
            
            bundle = T81Bundle(
                bundle_ref=bundle_ref,
                schema="t81.ai.task.assess-fixed.bundle.v1",
                source_result_ref=result_ref,
                source_provenance_ref=provenance_ref,
                action_ref=action_ref,
                record_ref=record_ref
            )
            
            print(f"  ✅ Bundle created: {bundle_ref}")
            return bundle
            
        finally:
            # Cleanup temp files
            for path in [model_config_path, policy_path]:
                if 'path' in locals() and os.path.exists(path):
                    os.unlink(path)
    
    def get_bundle_details(self, bundle_ref: str) -> Optional[T81Bundle]:
        """Get bundle details from CanonFS."""
        try:
            # Get bundle artifact
            with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
                bundle_path = f.name
            
            self._run_t81_command([
                "canonfs", "get", bundle_ref,
                "--canonfs-root", self.canonfs_root,
                "--out", bundle_path,
                "--json"
            ])
            
            with open(bundle_path, 'r') as f:
                bundle_data = json.load(f)
            
            schema = bundle_data.get("schema")
            if not schema.startswith("t81.ai.task."):
                print(f"❌ Unsupported bundle schema: {schema}")
                return None
            
            # Extract required fields according to contract
            return T81Bundle(
                bundle_ref=bundle_ref,
                schema=schema,
                source_result_ref=bundle_data.get("source_result_ref"),
                source_provenance_ref=bundle_data.get("source_provenance_ref"),
                action_ref=bundle_data.get("action_ref"),
                record_ref=bundle_data.get("record_ref")
            )
            
        except Exception as e:
            print(f"❌ Failed to get bundle {bundle_ref}: {e}")
            return None
    
    def consume_bundle(self, bundle_ref: str) -> Dict[str, Any]:
        """Consume bundle following T81 Bundle Consumption Contract."""
        
        bundle = self.get_bundle_details(bundle_ref)
        if not bundle:
            return {"error": "Bundle not found or invalid"}
        
        print(f"📦 Consuming bundle: {bundle_ref}")
        print(f"   Schema: {bundle.schema}")
        
        # Follow consumption contract: bundle first, then dereference as needed
        consumption_result = {
            "bundle_ref": bundle.bundle_ref,
            "schema": bundle.schema,
            "family": bundle.schema.split(".")[2],  # extract family name
            "source_result_ref": bundle.source_result_ref,
            "source_provenance_ref": bundle.source_provenance_ref,
            "action_ref": bundle.action_ref,
            "record_ref": bundle.record_ref
        }
        
        # Get family-specific record details if needed
        try:
            with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
                record_path = f.name
            
            self._run_t81_command([
                "canonfs", "get", bundle.record_ref,
                "--canonfs-root", self.canonfs_root,
                "--out", record_path,
                "--json"
            ])
            
            with open(record_path, 'r') as f:
                record_data = json.load(f)
            
            consumption_result["record_details"] = record_data
            
            # Extract family-specific fields
            if bundle.schema == "t81.ai.task.assess-fixed.bundle.v1":
                consumption_result["selected_action"] = record_data.get("selected_action")
                consumption_result["selected_path"] = record_data.get("selected_path")
            elif bundle.schema == "t81.ai.task.route-fixed.bundle.v1":
                consumption_result["selected_action"] = record_data.get("selected_action")
                consumption_result["selected_path"] = record_data.get("selected_path")
            elif bundle.schema == "t81.ai.task.classify-fixed.bundle.v1":
                consumption_result["selected_rule_set"] = record_data.get("selected_rule_set")
                consumption_result["rule_set_ref"] = record_data.get("rule_set_ref")
                
        except Exception as e:
            print(f"⚠️  Could not retrieve record details: {e}")
        
        return consumption_result
    
    def list_bundles(self) -> List[str]:
        """List all bundles in CanonFS."""
        try:
            # This would need to be implemented in T81 CLI
            # For now, return empty list
            print("📋 Bundle listing not yet implemented in T81 CLI")
            return []
        except Exception as e:
            print(f"❌ Failed to list bundles: {e}")
            return []

def main():
    """CLI interface for T81 integrated marketplace."""
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python3 t81_integrated_bundle_marketplace.py <command> [args]")
        print("Commands:")
        print("  create-assess-fixed <model_config_file> <input_file>  Create assess-fixed bundle")
        print("  get-bundle <bundle_ref>                      Get bundle details")
        print("  consume-bundle <bundle_ref>                    Consume bundle following contract")
        print("  list-bundles                                  List all bundles")
        print("")
        print("Example model_config_file:")
        print("""{
  "model_id": "gpt4_turbo",
  "model_name": "GPT-4 Turbo",
  "provider": "OpenAI",
  "model_type": "foundation",
  "api_endpoint": "https://api.openai.com/v1/chat/completions",
  "compliance_level": "ENTERPRISE",
  "pricing_model": "per_token",
  "integration_status": "ACTIVE"
}""")
        sys.exit(1)
    
    command = sys.argv[1]
    
    try:
        marketplace = T81IntegratedMarketplace()
        
        if command == "create-assess-fixed":
            if len(sys.argv) < 4:
                print("Error: model_config_file and input_file required")
                sys.exit(1)
            
            with open(sys.argv[2], 'r') as f:
                model_config = json.load(f)
            
            with open(sys.argv[3], 'r') as f:
                input_data = json.load(f)
            
            model_integration = ModelIntegration(**model_config)
            bundle = marketplace.create_assess_fixed_bundle(model_integration, input_data)
            
            if bundle:
                print(f"\n✅ Bundle created successfully!")
                print(f"Bundle Reference: {bundle.bundle_ref}")
                print(f"Schema: {bundle.schema}")
                # Also output machine-readable format for scripting
                print(f"BUNDLE_REF:{bundle.bundle_ref}")
            
        elif command == "get-bundle":
            if len(sys.argv) < 3:
                print("Error: bundle_ref required")
                sys.exit(1)
            
            bundle_ref = sys.argv[2]
            bundle = marketplace.get_bundle_details(bundle_ref)
            
            if bundle:
                print(f"\n📦 Bundle Details:")
                print(f"Reference: {bundle.bundle_ref}")
                print(f"Schema: {bundle.schema}")
                print(f"Source Result: {bundle.source_result_ref}")
                print(f"Source Provenance: {bundle.source_provenance_ref}")
                print(f"Action: {bundle.action_ref}")
                print(f"Record: {bundle.record_ref}")
            else:
                print(f"❌ Bundle not found: {bundle_ref}")
        
        elif command == "consume-bundle":
            if len(sys.argv) < 3:
                print("Error: bundle_ref required")
                sys.exit(1)
            
            bundle_ref = sys.argv[2]
            result = marketplace.consume_bundle(bundle_ref)
            
            if "error" not in result:
                print(f"\n📦 Bundle Consumption Result:")
                print(f"Bundle: {result['bundle_ref']}")
                print(f"Family: {result['family']}")
                print(f"Schema: {result['schema']}")
                
                if 'selected_action' in result:
                    print(f"Selected Action: {result['selected_action']}")
                if 'selected_path' in result:
                    print(f"Selected Path: {result['selected_path']}")
                if 'selected_rule_set' in result:
                    print(f"Selected Rule Set: {result['selected_rule_set']}")
                
                if 'record_details' in result:
                    print(f"\n📋 Record Details:")
                    print(json.dumps(result['record_details'], indent=2))
            else:
                print(f"❌ {result['error']}")
        
        elif command == "list-bundles":
            bundles = marketplace.list_bundles()
            print(f"\n📋 Available Bundles:")
            for bundle_ref in bundles:
                print(f"  {bundle_ref}")
        
        else:
            print(f"Unknown command: {command}")
            sys.exit(1)
            
    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
