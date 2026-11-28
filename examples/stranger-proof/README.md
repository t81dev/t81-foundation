# Verify in 60 Seconds: Outsider-Reproducible BitNet-2B + Signed MCP Proof

This walkthrough proves T81 Foundation's core deterministic execution, policy-gated MCP airlock, and offline Ed25519 receipt verification using **only the installed `t81` Python package** — no repository clone, no C++ compiler, and no network access once installed.

## 1. Install `t81`

```bash
pip install t81
```

The `t81` wheel (`CPython 3.9–3.14` on Linux `x86_64`/`aarch64`, macOS `arm64`/`x86_64`, and Windows `AMD64`) bundles both the compiled C++23 `_t81` extension and the native `t81` CLI binary (`t81 mcp serve`, `t81 receipt export`).

## 2. Run the Single-Command Proof

Run from any empty directory (for example, `cd /tmp`):

```bash
python -m t81.examples.stranger_proof --keep-dir ./t81_proof_out
```

(Or invoke the installed console script directly: `t81-stranger-proof --keep-dir ./t81_proof_out`.)

### What It Executes & Asserts

1. **Pinned BitNet-2B Streaming Ingest (`--chunk-bytes`, Peak RSS `< 512 MiB`):**
   - Materializes a deterministic `256x2048` F16 BitNet b1.58-2B representative shard (`524,288` weights, `model.layers.0.mlp.gate_proj.weight`).
   - Streams it in bounded chunks (default `1 MiB`) into Dual-Rail `(P, N)` bitplanes (`4.0x` vs INT8) and `SBDR1T2D`, persists the `SBDR1T2D` blob in `CanonFS`, verifies bit-exact round-trip and chunk-size determinism against the published pinned manifest digest, and asserts peak RSS `< 512 MiB`.
2. **Governed DPE + MCP Session (`t81 mcp serve --root <tmp>`):**
   - Launches an ephemeral native C++ MCP server jailed to a temporary `--root` with an immutable (`0444`) `axion_root.apl` policy.
   - Audits a 64-trit Dual-Rail flip budget via `t81.diff_check` (`ADMIT_WITHIN_FLIP_BUDGET`).
   - Proves fail-closed denial on a direct Stochastic -> Deterministic bypass (`ERR_PLANE_BYPASS_DENIED`, `trit=-1`, zero file mutation) and on an out-of-root path escape attempt.
   - Commits the BitNet-2B manifest-backed `State C` (`trit=+1`) via `t81.tx_begin` + `t81.tx_commit`, sealing an Ed25519 receipt in `.t81_receipt_ledger`.
3. **Offline Forensic Bundle Export, `verify_dag` & 1-Byte Tamper Rejection:**
   - Exports the receipt via `t81 receipt export`, stops the MCP server, and verifies `manifest.sha3-512` + native C++23 RFC 8032 / Wycheproof Ed25519 `t81.verify_dag` completely offline.
   - Flips 1 byte in a copy of `receipt_raw.json` and verifies fail-closed rejection (`ERR_RECEIPT_VERIFY_FAILED`).

## 3. Standalone Offline Verification

With no server running and no network access, re-verify the exported forensic bundle directory independently:

```bash
python -m t81.examples.stranger_proof --verify-bundle ./t81_proof_out/forensic_bundle
```

For machine-readable output, pass `--json` to either command.

## Expected Pinned Output

```text
T81 Outsider-Reproducible Proof (v2.3.0) — PASS
  [1/3] BitNet-2B Streaming Ingest + CanonFS:
    Manifest SHA3-512:   sha3-512:3e0514756617d33945f0df4acfe3dbee222eafb3ccf92cd75861329332a812ca8f27364362449a697016cac02e384d55c3bb59979810d15b570d6b9046c45df3
    Compression vs INT8: 4.0x (524288 trits -> 131072 B)
    Peak RSS:            < 512 MiB
  [2/3] Governed DPE + MCP Session (t81 mcp serve):
    Dual-Rail Diff Gate: ADMIT_WITHIN_FLIP_BUDGET
    Plane Bypass Denied: ERR_PLANE_BYPASS_DENIED (trit=-1)
    Out-of-Root Denied:  True
    Governed Commit:     trit=+1
  [3/3] Offline Forensic Export + Ed25519 verify_dag + Tamper Check:
    Offline Manifest:    PASS
    Offline Ed25519 DAG: PASS (verified_nodes=1)
    1-Byte Tamper Check: REJECTED (ERR_RECEIPT_VERIFY_FAILED)
```

## Scope & Research Boundaries

This walkthrough demonstrates implemented deterministic storage, policy-gated MCP execution, and offline cryptographic verification mechanisms on conventional binary hardware. It does **not** establish production readiness, general ternary speed superiority, or physical ternary silicon advantages. Active research holds (`SCG-08 Gate 2 NO-GO`, `F01-HOST-BOLQ-v1 NOT ADOPTED`, `AXION-AI-001`, `H01` physical evaluation blocked, `T81-AIOPT-001`, Horizon D `DEC-004` Planned / Design Target) remain unchanged.
