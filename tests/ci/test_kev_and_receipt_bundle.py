#!/usr/bin/env python3
"""Integration tests for KEV/Advisory fail-closed containment and `t81 receipt export`."""

from __future__ import annotations

import hashlib
import json
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
T81_BIN = os.environ.get("T81_BIN", str(ROOT / "build" / "t81"))


def _sha3_512(data: bytes) -> str:
    return "sha3-512:" + hashlib.sha3_512(data).hexdigest()


class KevAndReceiptBundleTest(unittest.TestCase):
    def test_kev_advisory_gating_and_forensic_receipt_export(self) -> None:
        self.assertTrue(Path(T81_BIN).exists(), f"missing t81 binary: {T81_BIN}")
        with tempfile.TemporaryDirectory(prefix="t81-kev-receipt-") as td:
            root = Path(td)
            bak_dir = root / "bak"
            bak_dir.mkdir()
            out_dir = root / "forensic_bundle"

            v0 = b'{"artifact":"v0"}\n'
            v1 = b'{"artifact":"v1-verified"}\n'
            target = root / "artifact.json"
            target.write_bytes(v0)
            v0_sha = _sha3_512(v0)
            v1_sha = _sha3_512(v1)

            pol = root / "axion_root.apl"
            pol.write_text(
                "(policy\n"
                "  (tier 1)\n"
                f'  (allowed-tensor-hashes ["{v0_sha}" "{v1_sha}"])\n'
                '  (require-axion-event (reason "kev-block:CVE-2026-9999"))\n'
                '  (require-axion-event (reason "defer-advisory:CVE-2026-7777")))\n',
                encoding="utf-8",
            )
            os.chmod(pol, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

            try:
                proc = subprocess.Popen(
                    [T81_BIN, "mcp", "serve", "--root", str(root)],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    text=True,
                    bufsize=1,
                )
                rid = 0

                def call(name: str, arguments: dict[str, Any]) -> dict[str, Any]:
                    nonlocal rid
                    rid += 1
                    assert proc.stdin is not None and proc.stdout is not None
                    req = {
                        "jsonrpc": "2.0",
                        "id": rid,
                        "method": "tools/call",
                        "params": {"name": name, "arguments": arguments},
                    }
                    proc.stdin.write(json.dumps(req) + "\n")
                    proc.stdin.flush()
                    return json.loads(proc.stdout.readline())["result"]["structured"]

                try:
                    # 1. Blocked KEV advisory -> -1 fail-closed
                    g_kev = call(
                        "t81.gate_check",
                        {
                            "artifact_path": str(target),
                            "policy_path": str(pol),
                            "advisory_id": "CVE-2026-9999",
                        },
                    )
                    self.assertFalse(g_kev["ok"])
                    self.assertEqual(g_kev["trit"], -1)
                    self.assertEqual(g_kev["code"], "ERR_KEV_ADVISORY_BLOCKED")

                    # 2. Deferred advisory -> 0 fail-closed
                    g_def = call(
                        "t81.gate_check",
                        {
                            "artifact_path": str(target),
                            "policy_path": str(pol),
                            "advisory_id": "CVE-2026-7777",
                        },
                    )
                    self.assertFalse(g_def["ok"])
                    self.assertEqual(g_def["trit"], 0)
                    self.assertEqual(g_def["code"], "DEFER_ADVISORY_REVIEW")

                    # 3. Manifest digest mismatch -> -1 fail-closed
                    g_mis = call(
                        "t81.gate_check",
                        {
                            "artifact_path": str(target),
                            "policy_path": str(pol),
                            "expected_manifest_sha3_512": "sha3-512:" + ("0" * 128),
                        },
                    )
                    self.assertFalse(g_mis["ok"])
                    self.assertEqual(g_mis["code"], "ERR_MANIFEST_DIGEST_MISMATCH")

                    # 4. tx_commit blocked by KEV -> zero mutation & zero orphan files
                    b1 = call(
                        "t81.tx_begin",
                        {
                            "target_path": str(target),
                            "backup_dir": str(bak_dir),
                            "agent_id": "kev-agent",
                            "isolated": True,
                        },
                    )
                    Path(b1["staging_path"]).write_bytes(v1)
                    c1 = call(
                        "t81.tx_commit",
                        {
                            "tx_id": b1["tx_id"],
                            "policy_path": str(pol),
                            "agent_id": "kev-agent",
                            "advisory_id": "CVE-2026-9999",
                        },
                    )
                    self.assertFalse(c1["ok"])
                    self.assertEqual(c1["code"], "ERR_KEV_ADVISORY_BLOCKED")
                    self.assertEqual(target.read_bytes(), v0)
                    orphans = [
                        p.name
                        for p in root.rglob("*")
                        if ".stage" in p.name or ".bak" in p.name or p.name.endswith(".tmp")
                    ]
                    self.assertEqual(orphans, [])

                    # 5. Clean tx_commit -> +1 sealed signed receipt in CanonFS ledger
                    b2 = call(
                        "t81.tx_begin",
                        {
                            "target_path": str(target),
                            "backup_dir": str(bak_dir),
                            "agent_id": "clean-agent",
                            "isolated": True,
                        },
                    )
                    Path(b2["staging_path"]).write_bytes(v1)
                    c2 = call(
                        "t81.tx_commit",
                        {
                            "tx_id": b2["tx_id"],
                            "policy_path": str(pol),
                            "agent_id": "clean-agent",
                            "expected_manifest_sha3_512": v1_sha,
                        },
                    )
                    self.assertTrue(c2["ok"], c2)
                    self.assertEqual(c2["trit"], 1)
                    receipt_ref = c2["evidence_canonfs_ref"]
                    self.assertEqual(len(receipt_ref), 64)
                finally:
                    if proc.stdin:
                        proc.stdin.close()
                    if proc.stdout:
                        proc.stdout.close()
                    proc.wait(timeout=5)

                # 6. Export and verify offline forensic bundle via `t81 receipt export`
                exp = subprocess.run(
                    [
                        T81_BIN,
                        "receipt",
                        "export",
                        "--receipt",
                        receipt_ref,
                        "--ledger",
                        str(root / ".t81_receipt_ledger"),
                        "--out",
                        str(out_dir),
                        "--json",
                    ],
                    capture_output=True,
                    text=True,
                    check=True,
                )
                exp_json = json.loads(exp.stdout)
                self.assertTrue(exp_json["ok"])
                self.assertTrue(exp_json["ed25519_verified"])
                self.assertTrue((out_dir / "bundle.json").exists())
                self.assertTrue((out_dir / "receipt_raw.json").exists())
                self.assertTrue((out_dir / "manifest.sha3-512").exists())
                # Ensure zero .tmp orphans in export dir
                self.assertEqual(list(out_dir.glob("*.tmp")), [])
            finally:
                if pol.exists():
                    os.chmod(pol, stat.S_IRUSR | stat.S_IWUSR)


if __name__ == "__main__":
    unittest.main()
