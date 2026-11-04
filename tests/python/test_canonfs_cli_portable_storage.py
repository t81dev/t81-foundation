"""Exercise the native CLI without a shell, including Unicode references on Windows."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


class PortableStorageCLI(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="t81-cli-portable-")
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.root = self.base / "store"
        self.env = os.environ.copy()
        self.env.pop("T81_CANONFS_ROOT", None)

    def cli(self, *args, ok=True, root=None):
        result = subprocess.run(
            [str(CLI), "canonfs", *map(str, args), "--canonfs-root", str(root or self.root)],
            cwd=self.base, env=self.env, capture_output=True, encoding="utf-8", timeout=30,
        )
        if ok:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        return result

    def put(self, payload):
        source = self.base / "input.bin"
        source.write_bytes(payload)
        ref = self.cli("put-file", source).stdout.strip()
        self.assertTrue(ref.startswith("sha3-256:"), ref)
        return ref

    def test_canonical_round_trip_and_snapshot(self):
        # Also used by the storage-format tests; both identifiers contain < or >.
        vectors = [b"canonfs-read-verify-payload", b"canonfs-identity-contract",
                   bytes(range(256)) + b"\x00\r\n\xff"]
        expected = {}
        for payload in vectors:
            ref = self.put(payload)
            if payload in vectors[:2]:
                self.assertTrue(set(ref.removeprefix("sha3-256:")) & set("<>"), ref)
            key = "h1-" + hashlib.sha3_512(payload).digest()[:32].hex()
            block = self.root / "objects" / (key + ".blk")
            self.assertTrue(block.is_file())
            expected[key] = ref
            serialized = block.read_bytes()
            self.assertEqual(self.put(payload), ref)
            self.assertEqual(block.read_bytes(), serialized)
            listing = json.loads(self.cli("ls", "--json").stdout)
            self.assertEqual({item["hash"] for item in listing["objects"]}, set(expected.values()))
            self.assertIn(ref + "  ", self.cli("ls").stdout)
            restored = self.base / "restored.bin"
            self.cli("get", ref, "--out", restored)
            self.assertEqual(restored.read_bytes(), payload)
            for operation in ("stat", "verify"):
                info = json.loads(self.cli(operation, ref, "--json").stdout)
                self.assertTrue(info["ok"])
                self.assertEqual(info["hash"], ref)
                self.assertEqual(info["size_bytes"], len(payload))
                self.assertEqual(info["sha3_512"], hashlib.sha3_512(payload).hexdigest())
            self.assertEqual(block.read_bytes(), serialized)
        self.cli("snapshot", "--json")
        manifests = list((self.root / "snapshots").glob("*/MANIFEST.txt"))
        self.assertEqual(len(manifests), 1)
        entries = dict(line.split("\t") for line in manifests[0].read_text(encoding="utf-8").splitlines())
        for key, ref in expected.items():
            self.assertEqual(entries["objects/" + key + ".blk"], ref)

    def test_malformed_filenames_preserve_data(self):
        payload = b"canonfs-read-verify-payload"
        ref = self.put(payload)
        original = {p.name: p.read_bytes() for p in (self.root / "objects").iterdir()}
        malformed = ["h1-00", "h1-" + "A" * 64, "h1-" + "g" * 64,
                     "h2-" + "0" * 64, "h1-" + "0" * 65, "legacy"]
        for stem in malformed:
            with self.subTest(stem=stem):
                bad = self.root / "objects" / (stem + ".blk")
                bad.write_bytes(b"preserve-malformed")
                for flags in ((), ("--json",)):
                    result = self.cli("ls", *flags, ok=False)
                    self.assertIn("invalid portable object filename", result.stdout + result.stderr)
                    self.assertIn(bad.name, result.stdout + result.stderr)
                self.assertEqual(bad.read_bytes(), b"preserve-malformed")
                for name, data in original.items():
                    self.assertEqual((self.root / "objects" / name).read_bytes(), data)
                bad.unlink()
                restored = self.base / "restored.bin"
                self.cli("get", ref, "--out", restored)
                self.assertEqual(restored.read_bytes(), payload)
                self.cli("verify", ref)

    def test_legacy_store_requires_explicit_migration(self):
        objects = self.root / "objects"
        objects.mkdir(parents=True)
        legacy = objects / "legacy.blk"
        legacy.write_bytes(b"preserve-legacy")
        source = self.base / "input.bin"
        source.write_bytes(b"new-object")
        result = self.cli("put-file", source, ok=False)
        self.assertIn("requires explicit migration", result.stdout + result.stderr)
        self.assertEqual(legacy.read_bytes(), b"preserve-legacy")
        self.assertFalse((self.root / ".canonfs-format").exists())
        self.assertEqual(list(objects.iterdir()), [legacy])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", type=Path, required=True)
    args, remaining = parser.parse_known_args()
    CLI = args.cli.resolve()
    unittest.main(argv=[__file__, *remaining], verbosity=2)
