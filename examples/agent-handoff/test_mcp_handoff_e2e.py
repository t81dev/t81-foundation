#!/usr/bin/env python3
"""End-to-end two-agent MCP-to-MCP receipt handoff test (RFC-00D4 Companion Profile).

Proves the full Agent A -> offline verification -> Agent B downstream MCP chain:
  1. Valid signed handoff: Agent A commits State A -> State C under immutable
     `axion_root.apl` via `t81 mcp serve` and exports an offline forensic bundle;
     Agent B verifies offline (`HANDOFF_VERIFIED`), then executes its own jailed
     `t81 mcp serve` session to commit a downstream State D anchored to Agent A's
     `receipt_canonfs_ref` and `state_c_ref`.
  2. Fail-closed rejections before downstream execution:
     - Missing Ed25519 signature (`ERR_MISSING_SIGNATURE`)
     - Tampered manifest/payload (`ERR_MANIFEST_DIGEST_MISMATCH` & `ERR_RECEIPT_VERIFY_FAILED`)
     - Mismatched `expected_policy_sha3_512` (`ERR_POLICY_DIGEST_MISMATCH`)
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from typing import Any

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

import handoff_demo  # noqa: E402


def _agent_b_consume_and_commit_downstream(
    handoff_dir: Path,
    *,
    expected_policy_sha3_512: str,
    downstream_ws: Path,
    t81_bin: Path,
) -> dict[str, Any]:
    """Verify Agent A's bundle offline; only if verified, run Agent B's MCP commit."""
    ver = handoff_demo.agent_b_verify_handoff(
        handoff_dir, expected_policy_sha3_512=expected_policy_sha3_512
    )
    if not ver["ok"]:
        return {
            "ok": False,
            "stage": "offline_verify",
            "code": ver["code"],
            "downstream_committed": False,
        }

    # Offline verification passed: launch Agent B's own jailed MCP session.
    downstream_ws.mkdir(parents=True, exist_ok=True)
    bak = downstream_ws / "bak"
    bak.mkdir(exist_ok=True)

    state_b0 = b'{"stage":"agent-b-init","upstream_receipt":null}\n'
    state_b1 = (
        json.dumps(
            {
                "stage": "agent-b-verified-downstream",
                "upstream_producer": ver["producer"],
                "upstream_receipt_ref": ver["receipt_canonfs_ref"],
                "upstream_state_c_ref": ver["state_c_ref"],
            },
            sort_keys=True,
            separators=(",", ":"),
        )
        + "\n"
    ).encode("utf-8")

    target = downstream_ws / "downstream.json"
    target.write_bytes(state_b0)
    b0_sha = handoff_demo._sha3_512(state_b0)
    b1_sha = handoff_demo._sha3_512(state_b1)

    pol_b = downstream_ws / "axion_root.apl"
    pol_b.write_text(
        "(policy\n"
        "  (tier 1)\n"
        f'  (allowed-tensor-hashes ["{b0_sha}" "{b1_sha}"]))\n',
        encoding="utf-8",
    )
    os.chmod(pol_b, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

    try:
        proc = subprocess.Popen(
            [str(t81_bin), "mcp", "serve", "--root", str(downstream_ws)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
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
            b = call(
                "t81.tx_begin",
                {
                    "target_path": str(target),
                    "backup_dir": str(bak),
                    "agent_id": "agent-b",
                    "isolated": True,
                },
            )
            Path(b["staging_path"]).write_bytes(state_b1)
            c = call(
                "t81.tx_commit",
                {
                    "tx_id": b["tx_id"],
                    "policy_path": str(pol_b),
                    "agent_id": "agent-b",
                    "source_plane": "hybrid",
                    "target_plane": "deterministic",
                    "transition_kind": "transition_path",
                    "reproducibility_class": "seeded_probabilistic",
                    "hybrid_reconciled": True,
                    "state_a_ref": b0_sha,
                    "expected_manifest_sha3_512": b1_sha,
                },
            )
            if not (c.get("ok") is True and c.get("trit") == 1):
                raise RuntimeError(f"Agent B downstream tx_commit failed: {c}")
            return {
                "ok": True,
                "stage": "downstream_committed",
                "code": "HANDOFF_AND_DOWNSTREAM_COMMITTED",
                "downstream_committed": True,
                "upstream_receipt_ref": ver["receipt_canonfs_ref"],
                "downstream_receipt_ref": c["evidence_canonfs_ref"],
                "downstream_state_c_ref": c["state_c_ref"],
            }
        finally:
            if proc.stdin:
                proc.stdin.close()
            if proc.stdout:
                proc.stdout.close()
            if proc.stderr:
                proc.stderr.close()
            proc.wait(timeout=5)
    finally:
        if pol_b.exists():
            os.chmod(pol_b, stat.S_IRUSR | stat.S_IWUSR)


class TwoAgentMcpHandoffE2ETest(unittest.TestCase):
    def test_mcp_to_mcp_valid_handoff_and_fail_closed_gate(self) -> None:
        t81_bin = handoff_demo.find_t81_bin()
        with tempfile.TemporaryDirectory(prefix="t81-mcp-handoff-e2e-") as td:
            root = Path(td)
            good_handoff = root / "handoff_good"
            env = handoff_demo.agent_a_produce_handoff(good_handoff, t81_bin=t81_bin)

            # 1. Valid signed handoff -> Agent B verifies offline and commits downstream via MCP.
            ds_good = root / "agent_b_good"
            ok = _agent_b_consume_and_commit_downstream(
                good_handoff,
                expected_policy_sha3_512=env["expected_policy_sha3_512"],
                downstream_ws=ds_good,
                t81_bin=t81_bin,
            )
            self.assertTrue(ok["ok"], ok)
            self.assertTrue(ok["downstream_committed"])
            self.assertEqual(ok["code"], "HANDOFF_AND_DOWNSTREAM_COMMITTED")
            self.assertEqual(len(ok["downstream_receipt_ref"]), 64)
            self.assertTrue((ds_good / "downstream.json").is_file())

            # 2. Missing signature -> FAIL-CLOSED; Agent B never creates downstream workspace.
            h_miss = root / "handoff_missing_sig"
            shutil.copytree(good_handoff, h_miss)
            raw_p = h_miss / "forensic_bundle" / "receipt_raw.json"
            raw_obj = json.loads(raw_p.read_text(encoding="utf-8"))
            raw_obj["ed25519_signature_b64"] = ""
            raw_p.write_text(json.dumps(raw_obj, separators=(",", ":")), encoding="utf-8")
            ds_miss = root / "agent_b_miss"
            r_miss = _agent_b_consume_and_commit_downstream(
                h_miss,
                expected_policy_sha3_512=env["expected_policy_sha3_512"],
                downstream_ws=ds_miss,
                t81_bin=t81_bin,
            )
            self.assertFalse(r_miss["ok"])
            self.assertFalse(r_miss["downstream_committed"])
            self.assertEqual(r_miss["code"], "ERR_MISSING_SIGNATURE")
            self.assertFalse(ds_miss.exists())

            # 3. Tampered manifest & tampered payload (even with rewritten manifest) -> FAIL-CLOSED.
            h_tamp = root / "handoff_tampered"
            shutil.copytree(good_handoff, h_tamp)
            t_raw = h_tamp / "forensic_bundle" / "receipt_raw.json"
            t_raw.write_text(
                t_raw.read_text(encoding="utf-8").replace('"agent-a"', '"agent-Z"', 1),
                encoding="utf-8",
            )
            ds_tamp = root / "agent_b_tamp"
            r_man = _agent_b_consume_and_commit_downstream(
                h_tamp,
                expected_policy_sha3_512=env["expected_policy_sha3_512"],
                downstream_ws=ds_tamp,
                t81_bin=t81_bin,
            )
            self.assertFalse(r_man["ok"])
            self.assertEqual(r_man["code"], "ERR_MANIFEST_DIGEST_MISMATCH")
            self.assertFalse(ds_tamp.exists())

            # Rewrite manifest to isolate Ed25519 signature rejection.
            b_bytes = (h_tamp / "forensic_bundle" / "bundle.json").read_bytes()
            r_bytes = t_raw.read_bytes()
            (h_tamp / "forensic_bundle" / "manifest.sha3-512").write_text(
                f"{handoff_demo._sha3_512(b_bytes)}  bundle.json\n"
                f"{handoff_demo._sha3_512(r_bytes)}  receipt_raw.json\n",
                encoding="utf-8",
            )
            r_sig = _agent_b_consume_and_commit_downstream(
                h_tamp,
                expected_policy_sha3_512=env["expected_policy_sha3_512"],
                downstream_ws=ds_tamp,
                t81_bin=t81_bin,
            )
            self.assertFalse(r_sig["ok"])
            self.assertEqual(r_sig["code"], "ERR_RECEIPT_VERIFY_FAILED")
            self.assertFalse(ds_tamp.exists())

            # 4. Mismatched policy digest -> FAIL-CLOSED; downstream never executes.
            ds_pol = root / "agent_b_pol"
            r_pol = _agent_b_consume_and_commit_downstream(
                good_handoff,
                expected_policy_sha3_512="sha3-512:" + ("a" * 128),
                downstream_ws=ds_pol,
                t81_bin=t81_bin,
            )
            self.assertFalse(r_pol["ok"])
            self.assertFalse(r_pol["downstream_committed"])
            self.assertEqual(r_pol["code"], "ERR_POLICY_DIGEST_MISMATCH")
            self.assertFalse(ds_pol.exists())


if __name__ == "__main__":
    unittest.main()
