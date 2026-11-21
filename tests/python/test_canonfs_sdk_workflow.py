"""Tests for CanonRef digest reconstruction and the two-process Python SDK workflow."""

from __future__ import annotations

import json
import shlex
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import t81

REPO_ROOT = Path(__file__).resolve().parents[2]
WORKFLOW_SCRIPT = REPO_ROOT / "examples" / "python" / "canonfs_decision_workflow.py"


class CanonRefAndWorkflowTest(unittest.TestCase):
    def test_printed_commands_reuse_digests_in_fresh_processes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            first = subprocess.run(
                [
                    sys.executable,
                    str(WORKFLOW_SCRIPT),
                    "run",
                    "--store",
                    str(Path(tmp) / "store with spaces"),
                    "--score",
                    "75",
                    "--next-approve-threshold",
                    "80",
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            first_record = json.loads(first.stdout)
            self.assertEqual(first_record["decision"], 1)
            read, rerun = (shlex.split(line.split(": ", 1)[1]) for line in first.stderr.splitlines())
            read_back = subprocess.check_output(read, text=True)
            self.assertEqual(read_back, first_record["payload"] + "\n")
            second = subprocess.run(rerun, check=True, capture_output=True, text=True)
            second_record = json.loads(second.stdout)
            self.assertEqual(second_record["decision"], 0)
            read, compare = (shlex.split(line.split(": ", 1)[1]) for line in second.stderr.splitlines())
            read_back = subprocess.check_output(read, text=True)
            self.assertEqual(read_back, second_record["payload"] + "\n")
            comparison = json.loads(subprocess.check_output(compare, text=True))
            self.assertEqual(comparison["digest_a"], first_record["digest"])
            self.assertEqual(comparison["digest_b"], second_record["digest"])
            self.assertFalse(comparison["same_bytes"])

    def test_canonref_surface_on_existing_export(self) -> None:
        self.assertIn("CanonRef", t81.__all__)
        self.assertTrue(callable(t81.CanonRef.from_bytes))
        self.assertTrue(callable(t81.CanonRef.from_hex))
        self.assertTrue(callable(t81.CanonRef.hex))

    def test_canonref_roundtrip_and_value_semantics(self) -> None:
        driver = t81.make_in_memory_driver()
        payload = b'{"decision":1,"score":75}'
        ref = driver.write_object(t81.ObjectType.RawBlock, payload)

        digest_hex = ref.hex()
        digest_bytes = ref.hash_bytes

        self.assertEqual(len(digest_bytes), 32)
        self.assertEqual(len(digest_hex), 64)
        self.assertEqual(digest_hex, digest_bytes.hex())

        from_hex = t81.CanonRef.from_hex(digest_hex)
        from_upper = t81.CanonRef.from_hex(digest_hex.upper())
        from_bytes = t81.CanonRef.from_bytes(digest_bytes)

        self.assertEqual(from_hex, ref)
        self.assertEqual(from_upper, ref)
        self.assertEqual(from_bytes, ref)
        self.assertEqual(from_upper.hex(), digest_hex)
        self.assertEqual(hash(from_hex), hash(ref))
        self.assertEqual(hash(from_bytes), hash(ref))
        self.assertIn(from_hex, {ref})
        self.assertNotEqual(ref, digest_hex)
        self.assertIn(digest_hex, repr(ref))
        self.assertEqual(driver.read_object_bytes(from_hex), payload)
        self.assertEqual(driver.read_object_bytes(from_bytes), payload)

    def test_malformed_hex_digests_rejected(self) -> None:
        valid = "ab" * 32
        malformed_strings = [
            "",
            "ab" * 31,  # 62 chars (too short)
            "a" * 63,  # 63 chars
            "a" * 65,  # 65 chars (too long)
            "z" * 64,  # non-hex
            "g" + "0" * 63,  # non-hex boundary
            " " + valid,  # leading whitespace
            valid + "\n",  # trailing newline
            "0x" + valid[:62],  # 0x prefix
            "0x" + valid,  # 0x prefix + full length
            "h1-" + valid[:61],  # filesystem storage key prefix
            "h1-" + valid,  # full filesystem storage key
        ]
        for bad in malformed_strings:
            with self.subTest(bad=bad):
                with self.assertRaises(ValueError):
                    t81.CanonRef.from_hex(bad)

        for wrong_type in (None, 123, b"\x00" * 32, b"00" * 32):
            with self.subTest(wrong_type=wrong_type):
                with self.assertRaises(TypeError):
                    t81.CanonRef.from_hex(wrong_type)  # type: ignore[arg-type]

    def test_malformed_bytes_digests_rejected(self) -> None:
        for bad in (b"", b"\x00" * 31, b"\x00" * 33, b"\xff" * 64):
            with self.subTest(length=len(bad)):
                with self.assertRaises(ValueError):
                    t81.CanonRef.from_bytes(bad)

        for wrong_type in (None, 123, "00" * 32):
            with self.subTest(wrong_type=wrong_type):
                with self.assertRaises(TypeError):
                    t81.CanonRef.from_bytes(wrong_type)  # type: ignore[arg-type]

    def test_valid_absent_digest_fails_closed(self) -> None:
        absent_zero = t81.CanonRef.from_hex("00" * 32)
        absent_ff = t81.CanonRef.from_bytes(b"\xff" * 32)

        mem = t81.make_in_memory_driver()
        for ref in (absent_zero, absent_ff):
            with self.assertRaises(RuntimeError) as ctx:
                mem.read_object_bytes(ref)
            self.assertIn("NotFound", str(ctx.exception))

        with tempfile.TemporaryDirectory() as tmp:
            pers = t81.make_persistent_driver(tmp)
            for ref in (absent_zero, absent_ff):
                with self.assertRaises(RuntimeError) as ctx:
                    pers.read_object_bytes(ref)
                self.assertIn("NotFound", str(ctx.exception))

    def test_two_process_roundtrip_hex_and_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            store = Path(tmp) / "canonfs_store"

            # Process 1: execute T81Lang decision table and persist result bytes.
            run_proc = subprocess.run(
                [
                    sys.executable,
                    str(WORKFLOW_SCRIPT),
                    "run",
                    "--store",
                    str(store),
                    "--score",
                    "75",
                    "--risk-flag",
                    "0",
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            written = json.loads(run_proc.stdout)
            self.assertEqual(written["decision"], 1)
            digest_hex = written["digest"]
            expected_bytes = written["payload"].encode("utf-8")
            self.assertEqual(len(digest_hex), 64)

            # Process 2 (fresh interpreter): retrieve exact bytes using only store path + hex digest.
            read_proc = subprocess.run(
                [
                    sys.executable,
                    str(WORKFLOW_SCRIPT),
                    "read",
                    "--store",
                    str(store),
                    "--digest",
                    digest_hex,
                ],
                check=True,
                capture_output=True,
            )
            self.assertEqual(read_proc.stdout.rstrip(b"\n"), expected_bytes)

            # Process 3 (fresh interpreter): retrieve exact bytes using only store path + raw 32 bytes.
            raw_read_code = (
                "import sys, t81\n"
                "driver = t81.make_persistent_driver(sys.argv[1])\n"
                "ref = t81.CanonRef.from_bytes(bytes.fromhex(sys.argv[2]))\n"
                "sys.stdout.buffer.write(driver.read_object_bytes(ref))\n"
            )
            raw_proc = subprocess.run(
                [sys.executable, "-c", raw_read_code, str(store), digest_hex],
                check=True,
                capture_output=True,
            )
            self.assertEqual(raw_proc.stdout, expected_bytes)

    def test_modify_program_rerun_and_compare_in_fresh_processes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            store = Path(tmp) / "canonfs_store"

            # Run A: default approve threshold (70) with score 75 -> approve (+1).
            res_a1 = json.loads(
                subprocess.run(
                    [
                        sys.executable,
                        str(WORKFLOW_SCRIPT),
                        "run",
                        "--store",
                        str(store),
                        "--score",
                        "75",
                        "--approve-threshold",
                        "70",
                    ],
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout
            )
            # Repeat Run A: deterministic execution + content addressing -> identical digest.
            res_a2 = json.loads(
                subprocess.run(
                    [
                        sys.executable,
                        str(WORKFLOW_SCRIPT),
                        "run",
                        "--store",
                        str(store),
                        "--score",
                        "75",
                        "--approve-threshold",
                        "70",
                    ],
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout
            )
            self.assertEqual(res_a1["decision"], 1)
            self.assertEqual(res_a1["digest"], res_a2["digest"])

            # Run B: raise approve threshold to 80 with same score 75 -> review (0), new digest.
            res_b = json.loads(
                subprocess.run(
                    [
                        sys.executable,
                        str(WORKFLOW_SCRIPT),
                        "run",
                        "--store",
                        str(store),
                        "--score",
                        "75",
                        "--approve-threshold",
                        "80",
                    ],
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout
            )
            self.assertEqual(res_b["decision"], 0)
            self.assertNotEqual(res_a1["digest"], res_b["digest"])

            # Fresh process compares both stored artifacts by digest.
            cmp_res = json.loads(
                subprocess.run(
                    [
                        sys.executable,
                        str(WORKFLOW_SCRIPT),
                        "compare",
                        "--store",
                        str(store),
                        "--digest-a",
                        res_a1["digest"],
                        "--digest-b",
                        res_b["digest"],
                    ],
                    check=True,
                    capture_output=True,
                    text=True,
                ).stdout
            )
            self.assertFalse(cmp_res["same_digest"])
            self.assertFalse(cmp_res["same_bytes"])
            self.assertEqual(cmp_res["record_a"]["decision"], 1)
            self.assertEqual(cmp_res["record_b"]["decision"], 0)


if __name__ == "__main__":
    unittest.main()
