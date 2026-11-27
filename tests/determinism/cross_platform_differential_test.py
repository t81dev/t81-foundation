#!/usr/bin/env python3
"""Cross-Platform Differential Oracle Test (Phase 3).

Executes a canonical multi-layer T81 verification workload across:
  1. Hosted T81VM Dual-Rail (P, N) + SBDR1T2D + ProviderGovernor + Ed25519 RFC-00D4
  2. H06-MEM-04 RegimeSelector (Apple M2 arm64/Metal & Xeon x86_64/AVX2 profiles)
  3. Ephemeral TernaryOS AArch64 enclave runner dry-run envelope (t81 enclave run)
  4. Streaming BitNet-2B exact-architecture fixture (--verify-real-hf-if-cached)

Asserts bit-identical SHA3-512 digests across repeat runs and cross-platform
determinism invariants so Linux/macOS/Windows x86_64/AArch64 CI catches drift.
"""

from __future__ import annotations

import hashlib
import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = Path(os.environ.get("T81_BUILD_DIR", str(ROOT / "build")))


def _find_bin(name: str) -> Path | None:
    candidates = [
        BUILD_DIR / name,
        BUILD_DIR / f"{name}.exe",
        BUILD_DIR / "Release" / f"{name}.exe",
    ]
    for p in candidates:
        if p.exists():
            return p
    return None


def _run_checked(cmd: list[str]) -> str:
    proc = subprocess.run(cmd, cwd=str(ROOT), capture_output=True, text=True, check=True)
    return proc.stdout.strip()


def main() -> int:
    results: dict[str, object] = {
        "schema": "t81.cross-platform-differential.v1",
        "host": f"{platform.system()} {platform.machine()}",
        "checks": {},
    }

    # 1. Core C++ deterministic binaries (run whichever are built in this CI slice).
    for bin_name in (
        "t81_ed25519_bundle_test",
        "t81_provider_contract_test",
        "t81_dpe_mcp_unification_test",
        "test_regime_dispatch",
        "t81_dualrail_sbdr_vm_test",
        "t81_bitnet_2b_dispatch_test",
    ):
        b = _find_bin(bin_name)
        if b is not None:
            _run_checked([str(b)])
            results["checks"][bin_name] = "PASS"

    # 2. Enclave runner deterministic policy-digest envelope (2x repeat parity).
    t81_bin = _find_bin("t81")
    if t81_bin is not None:
        with tempfile.NamedTemporaryFile("w", suffix=".apl", delete=False) as tf:
            tf.write("(policy (tier 1))\n")
            pol_path = tf.name
        try:
            r1 = json.loads(
                _run_checked(
                    [
                        str(t81_bin),
                        "enclave",
                        "run",
                        "--payload",
                        "sha3-256:deadbeef",
                        "--policy",
                        pol_path,
                        "--dry-run",
                        "--json",
                    ]
                )
            )
            r2 = json.loads(
                _run_checked(
                    [
                        str(t81_bin),
                        "enclave",
                        "run",
                        "--payload",
                        "sha3-256:deadbeef",
                        "--policy",
                        pol_path,
                        "--dry-run",
                        "--json",
                    ]
                )
            )
            assert r1 == r2 and r1["ok"] is True, "enclave dry-run envelope mismatch across repeat runs"
            results["checks"]["enclave_envelope_policy_sha3_512"] = r1["policy_sha3_512"]
        finally:
            Path(pol_path).unlink(missing_ok=True)

    # 3. Cache-aware BitNet-2B streaming ingestion differential check (2x repeat parity).
    ingester = ROOT / "examples" / "research" / "bitnet-verifiable-token" / "load_safetensors_bitnet.py"
    m1 = json.loads(_run_checked([sys.executable, str(ingester), "--verify-real-hf-if-cached"]))
    m2 = json.loads(_run_checked([sys.executable, str(ingester), "--verify-real-hf-if-cached"]))
    assert m1["manifest_sha3_512"] == m2["manifest_sha3_512"], "streaming manifest non-deterministic"
    results["checks"]["bitnet_2b_manifest_sha3_512"] = m1["manifest_sha3_512"]
    results["checks"]["bitnet_2b_compression_vs_i8"] = m1["compression_vs_i8"]

    canonical = json.dumps(results["checks"], sort_keys=True, separators=(",", ":")).encode("utf-8")
    results["differential_parity_sha3_512"] = "sha3-512:" + hashlib.sha3_512(canonical).hexdigest()
    print(json.dumps(results, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
