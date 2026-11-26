"""Exercise the installed Python package without importing the source tree."""

import doctest
import operator
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import t81


class InstalledSdkTest(unittest.TestCase):
    def test_package_examples(self):
        result = doctest.testmod(t81)
        self.assertGreater(result.attempted, 0)
        self.assertEqual(result.failed, 0)

    def test_step_limit_is_resumable_and_convenience_api_checks_halt(self):
        source = "fn main() -> i32 { return 42; }"
        vm = t81.VM()
        vm.load_program(t81.compile(source))
        for limit in (0, 1):
            with self.assertRaises(t81.StepLimitExceeded):
                vm.run_to_halt(max_steps=limit)
            self.assertFalse(vm.halted)
        self.assertEqual(vm.get_register(2), 0)
        self.assertIsNone(vm.run_to_halt())
        self.assertTrue(vm.halted)
        self.assertEqual(vm.get_register(2), 42)
        self.assertIsNone(vm.run_to_halt(max_steps=0))
        with self.assertRaises(t81.StepLimitExceeded):
            t81.compile_and_run(source, max_steps=1)
        self.assertEqual(t81.compile_and_run(source), 42)

    def test_axion_events_are_read_only_snapshots(self):
        vm = t81.VM()
        vm.load_program(t81.compile("@axion_verify\nfn main() -> i32 { return 7; }"))
        vm.run_to_halt()
        events = vm.axion_events
        self.assertEqual([event.reason for event in events], vm.axion_log)
        policy = next(event for event in events if event.reason == "Default policy: allow all")
        self.assertEqual(policy.verdict, t81.VerdictKind.Allow)
        self.assertIsInstance(policy.pc, int)
        self.assertIsInstance(policy.opcode, int)
        self.assertIsInstance(policy.reason_code, str)
        with self.assertRaises(AttributeError):
            policy.reason = "changed"
        vm.load_program(t81.compile("fn main() -> i32 { return 0; }"))
        self.assertEqual(policy.reason, "Default policy: allow all")

    def test_dynamic_tensor_rejects_mismatched_replacement_before_mutation(self):
        for cls, wrap in ((t81.T729DynamicTensor, float), (t81.T729IntTensor, t81.T81Int)):
            with self.subTest(tensor=cls.__name__):
                original = [wrap(i) for i in range(4)]
                tensor = cls([2, 2], original)
                with self.assertRaisesRegex(ValueError, "data size mismatch"):
                    tensor.data = original[:3]
                self.assertEqual(tensor.shape, [2, 2])
                self.assertEqual(tensor.size(), 4)
                self.assertEqual(list(map(int, tensor.data)), [0, 1, 2, 3])
                tensor.data = list(reversed(original))
                self.assertEqual(list(map(int, tensor.data)), [3, 2, 1, 0])

    def test_fixed_tensors_support_python_indices_and_exact_integer_conversion(self):
        vector = t81.Tensor1D3()
        matrix = t81.Tensor2D33()
        vector[-1] = t81.T81Int(-7)
        matrix[-1, -2] = t81.T81Int(11)
        self.assertEqual(int(vector[2]), -7)
        self.assertEqual(operator.index(matrix[2, 1]), 11)
        for index in (-4, 3, 1 << 100):
            with self.assertRaises(IndexError):
                _ = vector[index]
        with self.assertRaises(IndexError):
            _ = matrix[0, -4]
        with self.assertRaises(TypeError):
            _ = matrix[0, 0.5]
        for sign in (1, -1):
            for cls in (t81.T81Int, t81.BigInt):
                wide = cls(sign * 2**32) * cls(2**32)
                self.assertEqual(int(wide), sign * 2**64)

    def test_canonfs_digest_recovers_fixture_in_another_process(self):
        with tempfile.TemporaryDirectory() as tmp:
            store = str(Path(tmp) / "fixture-store")
            driver = t81.make_persistent_driver(store)
            payload = b"123\n\x00\xff"
            ref = driver.write_object(t81.ObjectType.RawBlock, payload)
            self.assertTrue(ref.to_cli_handle().startswith("sha3-256:"))
            output = subprocess.check_output(
                [
                    sys.executable, "-c",
                    "import sys,t81; ref=t81.CanonRef.from_hex(sys.argv[2]); "
                    "driver=t81.make_persistent_driver(sys.argv[1]); "
                    "sys.stdout.buffer.write(driver.read_object_bytes(ref))",
                    store, ref.hex(),
                ]
            )
            self.assertEqual(output, payload)

    def test_m3_tensor_matmul_and_zero_copy_buffer_protocol(self):
        v1 = t81.Tensor1D3()
        v2 = t81.Tensor1D3()
        for i, val in enumerate((1, 0, -1)):
            v1[i] = t81.T81Int(val)
            v2[i] = t81.T81Int(val)
        self.assertEqual(int(v1 @ v2), 2)

        a = t81.T729IntTensor([2, 3], [t81.T81Int(x) for x in (1, 0, -1, -1, 1, 0)])
        b = t81.T729IntTensor([3, 2], [t81.T81Int(x) for x in (1, -1, 0, 1, -1, 1)])
        c = a @ b
        self.assertEqual(c.shape, [2, 2])
        self.assertEqual([int(x) for x in c.data], [2, -2, -1, 2])

        dyn = t81.T729DynamicTensor([2, 2], [1.5, -2.0, 0.5, 4.0])
        mv = memoryview(dyn)
        self.assertEqual(mv.shape, (2, 2))
        self.assertEqual(mv.format, "f")
        self.assertEqual(dyn.__array_interface__["shape"], (2, 2))

    def test_m3_provider_governor_and_native_ed25519_verify_dag(self):
        kp = t81.ed25519_generate_keypair()
        s1 = t81.sign_rfc00d4_payload(
            kp["private_key_pem"], '{"action_type":"a"}', '{"h":"1"}', '{"verdict":"Allow"}'
        )
        s2 = t81.sign_rfc00d4_payload(
            kp["private_key_pem"], '{"action_type":"b"}', '{"h":"2"}', '{"verdict":"Allow"}'
        )
        ok = t81.verify_dag([
            {
                "bundle_id": "b1",
                "parent_bundle_ids": [],
                "canonical_signing_payload": s1["canonical_signing_payload"],
                "signature_b64": s1["signature_b64"],
                "public_key_pem": s1["public_key_pem"],
            },
            {
                "bundle_id": "b2",
                "parent_bundle_ids": ["b1"],
                "canonical_signing_payload": s2["canonical_signing_payload"],
                "signature_b64": s2["signature_b64"],
                "public_key_pem": s2["public_key_pem"],
            },
        ])
        self.assertTrue(ok["ok"])
        self.assertEqual(ok["verified_nodes"], 2)

        bad = t81.verify_dag([
            {
                "bundle_id": "b1",
                "parent_bundle_ids": [],
                "canonical_signing_payload": s1["canonical_signing_payload"] + " ",
                "signature_b64": s1["signature_b64"],
                "public_key_pem": s1["public_key_pem"],
            },
        ])
        self.assertFalse(bad["ok"])
        self.assertEqual(bad["code"], "ERR_RECEIPT_VERIFY_FAILED")

        policy = '(policy (tier 1))'
        gov = t81.ProviderGovernor(policy)
        cp = t81.ControlledProvider()
        denied = gov.execute_controlled(cp, "hello", user_consent_verified=False, seed=42)
        self.assertFalse(denied["ok"])
        allowed = gov.execute_controlled(cp, "hello", user_consent_verified=True, seed=42)
        self.assertTrue(allowed["ok"])
        self.assertEqual(allowed["trit"], 1)


if __name__ == "__main__":
    unittest.main()
