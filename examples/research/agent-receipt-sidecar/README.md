# T81 Agent-Native Receipt & Policy Sidecar (MCP / JSON-RPC 2.0)

> **New here?** Start with the **[60-Second Quickstart](QUICKSTART.md)** (`./build/t81 mcp serve` in `~110 µs` or signed `RFC-00D4` receipts in `~10 ms`).

This prototype implements Experiments 1 & 2 for using T81 as a deterministic **Flight Recorder & Gatekeeper** around autonomous AI agents:

- **[`t81_receipt_sidecar.py`](t81_receipt_sidecar.py)**: A warm stdio JSON-RPC 2.0 / MCP-compatible server exposing:
  - `t81.gate_and_seal` — fail-closed Axion `.apl` policy admission + `t81 canonfs import` + signed `RFC-00D4` receipt.
  - `t81.verify_receipt` — offline verification of `RFC-00D4` schema, trusted `.apl` policy hash/admission, Ed25519 signature, and optional CanonFS `fsck --verify-hashes`.
  - `t81.canonfs_fsck` — direct `t81 canonfs fsck --verify-hashes --json` wrapper.
  - `t81.model_diff` — direct `t81 model diff --json` wrapper for AI-to-AI model review.
- **[`run_agent_gauntlet.py`](run_agent_gauntlet.py)**: Runs the 5-scenario Compromised-Agent Gauntlet (Experiment 1) and a 100-call warm resident verification latency benchmark (Experiment 2).

## Run

```bash
python3 examples/research/agent-receipt-sidecar/run_agent_gauntlet.py
bash scripts/ci/run_agent_receipt_dogfood_smoke.sh
```

## MCP Client Configuration

See [`mcp_config.example.json`](mcp_config.example.json) for registering `t81-receipt-sidecar` as a stdio MCP server in an AI coding agent.

## Explicit Scope & Limits

- Uses `openssl pkeyutl` (Ed25519) for detached signatures; native C++ key management in `crypto/` remains future work.
- Fast-path `t81.verify_receipt` verifies policy, signature, and optional CanonFS integrity in milliseconds; full `t81 ai task assess-fixed` VM replay remains available for offline/high-consequence audit via [`examples/research/accountable-decision-bundle/adb_tool.py`](../accountable-decision-bundle/adb_tool.py).
