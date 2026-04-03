#!/usr/bin/env python3
"""Simple Python library for consuming T81 decision bundles."""

import json
import subprocess
import tempfile
from pathlib import Path
from typing import Optional, Dict, Any


class BundleConsumer:
    """Consumer for T81 canonical decision bundles."""
    
    def __init__(self, t81_binary: str = "./build/t81", canonfs_root: str = ".t81_canonfs"):
        self.t81_binary = t81_binary
        self.canonfs_root = canonfs_root
    
    def get_bundle(self, bundle_ref: str) -> Dict[str, Any]:
        """Retrieve bundle JSON from CanonFS."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            result = subprocess.run([
                self.t81_binary, "canonfs", "get", bundle_ref,
                "--canonfs-root", self.canonfs_root,
                "--out", f.name, "--json"
            ], capture_output=True, text=True)
            
            if result.returncode != 0:
                raise Exception(f"Failed to retrieve bundle {bundle_ref}: {result.stderr}")
            
            with open(f.name, 'r') as bundle_file:
                return json.load(bundle_file)
    
    def read_field(self, ref: str, schema: str, field: str) -> str:
        """Extract field from artifact."""
        result = subprocess.run([
            self.t81_binary, "artifact", "read-field", ref,
            "--schema", schema, "--field", field,
            "--canonfs-root", self.canonfs_root
        ], capture_output=True, text=True)
        
        if result.returncode != 0:
            raise Exception(f"Failed to read field {field} from {ref}: {result.stderr}")
        
        return result.stdout.strip()
    
    def summarize_bundle(self, bundle_ref: str) -> Dict[str, str]:
        """Extract key decision fields from bundle."""
        bundle = self.get_bundle(bundle_ref)
        bundle_schema = bundle.get("schema", "")
        
        # Determine family and expected record schema
        family_configs = {
            "t81.ai.task.assess-fixed.bundle.v1": {
                "family": "assess-fixed",
                "record_schema": "t81.ai.task.assess-fixed.host-action-record.v1",
                "primary_field": "selected_action",
                "secondary_field": "selected_path"
            },
            "t81.ai.task.route-fixed.bundle.v1": {
                "family": "route-fixed", 
                "record_schema": "t81.ai.task.route-fixed.path-selection-record.v1",
                "primary_field": "selected_action",
                "secondary_field": "selected_path"
            },
            "t81.ai.task.classify-fixed.bundle.v1": {
                "family": "classify-fixed",
                "record_schema": "t81.ai.task.classify-fixed.rule-selection-record.v1", 
                "primary_field": "selected_rule_set",
                "secondary_field": "rule_set_ref"
            }
        }
        
        config = family_configs.get(bundle_schema)
        if not config:
            raise Exception(f"Unsupported bundle schema: {bundle_schema}")
        
        # Extract bundle references
        record_ref = self.read_field(bundle_ref, bundle_schema, "record_ref")
        action_ref = self.read_field(bundle_ref, bundle_schema, "action_ref")
        source_result_ref = self.read_field(bundle_ref, bundle_schema, "source_result_ref")
        source_provenance_ref = self.read_field(bundle_ref, bundle_schema, "source_provenance_ref")
        
        # Extract decision fields
        primary_value = self.read_field(record_ref, config["record_schema"], config["primary_field"])
        secondary_value = self.read_field(record_ref, config["record_schema"], config["secondary_field"])
        
        return {
            "bundle_ref": bundle_ref,
            "bundle_schema": bundle_schema,
            "family": config["family"],
            "record_ref": record_ref,
            "action_ref": action_ref,
            "source_result_ref": source_result_ref,
            "source_provenance_ref": source_provenance_ref,
            "record_schema": config["record_schema"],
            config["primary_field"]: primary_value,
            config["secondary_field"]: secondary_value
        }


# Example usage
if __name__ == "__main__":
    consumer = BundleConsumer()
    
    # Example bundle reference (replace with actual)
    # bundle_ref = "sha3-256:example_bundle_ref"
    
    # For testing, create a simple test case
    print("Bundle Consumer Library - Ready for use")
    print("Usage:")
    print("  consumer = BundleConsumer()")
    print("  summary = consumer.summarize_bundle('<bundle_ref>')")
    print("  print(summary)")
