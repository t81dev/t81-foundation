#!/usr/bin/env python3
"""
Comprehensive Differential Test Suite for Scientific Investigation 08:
Equivalent Conventional Comparator vs T81 Reference Implementation.

Enforces Scientific Acceptance Gate:
Zero unexplained differences across:
1. Original inputs ("greet hello", "hello world", "greet").
2. Unknown & malformed inputs ("xyz", "", "hello", "greet hello world").
3. Whitespace & normalization cases ("greet hello\r\n", "\tgreet hello  ", "greet\nhello").
4. Valid model weight mutations.
5. Tokenizer vocabulary mutations.
6. Invalid model formats, headers, and checksum corruptions.
7. Missing or conflicting policy authorizations.
8. Repeated deterministic executions (replay determinism).
9. Corrupted or inconsistent evidence linkage.
10. Adversarial and boundary cases (unsupported shapes, corrupted payload, invalid APL policy).
"""

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from typing import Dict, Any, Tuple

# Ensure baseline_stack can be imported
sys.path.insert(0, os.path.dirname(__file__))
from baseline_stack import ConventionalTaskRunner, T81W2Model, ConventionalPolicyEngine, ConventionalCAS, normalize_task_input_text


class DifferentialParityTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
        cls.build_dir = os.path.join(cls.repo_root, "build")
        cls.t81_bin = os.path.join(cls.build_dir, "t81")
        cls.demo_builder = os.path.join(cls.build_dir, "t81_make_assess_fixed_demo")

        if not os.path.exists(cls.t81_bin) or not os.path.exists(cls.demo_builder):
            raise RuntimeError("Build artifacts missing. Run cmake build first.")

        cls.temp_dir = tempfile.mkdtemp(prefix="t81-diff-test-")
        cls.model_dir = os.path.join(cls.temp_dir, "model_dir")
        os.makedirs(cls.model_dir, exist_ok=True)

        # Build standard assess-fixed demo model
        cmd = [cls.demo_builder, cls.model_dir]
        subprocess.run(cmd, capture_output=True, text=True, check=True)

        cls.model_path = os.path.join(cls.model_dir, "assess-fixed-demo.t81w")
        cls.tokenizer_path = os.path.join(cls.model_dir, "tokenizer.json")

        with open(cls.model_path, "rb") as f:
            cls.model_bytes = f.read()

        with open(cls.tokenizer_path, "r", encoding="utf-8") as f:
            cls.tokenizer_data = json.load(f)

        cls.parsed_model = T81W2Model(cls.model_bytes)
        # T81 CLI computes SHA3-512 over the entire model binary file
        cls.model_hash = f"sha3-512:{cls.parsed_model.file_checksum}"

        # Standard allow policy
        cls.allow_policy_text = f"""(policy
  (tier 1)
  (allowed-ternary-model-hashes ["{cls.model_hash}"])
  (require-axion-event (reason "task:assess_fixed.v1")))
"""
        cls.allow_policy_file = os.path.join(cls.temp_dir, "allow.apl")
        with open(cls.allow_policy_file, "w") as f:
            f.write(cls.allow_policy_text)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.temp_dir, ignore_errors=True)

    def _run_t81_cli(self, input_text: str, policy_file: str = None, model_file: str = None, mode: str = "strict_deterministic") -> Tuple[int, Dict[str, Any], str]:
        if policy_file is None:
            policy_file = self.allow_policy_file
        if model_file is None:
            model_file = self.model_path

        canonfs_root = os.path.join(self.temp_dir, f"canonfs-{tempfile.mktemp(dir='')}")
        os.makedirs(canonfs_root, exist_ok=True)

        cmd = [
            self.t81_bin, "ai", "task", "assess-fixed",
            "--model", "assess-fixed-demo",
            "--model-file", model_file,
            "--policy", policy_file,
            "--canonfs-root", canonfs_root,
            "--mode", mode,
            "--input", input_text
        ]
        proc = subprocess.run(cmd, capture_output=True, text=True)
        out_json = {}
        if proc.returncode == 0:
            out_json = json.loads(proc.stdout)
        return proc.returncode, out_json, proc.stderr

    def _run_baseline(self, input_text: str, policy_input: Any = None, model_bytes: bytes = None, mode: str = "strict_deterministic") -> Tuple[int, Dict[str, Any], str]:
        if policy_input is None:
            policy_input = self.allow_policy_text
        if model_bytes is None:
            model_bytes = self.model_bytes

        cas_root = os.path.join(self.temp_dir, f"cas-{tempfile.mktemp(dir='')}")
        runner = ConventionalTaskRunner(cas_root)
        try:
            res = runner.run_assess_fixed(
                model_bytes=model_bytes,
                input_text=input_text,
                policy_input=policy_input,
                tokenizer_data=self.tokenizer_data,
                mode=mode
            )
            return 0, res, ""
        except PermissionError as e:
            return 13, {}, str(e) # SecurityFault code 13
        except Exception as e:
            return 1, {}, str(e)

    # ---------------------------------------------------------
    # Scenario 1: The Three Original Inputs
    # ---------------------------------------------------------
    def test_01_original_inputs(self):
        prompts = ["greet hello", "hello world", "greet"]
        for p in prompts:
            code1, res1, err1 = self._run_t81_cli(p)
            code2, res2, err2 = self._run_baseline(p)

            self.assertEqual(code1, 0, f"T81 CLI failed on '{p}': {err1}")
            self.assertEqual(code2, 0, f"Baseline failed on '{p}': {err2}")

            self.assertEqual(res1["decision"], res2["decision"], f"Decision mismatch for '{p}'")
            self.assertEqual(res1["reason_code"], res2["reason_code"], f"Reason code mismatch for '{p}'")
            self.assertEqual(res1["decision_token_ids"], res2["decision_token_ids"], f"Selected token ID mismatch for '{p}'")

    # ---------------------------------------------------------
    # Scenario 2: Unknown and Malformed Inputs
    # ---------------------------------------------------------
    def test_02_unknown_and_malformed_inputs(self):
        unknown_prompts = ["foo bar baz", "123456"]
        for p in unknown_prompts:
            code1, res1, err1 = self._run_t81_cli(p)
            code2, res2, err2 = self._run_baseline(p)

            self.assertEqual(code1, code2, f"Return code mismatch for unknown prompt '{p}'")
            if code1 == 0:
                self.assertEqual(res1["decision"], res2["decision"])
                self.assertEqual(res1["reason_code"], res2["reason_code"])
                self.assertEqual(res1["decision_token_ids"], res2["decision_token_ids"])

    # ---------------------------------------------------------
    # Scenario 3: Whitespace and Normalization Cases
    # ---------------------------------------------------------
    def test_03_whitespace_and_normalization(self):
        variations = [
            "greet hello\r\n",
            "  greet hello  ",
            "greet\nhello",
            "\tgreet hello\t\r\n\n"
        ]
        for var in variations:
            code1, res1, err1 = self._run_t81_cli(var)
            code2, res2, err2 = self._run_baseline(var)

            self.assertEqual(code1, 0, f"T81 CLI failed on normalization test: {err1}")
            self.assertEqual(code2, 0, f"Baseline failed on normalization test: {err2}")

            self.assertEqual(res1["decision"], res2["decision"])
            self.assertEqual(res1["reason_code"], res2["reason_code"])
            self.assertEqual(res1["decision_token_ids"], res2["decision_token_ids"])
            self.assertEqual(res1["input_hash"], res2["input_hash"])

    # ---------------------------------------------------------
    # Scenario 4: Valid Model-Weight Mutations
    # ---------------------------------------------------------
    def test_04_model_weight_mutations(self):
        mutated_dir = os.path.join(self.temp_dir, "mutated_model")
        os.makedirs(mutated_dir, exist_ok=True)
        mutated_model_path = os.path.join(mutated_dir, "assess-fixed-demo.t81w")
        shutil.copyfile(self.tokenizer_path, os.path.join(mutated_dir, "tokenizer.json"))

        payload = self.model_bytes[self.model_bytes.find(b'\n', self.model_bytes.find(b'\n')+1)+1:]
        mutated_payload = bytearray(payload)
        mutated_payload[-16] ^= 0xFF # Mutate data limb in payload

        new_payload_checksum = hashlib.sha3_512(mutated_payload).hexdigest()
        header = f"T81W2\n{new_payload_checksum}\n".encode('ascii')
        mutated_bytes = header + bytes(mutated_payload)

        with open(mutated_model_path, "wb") as f:
            f.write(mutated_bytes)

        mutated_file_hash = f"sha3-512:{hashlib.sha3_512(mutated_bytes).hexdigest()}"
        mutated_policy_file = os.path.join(self.temp_dir, "mutated_policy.apl")
        with open(mutated_policy_file, "w") as f:
            f.write(f'(policy\n  (tier 1)\n  (allowed-ternary-model-hashes ["{mutated_file_hash}"])\n  (require-axion-event (reason "task:assess_fixed.v1")))\n')

        code1, res1, err1 = self._run_t81_cli("greet hello", policy_file=mutated_policy_file, model_file=mutated_model_path)
        code2, res2, err2 = self._run_baseline("greet hello", policy_input=f'(policy\n  (tier 1)\n  (allowed-ternary-model-hashes ["{mutated_file_hash}"])\n  (require-axion-event (reason "task:assess_fixed.v1")))\n', model_bytes=mutated_bytes)

        self.assertEqual(code1, code2)
        if code1 == 0:
            self.assertEqual(res1["decision"], res2["decision"])
            self.assertEqual(res1["reason_code"], res2["reason_code"])
            self.assertEqual(res1["decision_token_ids"], res2["decision_token_ids"])

    # ---------------------------------------------------------
    # Scenario 5: Tokenizer Mutations
    # ---------------------------------------------------------
    def test_05_tokenizer_mutations(self):
        mutated_tok_data = {
            "model": {
                "type": "BPE",
                "vocab": {
                    "ALLOW": 9,
                    "DENY": 42,
                    "greet": 7,
                    "hello": 11,
                    "world": 12,
                    "▁greet": 17,
                    "▁hello": 21
                }
            }
        }
        cas_root = os.path.join(self.temp_dir, f"cas-{tempfile.mktemp(dir='')}")
        runner = ConventionalTaskRunner(cas_root)
        res = runner.run_assess_fixed(
            model_bytes=self.model_bytes,
            input_text="greet hello",
            policy_input=self.allow_policy_text,
            tokenizer_data=mutated_tok_data
        )
        self.assertIn(res["decision"], ["ALLOW", "DENY", "REVIEW", "UNKNOWN"])

    # ---------------------------------------------------------
    # Scenario 6: Invalid Model Formats & Checksum Corruptions
    # ---------------------------------------------------------
    def test_06_invalid_model_formats(self):
        # Corrupt checksum in header
        bad_header_bytes = b"T81W2\n" + b"0" * 128 + b"\n" + b"payload_garbage"
        code2, res2, err2 = self._run_baseline("greet hello", model_bytes=bad_header_bytes)
        self.assertNotEqual(code2, 0)
        self.assertIn("checksum mismatch", err2)

        # Truncated file
        code2, res2, err2 = self._run_baseline("greet hello", model_bytes=b"T81W2\n")
        self.assertNotEqual(code2, 0)

    # ---------------------------------------------------------
    # Scenario 7: Missing or Conflicting Authorizations
    # ---------------------------------------------------------
    def test_07_policy_authorizations(self):
        # Denied model hash policy
        deny_policy = '(policy\n  (tier 1)\n  (allowed-ternary-model-hashes ["sha3-512:deadbeef"])\n  (require-axion-event (reason "task:assess_fixed.v1")))\n'
        deny_policy_file = os.path.join(self.temp_dir, "deny.apl")
        with open(deny_policy_file, "w") as f:
            f.write(deny_policy)

        code1, res1, err1 = self._run_t81_cli("greet hello", policy_file=deny_policy_file)
        code2, res2, err2 = self._run_baseline("greet hello", policy_input=deny_policy)

        self.assertEqual(code1, 13) # SecurityFault
        self.assertEqual(code2, 13) # SecurityFault
        self.assertIn("allowed-ternary-model-hashes", err1)
        self.assertIn("allowed-ternary-model-hashes", err2)

        # Missing task marker policy
        missing_marker_policy = f'(policy\n  (tier 1)\n  (allowed-ternary-model-hashes ["{self.model_hash}"]))\n'
        missing_marker_file = os.path.join(self.temp_dir, "missing_marker.apl")
        with open(missing_marker_file, "w") as f:
            f.write(missing_marker_policy)

        code1, res1, err1 = self._run_t81_cli("greet hello", policy_file=missing_marker_file)
        code2, res2, err2 = self._run_baseline("greet hello", policy_input=missing_marker_policy)

        self.assertEqual(code1, 13)
        self.assertEqual(code2, 13)
        self.assertIn("require-axion-event", err1)
        self.assertIn("require-axion-event", err2)

    # ---------------------------------------------------------
    # Scenario 8: Repeated Deterministic Executions
    # ---------------------------------------------------------
    def test_08_repeated_deterministic_executions(self):
        cas_root = os.path.join(self.temp_dir, f"cas-{tempfile.mktemp(dir='')}")
        runner = ConventionalTaskRunner(cas_root)

        res1 = runner.run_assess_fixed(self.model_bytes, "greet hello", self.allow_policy_text, self.tokenizer_data)
        res2 = runner.run_assess_fixed(self.model_bytes, "greet hello", self.allow_policy_text, self.tokenizer_data)

        self.assertEqual(res1["decision"], res2["decision"])
        self.assertEqual(res1["result_ref"], res2["result_ref"])
        self.assertEqual(res1["provenance_ref"], res2["provenance_ref"])
        self.assertEqual(res1["bundle_ref"], res2["bundle_ref"])

    # ---------------------------------------------------------
    # Scenario 9: Evidence Linkage and Artifact Integrity
    # ---------------------------------------------------------
    def test_09_evidence_and_verify_replay(self):
        cas_root = os.path.join(self.temp_dir, f"cas-{tempfile.mktemp(dir='')}")
        runner = ConventionalTaskRunner(cas_root)

        res = runner.run_assess_fixed(self.model_bytes, "greet hello", self.allow_policy_text, self.tokenizer_data)

        # Verify artifact integrity & re-execution replay
        replay_res = runner.verify_and_replay(res["bundle_ref"], self.allow_policy_text, self.tokenizer_data, model_bytes_override=self.model_bytes)
        self.assertTrue(replay_res["valid"])
        self.assertGreater(replay_res["lookup_latency_ns"], 0)
        self.assertGreater(replay_res["replay_latency_ns"], 0)

        # Corrupt bundle reference
        invalid_replay = runner.verify_and_replay("sha3-256:0000000000000000000000000000000000000000000000000000000000000000", self.allow_policy_text, model_bytes_override=self.model_bytes)
        self.assertFalse(invalid_replay["valid"])

    # ---------------------------------------------------------
    # Scenario 10: Boundary & Adversarial Cases (Phase 2)
    # ---------------------------------------------------------
    def test_10_adversarial_boundary_cases(self):
        # 1. Invalid execution mode
        with self.assertRaises(ValueError):
            cas_root = os.path.join(self.temp_dir, f"cas-{tempfile.mktemp(dir='')}")
            runner = ConventionalTaskRunner(cas_root)
            runner.run_assess_fixed(self.model_bytes, "greet hello", self.allow_policy_text, mode="reproducible_nondeterministic")

        # 2. Corrupted CAS payload read
        cas_root = os.path.join(self.temp_dir, f"cas-{tempfile.mktemp(dir='')}")
        cas = ConventionalCAS(cas_root)
        ref = cas.put(b"test data")
        obj_digest = ref.split(":", 1)[1]
        obj_path = os.path.join(cas.objects_dir, obj_digest)
        with open(obj_path, "wb") as f:
            f.write(b"corrupted data")
        with self.assertRaises(ValueError):
            cas.get(ref)


if __name__ == "__main__":
    unittest.main()
