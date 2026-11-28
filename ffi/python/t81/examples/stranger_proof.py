#!/usr/bin/env python3
"""Outsider-Reproducible Golden Path for T81 Foundation (v2.3.0+).

Runs end-to-end with zero network access and zero repository checkout once
``t81`` is installed:

  1. Pinned BitNet-2B Streaming Ingest:
     Materializes a deterministic, bit-exact BitNet b1.58-2B representative F16
     shard (256x2048 = 524,288 weights), ingests it via bounded ``--chunk-bytes``
     streaming into Dual-Rail (P, N) + ``SBDR1T2D``, stores the artifact in a
     persistent ``CanonFS`` store, verifies chunk-size determinism against the
     published pinned SHA3-512 manifest digest, and asserts peak RSS < 512 MB.

  2. Governed DPE + MCP Session (``t81 mcp serve``):
     Launches an ephemeral native C++ MCP server jailed to a temp ``--root``
     with an immutable ``axion_root.apl`` policy, audits Dual-Rail flip budget
     (``t81.diff_check``), proves fail-closed denial on both direct Three-Plane
     bypass (``ERR_PLANE_BYPASS_DENIED``) and out-of-root path escape, and
     commits the BitNet-2B manifest-backed ``State C`` (``+1``) with an Ed25519
     signed receipt persisted in ``.t81_receipt_ledger``.

  3. Offline Forensic Bundle Export, Verification & 1-Byte Tamper Check:
     Exports the signed receipt via ``t81 receipt export``, tears down the
     server, verifies ``manifest.sha3-512`` + native C++23 Ed25519 ``verify_dag``
     completely offline, and verifies that flipping 1 byte in the exported
     bundle is rejected fail-closed (``ERR_RECEIPT_VERIFY_FAILED``).

Usage:
  python -m t81.examples.stranger_proof
  python -m t81.examples.stranger_proof --json
  python -m t81.examples.stranger_proof --keep-dir ./proof_bundle
  python -m t81.examples.stranger_proof --verify-bundle ./proof_bundle/forensic_bundle
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import time
from typing import Any

import t81

# Published pinned digests for the deterministic BitNet b1.58-2B representative shard
# (256x2048 F16 = 524,288 weights, model.layers.0.mlp.gate_proj.weight).
PINNED_BITNET_2B_MANIFEST_SHA3_512 = (
    "sha3-512:3e0514756617d33945f0df4acfe3dbee222eafb3ccf92cd75861329332a812ca"
    "8f27364362449a697016cac02e384d55c3bb59979810d15b570d6b9046c45df3"
)
PINNED_BITNET_2B_TENSOR_SHA3_512 = (
    "sha3-512:6f53273ca98ae298dadcc8e57b8a1c576e35705e9163caf0ade1d6f5e9ab571e"
    "cddbdb0c2a21c5a9b6e4542f9be287cf7f16a77242b50e5c16f5b07f00070f7d"
)
PEAK_RSS_LIMIT_MB = 512.0


def _sha3_512_prefixed(data: bytes) -> str:
    h = t81.sha3_512_hex(data)
    return h if h.startswith("sha3-512:") else f"sha3-512:{h}"


def _peak_rss_mb() -> float:
    """Return process peak RSS in MiB across macOS, Linux, and Windows."""
    if os.name == "nt":
        class _ProcessMemoryCounters(ctypes.Structure):
            _fields_ = [
                ("cb", ctypes.c_ulong),
                ("PageFaultCount", ctypes.c_ulong),
                ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t),
                ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t),
            ]

        counters = _ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        handle = ctypes.windll.kernel32.GetCurrentProcess()
        if ctypes.windll.psapi.GetProcessMemoryInfo(
            handle, ctypes.byref(counters), counters.cb
        ):
            return round(counters.PeakWorkingSetSize / (1024.0 * 1024.0), 2)
        return 0.0

    import resource

    usage = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    if sys.platform == "darwin":
        return round(usage / (1024.0 * 1024.0), 2)
    return round(usage / 1024.0, 2)


def _decode_f16(blob: bytes) -> list[float]:
    count = len(blob) // 2
    return list(struct.unpack(f"<{count}e", blob))


def _pack_dualrail_64(trits: list[int]) -> tuple[bytes, bytes]:
    words = (len(trits) + 63) // 64
    pos = bytearray(words * 8)
    neg = bytearray(words * 8)
    for w in range(words):
        p = 0
        n = 0
        base = w * 64
        lim = min(len(trits), base + 64)
        for i in range(base, lim):
            t = trits[i]
            if t > 0:
                p |= 1 << (i - base)
            elif t < 0:
                n |= 1 << (i - base)
        struct.pack_into("<Q", pos, w * 8, p)
        struct.pack_into("<Q", neg, w * 8, n)
    return bytes(pos), bytes(neg)


def _unpack_dualrail_64(pos: bytes, neg: bytes, n_trits: int) -> list[int]:
    out = [0] * n_trits
    words = len(pos) // 8
    for w in range(words):
        p = struct.unpack_from("<Q", pos, w * 8)[0]
        n = struct.unpack_from("<Q", neg, w * 8)[0]
        if (p & n) != 0:
            raise ValueError("dual-rail disjoint invariant violated (pos & neg != 0)")
        base = w * 64
        lim = min(n_trits, base + 64)
        for i in range(base, lim):
            shift = i - base
            out[i] = ((p >> shift) & 1) - ((n >> shift) & 1)
    return out


def _pack_sbdr1t2d(trits: list[int], k_dim: int, n_dim: int) -> bytes:
    w64 = (k_dim + 63) // 64
    total_words = n_dim * w64
    bp = [0] * total_words
    bn = [0] * total_words
    for k in range(k_dim):
        w = k >> 6
        bit = 1 << (k & 63)
        row_off = k * n_dim
        for n in range(n_dim):
            v = trits[row_off + n]
            if v > 0:
                bp[n * w64 + w] |= bit
            elif v < 0:
                bn[n * w64 + w] |= bit
    mask_words = (total_words + 63) // 64
    mask = [0] * mask_words
    payload = bytearray()
    for wi in range(total_words):
        if (bp[wi] | bn[wi]) != 0:
            mask[wi >> 6] |= 1 << (wi & 63)
            payload.extend(struct.pack("<QQ", bp[wi], bn[wi]))
    out = bytearray(b"SBDR1T2D")
    out.extend(struct.pack("<QQ", k_dim, n_dim))
    for mw in mask:
        out.extend(struct.pack("<Q", mw))
    out.extend(payload)
    return bytes(out)


def _write_pinned_bitnet_2b_shard(path: Path) -> None:
    rows, cols = 256, 2048
    vals = [(((i * 29) % 13) - 6) * 0.125 for i in range(rows * cols)]
    blob = struct.pack(f"<{len(vals)}e", *vals)
    header = {
        "__metadata__": {"format": "pt", "architecture": "bitnet-b1.58-2B-4T"},
        "model.layers.0.mlp.gate_proj.weight": {
            "dtype": "F16",
            "shape": [rows, cols],
            "data_offsets": [0, len(blob)],
        },
    }
    hdr = json.dumps(header, separators=(",", ":")).encode("utf-8")
    path.write_bytes(struct.pack("<Q", len(hdr)) + hdr + blob)


def _ingest_bitnet_streaming(
    path: Path, *, chunk_bytes: int, emit_sbdr2d: bool = True
) -> tuple[dict[str, Any], bytes | None]:
    chunk_elems = max(4096, chunk_bytes // 2)
    sbdr2d_blob: bytes | None = None
    with path.open("rb") as f:
        header_len = struct.unpack("<Q", f.read(8))[0]
        header = json.loads(f.read(header_len).decode("utf-8"))
        data_base = 8 + header_len
        manifest_hasher = hashlib.sha3_512()
        tensors_meta = []
        total_trits = 0
        total_dualrail_bytes = 0
        for name in sorted(k for k in header.keys() if k != "__metadata__"):
            info = header[name]
            shape = info["shape"]
            start, end = info["data_offsets"]
            n_elems = (end - start) // 2

            # Pass 1: streaming absmean gamma in O(1) RAM.
            abs_sum = 0.0
            f.seek(data_base + start)
            rem = n_elems
            while rem > 0:
                take = min(rem, chunk_elems)
                vals = _decode_f16(f.read(take * 2))
                abs_sum += sum(abs(v) for v in vals)
                rem -= take
            gamma = max(abs_sum / max(n_elems, 1), 1e-6)

            # Pass 2: streaming BitNet b1.58 quantization + Dual-Rail packing.
            pos_chunks: list[bytes] = []
            neg_chunks: list[bytes] = []
            f.seek(data_base + start)
            rem = n_elems
            while rem > 0:
                take = min(rem, chunk_elems)
                vals = _decode_f16(f.read(take * 2))
                trits = [
                    int(math.floor(max(-1.0, min(1.0, v / gamma)) + 0.5))
                    if v >= 0.0
                    else int(math.ceil(max(-1.0, min(1.0, v / gamma)) - 0.5))
                    for v in vals
                ]
                p_b, n_b = _pack_dualrail_64(trits)
                pos_chunks.append(p_b)
                neg_chunks.append(n_b)
                rem -= take

            pos_all = b"".join(pos_chunks)
            neg_all = b"".join(neg_chunks)
            digest = _sha3_512_prefixed(pos_all + neg_all)
            manifest_hasher.update(f"{name}|{shape}|{gamma:.9f}|{digest}\n".encode("utf-8"))
            total_trits += n_elems
            total_dualrail_bytes += len(pos_all) + len(neg_all)
            if emit_sbdr2d and len(shape) == 2:
                rows, cols = shape
                sbdr2d_blob = _pack_sbdr1t2d(
                    _unpack_dualrail_64(pos_all, neg_all, rows * cols), rows, cols
                )
            tensors_meta.append(
                {
                    "name": name,
                    "shape": shape,
                    "absmean_gamma": round(gamma, 9),
                    "dualrail_bytes": len(pos_all) + len(neg_all),
                    "sha3_512": digest,
                }
            )
    result = {
        "mode": "streaming",
        "chunk_bytes": chunk_bytes,
        "total_trits": total_trits,
        "total_dualrail_bytes": total_dualrail_bytes,
        "compression_vs_i8": round(total_trits / max(total_dualrail_bytes, 1), 3),
        "manifest_sha3_512": f"sha3-512:{manifest_hasher.hexdigest()}",
        "tensors": tensors_meta,
    }
    return result, sbdr2d_blob


def _extract_canonical_signing_payload(raw_text: str) -> str:
    pat = '"payload":'
    start = raw_text.find(pat)
    end = raw_text.find(',"ed25519_signature_b64":')
    if start == -1 or end == -1 or end <= start + len(pat):
        raise ValueError("receipt_raw.json missing canonical payload slice")
    return raw_text[start + len(pat) : end]


def verify_forensic_bundle(bundle_dir: Path) -> dict[str, Any]:
    """Verify an exported ``t81 receipt export`` directory 100% offline.

    Checks:
      1. ``manifest.sha3-512`` matches recomputed SHA3-512 of ``bundle.json``
         and ``receipt_raw.json``.
      2. ``bundle.json``'s ``raw_sha3_512`` matches ``receipt_raw.json``.
      3. Native C++23 Ed25519 ``t81.verify_dag`` verifies the exact canonical
         signing payload, signature, and public key from ``receipt_raw.json``.
    """
    bundle_path = bundle_dir / "bundle.json"
    raw_path = bundle_dir / "receipt_raw.json"
    manifest_path = bundle_dir / "manifest.sha3-512"

    bundle_bytes = bundle_path.read_bytes()
    raw_bytes = raw_path.read_bytes()
    manifest_lines = [
        line.strip()
        for line in manifest_path.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]

    expected_hashes: dict[str, str] = {}
    for line in manifest_lines:
        parts = line.split()
        if len(parts) == 2:
            expected_hashes[parts[1]] = parts[0]

    actual_bundle_sha = _sha3_512_prefixed(bundle_bytes)
    actual_raw_sha = _sha3_512_prefixed(raw_bytes)
    manifest_ok = (
        expected_hashes.get("bundle.json") == actual_bundle_sha
        and expected_hashes.get("receipt_raw.json") == actual_raw_sha
    )

    bundle_obj = json.loads(bundle_bytes.decode("utf-8"))
    raw_text = raw_bytes.decode("utf-8")
    raw_obj = json.loads(raw_text)
    payload_json = _extract_canonical_signing_payload(raw_text)

    dag_res = t81.verify_dag(
        [
            {
                "bundle_id": bundle_obj["receipt_canonfs_ref"],
                "parent_bundle_ids": [],
                "canonical_signing_payload": payload_json,
                "signature_b64": raw_obj["ed25519_signature_b64"],
                "public_key_pem": raw_obj["public_key_pem"],
            }
        ]
    )
    ed25519_ok = bool(dag_res.get("ok")) and dag_res.get("verified_nodes") == 1
    raw_ref_ok = bundle_obj.get("raw_sha3_512") == actual_raw_sha

    return {
        "ok": bool(manifest_ok and raw_ref_ok and ed25519_ok),
        "manifest_ok": manifest_ok,
        "raw_sha3_512_ok": raw_ref_ok,
        "ed25519_dag_ok": ed25519_ok,
        "ed25519_error_code": dag_res.get("code", ""),
        "verified_nodes": dag_res.get("verified_nodes", 0),
        "receipt_canonfs_ref": bundle_obj.get("receipt_canonfs_ref", ""),
        "bundle_sha3_512": actual_bundle_sha,
        "raw_sha3_512": actual_raw_sha,
    }


def _make_tampered_copy(bundle_dir: Path, tamper_dir: Path) -> None:
    shutil.copytree(bundle_dir, tamper_dir)
    raw_path = tamper_dir / "receipt_raw.json"
    raw_text = raw_path.read_text(encoding="utf-8")
    # Flip 1 byte inside the signed canonical payload while keeping valid JSON.
    needle = '"agent_id":"stranger-proof"'
    replacement = '"agent_id":"stranger-prooX"'
    if needle not in raw_text:
        raise RuntimeError("expected agent_id marker not found in receipt_raw.json")
    raw_path.write_text(raw_text.replace(needle, replacement, 1), encoding="utf-8")


def run_stranger_proof(
    *,
    chunk_bytes: int = 1048576,
    keep_dir: Path | None = None,
    t81_bin: Path | None = None,
) -> dict[str, Any]:
    """Run the full 3-stage outsider-reproducible proof and return structured JSON."""
    t0 = time.monotonic()
    cli_bin = t81_bin if t81_bin is not None else t81.cli_path()

    with tempfile.TemporaryDirectory(prefix="t81-stranger-proof-") as outer_td:
        outer = Path(outer_td)
        workspace = outer / "mcp_root"
        workspace.mkdir()
        bak_dir = workspace / "bak"
        bak_dir.mkdir()
        outside_file = outer / "outside_secret.txt"
        outside_file.write_text("do-not-read-outside-root\n", encoding="utf-8")

        # ── Stage 1: Pinned BitNet-2B Streaming Ingest + CanonFS Persistence ─
        shard_path = workspace / "bitnet_b158_2b_shard0.safetensors"
        _write_pinned_bitnet_2b_shard(shard_path)

        ingest_a, sbdr2d_blob = _ingest_bitnet_streaming(
            shard_path, chunk_bytes=chunk_bytes, emit_sbdr2d=True
        )
        ingest_b, _ = _ingest_bitnet_streaming(
            shard_path, chunk_bytes=max(chunk_bytes // 2, 16384), emit_sbdr2d=False
        )
        if ingest_a["manifest_sha3_512"] != ingest_b["manifest_sha3_512"]:
            raise AssertionError("BitNet-2B streaming chunk-size determinism failed")
        if ingest_a["manifest_sha3_512"] != PINNED_BITNET_2B_MANIFEST_SHA3_512:
            raise AssertionError(
                f"Pinned BitNet-2B manifest mismatch: {ingest_a['manifest_sha3_512']}"
            )
        if ingest_a["tensors"][0]["sha3_512"] != PINNED_BITNET_2B_TENSOR_SHA3_512:
            raise AssertionError("Pinned BitNet-2B tensor digest mismatch")
        if ingest_a["compression_vs_i8"] != 4.0:
            raise AssertionError(f"Expected 4.0x vs I8, got {ingest_a['compression_vs_i8']}")
        if sbdr2d_blob is None or sbdr2d_blob[:8] != b"SBDR1T2D":
            raise AssertionError("Invalid SBDR1T2D header")

        # Persist SBDR1T2D shard in CanonFS and verify bit-exact recovery.
        canonfs_driver = t81.make_persistent_driver(str(workspace / ".t81_canonfs"))
        sbdr_ref = canonfs_driver.write_object(t81.ObjectType.RawBlock, sbdr2d_blob)
        recovered = canonfs_driver.read_object_bytes(sbdr_ref)
        if recovered != sbdr2d_blob:
            raise AssertionError("CanonFS SBDR1T2D round-trip mismatch")

        peak_rss_mb = _peak_rss_mb()
        if peak_rss_mb >= PEAK_RSS_LIMIT_MB:
            raise AssertionError(
                f"Peak RSS {peak_rss_mb:.2f} MiB exceeded {PEAK_RSS_LIMIT_MB:.0f} MiB budget"
            )

        # ── Stage 2: Governed DPE + MCP Session (`t81 mcp serve`) ────────────
        state_a_bytes = b'{"plane":"deterministic","state":"A","model":"unloaded"}\n'
        state_c_bytes = (
            json.dumps(
                {
                    "plane": "deterministic",
                    "state": "C",
                    "model": "bitnet-b1.58-2B-4T-shard0",
                    "bitnet_manifest_sha3_512": ingest_a["manifest_sha3_512"],
                    "sbdr2d_canonfs_ref": sbdr_ref.hex(),
                },
                sort_keys=True,
                separators=(",", ":"),
            )
            + "\n"
        ).encode("utf-8")
        untrusted_bytes = b'{"plane":"stochastic","state":"UNTRUSTED_BYPASS"}\n'

        target_path = workspace / "state.json"
        target_path.write_bytes(state_a_bytes)
        state_a_ref = _sha3_512_prefixed(state_a_bytes)
        state_c_expected = _sha3_512_prefixed(state_c_bytes)

        # Dual-Rail 64-trit bitplanes for t81.diff_check (1/64 flip = 1.5625% <= 5.0%).
        base_pos = workspace / "base_pos.bin"
        base_neg = workspace / "base_neg.bin"
        cand_pos = workspace / "cand_pos.bin"
        cand_neg = workspace / "cand_neg.bin"
        base_pos.write_bytes(b"\x0f\x00\x00\x00\x00\x00\x00\x00")
        base_neg.write_bytes(b"\xf0\x00\x00\x00\x00\x00\x00\x00")
        cand_pos.write_bytes(b"\x0e\x00\x00\x00\x00\x00\x00\x00")
        cand_neg.write_bytes(b"\xf0\x00\x00\x00\x00\x00\x00\x00")

        policy_path = workspace / "axion_root.apl"
        policy_path.write_text(
            "(policy\n"
            "  (tier 1)\n"
            f'  (allowed-tensor-hashes ["{state_a_ref}" "{state_c_expected}"]))\n',
            encoding="utf-8",
        )
        os.chmod(policy_path, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

        try:
            proc = subprocess.Popen(
                [str(cli_bin), "mcp", "serve", "--root", str(workspace)],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
            )
            rid = 0

            def mcp_call(name: str, arguments: dict[str, Any]) -> dict[str, Any]:
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
                line = proc.stdout.readline()
                if not line:
                    err = proc.stderr.read() if proc.stderr else ""
                    raise RuntimeError(f"MCP EOF on {name}: {err}")
                return json.loads(line)

            try:
                # 2a. Hybrid Dual-Rail flip-budget check.
                diff_resp = mcp_call(
                    "t81.diff_check",
                    {
                        "base_pos_path": str(base_pos),
                        "base_neg_path": str(base_neg),
                        "candidate_pos_path": str(cand_pos),
                        "candidate_neg_path": str(cand_neg),
                        "max_flip_pct": 5.0,
                    },
                )["result"]["structured"]
                if not (
                    diff_resp.get("ok") is True
                    and diff_resp.get("allowed") is True
                    and diff_resp.get("verdict") == "ADMIT_WITHIN_FLIP_BUDGET"
                ):
                    raise AssertionError(f"Unexpected diff_check verdict: {diff_resp}")

                # 2b. Fail-closed check 1: Direct Stochastic -> Deterministic bypass denied.
                b_bypass = mcp_call(
                    "t81.tx_begin",
                    {
                        "target_path": str(target_path),
                        "backup_dir": str(bak_dir),
                        "agent_id": "stranger-bypass",
                        "isolated": True,
                    },
                )["result"]["structured"]
                Path(b_bypass["staging_path"]).write_bytes(untrusted_bytes)
                c_bypass = mcp_call(
                    "t81.tx_commit",
                    {
                        "tx_id": b_bypass["tx_id"],
                        "policy_path": str(policy_path),
                        "agent_id": "stranger-bypass",
                        "source_plane": "stochastic",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "reproducibility_class": "external_probabilistic",
                        "hybrid_reconciled": False,
                        "state_a_ref": state_a_ref,
                    },
                )["result"]["structured"]
                if c_bypass.get("ok") is not False or c_bypass.get("code") != "ERR_PLANE_BYPASS_DENIED":
                    raise AssertionError(f"Expected ERR_PLANE_BYPASS_DENIED, got {c_bypass}")
                if target_path.read_bytes() != state_a_bytes:
                    raise AssertionError("Denied bypass mutated state.json")

                # 2c. Fail-closed check 2: Out-of-root path escape denied by MCP jail.
                escape_raw = mcp_call(
                    "t81.gate_check",
                    {
                        "artifact_path": str(outside_file),
                        "policy_path": str(policy_path),
                    },
                )
                escape_result = escape_raw.get("result", {})
                escape_struct = escape_result.get("structured", {})
                out_of_root_denied = (
                    "error" in escape_raw
                    or escape_result.get("isError") is True
                    or escape_struct.get("ok") is False
                )
                if not out_of_root_denied:
                    raise AssertionError(f"Out-of-root path was not denied: {escape_raw}")

                # 2d. Positive governed commit (+1) linking BitNet-2B manifest -> State C.
                b_ok = mcp_call(
                    "t81.tx_begin",
                    {
                        "target_path": str(target_path),
                        "backup_dir": str(bak_dir),
                        "agent_id": "stranger-proof",
                        "isolated": True,
                    },
                )["result"]["structured"]
                Path(b_ok["staging_path"]).write_bytes(state_c_bytes)
                c_ok = mcp_call(
                    "t81.tx_commit",
                    {
                        "tx_id": b_ok["tx_id"],
                        "policy_path": str(policy_path),
                        "agent_id": "stranger-proof",
                        "source_plane": "hybrid",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "reproducibility_class": "seeded_probabilistic",
                        "hybrid_reconciled": True,
                        "state_a_ref": state_a_ref,
                        "expected_manifest_sha3_512": state_c_expected,
                    },
                )["result"]["structured"]
                if c_ok.get("ok") is not True or c_ok.get("trit") != 1:
                    raise AssertionError(f"Governed tx_commit failed: {c_ok}")
                if c_ok.get("state_c_ref") != state_c_expected:
                    raise AssertionError("Committed state_c_ref mismatch")
                receipt_ref = c_ok["evidence_canonfs_ref"]
                if len(receipt_ref) != 64:
                    raise AssertionError(f"Invalid 64-hex receipt ref: {receipt_ref}")
            finally:
                if proc.stdin:
                    proc.stdin.close()
                if proc.stdout:
                    proc.stdout.close()
                if proc.stderr:
                    proc.stderr.close()
                proc.wait(timeout=5)

            # ── Stage 3: Offline Forensic Bundle Export & Verification ───────
            bundle_dir = outer / "forensic_bundle"
            exp = subprocess.run(
                [
                    str(cli_bin),
                    "receipt",
                    "export",
                    "--receipt",
                    receipt_ref,
                    "--ledger",
                    str(workspace / ".t81_receipt_ledger"),
                    "--out",
                    str(bundle_dir),
                    "--json",
                ],
                capture_output=True,
                text=True,
                check=True,
            )
            exp_json = json.loads(exp.stdout)
            if not (exp_json.get("ok") and exp_json.get("ed25519_verified")):
                raise AssertionError(f"receipt export failed: {exp_json}")

            # Verify exported bundle 100% offline (MCP server is stopped).
            offline_ok = verify_forensic_bundle(bundle_dir)
            if not offline_ok["ok"]:
                raise AssertionError(f"Offline verification failed: {offline_ok}")

            # Negative 1-byte tamper check.
            tamper_dir = outer / "forensic_bundle_tampered"
            _make_tampered_copy(bundle_dir, tamper_dir)
            offline_tampered = verify_forensic_bundle(tamper_dir)
            if offline_tampered["ok"] or offline_tampered["ed25519_dag_ok"]:
                raise AssertionError(f"Tampered bundle was not rejected: {offline_tampered}")
            if offline_tampered["ed25519_error_code"] != "ERR_RECEIPT_VERIFY_FAILED":
                raise AssertionError(
                    f"Expected ERR_RECEIPT_VERIFY_FAILED, got {offline_tampered}"
                )

            saved_bundle_path: str | None = None
            if keep_dir is not None:
                dst = keep_dir / "forensic_bundle"
                if dst.exists():
                    shutil.rmtree(dst)
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copytree(bundle_dir, dst)
                saved_bundle_path = str(dst)

        finally:
            if policy_path.exists():
                os.chmod(policy_path, stat.S_IRUSR | stat.S_IWUSR)

    elapsed = round(time.monotonic() - t0, 3)
    return {
        "schema": "t81.stranger-proof.v1",
        "ok": True,
        "elapsed_seconds": elapsed,
        "bitnet_2b": {
            "total_trits": ingest_a["total_trits"],
            "total_dualrail_bytes": ingest_a["total_dualrail_bytes"],
            "compression_vs_i8": ingest_a["compression_vs_i8"],
            "manifest_sha3_512": ingest_a["manifest_sha3_512"],
            "tensor_sha3_512": ingest_a["tensors"][0]["sha3_512"],
            "sbdr2d_canonfs_ref": sbdr_ref.hex(),
            "chunk_bytes": chunk_bytes,
            "peak_rss_mb": peak_rss_mb,
            "peak_rss_limit_mb": PEAK_RSS_LIMIT_MB,
        },
        "mcp_session": {
            "diff_check_verdict": diff_resp["verdict"],
            "plane_bypass_code": c_bypass["code"],
            "plane_bypass_trit": c_bypass["trit"],
            "out_of_root_denied": out_of_root_denied,
            "commit_trit": c_ok["trit"],
            "state_a_ref": c_ok["state_a_ref"],
            "state_c_ref": c_ok["state_c_ref"],
            "receipt_canonfs_ref": receipt_ref,
        },
        "offline_verification": {
            "manifest_ok": offline_ok["manifest_ok"],
            "ed25519_dag_ok": offline_ok["ed25519_dag_ok"],
            "verified_nodes": offline_ok["verified_nodes"],
            "bundle_sha3_512": offline_ok["bundle_sha3_512"],
            "tamper_rejected": not offline_tampered["ok"],
            "tamper_manifest_ok": offline_tampered["manifest_ok"],
            "tamper_ed25519_dag_ok": offline_tampered["ed25519_dag_ok"],
            "tamper_error_code": offline_tampered["ed25519_error_code"],
            "saved_bundle_dir": saved_bundle_path,
        },
    }


def _print_human_summary(res: dict[str, Any]) -> None:
    b = res["bitnet_2b"]
    m = res["mcp_session"]
    v = res["offline_verification"]
    print("T81 Outsider-Reproducible Proof (v2.3.0) — PASS")
    print(f"  Elapsed:               {res['elapsed_seconds']}s")
    print("  [1/3] BitNet-2B Streaming Ingest + CanonFS:")
    print(f"    Manifest SHA3-512:   {b['manifest_sha3_512']}")
    print(f"    SBDR1T2D CanonFS:    {b['sbdr2d_canonfs_ref']}")
    print(f"    Compression vs INT8: {b['compression_vs_i8']}x ({b['total_trits']} trits -> {b['total_dualrail_bytes']} B)")
    print(f"    Peak RSS:            {b['peak_rss_mb']} MiB (< {b['peak_rss_limit_mb']:.0f} MiB limit)")
    print("  [2/3] Governed DPE + MCP Session (t81 mcp serve):")
    print(f"    Dual-Rail Diff Gate: {m['diff_check_verdict']}")
    print(f"    Plane Bypass Denied: {m['plane_bypass_code']} (trit={m['plane_bypass_trit']})")
    print(f"    Out-of-Root Denied:  {m['out_of_root_denied']}")
    print(f"    Governed Commit:     trit=+{m['commit_trit']} (receipt={m['receipt_canonfs_ref']})")
    print("  [3/3] Offline Forensic Export + Ed25519 verify_dag + Tamper Check:")
    print(f"    Offline Manifest:    PASS (bundle={v['bundle_sha3_512'][:28]}...)")
    print(f"    Offline Ed25519 DAG: PASS (verified_nodes={v['verified_nodes']})")
    print(f"    1-Byte Tamper Check: REJECTED ({v['tamper_error_code']})")
    if v["saved_bundle_dir"]:
        print(f"    Saved Bundle Dir:    {v['saved_bundle_dir']}")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Run or verify the T81 outsider-reproducible BitNet-2B + signed MCP proof."
    )
    ap.add_argument(
        "--chunk-bytes",
        type=int,
        default=1048576,
        help="Streaming chunk budget in bytes for BitNet-2B ingest (default: 1 MiB).",
    )
    ap.add_argument(
        "--keep-dir",
        type=Path,
        default=None,
        help="Optional directory where the exported forensic_bundle/ will be preserved.",
    )
    ap.add_argument(
        "--verify-bundle",
        type=Path,
        default=None,
        help="Verify an existing exported forensic_bundle/ directory offline and exit.",
    )
    ap.add_argument(
        "--t81-bin",
        type=Path,
        default=None,
        help="Optional explicit path to the native t81 CLI binary.",
    )
    ap.add_argument(
        "--json",
        action="store_true",
        help="Emit machine-readable JSON output.",
    )
    args = ap.parse_args(argv)

    if args.verify_bundle is not None:
        vres = verify_forensic_bundle(args.verify_bundle)
        if args.json:
            print(json.dumps(vres, indent=2))
        else:
            status = "PASS" if vres["ok"] else "FAIL"
            print(
                f"Offline Forensic Bundle Verification: {status} "
                f"(manifest_ok={vres['manifest_ok']}, ed25519_dag_ok={vres['ed25519_dag_ok']}, "
                f"ref={vres['receipt_canonfs_ref']})"
            )
        return 0 if vres["ok"] else 1

    res = run_stranger_proof(
        chunk_bytes=args.chunk_bytes,
        keep_dir=args.keep_dir,
        t81_bin=args.t81_bin,
    )
    if args.json:
        print(json.dumps(res, indent=2))
    else:
        _print_human_summary(res)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
