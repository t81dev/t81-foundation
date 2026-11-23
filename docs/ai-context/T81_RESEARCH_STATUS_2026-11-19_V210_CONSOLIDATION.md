# T81 Research & Engineering Status Overlay — `v2.1.0` Consolidation (2026-11-19)

**Status:** Non-normative dated operational snapshot (supersedes earlier `2026-11-19` `v2.0.1` overlay for `main` state; preserves all historical research holds and negative findings).  
**Pinned Tag:** `v2.1.0` (`9ddeeccda`) · **Pinned `main`:** `f25387cb7` (plus this handoff commit)

## 1. What Landed Since `v2.0.1` (`f3b41b583` → `f25387cb7`)

- **Core `T81VM` Dual-Rail & Sparse-Block Path (`vm/tensor_helpers.cpp`, `vm/vm.cpp`, `tests/cpp/dualrail_sbdr_vm_test.cpp`):**
  - Wired ARM64 NEON + x86_64/MSVC AVX2 (`#elif defined(__AVX2__)`) Dual-Rail `(P, N)` popcount into `Opcode::TMatMul` with 64/128-trit block-zero activation skipping and accurate `ExactTrit` / `ExactInt` / `HostFloat` numeric-class propagation.
  - Wired direct `SBDR1T2D` object loading in `CanonFS` `TLoadHash` → lazy `CanonDualRailRhsCache` (dropping `full_stack_pipeline` `vm_e2e_us` from `21,070.8 µs` to `10,673.5 µs` with bit-exact oracle agreement and identical `SHA3-512` receipt).
  - Hardened `tensor_canon_hashes_` invalidation across VM reset, in-place `Opcode::TSet` mutation, and GC slot reclamation; covered permanently by CTest target `t81_dualrail_sbdr_vm_test`.
- **Hardened 15-Tool Native C++ MCP Server (`tools/cli/core/mcp_server.cpp`):**
  - Replaced substring JSON scanning with `ScopedJsonParser` (depth-capped at `64`, exact `id` preservation including spaced strings, `errno == ERANGE` + leading-whitespace rejection in `arg_double` / `arg_u64`), standard MCP `content` + `isError` envelopes, JSON-RPC 2.0 notification silence, and `ping`.
  - Added `t81.diff_check` (#13, Hamming flip-budget gate), `t81.memory_search` (#14, zero-float deterministic Dual-Rail RAG), `t81.adapter_apply` (#15, zero-dequantization saturated ternary adapter compose supporting dense and canonical `SBDR1` with atomic temp+rename writes), and fail-closed `VM_TRAP` vs real halting `t81.run_subgraph`.
  - Verified via `examples/research/agent-receipt-sidecar/test_native_mcp.py` (`15/15` tools, `~0.118 ms` mean warm `gate_check`).
- **BitNet Safetensors Ingester (`examples/research/bitnet-verifiable-token/load_safetensors_bitnet.py`):**
  - Added `BF16` decoding, per-tensor `absmean_gamma`, `--emit-sbdr` (`SBDR1`), `--emit-sbdr2d` (`SBDR1T2D`), and verified ingestion on real repo fixture `models/tiny-random-llama-exported.safetensors` (`21` tensors, `1,032,272` trits, `4.0×` vs `I8`).
- **CLI, Governance, & CI Boringness:**
  - Grouped `t81 --help` into 5 thematic sections, added `t81 help mcp` / `t81 mcp help` (stdout), updated `bash`/`zsh`/`fish` completions + manpage + `docs/user-guide/reference/cli-user-manual.md` (§4.26), and fixed `scripts/ci/check_cli_docs_parity.py` (`ok`, `24` topics).
  - Synced `CMakeLists.txt`, `pyproject.toml`, `include/t81/config.hpp`, `mcp_server.cpp` (`T81_VERSION_STR`), and `docs/governance/PUBLIC_API_LOCK.json` to `2.1.0`.
  - Unmasked `.github/workflows/ci.yml` governance steps with `set -euo pipefail`, fixed legacy paths in `check_docs_structure.py`, `check_docs_governance_hygiene.py`, `check_spec_code_alignment_baseline.py`, and `check_rfc_lifecycle_hygiene.py` (**18/18 governance scripts passing**), and refreshed AI status freshness docs (**`AI Experiments CI` green**).

## 2. Preserved Negative & Boundary Findings (Do Not Reopen Without New Controls)

- **Harley-Seal 3:2 CSA on Apple Silicon:** Loses to direct NEON `vcntq_u8` (`0.57×` on M2; independently reproduced on Apple M5 at `0.41 ms` vs `0.28 ms` on Issue #555).
- **1-Stage Ternary KV-Cache Attention (`D=128`):** Suffers integer tie collisions (`18.2%–40.6%` Recall@10); 2-stage top-64 pre-filter → FP32 rerank reached `85.6%` vs `57.0%` binary on synthetic sparse needles, parked pending real LLM KV traces.
- **Ternary Hyperdimensional Computing (VSA, `D=10,240`):** Ties fair 50/50 tie-broken 1-bit Binary VSA on dense random capacity while using `2×` RAM (`3.23×` faster than NEON `int8_t`); parked unless wildcard-`0` masking is required.
- **`INT4 × Ternary` 4-Plane Bit-Sliced Kernel (Apple M2):** Exact match, `1.42×` vs NEON `vdotq_s32` on bandwidth-bound `1×4096×4096` decode (below `1.50×` falsifier), and **`0.83×` (slower)** on compute-bound `256×512×256` prefill. Dual-Rail popcount's compute win over `vdotq_s32` holds for 2-plane ternary activations (`1.8×–2.3×`), not 4-plane `INT4`.
- **All Prior Holds Intact:** `F01` (`M1–M6 STOP`), `SCG-08 Gate 2` (`NO-GO`), `H01/10A` (`BLOCKED`), `AXION-AI-001` (`HD01–HD06` open).

## 3. Recommended Next Steps (Fresh Session)

1. **Human Release Ops:** Configure PyPI Trusted Publisher (OIDC) on `pypi.org` for `t81` and publish GitHub Release notes for `v2.1.0`.
2. **Issue #555 Cross-Check:** When the external contributor shares `bench_gemm_dual_rail.cpp` (Apple M5), cross-check it against `BM_DualRailTrit.cpp` on M2 and x86_64 AVX2.
3. **Real HuggingFace BitNet Checkpoint Validation:** Run `load_safetensors_bitnet.py --emit-sbdr2d` on a real BitNet b1.58 checkpoint (e.g., `microsoft/bitnet-b1.58-2B-4T`) to measure real-world `SBDR1T2D` compression, `absmean_gamma` accuracy, and `T81VM` execution.
4. **RFC-00D1 Maintenance Lane:** Continue `docs/BUILDABLE_NEXT_STEPS.md` CanonFS import/export negative-path hardening.
