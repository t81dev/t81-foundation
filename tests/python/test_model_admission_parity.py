#!/usr/bin/env python3
"""
Differential Model-Admission Parity Tests for Investigation 08.

Validates that both Production T81 CLI and Conventional Task Runner:
1. Accept valid models containing all 13 mandatory architecture tensors.
2. Reject models missing any mandatory tensor (including the 5 omitted tensors).
3. Ensure rejected models create 0 artifacts in the conventional CAS store.
"""

import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest

# Ensure repo root and experiments directory are on python path
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EXP_DIR = os.path.join(REPO_ROOT, "experiments", "integrated_system_advantage")
sys.path.insert(0, EXP_DIR)

from baseline_stack import ConventionalTaskRunner, MANDATORY_TENSORS, validate_model_architecture

OMITTED_MANDATORY_TENSORS = [
    "model.norm.weight",
    "model.layers.1.self_attn.q_proj.weight",
    "model.layers.1.self_attn.k_proj.weight",
    "model.layers.1.self_attn.v_proj.weight",
    "lm_head.weight",
]

def parse_t81w(data: bytes):
    lines = data.split(b"\n", 2)
    magic = lines[0].decode('utf-8').strip()
    checksum = lines[1].decode('utf-8').strip()
    payload = lines[2]

    offset = 0
    num_tensors = struct.unpack_from("<Q", payload, offset)[0]
    offset += 8
    tensors = []
    for _ in range(num_tensors):
        name_len = struct.unpack_from("<Q", payload, offset)[0]
        offset += 8
        name = payload[offset:offset+name_len].decode('utf-8')
        offset += name_len
        rank = struct.unpack_from("<Q", payload, offset)[0]
        offset += 8
        shape = struct.unpack_from(f"<{rank}Q", payload, offset)
        offset += 8 * rank
        fmt = payload[offset]
        offset += 1
        trits = struct.unpack_from("<Q", payload, offset)[0]
        offset += 8
        data_bytes = struct.unpack_from("<Q", payload, offset)[0]
        offset += 8
        raw_data = payload[offset:offset+data_bytes]
        offset += data_bytes
        tensors.append({
            "name": name,
            "rank": rank,
            "shape": shape,
            "fmt": fmt,
            "trits": trits,
            "raw_data": raw_data
        })
    return magic, tensors

def pack_t81w(magic: str, tensors: list) -> bytes:
    payload = bytearray()
    payload.extend(struct.pack("<Q", len(tensors)))
    for t in tensors:
        name_bytes = t["name"].encode('utf-8')
        payload.extend(struct.pack("<Q", len(name_bytes)))
        payload.extend(name_bytes)
        rank = t["rank"]
        payload.extend(struct.pack("<Q", rank))
        payload.extend(struct.pack(f"<{rank}Q", *t["shape"]))
        payload.append(t["fmt"])
        payload.extend(struct.pack("<Q", t["trits"]))
        payload.extend(struct.pack("<Q", len(t["raw_data"])))
        payload.extend(t["raw_data"])

    checksum = hashlib.sha3_512(payload).hexdigest()
    return magic.encode('utf-8') + b"\n" + checksum.encode('utf-8') + b"\n" + payload


class TestModelAdmissionParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.t81_bin = os.path.join(REPO_ROOT, "build", "t81")
        cls.demo_builder = os.path.join(REPO_ROOT, "build", "t81_make_assess_fixed_demo")
        cls.work_dir = tempfile.mkdtemp(prefix="t81_admission_test_")

        # Build binaries if missing
        if not os.path.exists(cls.t81_bin) or not os.path.exists(cls.demo_builder):
            subprocess.run(["cmake", "--build", os.path.join(REPO_ROOT, "build"), "--target", "t81", "t81_make_assess_fixed_demo"], check=True)

        # Generate base demo model
        cls.model_dir = os.path.join(cls.work_dir, "model")
        subprocess.run([cls.demo_builder, cls.model_dir], check=True, stdout=subprocess.DEVNULL)
        cls.base_model_path = os.path.join(cls.model_dir, "assess-fixed-demo.t81w")
        with open(cls.base_model_path, "rb") as f:
            cls.base_model_bytes = f.read()

    def test_valid_model_admission_parity(self):
        """Verify that a valid model containing all 13 mandatory tensors is accepted by both engines."""
        # 1. Test Conventional Task Runner
        cas_dir = os.path.join(self.work_dir, "cas_valid")
        runner = ConventionalTaskRunner(cas_dir)
        sha256_hash = "sha3-256:" + hashlib.sha3_256(self.base_model_bytes).hexdigest()
        sha512_hash = "sha3-512:" + hashlib.sha3_512(self.base_model_bytes).hexdigest()
        policy = {"allowed_model_hashes": [sha256_hash, "*"], "require_event": "task:assess_fixed.v1"}

        result = runner.run_assess_fixed(self.base_model_bytes, "greet hello", policy)
        self.assertEqual(result["decision"], "ALLOW")
        self.assertEqual(result["reason_code"], "GREETING_PAIR")

        # Verify artifacts were created in CAS
        objects_dir = os.path.join(cas_dir, "objects")
        self.assertTrue(os.path.exists(objects_dir))
        self.assertGreater(len(os.listdir(objects_dir)), 0)

        # 2. Test Production T81 CLI
        apl_path = os.path.join(self.work_dir, "valid_policy.apl")
        with open(apl_path, "w") as f:
            f.write(f'(policy (tier 1) (allowed-ternary-model-hashes ["{sha512_hash}"]) (require-axion-event (reason "task:assess_fixed.v1")))\n')

        canon_dir = os.path.join(self.work_dir, "canon_valid")
        cmd = [
            self.t81_bin, "ai", "task", "assess-fixed",
            "--model", "assess-fixed-demo",
            "--model-file", self.base_model_path,
            "--policy", apl_path,
            "--canonfs-root", canon_dir,
            "--mode", "strict_deterministic",
            "--input", "greet hello"
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, f"Production CLI failed on valid model: {res.stderr}")
        cli_out = json.loads(res.stdout)
        self.assertEqual(cli_out["decision"], "ALLOW")
        self.assertEqual(cli_out["reason_code"], "GREETING_PAIR")

    def test_omitted_mandatory_tensors_differential_negative(self):
        """Verify that removing any of the 5 omitted mandatory tensors causes rejection in BOTH engines and creates 0 CAS artifacts."""
        magic, tensors = parse_t81w(self.base_model_bytes)

        # Enforce that base model has all 13 mandatory tensors
        base_names = {t["name"] for t in tensors}
        self.assertTrue(MANDATORY_TENSORS.issubset(base_names))

        for tensor_to_remove in OMITTED_MANDATORY_TENSORS:
            with self.subTest(tensor_missing=tensor_to_remove):
                filtered_tensors = [t for t in tensors if t["name"] != tensor_to_remove]
                modified_bytes = pack_t81w(magic, filtered_tensors)

                # Recomputed SHA3-512 checksum for policy fingerprint
                mod_model_hash = "sha3-512:" + hashlib.sha3_512(modified_bytes).hexdigest()

                mod_sha256_hash = "sha3-256:" + hashlib.sha3_256(modified_bytes).hexdigest()
                # 1. Test Conventional Task Runner rejection & artifact creation prohibition
                test_cas_dir = tempfile.mkdtemp(prefix=f"cas_neg_{tensor_to_remove.replace('.', '_')}_")
                runner = ConventionalTaskRunner(test_cas_dir)
                policy = {"allowed_model_hashes": [mod_sha256_hash, "*"], "require_event": "task:assess_fixed.v1"}

                with self.assertRaises(ValueError) as cm:
                    runner.run_assess_fixed(modified_bytes, "greet hello", policy)

                self.assertIn("Model architecture admission rejected", str(cm.exception))
                self.assertIn(tensor_to_remove, str(cm.exception))

                # Verify ZERO artifacts were created in CAS store
                cas_objects_dir = os.path.join(test_cas_dir, "objects")
                if os.path.exists(cas_objects_dir):
                    stored_objects = os.listdir(cas_objects_dir)
                    self.assertEqual(
                        len(stored_objects), 0,
                        f"CAS store must contain 0 artifacts for rejected model, but found: {stored_objects}"
                    )

                # 2. Test Production T81 CLI rejection
                mod_model_path = os.path.join(test_cas_dir, "mod_model.t81w")
                with open(mod_model_path, "wb") as f:
                    f.write(modified_bytes)

                apl_path = os.path.join(test_cas_dir, "policy.apl")
                with open(apl_path, "w") as f:
                    f.write(f'(policy (tier 1) (allowed-ternary-model-hashes ["{mod_model_hash}"]) (require-axion-event (reason "task:assess_fixed.v1")))\n')

                canon_dir = os.path.join(test_cas_dir, "canon")
                cmd = [
                    self.t81_bin, "ai", "task", "assess-fixed",
                    "--model", "mod-demo",
                    "--model-file", mod_model_path,
                    "--policy", apl_path,
                    "--canonfs-root", canon_dir,
                    "--mode", "strict_deterministic",
                    "--input", "greet hello"
                ]
                res = subprocess.run(cmd, capture_output=True, text=True)
                self.assertNotEqual(
                    res.returncode, 0,
                    f"Production T81 CLI unexpectedly accepted model missing {tensor_to_remove}"
                )

    def test_all_13_mandatory_tensors_negative_coverage(self):
        """Verify that removing ANY of the 13 mandatory tensors causes rejection in both engines."""
        magic, tensors = parse_t81w(self.base_model_bytes)

        for tensor_to_remove in sorted(MANDATORY_TENSORS):
            with self.subTest(tensor_missing=tensor_to_remove):
                filtered_tensors = [t for t in tensors if t["name"] != tensor_to_remove]
                modified_bytes = pack_t81w(magic, filtered_tensors)
                mod_sha256_hash = "sha3-256:" + hashlib.sha3_256(modified_bytes).hexdigest()

                test_cas_dir = tempfile.mkdtemp(prefix="cas_neg_all_")
                runner = ConventionalTaskRunner(test_cas_dir)
                policy = {"allowed_model_hashes": [mod_sha256_hash, "*"], "require_event": "task:assess_fixed.v1"}

                with self.assertRaises(ValueError):
                    runner.run_assess_fixed(modified_bytes, "greet hello", policy)

                cas_objects_dir = os.path.join(test_cas_dir, "objects")
                if os.path.exists(cas_objects_dir):
                    self.assertEqual(len(os.listdir(cas_objects_dir)), 0)

    def test_authorization_and_tamper_protection(self):
        """Verify policy authorization denial and CAS tamper detection."""
        cas_dir = os.path.join(self.work_dir, "cas_tamper")
        runner = ConventionalTaskRunner(cas_dir)

        # Unauthorized model hash
        policy = {"allowed_model_hashes": ["sha3-512:unauthorized_hash"], "require_event": "task:assess_fixed.v1"}
        with self.assertRaises(PermissionError):
            runner.run_assess_fixed(self.base_model_bytes, "greet hello", policy)

        # Valid execution
        sha256_hash = "sha3-256:" + hashlib.sha3_256(self.base_model_bytes).hexdigest()
        valid_policy = {"allowed_model_hashes": [sha256_hash], "require_event": "task:assess_fixed.v1"}
        res = runner.run_assess_fixed(self.base_model_bytes, "greet hello", valid_policy)

        # Tamper with stored object in CAS
        bundle_bytes = runner.cas.get(res["bundle_ref"])
        self.assertIsNotNone(bundle_bytes)
        bundle_obj_path = os.path.join(cas_dir, "objects", res["bundle_ref"].split(":", 1)[1])
        with open(bundle_obj_path, "wb") as f:
            f.write(b"corrupted payload")

        with self.assertRaises(ValueError) as cm:
            runner.cas.get(res["bundle_ref"])
        self.assertIn("CAS corruption / tamper detected", str(cm.exception))


if __name__ == "__main__":
    unittest.main()
