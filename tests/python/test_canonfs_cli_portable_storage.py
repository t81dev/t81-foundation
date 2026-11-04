"""Exercise the native CLI without a shell, including Unicode references on Windows."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest



DIGITS = [
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X", "Y", "Z",
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
    "t", "u", "v", "w", "x", "y", "z",
    "-", "−", "×", "÷", "=", "<", ">", "≤", "≥", "≠", "≈", "∞", "λ", "μ", "π", "σ", "τ", "ω", "Γ"
]


def encode_base81(raw_bytes: bytes) -> str:
    if not raw_bytes:
        return ""
    val = int.from_bytes(raw_bytes, "big")
    if val == 0:
        return DIGITS[0]
    digits = []
    while val > 0:
        digits.append(DIGITS[val % 81])
        val //= 81
    return "".join(reversed(digits))


def canon_hash81(data: bytes) -> str:
    raw_32 = hashlib.sha3_512(data).digest()[:32]
    return encode_base81(raw_32)


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
        snapshot = json.loads(self.cli("snapshot", "--json").stdout)
        public_id = snapshot["hash"]
        self.assertTrue(public_id)
        manifests = list((self.root / "snapshots").glob("*/MANIFEST.txt"))
        self.assertEqual(len(manifests), 1)
        manifest_bytes = manifests[0].read_bytes()
        self.assertEqual(manifests[0].parent.name,
                         "h1-" + hashlib.sha3_512(manifest_bytes).digest()[:32].hex())
        self.assertNotEqual(manifests[0].parent.name, public_id)
        self.assertFalse(set("<>:\\|?*") & set(manifests[0].parent.name))
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



    def test_complete_object_lifecycle_and_stat_verify(self):
        payload = b"lifecycle-verification-" + bytes(range(64))
        ref = self.put(payload)
        key = "h1-" + hashlib.sha3_512(payload).digest()[:32].hex()
        self.assertTrue((self.root / "objects" / f"{key}.blk").exists())

        # ls
        listing = json.loads(self.cli("ls", "--json").stdout)
        found = [o for o in listing["objects"] if o["hash"] == ref]
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0]["size_bytes"], len(payload))

        # get
        out_file = self.base / "got.bin"
        self.cli("get", ref, "--out", out_file)
        self.assertEqual(out_file.read_bytes(), payload)

        # stat
        stat_res = json.loads(self.cli("stat", ref, "--json").stdout)
        self.assertTrue(stat_res["ok"])
        self.assertEqual(stat_res["hash"], ref)
        self.assertEqual(stat_res["size_bytes"], len(payload))
        self.assertEqual(stat_res["sha3_512"], hashlib.sha3_512(payload).hexdigest())

        # verify
        ver_res = json.loads(self.cli("verify", ref, "--json").stdout)
        self.assertTrue(ver_res["ok"])
        self.assertEqual(ver_res["hash"], ref)

    def test_snapshot_reserved_characters_and_determinism(self):
        p1 = b"canonfs-read-verify-payload"
        ref1 = self.put(p1)
        self.assertTrue(set(ref1.removeprefix("sha3-256:")) & set("<>"), ref1)

        snap1 = json.loads(self.cli("snapshot", "--json").stdout)
        hash1 = snap1["hash"]
        snap1_repeat = json.loads(self.cli("snapshot", "--json").stdout)
        self.assertEqual(snap1_repeat["hash"], hash1)

        snap_dirs = list((self.root / "snapshots").iterdir())
        self.assertEqual(len(snap_dirs), 1)
        dir_name = snap_dirs[0].name
        self.assertTrue(dir_name.startswith("h1-"))
        self.assertEqual(len(dir_name), 67)
        self.assertFalse(set("<>:\\|?*") & set(dir_name))

    def test_snapshot_diff_and_rollback(self):
        p1 = b"initial-state-data"
        self.put(p1)
        snap1 = json.loads(self.cli("snapshot", "--json").stdout)["hash"]

        p2 = b"second-state-data"
        self.put(p2)
        snap2 = json.loads(self.cli("snapshot", "--json").stdout)["hash"]
        self.assertNotEqual(snap1, snap2)

        # snapshot-diff
        diff_res = self.cli("snapshot-diff", snap1, snap2, "--json", ok=False)
        diff_data = json.loads(diff_res.stdout)
        self.assertFalse(diff_data["ok"])
        self.assertEqual(diff_data["lhs"], snap1)
        self.assertEqual(diff_data["rhs"], snap2)
        self.assertTrue(any("objects/" in line for line in diff_data["only_rhs"]))

        # rollback dry-run
        dry_res = self.cli("rollback", "--dry-run", "--to", snap1, "--json")
        self.assertTrue(json.loads(dry_res.stdout)["ok"])

        # rollback execution
        rb_res = self.cli("rollback", "--to", snap1, "--json")
        self.assertTrue(json.loads(rb_res.stdout)["ok"])

        # Verify payload file restored across rollback
        # Note: CanonFS content-addressed storage preserves immutable objects in objects/,
        # while rollback restores root-level non-objects files (and snapshot manifests).
        # We test extra tree files restoration here:
        extra_file = self.root / "state_data.txt"
        extra_file.write_text("v2-data", encoding="utf-8")
        self.cli("rollback", "--to", snap1, "--json")
        self.assertFalse(extra_file.exists())

        # Verify get of original object p1 succeeds
        ls_after = json.loads(self.cli("ls", "--json").stdout)
        obj_hashes = {o["hash"] for o in ls_after["objects"]}
        ref1 = "sha3-256:" + canon_hash81(p1)
        self.assertIn(ref1, obj_hashes)
        restored = self.base / "restored1.bin"
        self.cli("get", ref1, "--out", restored)
        self.assertEqual(restored.read_bytes(), p1)

    def test_malformed_snapshot_hash_rejection(self):
        bad_hashes = ["", "invalid!!", "sha3-256:123", "~@#$%", "not_a_valid_hash_at_all!"]
        for bad in bad_hashes:
            with self.subTest(bad=bad):
                res = self.cli("rollback", "--dry-run", "--to", bad, ok=False)
                err1 = (res.stdout + res.stderr).lower()
                self.assertTrue("invalid" in err1 or "empty" in err1 or "not found" in err1, err1)
                res2 = self.cli("rollback", "--to", bad, ok=False)
                err2 = (res2.stdout + res2.stderr).lower()
                self.assertTrue("invalid" in err2 or "empty" in err2 or "not found" in err2, err2)

    def test_legacy_snapshot_directory_rejection(self):
        snapshots = self.root / "snapshots"
        snapshots.mkdir(parents=True)
        legacy_dir = snapshots / "legacy_raw_snapshot_hash"
        legacy_dir.mkdir()
        (legacy_dir / "MANIFEST.txt").write_text("dummy\n", encoding="utf-8")

        res = self.cli("rollback", "--dry-run", "--to", "legacy_raw_snapshot_hash", ok=False)
        self.assertIn("invalid snapshot hash", (res.stdout + res.stderr).lower())
        self.assertTrue((legacy_dir / "MANIFEST.txt").exists())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", type=Path, required=True)
    args, remaining = parser.parse_known_args()
    CLI = args.cli.resolve()
    unittest.main(argv=[__file__, *remaining], verbosity=2)
