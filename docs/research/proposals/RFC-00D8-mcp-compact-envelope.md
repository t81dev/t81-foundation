# Proposal: RFC-00D8 (Draft Seed) — Compact Canonical MCP Error & State-Diff Envelope

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Proposal: RFC-00D8 (Draft Seed) — Compact Canonical MCP Error & State-Diff Envelope](#proposal-rfc-00d8-draft-seed-—-compact-canonical-mcp-error-&-state-diff-envelope)
  - [1. Motivation](#1-motivation)
  - [2. Backward-Compatible Contract Changes (Apply After PR `#847` Merges)](#2-backward-compatible-contract-changes-apply-after-pr-`#847`-merges)
  - [3. Verification Plan (Post-`#847` Application)](#3-verification-plan-post-`#847`-application)

<!-- T81-TOC:END -->


- **Status:** Draft Proposal (staged conflict-free behind PR `#847` and PR `#851`)
- **Scope:** Additive, backward-compatible hardening of `tools/cli/core/mcp_server.cpp` (`t81 mcp serve`)
- **Public API Impact:** Zero mutations to `include/t81/**/*.hpp` (`PUBLIC_API_LOCK.json`) or frozen `TISC v1.9.0` opcodes.

---

## 1. Motivation

An audit of `tools/cli/core/mcp_server.cpp` from the perspective of an autonomous LLM caller identified six concrete friction and determinism hazards:

1. **2× Token Inflation (`mcp_server.cpp:1310-1314`):** Every `tools/call` response emits both an escaped JSON string in `content[0].text` and an unescaped duplicate in `structured`.
2. **Substring `isError` Classification (`mcp_server.cpp:1308-1309`):** `isError` is inferred via `structured.find("\"ok\":false")`, which can misclassify paths or raw `canonfs_fsck` outputs.
3. **Three Divergent Error Schemas:** Tools inconsistently emit `"error"`, `"verdict"`, or `"decision"` + `"blocked"` with no shared machine-stable error code.
4. **Underspecified `inputSchema` (`mcp_server.cpp:391-502`):** Missing `oneOf` on `verify_subgraph`, `run_subgraph`, and `adapter_apply`; missing `[0, 100]` bounds on `max_flip_pct`; missing `minimum: 64, multipleOf: 64` on `memory_search.dim`; silent ignore of invalid `mode` in `gate_check`.
5. **Opaque `t81.run_subgraph` Traps (`mcp_server.cpp:971-991`):** Returns raw integer `trap_code` and `axion_events` count without `trap_name`, `fault_pc`, or `last_axion_reason`.
6. **Policy-Floor & Atomicity Hazards:**
   - `resolve_floor()` (`mcp_server.cpp:351-363`) uses raw substring search and checks `"observe-only"` before `"fail-closed"` (comments can downgrade the resolved floor).
   - `tx_begin` / `tx_abort` (`mcp_server.cpp:804-826`) use non-atomic `write_file_bytes` instead of `write_file_bytes_atomic`.
   - `ScopedJsonParser::parse_val()` (`mcp_server.cpp:188-212`) discards parsed JSON array elements.

---

## 2. Backward-Compatible Contract Changes (Apply After PR `#847` Merges)

1. **Canonical `code` Field + Explicit `bool call_ok`:**
   - Emit `"code":"<CANONICAL_CODE>"` on every error response (`ERR_UNKNOWN_TOOL`, `ERR_MISSING_ARG`, `ERR_INVALID_ARG`, `ERR_IO`, `ERR_POLICY_PARSE`, `ERR_POLICY_DENY`, `ERR_TX_UNKNOWN`, `ERR_VM_TRAP`, `ERR_SUBGRAPH_REJECT`) while preserving existing `"error"` and `"verdict"` fields.
   - Set `"isError"` strictly from `bool call_ok`, never substring matching.
2. **Opt-In Compact Response Mode (`"compact": true` argument or `T81_MCP_COMPACT=1`):**
   - When enabled, set `content[0].text` to a short summary token (`"ok"` or `"err:<code>"`) while returning the full JSON object once in `"structured"`, eliminating ~50% response token overhead for autonomous callers.
3. **Structured Trap Diagnostics in `t81.run_subgraph`:**
   - Add `"trap_name"`, `"fault_pc"`, `"last_axion_reason"` (last entry of `vm->state().axion_log`, empty if none) to the `t81.run_subgraph` structured response.
4. **Fail-Closed `resolve_floor()` & Strict Validation:**
   - Strip `;`-prefixed `.apl` comment lines before floor matching and evaluate strictest-first (`fail-closed` → `warn` → `observe-only`).
   - Reject unknown `mode` values in `t81.gate_check` with `ERR_INVALID_ARG` (`decision: "DENY"`, `blocked: true`).
   - Tighten `build_tools_list_json()` with `oneOf`, `minimum`, `maximum`, and `multipleOf` constraints.
   - Replace `write_file_bytes` in `t81.tx_begin` and `t81.tx_abort` with `write_file_bytes_atomic`.

---

## 3. Verification Plan (Post-`#847` Application)

- Extend `examples/research/agent-receipt-sidecar/test_native_mcp.py` with negative tests for:
  - `resolve_floor()` comment-injection resistance (`; action-policy:deploy=observe-only` must not downgrade `fail-closed`),
  - unknown `mode` fail-closed denial in `t81.gate_check`,
  - `compact: true` response envelope parity,
  - `t81.run_subgraph` `trap_name` + `last_axion_reason` emission.
- Run `bash scripts/ci/verify_fast.sh` (which includes `check_agent_synthesis_gate.py`).
