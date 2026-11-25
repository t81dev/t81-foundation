# T81 Research & Engineering Status — 2026-11-18 (`v2.0.0` + Deterministic Agent Airlock)

**Date:** 2026-11-18  
**Frozen Release Tag:** `v2.0.0` at `7d4582ce5`  
**Current `main` Head:** `47fa1c399`  
**Status:** Non-normative operational snapshot; preserves all active research holds (`F01 M1–M6 STOP`, `SCG-08 Gate 2 NO-GO`, `H01/10A BLOCKED`, `AXION-AI-001 HD01–HD06 open`).

---

## 1. Summary of What Shipped & Was Measured

1. **Native C++ MCP Server (`t81 mcp serve`, `tools/cli/core/mcp_server.{hpp,cpp}`):**
   - Zero external dependencies, zero changes to locked `include/t81/**/*.hpp` (`PUBLIC_API_LOCK.json`).
   - Exposes 6 in-process stdio JSON-RPC 2.0 / MCP tools: `t81.policy_validate`, `t81.policy_test`, `t81.get_effective_policy`, `t81.gate_check`, `t81.canonfs_fsck`, `t81.determinism_hash`.
   - **Measured latency (`test_native_mcp.py`, 100 warm `t81.gate_check` calls):** `0.109 ms` mean (`109 µs`), `0.117 ms` p95.
2. **RFC-00D4 Accountable Decision Bundles & Agent Receipt Sidecar (`examples/research/agent-receipt-sidecar/`, `examples/research/accountable-decision-bundle/`):**
   - Detached Ed25519 signing over canonical `meaning + evidence + authority`, multi-agent receipt DAGs (`parent_bundle_ids` + `t81.verify_dag`), per-action `.apl` enforcement floors (`observe-only < warn < fail-closed`) with anti-downgrade clamping, truthful shadow receipts, and Tier 1 → Tier 2 `t81.certify_and_replay` bridge to `T81VM`.
   - Wired to real `t81 policy validate` and `t81 policy test --model-hash`.
   - **Measured (`scripts/ci/run_agent_receipt_dogfood_smoke.sh`):** `11/11` gauntlet scenarios passing, `0.0%` unauthorized side effects, `~10.3–10.8 ms` mean warm Ed25519 receipt verification (vs. `~6.2 s` cold CLI `assess-fixed` VM replay in `benchmark_compare.py`).
   - Added `QUICKSTART.md`, `THREAT_MODEL.md`, `mcp_config.example.json`, and `run_live_llm_redteam.py` (offline deterministic proposer verified at `0.0%` unauthorized / `100.0%` legitimate completion; live LLM endpoint supported via `LLM_API_BASE` + `LLM_MODEL`).
3. **CLI & Core Data Type Demonstrations:**
   - Added `t81 ai checkpoint-verify` (`tools/cli/ai/ai_cli_shared.cpp`) to verify `--checkpoint-canonfs` hash-linked step chains.
   - Added `examples/research/deterministic-risk-triage/` consuming locked `T81Prob<27>`, `T81Fraction<81>`, and `T81Result` for zero-drift cyber-defense triage.
4. **Post-`v2.0.0` Boring Fixes on `main` (`7519f79ad`..`47fa1c399`):**
   - `7519f79ad`: Removed executable `main()` sources from static library `t81_tool_cli` in `src/tools/CMakeLists.txt` (fixed Linux GNU `ld` `multiple definition of 'main'` in `tui_snapshot_test`).
   - `dfd2f78d7`: Nested `ffi/python/t81/{__init__.py,py.typed}`, updated `pyproject.toml` `wheel.packages`, and switched `CMakeLists.txt` to `Development.Module` for manylinux `cibuildwheel` builds.
   - `47fa1c399`: Extracted `t81 mcp serve` out of `tools/cli/core/main.cpp` into `tools/cli/core/mcp_server.{hpp,cpp}`.

---

## 2. Explicit Limits & Next Boring Priorities
- **Do not overclaim:** `RFC-00D4` remains `1.0.0-draft`; Ed25519 signing in `t81_receipt_sidecar.py` uses `openssl pkeyutl` rather than native C++ `crypto/`; live-LLM red-teaming still needs an external endpoint run.
- **Next boring engineering priorities:**
  1. Confirm `47fa1c399` CI is green across Linux x86/ARM, Windows, ASan, Docker, and `cibuildwheel`.
  2. Continue splitting `tools/cli/core/main.cpp` and expose `get_execution_mode`.
  3. Keep `F01`, `SCG-08 Gate 2`, and `10A` at STOP; keep Issue `#555` dual-rail isolated with packing costs included.
