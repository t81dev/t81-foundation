#!/usr/bin/env python3
"""Two-Agent Offline Receipt Handoff (RFC-00D4 Companion Profile).

Agent A:
  - Runs a bounded `t81 mcp serve --root <workspace>` session under immutable
    `axion_root.apl`.
  - Commits a governed state transition (`tx_begin` -> `tx_commit`, trit=+1).
  - Exports an atomic offline forensic bundle (`t81 receipt export`) and writes
    `handoff_envelope.json` pinning `expected_policy_sha3_512` and `state_c_ref`.

Agent B (100% offline, zero live MCP server):
  - Verifies `manifest.sha3-512`, Ed25519 signature (`t81.verify_dag` when the
    Python SDK is installed, or OpenSSL Ed25519 fallback), `policy_sha3_512`
    binding, and artifact byte digest.
  - Fails closed on missing signatures, tampered payloads, or mismatched policy
    digests.
"""

from __future__ import annotations

import base64
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
from typing import Any

ROOT = Path(__file__).resolve().parents[2]


def _sha3_512(data: bytes) -> str:
    return "sha3-512:" + hashlib.sha3_512(data).hexdigest()


def find_t81_bin() -> Path:
    env_bin = os.environ.get("T81_BIN")
    if env_bin and Path(env_bin).is_file():
        return Path(env_bin)
    try:
        import t81  # type: ignore

        if hasattr(t81, "cli_path"):
            return Path(t81.cli_path())
    except Exception:
        pass
    for cand in (ROOT / "build" / "t81", ROOT / "build" / "t81.exe"):
        if cand.is_file():
            return cand
    found = shutil.which("t81")
    if found:
        return Path(found)
    raise FileNotFoundError("t81 CLI binary not found (build with `cmake --build build --target t81`)")


def _extract_canonical_payload(raw_text: str) -> str:
    pat = '"payload":'
    start = raw_text.find(pat)
    end = raw_text.find(',"ed25519_signature_b64":')
    if start == -1 or end == -1 or end <= start + len(pat):
        raise ValueError("receipt_raw.json missing canonical payload slice")
    return raw_text[start + len(pat) : end]


def _verify_ed25519_offline(
    bundle_id: str, payload_json: str, sig_b64: str, pub_pem: str
) -> tuple[bool, str]:
    try:
        import t81  # type: ignore

        res = t81.verify_dag(
            [
                {
                    "bundle_id": bundle_id,
                    "parent_bundle_ids": [],
                    "canonical_signing_payload": payload_json,
                    "signature_b64": sig_b64,
                    "public_key_pem": pub_pem,
                }
            ]
        )
        return (bool(res.get("ok")), str(res.get("code", "")))
    except ImportError:
        pass

    # Fallback to OpenSSL 3 Ed25519 rawin verification when running without Python wheel.
    with tempfile.TemporaryDirectory(prefix="t81-ed25519-verify-") as td:
        p = Path(td)
        pub_f = p / "pub.pem"
        msg_f = p / "msg.bin"
        sig_f = p / "sig.bin"
        pub_f.write_text(pub_pem, encoding="utf-8")
        msg_f.write_bytes(payload_json.encode("utf-8"))
        try:
            sig_f.write_bytes(base64.b64decode(sig_b64, validate=True))
        except Exception:
            return (False, "ERR_RECEIPT_VERIFY_FAILED")
        proc = subprocess.run(
            [
                "openssl",
                "pkeyutl",
                "-verify",
                "-pubin",
                "-inkey",
                str(pub_f),
                "-rawin",
                "-in",
                str(msg_f),
                "-sigfile",
                str(sig_f),
            ],
            capture_output=True,
        )
        return (proc.returncode == 0, "" if proc.returncode == 0 else "ERR_RECEIPT_VERIFY_FAILED")


