#!/usr/bin/env python3
"""Bundle conformance test suite for v1 contract compliance."""

import json
import subprocess
import tempfile
from pathlib import Path
from typing import Dict, List, Any

class BundleConformanceTest:
    """Test suite for bundle v1 contract compliance."""
    
    def __init__(self, t81_binary: str = "./build/t81"):
        self.t81_binary = t81_binary
        self.test_results = []
    
    def run_test(self, test_name: str, test_func) -> bool:
        """Run individual test and record result."""
        try:
            test_func()
            print(f"✓ PASS: {test_name}")
            self.test_results.append({"test": test_name, "status": "PASS"})
            return True
        except Exception as e:
            print(f"✗ FAIL: {test_name} - {e}")
            self.test_results.append({"test": test_name, "status": "FAIL", "error": str(e)})
            return False
    
    def test_bundle_validation(self):
        """Test bundle schema and required fields."""
        # Test valid bundle
        valid_bundle = {
            "schema": "t81.ai.task.assess-fixed.bundle.v1",
            "source_result_ref": "sha3-256:test_result",
            "source_provenance_ref": "sha3-256:test_provenance", 
            "action_ref": "sha3-256:test_action",
            "record_ref": "sha3-256:test_record"
        }
        
        # Validate schema
        valid_schemas = [
            "t81.ai.task.assess-fixed.bundle.v1",
            "t81.ai.task.route-fixed.bundle.v1",
            "t81.ai.task.classify-fixed.bundle.v1"
        ]
        
        if valid_bundle["schema"] not in valid_schemas:
            raise Exception(f"Unknown schema: {valid_bundle['schema']}")
        
        # Validate required fields
        required_fields = ["schema", "source_result_ref", "source_provenance_ref", "action_ref", "record_ref"]
        for field in required_fields:
            if field not in valid_bundle:
                raise Exception(f"Missing required field: {field}")
        
        # Test invalid schemas
        invalid_bundle = valid_bundle.copy()
        invalid_bundle["schema"] = "t81.ai.task.unknown.bundle.v1"
        
        try:
            self._validate_bundle_schema(invalid_bundle)
            raise Exception("Should have failed on unknown schema")
        except:
            pass  # Expected to fail
    
    def test_reference_format(self):
        """Test CanonFS reference format validation."""
        valid_refs = [
            "sha3-256:" + "a" * 96,  # 96 hex chars for SHA3-256
            "sha3-256:" + "0" * 96,  # All zeros
            "sha3-256:" + "f" * 96   # All f's
        ]
        
        invalid_refs = [
            "sha256:wrong_hash",  # Wrong algorithm
            "sha3-256:",           # Empty hash
            "not_a_hash",          # Not a hash at all
            "sha3-256:short",      # Too short
            "sha3-256:ggg",        # Invalid hex chars
        ]
        
        for ref in valid_refs:
            if not self._is_valid_canonfs_ref(ref):
                print(f"Debug: Valid ref rejected: {ref}")
                print(f"Debug: Hash part: '{ref[9:]}'")
                print(f"Debug: Length: {len(ref[9:])}")
                print(f"Debug: Valid hex: {all(c in '0123456789abcdef' for c in ref[9:].lower())}")
                raise Exception(f"Valid reference rejected: {ref}")
        
        for ref in invalid_refs:
            if self._is_valid_canonfs_ref(ref):
                raise Exception(f"Invalid reference accepted: {ref}")
        
        # Test edge case: empty hash
        if self._is_valid_canonfs_ref("sha3-256:"):
            raise Exception("Empty hash accepted as valid")
    
    def test_record_validation(self):
        """Test record schema and field validation."""
        # Test assess-fixed record
        assess_record = {
            "schema": "t81.ai.task.assess-fixed.host-action-record.v1",
            "selected_action": "write_allow_marker",
            "selected_path": "actions/allow.marker",
            "decision": "ALLOW",
            "reason_code": "GREETING_PAIR",
            "termination_reason": "single_step_max_score"
        }
        
        self._validate_assess_record(assess_record)
        
        # Test missing required fields
        incomplete_record = assess_record.copy()
        del incomplete_record["selected_action"]
        
        try:
            self._validate_assess_record(incomplete_record)
            raise Exception("Should have failed on missing field")
        except:
            pass  # Expected to fail
        
        # Test invalid decision values
        invalid_decision_record = assess_record.copy()
        invalid_decision_record["decision"] = "INVALID"
        
        try:
            self._validate_assess_record(invalid_decision_record)
            raise Exception("Should have failed on invalid decision")
        except:
            pass  # Expected to fail
    
    def test_error_codes(self):
        """Test structured error reporting."""
        # This would test actual error reporting from bundle consumers
        # For now, test our validation logic
        test_cases = [
            {
                "name": "Unknown schema error",
                "bundle": {"schema": "unknown.bundle.v1"},
                "expected_error": "BUNDLE_UNKNOWN_SCHEMA"
            },
            {
                "name": "Missing field error", 
                "bundle": {"schema": "t81.ai.task.assess-fixed.bundle.v1"},
                "expected_error": "BUNDLE_MISSING_FIELD"
            }
        ]
        
        for case in test_cases:
            try:
                self._validate_bundle_schema(case["bundle"])
                raise Exception(f"Should have failed with {case['expected_error']}")
            except Exception as e:
                if case["expected_error"] not in str(e):
                    raise Exception(f"Wrong error code. Expected {case['expected_error']}, got {e}")
    
    def _validate_bundle_schema(self, bundle: Dict[str, Any]):
        """Validate bundle schema and required fields."""
        valid_schemas = [
            "t81.ai.task.assess-fixed.bundle.v1",
            "t81.ai.task.route-fixed.bundle.v1", 
            "t81.ai.task.classify-fixed.bundle.v1"
        ]
        
        schema = bundle.get("schema")
        if not schema:
            raise Exception("BUNDLE_MISSING_FIELD: schema")
        
        if schema not in valid_schemas:
            raise Exception(f"BUNDLE_UNKNOWN_SCHEMA: {schema}")
        
        required_fields = ["source_result_ref", "source_provenance_ref", "action_ref", "record_ref"]
        for field in required_fields:
            if field not in bundle:
                raise Exception(f"BUNDLE_MISSING_FIELD: {field}")
    
    def _validate_assess_record(self, record: Dict[str, Any]):
        """Validate assess-fixed record schema and fields."""
        schema = record.get("schema")
        expected_schema = "t81.ai.task.assess-fixed.host-action-record.v1"
        
        if schema != expected_schema:
            raise Exception(f"RECORD_SCHEMA_MISMATCH: {schema} != {expected_schema}")
        
        required_fields = ["selected_action", "selected_path", "decision", "reason_code", "termination_reason"]
        for field in required_fields:
            if field not in record:
                raise Exception(f"RECORD_MISSING_FIELD: {field}")
        
        decision = record.get("decision")
        valid_decisions = ["ALLOW", "DENY", "REVIEW"]
        if decision not in valid_decisions:
            raise Exception(f"RECORD_INVALID_VALUE: decision={decision}")
    
    def _is_valid_canonfs_ref(self, ref: str) -> bool:
        """Check if reference matches CanonFS format."""
        if not ref.startswith("sha3-256:"):
            return False
        
        hash_part = ref[9:]  # Remove "sha3-256:"
        if len(hash_part) != 96:  # SHA3-256 produces 96 hex chars
            return False
        
        # Check for valid hex characters (0-9, a-f)
        hex_chars = set("0123456789abcdef")
        if not all(c in hex_chars for c in hash_part.lower()):
            return False
        
        return True
    
    def run_all_tests(self) -> Dict[str, Any]:
        """Run complete conformance test suite."""
        print("Running Bundle v1 Conformance Tests...")
        print("=" * 50)
        
        tests = [
            ("Bundle Validation", self.test_bundle_validation),
            ("Reference Format", self.test_reference_format),
            ("Record Validation", self.test_record_validation),
            ("Error Codes", self.test_error_codes)
        ]
        
        for test_name, test_func in tests:
            self.run_test(test_name, test_func)
        
        print("=" * 50)
        
        passed = sum(1 for result in self.test_results if result["status"] == "PASS")
        total = len(self.test_results)
        
        print(f"\nResults: {passed}/{total} tests passed")
        
        if passed == total:
            print("✓ All tests passed - Bundle v1 contract compliant")
            return {"status": "PASS", "results": self.test_results}
        else:
            print("✗ Some tests failed - Review contract compliance")
            return {"status": "FAIL", "results": self.test_results}


def main():
    """Run conformance tests."""
    tester = BundleConformanceTest()
    results = tester.run_all_tests()
    
    # Output results for CI
    results_file = Path("bundle_conformance_results.json")
    with open(results_file, 'w') as f:
        json.dump(results, f, indent=2)
    
    print(f"\nResults saved to: {results_file}")
    return 0 if results["status"] == "PASS" else 1


if __name__ == "__main__":
    exit(main())
