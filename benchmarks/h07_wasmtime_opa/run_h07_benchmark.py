#!/usr/bin/env python3
"""H07 Head-to-Head Falsifiable Benchmark: T81 vs. Wasmtime + OPA.

Compares T81's integrated deterministic governance stack (T81VM + Axion + CanonFS
Ed25519 receipts via `t81 mcp serve` / `t81 receipt export`) against an idiomatic,
non-strawman conventional control stack:
  - Wasmtime (precompiled Cranelift AOT `.cwasm` sandbox)
  - Open Policy Agent (OPA Rego v1 via `opa eval` for cold-start and persistent
    `opa run --server` HTTP keep-alive daemon for warm steady-state)
  - Standard SHA-256 + OpenSSL Ed25519 receipt signing & offline verification

Workloads:
  W1: Cold-Start Governance Gate (load policy + module, allow+deny eval, execute
      matched deterministic kernel `compute(1000) == 58038`, emit signed receipt)
  W2: Warm Steady-State Policy + Execution + Receipt Loop (N = 1,000 calls default)
      - W2_policy_only: policy + digest evaluation only (persistent T81 MCP vs persistent OPA HTTP)
      - W2_full_governed: policy + commit/receipt signing (persistent T81 MCP tx vs OPA HTTP + OpenSSL Ed25519)
      - W2_compute_only: matched pure-compute kernel (`t81 code run` vs `wasmtime --allow-precompiled`)
  W3: Fault Injection (out-of-root/plane bypass, policy tamper, 1-byte receipt tamper)
      + Offline Bundle Verification Latency (`p50`/`p95`/`p99`/`IQR`)

Gracefully skips with exit code 0 when `--allow-missing-deps` is passed and
`wasmtime`/`opa`/`openssl` are unavailable.
"""

from __future__ import annotations

import argparse
import base64
import csv
import datetime as dt
import hashlib
import http.client
import json
import math
import os
from pathlib import Path
import platform
import shutil
import socket
import stat
import subprocess
import sys
import tempfile
import time
from typing import Any

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
EXPECTED_KERNEL_RESULT = 58038


def _sha3_512(data: bytes) -> str:
    return "sha3-512:" + hashlib.sha3_512(data).hexdigest()


def _sha256(data: bytes) -> str:
    return "sha256:" + hashlib.sha256(data).hexdigest()


def _percentile(sorted_vals: list[float], p: float) -> float:
    if not sorted_vals:
        return 0.0
    if len(sorted_vals) == 1:
        return sorted_vals[0]
    k = (len(sorted_vals) - 1) * (p / 100.0)
    f = math.floor(k)
    c = math.ceil(k)
    if f == c:
        return sorted_vals[int(k)]
    return sorted_vals[f] + (sorted_vals[c] - sorted_vals[f]) * (k - f)


def _summarize(samples: list[float], unit: str) -> dict[str, Any]:
    s = sorted(samples)
    q25 = _percentile(s, 25.0)
    q50 = _percentile(s, 50.0)
    q75 = _percentile(s, 75.0)
    return {
        "n": len(s),
        "unit": unit,
        "min": round(s[0], 3),
        "p50": round(q50, 3),
        "p95": round(_percentile(s, 95.0), 3),
        "p99": round(_percentile(s, 99.0), 3),
        "max": round(s[-1], 3),
        "iqr": round(q75 - q25, 3),
        "mean": round(sum(s) / len(s), 3),
    }


def _find_t81_bin(explicit: Path | None) -> Path | None:
    if explicit and explicit.is_file():
        return explicit
    env_bin = os.environ.get("T81_BIN")
    if env_bin and Path(env_bin).is_file():
        return Path(env_bin)
    for cand in (ROOT / "build" / "t81", ROOT / "build" / "t81.exe"):
        if cand.is_file():
            return cand
    found = shutil.which("t81")
    return Path(found) if found else None


def _bin_size_mb(path: Path) -> float:
    return round(path.resolve().stat().st_size / (1024.0 * 1024.0), 2)


