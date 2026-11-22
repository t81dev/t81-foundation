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


if __name__ == "__main__":
    unittest.main()
