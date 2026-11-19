# 60-Second Quickstart: T81 Deterministic Agent Airlock (`t81 mcp serve`)

Drop T81 in front of any AI agent (Claude, Cursor, Antigravity, LangGraph, CrewAI) as a fail-closed policy gate and tamper-evident receipt verifier.

## 1. Build `t81` (if not already built)

```bash
cmake -S . -B build -G Ninja
cmake --build build --target t81 -j
```

## 2. Verify Native C++ MCP Server (`~110 µs` per gate check)

Run the built-in zero-dependency stdio MCP server test:

```bash
python3 examples/research/agent-receipt-sidecar/test_native_mcp.py
```

This starts `./build/t81 mcp serve` and verifies 6 in-process C++ MCP tools (`t81.policy_validate`, `t81.policy_test`, `t81.get_effective_policy`, `t81.gate_check`, `t81.canonfs_fsck`, `t81.determinism_hash`), including anti-downgrade clamping and 1-byte CanonFS tamper detection.

## 3. Run the 11-Scenario Compromised-Agent Gauntlet + Signed `RFC-00D4` Receipts

```bash
bash scripts/ci/run_agent_receipt_dogfood_smoke.sh
```

Demonstrates:
- `0.0%` unauthorized side effects across forged receipts, poisoned artifacts, mid-flight `.apl` policy revocation, mode-downgrade attacks, malformed `.apl` policies, 3-agent receipt DAG tampering, and CanonFS `.blk` tampering.
- `~10.3 ms` mean warm Ed25519 receipt verification (`t81.verify_receipt`).
- Tier 1 → Tier 2 `T81VM` exact-rational certification (`t81.certify_and_replay`).

## 4. Run Against a Live LLM (Optional)

Point the red-team harness at any local Ollama or OpenAI-compatible endpoint:

```bash
LLM_API_BASE="http://localhost:11434/v1" \
LLM_MODEL="llama3.2" \
python3 examples/research/agent-receipt-sidecar/run_live_llm_redteam.py
```

*(Without `LLM_API_BASE` / `LLM_MODEL`, it runs deterministically in offline simulated-proposer mode.)*

## 5. Plug Into Your MCP Client

For zero-dependency native C++ gating (`110 µs`):

```json
{
  "mcpServers": {
    "t81-native-mcp": {
      "command": "./build/t81",
      "args": ["mcp", "serve"]
    }
  }
}
```

For full signed `RFC-00D4` receipts + multi-agent `verify_dag` + `certify_and_replay`, see [`mcp_config.example.json`](mcp_config.example.json).

For explicit security guarantees and non-goals, read [`THREAT_MODEL.md`](THREAT_MODEL.md).