def agent_a_produce_handoff(handoff_dir: Path, t81_bin: Path | None = None) -> dict[str, Any]:
    """Agent A runs a bounded MCP session and exports an offline handoff package."""
    cli = t81_bin if t81_bin is not None else find_t81_bin()
    handoff_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="t81-agent-a-") as td:
        ws = Path(td)
        bak = ws / "bak"
        bak.mkdir()

        state_a = b'{"step":"init","owner":"agent-a","status":"pending"}\n'
        state_c = b'{"step":"verified","owner":"agent-a","status":"ready-for-agent-b"}\n'
        target = ws / "artifact.json"
        target.write_bytes(state_a)

        state_a_ref = _sha3_512(state_a)
        state_c_ref = _sha3_512(state_c)
        pol = ws / "axion_root.apl"
        pol.write_text(
            "(policy\n"
            "  (tier 1)\n"
            f'  (allowed-tensor-hashes ["{state_a_ref}" "{state_c_ref}"]))\n',
            encoding="utf-8",
        )
        policy_sha = _sha3_512(pol.read_bytes())
        os.chmod(pol, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

        try:
            proc = subprocess.Popen(
                [str(cli), "mcp", "serve", "--root", str(ws)],
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
                        "agent_id": "agent-a",
                        "isolated": True,
                    },
                )
                Path(b["staging_path"]).write_bytes(state_c)
                c = call(
                    "t81.tx_commit",
                    {
                        "tx_id": b["tx_id"],
                        "policy_path": str(pol),
                        "agent_id": "agent-a",
                        "source_plane": "hybrid",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "reproducibility_class": "seeded_probabilistic",
                        "hybrid_reconciled": True,
                        "state_a_ref": state_a_ref,
                        "expected_manifest_sha3_512": state_c_ref,
                    },
                )
                if not (c.get("ok") is True and c.get("trit") == 1):
                    raise RuntimeError(f"Agent A tx_commit failed: {c}")
                receipt_ref = c["evidence_canonfs_ref"]
                policy_sha = c.get("policy_sha3_512", policy_sha)
            finally:
                if proc.stdin:
                    proc.stdin.close()
                if proc.stdout:
                    proc.stdout.close()
                if proc.stderr:
                    proc.stderr.close()
                proc.wait(timeout=5)

            bundle_dir = handoff_dir / "forensic_bundle"
            subprocess.run(
                [
                    str(cli),
                    "receipt",
                    "export",
                    "--receipt",
                    receipt_ref,
                    "--ledger",
                    str(ws / ".t81_receipt_ledger"),
                    "--out",
                    str(bundle_dir),
                    "--json",
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            shutil.copy2(target, handoff_dir / "artifact.json")

            envelope = {
                "schema": "t81.agent-handoff-envelope.v1",
                "producer_agent_id": "agent-a",
                "consumer_agent_id": "agent-b",
                "expected_policy_sha3_512": policy_sha,
                "state_a_ref": state_a_ref,
                "state_c_ref": state_c_ref,
                "receipt_canonfs_ref": receipt_ref,
                "bundle_dir": "forensic_bundle",
                "artifact_file": "artifact.json",
            }
            (handoff_dir / "handoff_envelope.json").write_text(
                json.dumps(envelope, indent=2) + "\n", encoding="utf-8"
            )
            return envelope
        finally:
            if pol.exists():
                os.chmod(pol, stat.S_IRUSR | stat.S_IWUSR)


def agent_b_verify_handoff(
    handoff_dir: Path, *, expected_policy_sha3_512: str | None = None
) -> dict[str, Any]:
    """Agent B verifies Agent A's handoff package 100% offline, failing closed."""
    envelope = json.loads((handoff_dir / "handoff_envelope.json").read_text(encoding="utf-8"))
    pinned_policy = expected_policy_sha3_512 or envelope["expected_policy_sha3_512"]
    bundle_dir = handoff_dir / envelope["bundle_dir"]
    artifact_path = handoff_dir / envelope["artifact_file"]

    bundle_bytes = (bundle_dir / "bundle.json").read_bytes()
    raw_bytes = (bundle_dir / "receipt_raw.json").read_bytes()
    manifest_lines = [
        ln.strip()
        for ln in (bundle_dir / "manifest.sha3-512").read_text(encoding="utf-8").splitlines()
        if ln.strip()
    ]
    manifest_map = {ln.split()[1]: ln.split()[0] for ln in manifest_lines if len(ln.split()) == 2}

    raw_text = raw_bytes.decode("utf-8")
    raw_obj = json.loads(raw_text)
    bundle_obj = json.loads(bundle_bytes.decode("utf-8"))

    # 1. Fail closed on missing signature / public key.
    sig_b64 = raw_obj.get("ed25519_signature_b64", "")
    pub_pem = raw_obj.get("public_key_pem", "")
    if not sig_b64 or not pub_pem:
        return {"ok": False, "code": "ERR_MISSING_SIGNATURE"}

    # 2. Fail closed on manifest SHA3-512 mismatch.
    if (
        manifest_map.get("bundle.json") != _sha3_512(bundle_bytes)
        or manifest_map.get("receipt_raw.json") != _sha3_512(raw_bytes)
    ):
        return {"ok": False, "code": "ERR_MANIFEST_DIGEST_MISMATCH"}

    # 3. Fail closed on invalid Ed25519 signature over canonical payload.
    try:
        payload_json = _extract_canonical_payload(raw_text)
    except ValueError:
        return {"ok": False, "code": "ERR_RECEIPT_VERIFY_FAILED"}

    sig_ok, err_code = _verify_ed25519_offline(
        bundle_obj["receipt_canonfs_ref"], payload_json, sig_b64, pub_pem
    )
    if not sig_ok:
        return {"ok": False, "code": err_code or "ERR_RECEIPT_VERIFY_FAILED"}

    # 4. Fail closed on mismatched policy digest (RFC-00D4 authority.policy_sha3_512).
    payload_obj = raw_obj.get("payload", {})
    recorded_policy = payload_obj.get("authority", {}).get("policy_sha3_512")
    if recorded_policy != pinned_policy:
        return {
            "ok": False,
            "code": "ERR_POLICY_DIGEST_MISMATCH",
            "expected": pinned_policy,
            "actual": recorded_policy,
        }

    # 5. Fail closed on artifact digest mismatch.
    actual_artifact_sha = _sha3_512(artifact_path.read_bytes())
    if actual_artifact_sha != envelope["state_c_ref"]:
        return {
            "ok": False,
            "code": "ERR_ARTIFACT_DIGEST_MISMATCH",
            "expected": envelope["state_c_ref"],
            "actual": actual_artifact_sha,
        }

    return {
        "ok": True,
        "code": "HANDOFF_VERIFIED",
        "producer": envelope["producer_agent_id"],
        "consumer": envelope["consumer_agent_id"],
        "receipt_canonfs_ref": envelope["receipt_canonfs_ref"],
        "policy_sha3_512": recorded_policy,
        "state_c_ref": actual_artifact_sha,
    }


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="t81-handoff-demo-") as td:
        handoff_dir = Path(td) / "handoff"
        env = agent_a_produce_handoff(handoff_dir)
        res = agent_b_verify_handoff(
            handoff_dir, expected_policy_sha3_512=env["expected_policy_sha3_512"]
        )
        print(json.dumps(res, indent=2))
        return 0 if res["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