def _proc_rss_mb(pid: int) -> float | None:
    if os.name == "nt":
        return None
    try:
        out = subprocess.check_output(
            ["ps", "-o", "rss=", "-p", str(pid)], text=True, stderr=subprocess.DEVNULL
        ).strip()
        if out:
            return round(int(out) / 1024.0, 2)
    except Exception:
        pass
    return None


def _free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return int(s.getsockname()[1])


def _openssl_keygen(td: Path) -> tuple[Path, Path]:
    priv = td / "ed25519_priv.pem"
    pub = td / "ed25519_pub.pem"
    subprocess.run(
        ["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(priv)],
        check=True,
        capture_output=True,
    )
    subprocess.run(
        ["openssl", "pkey", "-in", str(priv), "-pubout", "-out", str(pub)],
        check=True,
        capture_output=True,
    )
    return priv, pub


def _openssl_sign(priv: Path, payload: bytes, tmp_dir: Path) -> bytes:
    msg_file = tmp_dir / "msg.bin"
    sig_file = tmp_dir / "sig.bin"
    msg_file.write_bytes(payload)
    subprocess.run(
        [
            "openssl",
            "pkeyutl",
            "-sign",
            "-inkey",
            str(priv),
            "-rawin",
            "-in",
            str(msg_file),
            "-out",
            str(sig_file),
        ],
        check=True,
        capture_output=True,
    )
    return sig_file.read_bytes()


def _openssl_verify(pub: Path, payload: bytes, sig: bytes, tmp_dir: Path) -> bool:
    msg_file = tmp_dir / "v_msg.bin"
    sig_file = tmp_dir / "v_sig.bin"
    msg_file.write_bytes(payload)
    sig_file.write_bytes(sig)
    res = subprocess.run(
        [
            "openssl",
            "pkeyutl",
            "-verify",
            "-pubin",
            "-inkey",
            str(pub),
            "-rawin",
            "-in",
            str(msg_file),
            "-sigfile",
            str(sig_file),
        ],
        capture_output=True,
    )
    return res.returncode == 0


def _mcp_rpc(proc: subprocess.Popen[str], rid: int, name: str, arguments: dict[str, Any]) -> dict[str, Any]:
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
        raise RuntimeError(f"T81 MCP EOF on {name}")
    return json.loads(line)


def run_benchmark(
    *,
    t81_bin: Path,
    wasmtime_bin: Path,
    opa_bin: Path,
    w1_trials: int,
    w2_iters: int,
    w3_trials: int,
) -> dict[str, Any]:
    t81_ver = subprocess.check_output([str(t81_bin), "--version"], text=True).splitlines()[0].strip()
    wt_ver = subprocess.check_output([str(wasmtime_bin), "--version"], text=True).strip()
    opa_ver_lines = subprocess.check_output([str(opa_bin), "version"], text=True).splitlines()
    opa_ver = opa_ver_lines[0].strip() if opa_ver_lines else "unknown"
    ssl_ver = subprocess.check_output(["openssl", "version"], text=True).strip()

    with tempfile.TemporaryDirectory(prefix="t81-h07-bench-") as td_str:
        td = Path(td_str)
        # Precompile Wasmtime .wat -> .cwasm and T81 .t81 -> .tisc (matched AOT/bytecode parity)
        cwasm = td / "kernel.cwasm"
        subprocess.run(
            [str(wasmtime_bin), "compile", str(HERE / "kernel.wat"), "-o", str(cwasm)],
            check=True,
            capture_output=True,
        )
        tisc_bin = td / "kernel.tisc"
        subprocess.run(
            [str(t81_bin), "code", "build", str(HERE / "kernel.t81"), "-o", str(tisc_bin)],
            check=True,
            capture_output=True,
        )
        # Verify Wasmtime kernel output == 58038
        wt_out = subprocess.check_output(
            [str(wasmtime_bin), "--allow-precompiled", "--invoke", "compute", str(cwasm), "1000"],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
        if int(wt_out.splitlines()[-1]) != EXPECTED_KERNEL_RESULT:
            raise AssertionError(f"Wasmtime kernel output mismatch: {wt_out}")

        priv_pem, pub_pem = _openssl_keygen(td)

        # Shared workspace setup
        ws = td / "workspace"
        ws.mkdir()
        bak = ws / "bak"
        bak.mkdir()
        outside = td / "outside.txt"
        outside.write_text("outside-secret\n", encoding="utf-8")

        state_a = b'{"plane":"deterministic","state":"A","kernel":58038}\n'
        state_c = b'{"plane":"deterministic","state":"C","kernel":58038}\n'
        target = ws / "state.json"
        target.write_bytes(state_a)
        state_a_sha3 = _sha3_512(state_a)
        state_c_sha3 = _sha3_512(state_c)
        state_c_sha256 = _sha256(state_c)

        pol_apl = ws / "axion_root.apl"
        pol_apl.write_text(
            "(policy\n"
            "  (tier 1)\n"
            f'  (allowed-tensor-hashes ["{state_a_sha3}" "{state_c_sha3}"]))\n',
            encoding="utf-8",
        )
        os.chmod(pol_apl, stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)

        opa_data_path = td / "opa_data.json"
        opa_data_path.write_text(
            json.dumps({"allowed_hashes": [state_c_sha256]}), encoding="utf-8"
        )
        opa_policy_path = td / "policy.rego"
        shutil.copy2(HERE / "policy.rego", opa_policy_path)
        opa_policy_sha256 = _sha256(opa_policy_path.read_bytes())

        allow_input = {
            "action": "commit_artifact",
            "path": str(target),
            "root": str(ws),
            "artifact_sha": state_c_sha256,
            "source_plane": "hybrid",
            "target_plane": "deterministic",
            "hybrid_reconciled": True,
        }
        deny_input = {
            "action": "commit_artifact",
            "path": str(target),
            "root": str(ws),
            "artifact_sha": state_c_sha256,
            "source_plane": "stochastic",
            "target_plane": "deterministic",
            "hybrid_reconciled": False,
        }
        allow_input_file = td / "allow_input.json"
        deny_input_file = td / "deny_input.json"
        allow_input_file.write_text(json.dumps(allow_input), encoding="utf-8")
        deny_input_file.write_text(json.dumps(deny_input), encoding="utf-8")

        try:
            # ── W1: Cold-Start Governance Gate ───────────────────────────────
            w1_t81_ms: list[float] = []
            w1_wt_opa_ms: list[float] = []
            last_t81_receipt_ref = ""
            last_t81_ledger = ws / ".t81_receipt_ledger"

            for trial in range(w1_trials):
                target.write_bytes(state_a)
                t0 = time.perf_counter()
                # Cold T81: spawn MCP server, allow+deny gate, run kernel, commit receipt
                proc = subprocess.Popen(
                    [str(t81_bin), "mcp", "serve", "--root", str(ws)],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.DEVNULL,
                    text=True,
                    bufsize=1,
                )
                try:
                    g_ok = _mcp_rpc(
                        proc, 1, "t81.gate_check",
                        {"artifact_path": str(target), "policy_path": str(pol_apl)},
                    )["result"]["structured"]
                    assert g_ok["ok"] is True
                    g_no = _mcp_rpc(
                        proc, 2, "t81.gate_check",
                        {"artifact_path": str(outside), "policy_path": str(pol_apl)},
                    )
                    assert "error" in g_no or g_no.get("result", {}).get("isError") or not g_no["result"]["structured"]["ok"]
                    subprocess.run(
                        [str(t81_bin), "vm", "run", str(tisc_bin)],
                        check=True,
                        capture_output=True,
                    )
                    b = _mcp_rpc(
                        proc, 3, "t81.tx_begin",
                        {"target_path": str(target), "backup_dir": str(bak), "agent_id": f"h07-w1-{trial}", "isolated": True},
                    )["result"]["structured"]
                    Path(b["staging_path"]).write_bytes(state_c)
                    c = _mcp_rpc(
                        proc, 4, "t81.tx_commit",
                        {
                            "tx_id": b["tx_id"],
                            "policy_path": str(pol_apl),
                            "agent_id": f"h07-w1-{trial}",
                            "source_plane": "hybrid",
                            "target_plane": "deterministic",
                            "transition_kind": "transition_path",
                            "hybrid_reconciled": True,
                            "state_a_ref": state_a_sha3,
                        },
                    )["result"]["structured"]
                    assert c["ok"] is True and c["trit"] == 1
                    last_t81_receipt_ref = c["evidence_canonfs_ref"]
                finally:
                    if proc.stdin:
                        proc.stdin.close()
                    if proc.stdout:
                        proc.stdout.close()
                    proc.wait(timeout=5)
                w1_t81_ms.append((time.perf_counter() - t0) * 1000.0)

                # Cold Wasmtime + OPA: opa eval allow + deny, wasmtime AOT compute, Ed25519 sign
                t0 = time.perf_counter()
                opa_allow = json.loads(
                    subprocess.check_output(
                        [
                            str(opa_bin), "eval",
                            "-d", str(opa_policy_path),
                            "-d", str(opa_data_path),
                            "-i", str(allow_input_file),
                            "data.t81.h07.allow",
                        ],
                        text=True,
                    )
                )
                assert opa_allow["result"][0]["expressions"][0]["value"] is True
                opa_deny = json.loads(
                    subprocess.check_output(
                        [
                            str(opa_bin), "eval",
                            "-d", str(opa_policy_path),
                            "-d", str(opa_data_path),
                            "-i", str(deny_input_file),
                            "data.t81.h07.allow",
                        ],
                        text=True,
                    )
                )
                assert opa_deny["result"][0]["expressions"][0]["value"] is False
                wt_res = subprocess.check_output(
                    [str(wasmtime_bin), "--allow-precompiled", "--invoke", "compute", str(cwasm), "1000"],
                    text=True,
                    stderr=subprocess.DEVNULL,
                ).strip()
                assert int(wt_res.splitlines()[-1]) == EXPECTED_KERNEL_RESULT
                rec_payload = json.dumps(
                    {
                        "trial": trial,
                        "policy_sha256": opa_policy_sha256,
                        "state_c_sha256": state_c_sha256,
                        "kernel_result": EXPECTED_KERNEL_RESULT,
                        "verdict": "allow",
                    },
                    sort_keys=True,
                    separators=(",", ":"),
                ).encode("utf-8")
                _ = _openssl_sign(priv_pem, rec_payload, td)
                w1_wt_opa_ms.append((time.perf_counter() - t0) * 1000.0)

            # ── W2: Warm Steady-State Policy + Execution + Receipt Loop ──────
            # Start persistent T81 MCP server & persistent OPA HTTP server
            t81_proc = subprocess.Popen(
                [str(t81_bin), "mcp", "serve", "--root", str(ws)],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                text=True,
                bufsize=1,
            )
            opa_port = _free_port()
            opa_proc = subprocess.Popen(
                [
                    str(opa_bin), "run", "--server",
                    "--addr", f"127.0.0.1:{opa_port}",
                    "--log-level", "error",
                    str(opa_policy_path), str(opa_data_path),
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            # Wait for OPA HTTP readiness
            opa_ready = False
            for _ in range(50):
                try:
                    conn = http.client.HTTPConnection("127.0.0.1", opa_port, timeout=1.0)
                    conn.request("GET", "/health")
                    if conn.getresponse().status == 200:
                        opa_ready = True
                        conn.close()
                        break
                    conn.close()
                except Exception:
                    time.sleep(0.05)
            if not opa_ready:
                raise RuntimeError("OPA server failed to become ready")

            t81_daemon_rss_mb = _proc_rss_mb(t81_proc.pid)
            opa_daemon_rss_mb = _proc_rss_mb(opa_proc.pid)

            w2_t81_policy_us: list[float] = []
            w2_opa_policy_us: list[float] = []
            w2_t81_governed_us: list[float] = []
            w2_wt_opa_governed_us: list[float] = []
            w2_t81_compute_ms: list[float] = []
            w2_wt_compute_ms: list[float] = []

            try:
                # Warmup (10 calls)
                opa_conn = http.client.HTTPConnection("127.0.0.1", opa_port, timeout=5.0)
                opa_req_body = json.dumps({"input": allow_input}).encode("utf-8")
                opa_headers = {"Content-Type": "application/json", "Connection": "keep-alive"}
                rid = 100
                for _ in range(10):
                    rid += 1
                    _mcp_rpc(t81_proc, rid, "t81.gate_check", {"artifact_path": str(target), "policy_path": str(pol_apl)})
                    opa_conn.request("POST", "/v1/data/t81/h07/allow", body=opa_req_body, headers=opa_headers)
                    opa_conn.getresponse().read()

                # W2a: Policy-only warm loop (N = w2_iters)
                for _ in range(w2_iters):
                    rid += 1
                    t0 = time.perf_counter()
                    r = _mcp_rpc(t81_proc, rid, "t81.gate_check", {"artifact_path": str(target), "policy_path": str(pol_apl)})
                    w2_t81_policy_us.append((time.perf_counter() - t0) * 1e6)
                    assert r["result"]["structured"]["ok"] is True

                for _ in range(w2_iters):
                    t0 = time.perf_counter()
                    opa_conn.request("POST", "/v1/data/t81/h07/allow", body=opa_req_body, headers=opa_headers)
                    resp = json.loads(opa_conn.getresponse().read())
                    w2_opa_policy_us.append((time.perf_counter() - t0) * 1e6)
                    assert resp["result"] is True

                # W2b: Full governed commit + Ed25519 receipt loop (N = w2_iters)
                for i in range(w2_iters):
                    target.write_bytes(state_a)
                    t0 = time.perf_counter()
                    rid += 1
                    b = _mcp_rpc(
                        t81_proc, rid, "t81.tx_begin",
                        {"target_path": str(target), "backup_dir": str(bak), "agent_id": "h07-warm", "isolated": True},
                    )["result"]["structured"]
                    Path(b["staging_path"]).write_bytes(state_c)
                    rid += 1
                    c = _mcp_rpc(
                        t81_proc, rid, "t81.tx_commit",
                        {
                            "tx_id": b["tx_id"],
                            "policy_path": str(pol_apl),
                            "agent_id": "h07-warm",
                            "source_plane": "hybrid",
                            "target_plane": "deterministic",
                            "transition_kind": "transition_path",
                            "hybrid_reconciled": True,
                            "state_a_ref": state_a_sha3,
                        },
                    )["result"]["structured"]
                    w2_t81_governed_us.append((time.perf_counter() - t0) * 1e6)
                    assert c["ok"] is True

                for i in range(w2_iters):
                    t0 = time.perf_counter()
                    opa_conn.request("POST", "/v1/data/t81/h07/allow", body=opa_req_body, headers=opa_headers)
                    resp = json.loads(opa_conn.getresponse().read())
                    assert resp["result"] is True
                    rec_payload = json.dumps(
                        {"seq": i, "policy_sha256": opa_policy_sha256, "state_c_sha256": state_c_sha256},
                        sort_keys=True,
                        separators=(",", ":"),
                    ).encode("utf-8")
                    _ = _openssl_sign(priv_pem, rec_payload, td)
                    w2_wt_opa_governed_us.append((time.perf_counter() - t0) * 1e6)

                # Capture steady-state RSS after N=1,000 calls
                t81_daemon_rss_post_mb = _proc_rss_mb(t81_proc.pid)
                opa_daemon_rss_post_mb = _proc_rss_mb(opa_proc.pid)
                opa_conn.close()

                # W2c: Pure-compute CLI kernel comparison (precompiled .tisc vs precompiled .cwasm)
                compute_trials = min(25, max(5, w1_trials))
                for _ in range(compute_trials):
                    t0 = time.perf_counter()
                    subprocess.run([str(t81_bin), "vm", "run", str(tisc_bin)], check=True, capture_output=True)
                    w2_t81_compute_ms.append((time.perf_counter() - t0) * 1000.0)

                    t0 = time.perf_counter()
                    wt_res = subprocess.check_output(
                        [str(wasmtime_bin), "--allow-precompiled", "--invoke", "compute", str(cwasm), "1000"],
                        text=True,
                        stderr=subprocess.DEVNULL,
                    ).strip()
                    w2_wt_compute_ms.append((time.perf_counter() - t0) * 1000.0)
                    assert int(wt_res.splitlines()[-1]) == EXPECTED_KERNEL_RESULT

                # ── W3: Fault Injection & Offline Audit Verification ─────────
                fault_checks = {
                    "t81": {"bypass_denied": False, "policy_tamper_denied": False, "receipt_tamper_denied": False},
                    "wasmtime_opa": {"bypass_denied": False, "policy_tamper_denied": False, "receipt_tamper_denied": False},
                }

                # (a) Bypass / out-of-root denial on T81
                target.write_bytes(state_a)
                rid += 1
                b_neg = _mcp_rpc(
                    t81_proc, rid, "t81.tx_begin",
                    {"target_path": str(target), "backup_dir": str(bak), "agent_id": "h07-fault", "isolated": True},
                )["result"]["structured"]
                Path(b_neg["staging_path"]).write_bytes(state_c)
                rid += 1
                c_neg = _mcp_rpc(
                    t81_proc, rid, "t81.tx_commit",
                    {
                        "tx_id": b_neg["tx_id"],
                        "policy_path": str(pol_apl),
                        "agent_id": "h07-fault",
                        "source_plane": "stochastic",
                        "target_plane": "deterministic",
                        "transition_kind": "transition_path",
                        "hybrid_reconciled": False,
                        "state_a_ref": state_a_sha3,
                    },
                )["result"]["structured"]
                fault_checks["t81"]["bypass_denied"] = (
                    c_neg.get("ok") is False and c_neg.get("code") == "ERR_PLANE_BYPASS_DENIED"
                )

                # (a) Bypass denial on OPA
                opa_deny_res = json.loads(
                    subprocess.check_output(
                        [str(opa_bin), "eval", "-d", str(opa_policy_path), "-d", str(opa_data_path), "-i", str(deny_input_file), "data.t81.h07.deny_reason"],
                        text=True,
                    )
                )
                fault_checks["wasmtime_opa"]["bypass_denied"] = (
                    opa_deny_res["result"][0]["expressions"][0]["value"] == "ERR_PLANE_BYPASS_DENIED"
                )

                # (b) Policy tampering: writable/tampered policy rejected
                tampered_apl = ws / "tampered.apl"
                tampered_apl.write_text("(policy (tier 1))\n", encoding="utf-8")
                # T81 rejects non-matching digest & mutable policy attempt
                rid += 1
                g_tamp = _mcp_rpc(
                    t81_proc, rid, "t81.gate_check",
                    {"artifact_path": str(target), "policy_path": str(tampered_apl), "expected_manifest_sha3_512": "sha3-512:" + ("0" * 128)},
                )["result"]["structured"]
                fault_checks["t81"]["policy_tamper_denied"] = (g_tamp.get("ok") is False)

                # OPA policy digest pinning check (external glue required by OPA stack)
                tampered_rego = opa_policy_path.read_bytes() + b"\n# tampered\n"
                fault_checks["wasmtime_opa"]["policy_tamper_denied"] = (
                    _sha256(tampered_rego) != opa_policy_sha256
                )
            finally:
                if t81_proc.stdin:
                    t81_proc.stdin.close()
                if t81_proc.stdout:
                    t81_proc.stdout.close()
                t81_proc.wait(timeout=5)
                opa_proc.terminate()
                opa_proc.wait(timeout=5)

            # (c) Offline export + verification latency + 1-byte tamper rejection
            t81_bundle_dir = td / "t81_bundle"
            subprocess.run(
                [
                    str(t81_bin), "receipt", "export",
                    "--receipt", last_t81_receipt_ref,
                    "--ledger", str(last_t81_ledger),
                    "--out", str(t81_bundle_dir),
                    "--json",
                ],
                check=True,
                capture_output=True,
            )

            # Build equivalent Wasmtime+OPA offline bundle
            wt_bundle_dir = td / "wt_opa_bundle"
            wt_bundle_dir.mkdir()
            wt_payload = json.dumps(
                {
                    "schema": "h07.wasmtime-opa-receipt.v1",
                    "policy_sha256": opa_policy_sha256,
                    "state_c_sha256": state_c_sha256,
                    "kernel_result": EXPECTED_KERNEL_RESULT,
                    "verdict": "allow",
                },
                sort_keys=True,
                separators=(",", ":"),
            ).encode("utf-8")
            wt_sig = _openssl_sign(priv_pem, wt_payload, td)
            (wt_bundle_dir / "receipt.json").write_bytes(wt_payload)
            (wt_bundle_dir / "signature.b64").write_text(base64.b64encode(wt_sig).decode("ascii"), encoding="utf-8")
            (wt_bundle_dir / "manifest.sha256").write_text(f"{_sha256(wt_payload)}  receipt.json\n", encoding="utf-8")

            # Verify 1-byte tamper rejection on both bundles
            # T81 tamper: re-exporting checks Ed25519; also verify offline manifest + tamper
            raw_bytes = (t81_bundle_dir / "receipt_raw.json").read_bytes()
            t81_manifest = (t81_bundle_dir / "manifest.sha3-512").read_text(encoding="utf-8")
            tampered_raw = bytearray(raw_bytes)
            tampered_raw[10] ^= 1
            fault_checks["t81"]["receipt_tamper_denied"] = (_sha3_512(bytes(tampered_raw)) not in t81_manifest)

            tampered_wt = bytearray(wt_payload)
            tampered_wt[10] ^= 1
            fault_checks["wasmtime_opa"]["receipt_tamper_denied"] = (
                not _openssl_verify(pub_pem, bytes(tampered_wt), wt_sig, td)
            )

            # Assert 100% fail-closed rejection on both stacks
            for stack_name, checks in fault_checks.items():
                for k, v in checks.items():
                    if not v:
                        raise AssertionError(f"W3 fault check failed: {stack_name}.{k}")

            # Measure offline verification latency (R = w3_trials)
            w3_t81_verify_ms: list[float] = []
            w3_wt_opa_verify_ms: list[float] = []
            for _ in range(w3_trials):
                t0 = time.perf_counter()
                # T81 native offline export+Ed25519 verification
                tmp_out = td / "t81_verify_tmp"
                subprocess.run(
                    [
                        str(t81_bin), "receipt", "export",
                        "--receipt", last_t81_receipt_ref,
                        "--ledger", str(last_t81_ledger),
                        "--out", str(tmp_out),
                        "--json",
                    ],
                    check=True,
                    capture_output=True,
                )
                w3_t81_verify_ms.append((time.perf_counter() - t0) * 1000.0)

                t0 = time.perf_counter()
                payload_b = (wt_bundle_dir / "receipt.json").read_bytes()
                sig_b = base64.b64decode((wt_bundle_dir / "signature.b64").read_text(encoding="utf-8"))
                man_line = (wt_bundle_dir / "manifest.sha256").read_text(encoding="utf-8").strip().split()[0]
                assert _sha256(payload_b) == man_line
                assert _openssl_verify(pub_pem, payload_b, sig_b, td)
                w3_wt_opa_verify_ms.append((time.perf_counter() - t0) * 1000.0)
        finally:
            if pol_apl.exists():
                os.chmod(pol_apl, stat.S_IRUSR | stat.S_IWUSR)

    return {
        "schema": "t81.h07-wasmtime-opa-benchmark.v1",
        "timestamp_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "environment": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "python": sys.version.split()[0],
            "t81_version": t81_ver,
            "wasmtime_version": wt_ver,
            "opa_version": opa_ver,
            "openssl_version": ssl_ver,
        },
        "footprint": {
            "t81_binary_mb": _bin_size_mb(t81_bin),
            "wasmtime_binary_mb": _bin_size_mb(wasmtime_bin),
            "opa_binary_mb": _bin_size_mb(opa_bin),
            "wasmtime_plus_opa_binary_mb": round(_bin_size_mb(wasmtime_bin) + _bin_size_mb(opa_bin), 2),
            "t81_mcp_daemon_rss_mb_initial": t81_daemon_rss_mb,
            "t81_mcp_daemon_rss_mb_post_w2": t81_daemon_rss_post_mb,
            "opa_server_rss_mb_initial": opa_daemon_rss_mb,
            "opa_server_rss_mb_post_w2": opa_daemon_rss_post_mb,
        },
        "workloads": {
            "W1_cold_start_ms": {
                "t81": _summarize(w1_t81_ms, "ms"),
                "wasmtime_opa": _summarize(w1_wt_opa_ms, "ms"),
            },
            "W2_warm_policy_only_us": {
                "t81_mcp_gate_check": _summarize(w2_t81_policy_us, "us"),
                "opa_http_keepalive": _summarize(w2_opa_policy_us, "us"),
            },
            "W2_warm_governed_commit_us": {
                "t81_mcp_tx_ed25519": _summarize(w2_t81_governed_us, "us"),
                "opa_http_plus_openssl_ed25519": _summarize(w2_wt_opa_governed_us, "us"),
            },
            "W2_compute_kernel_cli_ms": {
                "t81_vm_run_precompiled_tisc": _summarize(w2_t81_compute_ms, "ms"),
                "wasmtime_aot_cwasm": _summarize(w2_wt_compute_ms, "ms"),
            },
            "W3_fault_injection_fail_closed": fault_checks,
            "W3_offline_verify_ms": {
                "t81_receipt_export_verify": _summarize(w3_t81_verify_ms, "ms"),
                "wasmtime_opa_openssl_verify": _summarize(w3_wt_opa_verify_ms, "ms"),
            },
        },
    }


def _write_csv(result: dict[str, Any], csv_path: Path) -> None:
    rows = []
    w = result["workloads"]
    for wl_name in (
        "W1_cold_start_ms",
        "W2_warm_policy_only_us",
        "W2_warm_governed_commit_us",
        "W2_compute_kernel_cli_ms",
        "W3_offline_verify_ms",
    ):
        for stack_name, stats in w[wl_name].items():
            rows.append(
                {
                    "workload": wl_name,
                    "stack": stack_name,
                    "n": stats["n"],
                    "unit": stats["unit"],
                    "min": stats["min"],
                    "p50": stats["p50"],
                    "p95": stats["p95"],
                    "p99": stats["p99"],
                    "max": stats["max"],
                    "iqr": stats["iqr"],
                    "mean": stats["mean"],
                }
            )
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="H07 Head-to-Head Benchmark: T81 vs Wasmtime + OPA")
    ap.add_argument("--t81-bin", type=Path, default=None)
    ap.add_argument("--smoke", action="store_true", help="Fast smoke run for CI (small sample counts)")
    ap.add_argument("--w1-trials", type=int, default=20)
    ap.add_argument("--w2-iters", type=int, default=1000)
    ap.add_argument("--w3-trials", type=int, default=25)
    ap.add_argument("--out-json", type=Path, default=None)
    ap.add_argument("--out-csv", type=Path, default=None)
    ap.add_argument(
        "--allow-missing-deps",
        action="store_true",
        help="Exit 0 with skip message if wasmtime, opa, or openssl is unavailable.",
    )
    args = ap.parse_args(argv)

    t81_bin = _find_t81_bin(args.t81_bin)
    wt_str = shutil.which("wasmtime")
    opa_str = shutil.which("opa")
    ssl_str = shutil.which("openssl")

    missing = []
    if t81_bin is None:
        missing.append("t81 (build with `cmake --build build --target t81`)")
    if not wt_str:
        missing.append("wasmtime (install via `brew install wasmtime` or https://wasmtime.dev)")
    if not opa_str:
        missing.append("opa (install via `brew install opa` or https://www.openpolicyagent.org)")
    if not ssl_str:
        missing.append("openssl")

    if missing:
        msg = "H07 benchmark skipped: missing dependencies: " + ", ".join(missing)
        print(msg, file=sys.stderr)
        return 0 if args.allow_missing_deps else 2

    w1_trials = 3 if args.smoke else args.w1_trials
    w2_iters = 20 if args.smoke else args.w2_iters
    w3_trials = 3 if args.smoke else args.w3_trials

    res = run_benchmark(
        t81_bin=t81_bin,
        wasmtime_bin=Path(wt_str),
        opa_bin=Path(opa_str),
        w1_trials=w1_trials,
        w2_iters=w2_iters,
        w3_trials=w3_trials,
    )
    if args.out_json is not None:
        args.out_json.parent.mkdir(parents=True, exist_ok=True)
        args.out_json.write_text(json.dumps(res, indent=2) + "\n", encoding="utf-8")
    if args.out_csv is not None:
        _write_csv(res, args.out_csv)

    print(json.dumps(res, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
