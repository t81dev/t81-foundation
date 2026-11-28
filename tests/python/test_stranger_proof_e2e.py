#!/usr/bin/env python3
"""Out-of-tree E2E verification for the bundled ``t81.examples.stranger_proof`` workflow.

Runs ``python -m t81.examples.stranger_proof`` from an isolated temporary working
directory outside the repository root to prove zero hidden repo-relative dependencies.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import t81
from t81.examples.stranger_proof import (
    PEAK_RSS_LIMIT_MB,
    PINNED_BITNET_2B_MANIFEST_SHA3_512,
    PINNED_BITNET_2B_TENSOR_SHA3_512,
)


class StrangerProofOutOfTreeE2ETest(unittest.TestCase):
    def test_stranger_proof_runs_out_of_tree_and_verifies_offline_bundle(self) -> None:
        # Ensure the bundled or configured native CLI binary is discoverable.
        cli_bin = t81.cli_path()
        self.assertTrue(cli_bin.is_file(), f"missing t81 binary: {cli_bin}")

        with tempfile.TemporaryDirectory(prefix="t81-out-of-tree-") as td:
            isolated_cwd = Path(td)
            keep_dir = isolated_cwd / "saved_proof"

            # Strip any repo-root working-directory bias; rely strictly on installed t81.
            env = dict(os.environ)
            env.setdefault("T81_BIN", str(cli_bin))

            proc = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "t81.examples.stranger_proof",
                    "--json",
                    "--chunk-bytes",
                    "262144",
                    "--keep-dir",
                    str(keep_dir),
                ],
                cwd=str(isolated_cwd),
                env=env,
                capture_output=True,
                text=True,
                check=True,
            )
            res = json.loads(proc.stdout)
            self.assertTrue(res["ok"], res)
            self.assertLess(res["elapsed_seconds"], 60.0)

            b = res["bitnet_2b"]
            self.assertEqual(b["manifest_sha3_512"], PINNED_BITNET_2B_MANIFEST_SHA3_512)
            self.assertEqual(b["tensor_sha3_512"], PINNED_BITNET_2B_TENSOR_SHA3_512)
            self.assertEqual(b["compression_vs_i8"], 4.0)
            self.assertLess(b["peak_rss_mb"], PEAK_RSS_LIMIT_MB)

            m = res["mcp_session"]
            self.assertEqual(m["diff_check_verdict"], "ADMIT_WITHIN_FLIP_BUDGET")
            self.assertEqual(m["plane_bypass_code"], "ERR_PLANE_BYPASS_DENIED")
            self.assertEqual(m["plane_bypass_trit"], -1)
            self.assertTrue(m["out_of_root_denied"])
            self.assertEqual(m["commit_trit"], 1)
            self.assertEqual(len(m["receipt_canonfs_ref"]), 64)

            v = res["offline_verification"]
            self.assertTrue(v["manifest_ok"])
            self.assertTrue(v["ed25519_dag_ok"])
            self.assertEqual(v["verified_nodes"], 1)
            self.assertTrue(v["tamper_rejected"])
            self.assertFalse(v["tamper_manifest_ok"])
            self.assertFalse(v["tamper_ed25519_dag_ok"])
            self.assertEqual(v["tamper_error_code"], "ERR_RECEIPT_VERIFY_FAILED")

            # Standalone offline --verify-bundle invocation from the isolated cwd.
            bundle_dir = Path(v["saved_bundle_dir"])
            self.assertTrue((bundle_dir / "bundle.json").is_file())
            self.assertTrue((bundle_dir / "receipt_raw.json").is_file())
            self.assertTrue((bundle_dir / "manifest.sha3-512").is_file())

            vproc = subprocess.run(
                [
                    sys.executable,
                    "-m",
                    "t81.examples.stranger_proof",
                    "--verify-bundle",
                    str(bundle_dir),
                    "--json",
                ],
                cwd=str(isolated_cwd),
                env=env,
                capture_output=True,
                text=True,
                check=True,
            )
            vres = json.loads(vproc.stdout)
            self.assertTrue(vres["ok"], vres)
            self.assertTrue(vres["manifest_ok"])
            self.assertTrue(vres["ed25519_dag_ok"])
            self.assertEqual(vres["verified_nodes"], 1)


if __name__ == "__main__":
    unittest.main()
