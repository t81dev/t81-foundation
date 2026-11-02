"""Regression tests for operational state v2 schema and validator contract.

Covers:
- Observation envelope missing required fields (subject, observed_at, collector, retained_receipt, source_revision)
- Enforce exactly one meaningful source_revision alternative (commit_sha, inherit_from, or null value with nonempty reason)
- Reject omissions, empty objects {}, reason-only objects, and conflicting alternatives
- Malformed timestamps (missing timezone, invalid offset, non-ISO)
- Malformed commit SHAs
- Explicit unknown/unavailable values requiring reasons
- Source-revision inheritance
- stdlib validator vs Draft202012Validator behavior parity
"""
import copy
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
VALIDATOR_SCRIPT = ROOT / "scripts/validate_current_state.py"
SCHEMA_PATH = ROOT / "docs/ai-context/CURRENT_STATE.schema.json"
STATE_PATH = ROOT / "docs/ai-context/CURRENT_STATE.json"

try:
    from jsonschema import Draft202012Validator, FormatChecker
    HAVE_JSONSCHEMA = True
except ImportError:
    HAVE_JSONSCHEMA = False


class OperationalStateContractTest(unittest.TestCase):
    def setUp(self):
        self.schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        self.state = json.loads(STATE_PATH.read_text(encoding="utf-8"))
        if HAVE_JSONSCHEMA:
            self.oracle = Draft202012Validator(self.schema, format_checker=FormatChecker())

    def run_validator(self, state_dict, flags=None):
        """Run scripts/validate_current_state.py in a subprocess with candidate state."""
        import tempfile
        flags = flags or []
        with tempfile.TemporaryDirectory(prefix="t81-state-test-") as tmp:
            tmp_repo = Path(tmp) / "repo"
            tmp_docs = tmp_repo / "docs/ai-context"
            tmp_scripts = tmp_repo / "scripts"
            tmp_docs.mkdir(parents=True, exist_ok=True)
            tmp_scripts.mkdir(parents=True, exist_ok=True)

            (tmp_docs / "CURRENT_STATE.json").write_text(json.dumps(state_dict), encoding="utf-8")
            (tmp_docs / "CURRENT_STATE.schema.json").write_text(json.dumps(self.schema), encoding="utf-8")
            (tmp_scripts / "validate_current_state.py").write_bytes(VALIDATOR_SCRIPT.read_bytes())

            cmd = [sys.executable, *flags, str(tmp_scripts / "validate_current_state.py")]
            proc = subprocess.run(cmd, cwd=tmp, text=True, capture_output=True)
            return proc.returncode, proc.stdout, proc.stderr

    def assert_rejected(self, modified_state, expected_message_part=None):
        """Assert state fails both stdlib validator and Draft202012Validator."""
        code, stdout, stderr = self.run_validator(modified_state)
        self.assertNotEqual(code, 0, f"Expected validation failure, got 0. stderr: {stderr}")
        if expected_message_part:
            self.assertIn(expected_message_part, stderr)

        # Also check in optimized mode (-O)
        code_o, stdout_o, stderr_o = self.run_validator(modified_state, flags=["-O"])
        self.assertNotEqual(code_o, 0, f"Expected validation failure in -O mode, got 0. stderr: {stderr_o}")

        if HAVE_JSONSCHEMA:
            errors = list(self.oracle.iter_errors(modified_state))
            self.assertTrue(len(errors) > 0, "Oracle unexpectedly accepted invalid state")

    def test_canonical_state_passes(self):
        """Canonical checked-in state passes cleanly in both normal and -O mode."""
        code, stdout, stderr = self.run_validator(self.state)
        self.assertEqual(code, 0, f"Validation failed: {stderr}")
        self.assertIn("CURRENT_STATE: structure valid", stdout)

        code_o, stdout_o, stderr_o = self.run_validator(self.state, flags=["-O"])
        self.assertEqual(code_o, 0, f"Validation in -O mode failed: {stderr_o}")
        self.assertIn("CURRENT_STATE: structure valid", stdout_o)

        if HAVE_JSONSCHEMA:
            errors = list(self.oracle.iter_errors(self.state))
            self.assertEqual(errors, [])

    def test_envelope_missing_required_fields(self):
        """Observation envelopes require subject, observed_at, source_revision, collector, retained_receipt."""
        for field in ["subject", "observed_at", "source_revision", "collector", "retained_receipt"]:
            bad = copy.deepcopy(self.state)
            del bad["operational_views"]["2026-11-02"]["repository_observation"]["envelope"][field]
            self.assert_rejected(bad, "missing required keys")

    def test_source_revision_strict_alternatives(self):
        """Source revision must contain exactly one valid alternative."""
        repo_env = self.state["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]

        # Valid alternatives:
        valid_cases = [
            {"commit_sha": "a" * 40},
            {"inherit_from": "repository_observation"},
            {"value": None, "reason": "not available"},
        ]
        for v in valid_cases:
            good = copy.deepcopy(self.state)
            good["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]["source_revision"] = v
            code, stdout, stderr = self.run_validator(good)
            self.assertEqual(code, 0, f"Valid source_revision failed: {v}, error: {stderr}")

        # Invalid alternatives (must be rejected):
        invalid_cases = [
            ({}, "empty source_revision is forbidden"),
            ({"reason": "only reason"}, "reason-only source_revision is forbidden"),
            ({"commit_sha": "a" * 40, "inherit_from": "parent"}, "source_revision must have exactly one valid alternative"),
            ({"commit_sha": "a" * 40, "extra": "key"}, "unexpected keys"),
            ({"value": None}, "unavailable source_revision must have value and reason only"),
            ({"value": None, "reason": ""}, "nonempty reason required"),
            ({"value": None, "reason": "   "}, "nonempty reason required"),
            ({"inherit_from": ""}, "empty inherit_from string"),
            ({"inherit_from": 123}, "empty inherit_from string"),
            ({"commit_sha": "bad_sha"}, "invalid commit_sha format"),
        ]
        for inv, err_msg in invalid_cases:
            bad = copy.deepcopy(self.state)
            bad["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]["source_revision"] = inv
            self.assert_rejected(bad, err_msg)

    def test_observed_at_timestamp_timezone_required(self):
        """Observed_at timestamps require ISO/RFC 3339 timezone offset ('Z' or '+-HH:MM')."""
        bad_timestamps = [
            "2026-11-02T05:14:30",        # Naive (no timezone)
            "2026-11-02 05:14:30",        # Space separator
            "2026-11-02",                 # Date only
            "2026-13-01T00:00:00Z",       # Invalid month
            "2026-11-02T25:00:00Z",       # Invalid hour
            "2026-11-02T05:14:30+25:00",  # Invalid offset
            "2026-11-02T05:14:30Z\n",     # Trailing newline
            "",                           # Empty string
        ]
        for ts in bad_timestamps:
            bad = copy.deepcopy(self.state)
            bad["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]["observed_at"] = ts
            self.assert_rejected(bad)

    def test_observed_at_valid_timezone_offsets(self):
        """Valid timezone offsets (+00:00, -05:00, Z) must be accepted."""
        for ts in ["2026-11-02T05:14:30Z", "2026-11-02T00:14:30-05:00", "2026-11-02T07:14:30+02:00"]:
            good = copy.deepcopy(self.state)
            good["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]["observed_at"] = ts
            code, stdout, stderr = self.run_validator(good)
            self.assertEqual(code, 0, f"Expected valid timestamp {ts}, got: {stderr}")

    def test_source_revision_commit_sha_validation(self):
        """Commit SHAs must be exactly 40 lowercase hexadecimal digits."""
        bad_shas = [
            "1a6d5a3",                           # Truncated
            "1a6d5a36473004961248f0a85b2e06361ea2a38f0",  # 41 chars
            "1A6D5A36473004961248F0A85B2E06361EA2A38F",  # Uppercase
            "ga6d5a36473004961248f0a85b2e06361ea2a38f",  # Non-hex digit 'g'
            "1a6d5a36473004961248f0a85b2e06361ea2a38f\n", # Trailing newline
        ]
        for sha in bad_shas:
            bad = copy.deepcopy(self.state)
            bad["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]["source_revision"] = {"commit_sha": sha}
            self.assert_rejected(bad)

    def test_unknown_unavailable_source_revision_requires_reason(self):
        """Unknown or unavailable values with null value require an explanatory reason."""
        good = copy.deepcopy(self.state)
        # PR #658 has {"value": null, "reason": "pr_is_open"}
        code, stdout, stderr = self.run_validator(good)
        self.assertEqual(code, 0)

        # Missing reason
        bad = copy.deepcopy(self.state)
        bad["operational_views"]["2026-11-02"]["pr_observations"][2]["merge_sha"] = {"value": None}
        self.assert_rejected(bad)

        # Non-null value in unavailable structure
        bad_val = copy.deepcopy(self.state)
        bad_val["operational_views"]["2026-11-02"]["pr_observations"][2]["merge_sha"] = {"value": "unknown", "reason": "test"}
        self.assert_rejected(bad_val)

    def test_source_revision_inheritance(self):
        """Inherit_from must be a valid string pointing to the parent observation designation."""
        good = copy.deepcopy(self.state)
        # Check scientific observation inherits from repository_observation
        self.assertEqual(
            good["operational_views"]["2026-11-02"]["scientific_observations"][0]["envelope"]["source_revision"],
            {"inherit_from": "repository_observation"}
        )
        code, stdout, stderr = self.run_validator(good)
        self.assertEqual(code, 0)

        # Invalid type for inherit_from
        bad = copy.deepcopy(self.state)
        bad["operational_views"]["2026-11-02"]["scientific_observations"][0]["envelope"]["source_revision"] = {"inherit_from": 123}
        self.assert_rejected(bad)

    def test_historical_snapshot_preservation(self):
        """Historical 2026-10-31 snapshot structure remains intact and valid."""
        self.assertIn("2026-10-31", self.state["historical_snapshots"])
        hist = self.state["historical_snapshots"]["2026-10-31"]
        self.assertEqual(hist["schema_version"], "1.0.0")
        self.assertEqual(hist["repository"]["observed_head"], "e108fdad078f4d6dfcc427f1fd419ecbe48b4e05")
        self.assertEqual(hist["open_pr_observations"]["count"], 49)

        # Corrupting historical snapshot fails validation
        bad = copy.deepcopy(self.state)
        del bad["historical_snapshots"]["2026-10-31"]["repository"]["observed_head"]
        self.assert_rejected(bad, "missing required keys")

    def test_rejected_unsupported_ref(self):
        """The validator rejects unsupported or non-existent $ref values."""
        bad_schema = copy.deepcopy(self.schema)
        bad_schema["properties"]["selected_view"] = {"$ref": "https://example.com/schema.json"}
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp) / "repo"
            (repo / "docs/ai-context").mkdir(parents=True)
            (repo / "scripts").mkdir(parents=True)
            (repo / "docs/ai-context/CURRENT_STATE.json").write_text(json.dumps(self.state))
            (repo / "docs/ai-context/CURRENT_STATE.schema.json").write_text(json.dumps(bad_schema))
            (repo / "scripts/validate_current_state.py").write_bytes(VALIDATOR_SCRIPT.read_bytes())
            proc = subprocess.run([sys.executable, str(repo / "scripts/validate_current_state.py")],
                                  cwd=tmp, capture_output=True, text=True)
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn("unsupported $ref", proc.stderr)

    def test_unexpected_keys_rejected_at_all_levels(self):
        """Objects with additionalProperties=False reject unexpected keys."""
        bad = copy.deepcopy(self.state)
        bad["unexpected_root_key"] = "forbidden"
        self.assert_rejected(bad, "unexpected keys")

        bad2 = copy.deepcopy(self.state)
        bad2["operational_views"]["2026-11-02"]["repository_observation"]["envelope"]["unexpected"] = 123
        self.assert_rejected(bad2, "unexpected keys")

    def test_malformed_json_and_non_json_constants(self):
        """Validator rejects malformed JSON and non-JSON constants like NaN."""
        import tempfile
        for payload in ["{", '{"x": 1,}', "NaN"]:
            with tempfile.TemporaryDirectory() as tmp:
                repo = Path(tmp) / "repo"
                (repo / "docs/ai-context").mkdir(parents=True)
                (repo / "scripts").mkdir(parents=True)
                (repo / "docs/ai-context/CURRENT_STATE.json").write_text(payload)
                (repo / "docs/ai-context/CURRENT_STATE.schema.json").write_text(json.dumps(self.schema))
                (repo / "scripts/validate_current_state.py").write_bytes(VALIDATOR_SCRIPT.read_bytes())
                proc = subprocess.run([sys.executable, str(repo / "scripts/validate_current_state.py")],
                                      cwd=tmp, capture_output=True, text=True)
                self.assertNotEqual(proc.returncode, 0)
                self.assertIn("CURRENT_STATE: invalid", proc.stderr)


if __name__ == "__main__":
    unittest.main()
